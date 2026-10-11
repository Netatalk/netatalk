/*
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <gcrypt.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#ifdef SHADOWPW
#include <shadow.h>
#endif

#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/logger.h>
#include <atalk/uam.h>

#define PASSWDBUFLEN 256

/*! Number of bits for p which we generate. Everybody out there uses 512, so we beat them */
#define PRIMEBITS 1024

/*! hash a number to a 16-bit quantity */
#define dhxhash(a) ((((unsigned long) (a) >> 8) ^   \
                     (unsigned long) (a)) & 0xffff)

/* Some parameters need be maintained across calls */
static gcry_mpi_t p, Ra;
static gcry_mpi_t serverNonce;
static char *K_MD5hash = NULL;
static int K_hash_len;
static uint16_t ID;

enum dhx2_state {
    DHX2_STATE_IDLE,
    DHX2_STATE_EXPECT_CONT1,
    DHX2_STATE_EXPECT_CONT2,
};

static enum dhx2_state dhx2_state = DHX2_STATE_IDLE;

/*!
 * @brief Release a libgcrypt number and set its pointer to NULL
 *
 * @param[in,out] mpi  number to release, or a pointer to NULL
 */
static void dhx2_release_mpi(gcry_mpi_t *mpi)
{
    if (mpi != NULL && *mpi != NULL) {
        gcry_mpi_release(*mpi);
        *mpi = NULL;
    }
}

/*!
 * @brief Drop the state of a DHX2 exchange, so the next step has to start over
 *
 * The call releases p, Ra, and the server nonce, frees the session key,
 * clears the session ID, and returns to DHX2_STATE_IDLE.
 */
static void dhx2_clear_session(void)
{
    dhx2_release_mpi(&p);
    dhx2_release_mpi(&Ra);
    dhx2_release_mpi(&serverNonce);
    free(K_MD5hash);
    K_MD5hash = NULL;
    K_hash_len = 0;
    ID = 0;
    dhx2_state = DHX2_STATE_IDLE;
}

/* The initialization vectors for CAST128 are fixed by Apple. */
static unsigned char dhx_c2siv[] = { 'L', 'W', 'a', 'l', 'l', 'a', 'c', 'e' };
static unsigned char dhx_s2civ[] = { 'C', 'J', 'a', 'l', 'b', 'e', 'r', 't' };

/* Static variables used to communicate between the conversation function
 * and the server_login function */
static struct passwd *dhxpwd;

/*********************************************************
 * Crypto helper func to generate p and g for use in DH.
 * libgcrypt doesn't provide one directly.
 * Algorithm taken from GNUTLS:gnutls_dh_primes.c
 *********************************************************/

/*!
 * @brief Generate a prime p and a generator g for one Diffie-Hellman exchange
 *
 * The algorithm follows gnutls_dh_primes.c, since libgcrypt has no direct
 * call for it. The call generates up to 10 primes until one passes
 * gcry_prime_check. The bits value should be 768, 1024, 2048, 3072, or 4096.
 *
 * @param[out] ret_p  receives p, or NULL to drop it
 * @param[out] ret_g  receives g, or NULL to drop it
 * @param[in]  bits   size of p in bits
 * @returns 0, or AFPERR_MISC for a libgcrypt version mismatch or a failed
 *          generation
 */
static int
dh_params_generate(gcry_mpi_t *ret_p, gcry_mpi_t *ret_g, unsigned int bits)
{
    int result, times = 0, qbits;
    gcry_mpi_t g = NULL, prime = NULL;
    gcry_mpi_t *factors = NULL;
    gcry_error_t err;

    /* Version check should be the very first call because it
       makes sure that important subsystems are initialized. */
    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        LOG(log_error, logtype_uams,
            "DHX2: libgcrypt versions mismatch. Needs: %s Has: %s",
            UAM_NEED_LIBGCRYPT_VERSION, gcry_check_version(NULL));
        result = AFPERR_MISC;
        goto error;
    }

    if (bits < 256) {
        qbits = bits / 2;
    } else {
        qbits = (bits / 40) + 105;
    }

    /* better have an even number */
    if (qbits & 1) {
        qbits++;
    }

    /* find a prime number of size bits. */
    do {
        if (times) {
            gcry_mpi_release(prime);
            gcry_prime_release_factors(factors);
        }

        err = gcry_prime_generate(&prime, bits, qbits, &factors, NULL, NULL,
                                  GCRY_STRONG_RANDOM, GCRY_PRIME_FLAG_SPECIAL_FACTOR);

        if (err != 0) {
            result = AFPERR_MISC;
            goto error;
        }

        err = gcry_prime_check(prime, 0);
        times++;
    } while (err != 0 && times < 10);

    if (err != 0) {
        result = AFPERR_MISC;
        goto error;
    }

    /* generate the group generator. */
    err = gcry_prime_group_generator(&g, prime, factors, NULL);

    if (err != 0) {
        result = AFPERR_MISC;
        goto error;
    }

    gcry_prime_release_factors(factors);
    factors = NULL;

    if (ret_g) {
        *ret_g = g;
    } else {
        gcry_mpi_release(g);
    }

    if (ret_p) {
        *ret_p = prime;
    } else {
        gcry_mpi_release(prime);
    }

    return 0;
error:
    gcry_prime_release_factors(factors);
    gcry_mpi_release(g);
    gcry_mpi_release(prime);
    return result;
}

/*!
 * @brief Start a DHX2 exchange with a new Diffie-Hellman group and the public
 * key of the server
 *
 * The user needs a password hash, from the shadow file in a build that uses
 * one. Each exchange generates a new group of PRIMEBITS bits, so every login
 * attempt pays for a prime search. The reply holds the session ID, g in
 * 4 bytes, the length of p, p, and Ma = g^Ra mod p.
 *
 * @param[in]  obj      session object, which also seeds the session ID
 * @param[in]  ibuf     UAM data of the request, unused
 * @param[in]  ibuflen  length of ibuf, unused
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return, 0 on failure
 * @returns AFPERR_AUTHCONT, AFPERR_NOTAUTH without a password hash, or
 *          AFPERR_MISC
 */
static int dhx2_setup(void *obj, char *ibuf _U_, size_t ibuflen _U_,
                      char *rbuf, size_t *rbuflen)
{
    int ret;
    size_t nwritten;
    gcry_mpi_t g, Ma;
    char *Ra_binary = NULL;
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */
    uint16_t uint16;
    *rbuflen = 0;
    dhx2_clear_session();
    /* Initialize passwd/shadow */
#ifdef SHADOWPW

    if ((sp = getspnam(dhxpwd->pw_name)) == NULL) {
        LOG(log_info, logtype_uams, "DHX2: no shadow passwd entry for this user");
        return AFPERR_NOTAUTH;
    }

    dhxpwd->pw_passwd = sp->sp_pwdp;
#endif /* SHADOWPW */

    if (!dhxpwd->pw_passwd) {
        return AFPERR_NOTAUTH;
    }

    /* Initialize DH params */
    p = gcry_mpi_new(0);
    g = gcry_mpi_new(0);
    Ra = gcry_mpi_new(0);
    Ma = gcry_mpi_new(0);
    /* Generate p and g for DH */
    ret = dh_params_generate(&p, &g, PRIMEBITS);

    if (ret != 0) {
        LOG(log_info, logtype_uams, "DHX2: Couldn't generate p and g");
        ret = AFPERR_MISC;
        goto error;
    }

    /* Generate our random number Ra. */
    Ra_binary = calloc(1, PRIMEBITS / 8);

    if (Ra_binary == NULL) {
        ret = AFPERR_MISC;
        goto error;
    }

    gcry_randomize(Ra_binary, PRIMEBITS / 8, GCRY_STRONG_RANDOM);
    gcry_mpi_scan(&Ra, GCRYMPI_FMT_USG, Ra_binary, PRIMEBITS / 8, NULL);
    free(Ra_binary);
    Ra_binary = NULL;
    /* Ma = g^Ra mod p. This is our "public" key */
    gcry_mpi_powm(Ma, g, Ra, p);
    /* ------- DH Init done ------ */
    /* Start building reply packet */
    /* Session ID first */
    ID = dhxhash(obj);
    uint16 = htons(ID);
    memcpy(rbuf, &uint16, sizeof(uint16_t));
    rbuf += 2;
    *rbuflen += 2;

    /* g is next */
    if (gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *)rbuf, 4, &nwritten,
                       g) != 0) {
        ret = AFPERR_MISC;
        goto error;
    }

    if (nwritten < 4) {
        memmove(rbuf + 4 - nwritten, rbuf, nwritten);
        memset(rbuf, 0, 4 - nwritten);
    }

    rbuf += 4;
    *rbuflen += 4;
    /* len = length of p = PRIMEBITS/8 */
    uint16 = htons((uint16_t) PRIMEBITS / 8);
    memcpy(rbuf, &uint16, sizeof(uint16_t));
    rbuf += 2;
    *rbuflen += 2;

    /* p */
    if (gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *)rbuf, PRIMEBITS / 8,
                       &nwritten, p) != 0) {
        ret = AFPERR_MISC;
        goto error;
    }

    if (nwritten < PRIMEBITS / 8) {
        memmove(rbuf + (PRIMEBITS / 8) - nwritten, rbuf, nwritten);
        memset(rbuf, 0, (PRIMEBITS / 8) - nwritten);
    }

    rbuf += PRIMEBITS / 8;
    *rbuflen += PRIMEBITS / 8;

    /* Ma */
    if (gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *)rbuf, PRIMEBITS / 8,
                       &nwritten, Ma) != 0) {
        ret = AFPERR_MISC;
        goto error;
    }

    if (nwritten < PRIMEBITS / 8) {
        memmove(rbuf + (PRIMEBITS / 8) - nwritten, rbuf, nwritten);
        memset(rbuf, 0, (PRIMEBITS / 8) - nwritten);
    }

    rbuf += PRIMEBITS / 8;
    *rbuflen += PRIMEBITS / 8;
    ret = AFPERR_AUTHCONT;
    dhx2_state = DHX2_STATE_EXPECT_CONT1;
error:              /* We exit here anyway */
    /* We will only need p and Ra later, but mustn't forget to release it ! */
    gcry_mpi_release(g);
    gcry_mpi_release(Ma);

    if (ret != AFPERR_AUTHCONT) {
        *rbuflen = 0;
        dhx2_clear_session();
    }

    return ret;
}

/*!
 * @brief Look up the user of a DHX2 login and start the Diffie-Hellman exchange
 *
 * @param[in]     obj       session object
 * @param[in,out] username  user name from the request, receives the account
 *                          name of a match
 * @param[in]     ulen      size of the username buffer
 * @param[out]    uam_pwd   unused
 * @param[in]     ibuf      UAM data of the request
 * @param[in]     ibuflen   length of ibuf
 * @param[out]    rbuf      reply block
 * @param[out]    rbuflen   reply length on return
 * @returns AFPERR_AUTHCONT, AFPERR_NOTAUTH for an unknown user or one without a
 *          password hash, or AFPERR_MISC
 */
static int login(void *obj, char *username, int ulen,
                 struct passwd **uam_pwd _U_,
                 char *ibuf, size_t ibuflen,
                 char *rbuf, size_t *rbuflen)
{
    if ((dhxpwd = uam_getname(obj, username, ulen)) == NULL) {
        LOG(log_info, logtype_uams, "DHX2: unknown username");
        return AFPERR_NOTAUTH;
    }

    LOG(log_info, logtype_uams, "DHX2 login: %s", username);
    return dhx2_setup(obj, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief Read the user name of an FPLogin request and start a DHX2 login
 *
 * The request holds the name as a Pascal string, padded to an even address.
 * The checks run before any state exists, so a refusal needs no cleanup.
 *
 * @param[in]  obj      session object
 * @param[out] uam_pwd  unused until the second continuation
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFPERR_AUTHCONT, AFPERR_PARAM for a name longer than the buffer,
 *          AFPERR_NOTAUTH, or AFPERR_MISC
 */
static int passwd_login(void *obj, struct passwd **uam_pwd,
                        char *ibuf, size_t ibuflen,
                        char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t len, ulen;
    *rbuflen = 0;

    /* grab some of the options */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
        LOG(log_info, logtype_uams,
            "DHX2: uam_afpserver_option didn't meet uam_option_username -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    len = (unsigned char) * ibuf++;

    if (len > ulen) {
        LOG(log_info, logtype_uams, "DHX2: Signature Retieval Failure -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    memcpy(username, ibuf, len);
    ibuf += len;
    username[len] = '\0';

    /* pad to even boundary */
    if ((unsigned long) ibuf & 1) {
        ++ibuf;
    }

    return login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief Read the user name of an FPLoginExt request and start a DHX2 login
 *
 * The name comes as an AFP name of type 3, a UTF-8 string with a 2-byte
 * length.
 *
 * @param[in]  obj      session object
 * @param[in]  uname    user name field of the request
 * @param[out] uam_pwd  unused until the second continuation
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFPERR_AUTHCONT, AFPERR_PARAM for another name type or a bad length,
 *          AFPERR_NOTAUTH, or AFPERR_MISC
 */
static int passwd_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
                            char *ibuf, size_t ibuflen,
                            char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t len, ulen;
    uint16_t  temp16;
    *rbuflen = 0;

    /* grab some of the options */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
        LOG(log_info, logtype_uams,
            "DHX2: uam_afpserver_option didn't meet uam_option_username -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    if (*uname != 3) {
        return AFPERR_PARAM;
    }

    uname++;
    memcpy(&temp16, uname, sizeof(temp16));
    len = ntohs(temp16);

    if (!len || len > ulen) {
        LOG(log_info, logtype_uams, "DHX2: Signature Retrieval Failure -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    memcpy(username, uname + 2, len);
    username[len] = '\0';
    return login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief Derive the session key from the public key of the client and answer
 * its nonce
 *
 * The request holds the session ID, Mb, and the client nonce under CAST5-CBC.
 * The CAST5 secret is the MD5 hash of K = Mb^Ra mod p, with the fixed IV of
 * Apple for each direction. The reply holds the session ID plus 1 and,
 * encrypted, the client nonce plus 1 and a new server nonce. A success
 * releases p and Ra, and the next step is the second continuation.
 *
 * @param[in]     obj      session object, unused
 * @param[out]    uam_pwd  unused
 * @param[in,out] ibuf     UAM data of the request, decrypted in place
 * @param[in]     ibuflen  length of ibuf, which must be 2 + PRIMEBITS / 8 + 16
 * @param[out]    rbuf     reply block
 * @param[out]    rbuflen  reply length on return
 * @returns AFPERR_AUTHCONT, AFPERR_PARAM out of sequence or for a bad length,
 *          or AFPERR_MISC
 */
static int logincont1(void *obj _U_, struct passwd **uam_pwd _U_,
                      char *ibuf, size_t ibuflen,
                      char *rbuf, size_t *rbuflen)
{
    size_t nwritten;
    int ret;
    gcry_mpi_t Mb, K, clientNonce;
    unsigned char *K_bin = NULL;
    char serverNonce_bin[16];
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    uint16_t uint16;
    *rbuflen = 0;

    if (dhx2_state != DHX2_STATE_EXPECT_CONT1 || Ra == NULL) {
        LOG(log_info, logtype_uams,
            "DHX2: unexpected first login continuation");
        return AFPERR_PARAM;
    }

    Mb = gcry_mpi_new(0);
    K = gcry_mpi_new(0);
    clientNonce = gcry_mpi_new(0);
    serverNonce = gcry_mpi_new(0);

    /* Packet size should be: Session ID + Ma + Encrypted client nonce */
    if (ibuflen != 2 + PRIMEBITS / 8 + 16) {
        LOG(log_error, logtype_uams, "DHX2: Packet length not correct");
        ret = AFPERR_PARAM;
        goto error_noctx;
    }

    /* Skip session id */
    ibuf += 2;
    /* Extract Mb, client's "public" key */
    gcry_mpi_scan(&Mb, GCRYMPI_FMT_USG, ibuf, PRIMEBITS / 8, NULL);
    ibuf += PRIMEBITS / 8;
    /* Now finally generate the Key: K = Mb^Ra mod p */
    gcry_mpi_powm(K, Mb, Ra, p);
    /* We need K in binary form in order to ... */
    K_bin = calloc(1, PRIMEBITS / 8);

    if (K_bin == NULL) {
        ret = AFPERR_MISC;
        goto error_noctx;
    }

    gcry_mpi_print(GCRYMPI_FMT_USG, K_bin, PRIMEBITS / 8, &nwritten, K);

    if (nwritten < PRIMEBITS / 8) {
        memmove(K_bin + PRIMEBITS / 8 - nwritten, K_bin, nwritten);
        memset(K_bin, 0, PRIMEBITS / 8 - nwritten);
    }

    /* ... generate the MD5 hash of K. K_MD5hash is what we actually use ! */
    K_MD5hash = calloc(1, K_hash_len = gcry_md_get_algo_dlen(GCRY_MD_MD5));

    if (K_MD5hash == NULL) {
        ret = AFPERR_MISC;
        free(K_bin);
        K_bin = NULL;
        goto error_noctx;
    }

    gcry_md_hash_buffer(GCRY_MD_MD5, K_MD5hash, K_bin, PRIMEBITS / 8);
    free(K_bin);
    K_bin = NULL;
    /* FIXME: To support the Reconnect UAM, we need to store this key somewhere */
    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5, GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    /* Set key */
    ctxerror = gcry_cipher_setkey(ctx, K_MD5hash, K_hash_len);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    /* Set the initialization vector for client->server transfer. */
    ctxerror = gcry_cipher_setiv(ctx, dhx_c2siv, sizeof(dhx_c2siv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    /* Finally: decrypt client's md5_K(client nonce, C2SIV) inplace */
    ctxerror = gcry_cipher_decrypt(ctx, ibuf, 16, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    /* Pull out clients nonce */
    gcry_mpi_scan(&clientNonce, GCRYMPI_FMT_USG, ibuf, 16, NULL);
    /* Increment nonce */
    gcry_mpi_add_ui(clientNonce, clientNonce, 1);
    /* Generate our nonce and remember it for Logincont2 */
    /* We'll use this here */
    gcry_create_nonce(serverNonce_bin, 16);
    /* For use in Logincont2 */
    gcry_mpi_scan(&serverNonce, GCRYMPI_FMT_USG, serverNonce_bin, 16, NULL);
    /* ---- Start building reply packet ---- */
    /* Session ID + 1 first */
    uint16 = htons(ID + 1);
    memcpy(rbuf, &uint16, sizeof(uint16_t));
    rbuf += 2;
    *rbuflen += 2;
    /* Client nonce + 1 */
    gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *)rbuf, PRIMEBITS / 8, &nwritten,
                   clientNonce);

    if (nwritten < 16) {
        memmove(rbuf + 16 - nwritten, rbuf, nwritten);
        memset(rbuf, 0, 16 - nwritten);
    }

    /* Server nonce */
    memcpy(rbuf + 16, serverNonce_bin, 16);
    /* Set the initialization vector for server->client transfer. */
    ctxerror = gcry_cipher_setiv(ctx, dhx_s2civ, sizeof(dhx_s2civ));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    /* Encrypt md5_K(clientNonce+1, serverNonce) inplace */
    ctxerror = gcry_cipher_encrypt(ctx, rbuf, 32, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    rbuf += 32;
    *rbuflen += 32;
    ret = AFPERR_AUTHCONT;
    goto exit;
error_ctx:
    gcry_cipher_close(ctx);
error_noctx:
exit:

    if (ret == AFPERR_AUTHCONT) {
        dhx2_release_mpi(&Ra);
        dhx2_release_mpi(&p);
        dhx2_state = DHX2_STATE_EXPECT_CONT2;
    } else {
        dhx2_clear_session();
    }

    gcry_mpi_release(K);
    gcry_mpi_release(Mb);
    gcry_mpi_release(clientNonce);
    return ret;
}

/*!
 * @brief Check the server nonce and the password of a DHX2 login against the
 * password hash
 *
 * The request holds the session ID and, under CAST5-CBC, the server nonce
 * plus 1 and a 256-byte password buffer. Some clients add 10 more bytes. The
 * password goes to crypt_checkpass, or to crypt and a comparison. With shadow
 * passwords an expired password gives AFPERR_PWDEXPR, and that check runs
 * after a wrong password too. The call wipes the password.
 *
 * @param[in]     obj      session object, unused
 * @param[out]    uam_pwd  receives the user entry after a matching password
 * @param[in,out] ibuf     UAM data of the request, decrypted in place
 * @param[in]     ibuflen  length of ibuf, which must be 274 or 284
 * @param[out]    rbuf     reply block, unused
 * @param[out]    rbuflen  0 on return
 * @returns AFP_OK, AFPERR_NOTAUTH, AFPERR_PWDEXPR, AFPERR_PARAM, or AFPERR_MISC
 */
static int logincont2(void *obj _U_, struct passwd **uam_pwd,
                      char *ibuf, size_t ibuflen,
                      char *rbuf _U_, size_t *rbuflen)
{
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */
    int ret;
    char *p;
    char *password = NULL;
    gcry_mpi_t retServerNonce = NULL;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    *rbuflen = 0;

    if (dhx2_state != DHX2_STATE_EXPECT_CONT2 ||
            K_MD5hash == NULL || serverNonce == NULL) {
        LOG(log_info, logtype_uams,
            "DHX2: unexpected second login continuation");
        dhx2_clear_session();
        gcry_mpi_release(retServerNonce);
        return AFPERR_PARAM;
    }

    /* Packet size should be: Session ID + ServerNonce + Passwd buffer (evantually +10 extra bytes, see Apples Docs)*/
    if ((ibuflen != 2 + 16 + PASSWDBUFLEN) &&
            (ibuflen != 2 + 16 + PASSWDBUFLEN + 10)) {
        LOG(log_error, logtype_uams,
            "DHX2: Packet length not correct: %d. Should be 274 or 284.", ibuflen);
        ret = AFPERR_PARAM;
        goto error_noctx;
    }

    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5, GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    /* Set key */
    ctxerror = gcry_cipher_setkey(ctx, K_MD5hash, K_hash_len);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    /* Set the initialization vector for client->server transfer. */
    ctxerror = gcry_cipher_setiv(ctx, dhx_c2siv, sizeof(dhx_c2siv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    /* Skip Session ID */
    ibuf += 2;
    /* Finally: decrypt client's md5_K(serverNonce+1, passwor, C2SIV) inplace */
    ctxerror = gcry_cipher_decrypt(ctx, ibuf, 16 + PASSWDBUFLEN, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    password = ibuf + 16;
    /* Pull out nonce. Should be serverNonce+1 */
    ctxerror = gcry_mpi_scan(&retServerNonce, GCRYMPI_FMT_USG, ibuf, 16, NULL);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    gcry_mpi_sub_ui(retServerNonce, retServerNonce, 1);

    if (gcry_mpi_cmp(serverNonce, retServerNonce) != 0) {
        /* We're hacked!  */
        ret = AFPERR_NOTAUTH;
        goto error_ctx;
    }

    if (memchr(password, '\0', PASSWDBUFLEN) == NULL) {
        ret = AFPERR_NOTAUTH;
        goto error_ctx;
    }

    /* ---- Start authentication --- */
    ret = AFPERR_NOTAUTH;
#ifdef HAVE_CRYPT_CHECKPASS

    if (crypt_checkpass(password, dhxpwd->pw_passwd) == 0) {
#else
    p = crypt(password, dhxpwd->pw_passwd);

    if (p != NULL && strcmp(p, dhxpwd->pw_passwd) == 0) {
#endif
        *uam_pwd = dhxpwd;
        ret = AFP_OK;
    }

    explicit_bzero(password, PASSWDBUFLEN);
    password = NULL;
#ifdef SHADOWPW

    if ((sp = getspnam(dhxpwd->pw_name)) == NULL) {
        LOG(log_info, logtype_uams, "no shadow passwd entry for %s", dhxpwd->pw_name);
        ret = AFPERR_NOTAUTH;
        goto exit;
    }

    /* check for expired password */
    if (sp && sp->sp_max != -1 && sp->sp_lstchg) {
        time_t now = time(NULL) / (60 * 60 * 24);
        int32_t expire_days = sp->sp_lstchg - now + sp->sp_max;

        if (expire_days < 0) {
            LOG(log_info, logtype_uams, "password for user %s expired", dhxpwd->pw_name);
            ret = AFPERR_PWDEXPR;
            goto exit;
        }
    }

#endif /* SHADOWPW */
error_ctx:

    if (password != NULL) {
        explicit_bzero(password, PASSWDBUFLEN);
    }

    gcry_cipher_close(ctx);
error_noctx:
exit:
    dhx2_clear_session();
    gcry_mpi_release(retServerNonce);
    return ret;
}

/*!
 * @brief Route an FPLoginCont request to the step that the session ID and the
 * state expect
 *
 * The session ID goes to logincont1, and the ID plus 1 to logincont2. Any
 * other ID or step clears the exchange.
 *
 * @param[in]     obj      session object
 * @param[out]    uam_pwd  receives the user entry after a successful login
 * @param[in,out] ibuf     UAM data of the request
 * @param[in]     ibuflen  length of ibuf
 * @param[out]    rbuf     reply block
 * @param[out]    rbuflen  reply length on return
 * @returns the result of the step, or AFPERR_PARAM for an unexpected request
 */
static int passwd_logincont(void *obj, struct passwd **uam_pwd,
                            char *ibuf, size_t ibuflen,
                            char *rbuf, size_t *rbuflen)
{
    uint16_t retID;
    int ret;
    /* check for session id */
    memcpy(&retID, ibuf, sizeof(uint16_t));
    retID = ntohs(retID);

    if (retID == ID && dhx2_state == DHX2_STATE_EXPECT_CONT1) {
        ret = logincont1(obj, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
    } else if (retID == ID + 1 && dhx2_state == DHX2_STATE_EXPECT_CONT2) {
        ret = logincont2(obj, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
    } else {
        LOG(log_info, logtype_uams,
            "DHX2: Session ID mismatch or unexpected login continuation");
        dhx2_clear_session();
        ret = AFPERR_PARAM;
    }

    return ret;
}

/*!
 * @brief Register the DHX2 login of the passwd module
 *
 * The module has no logout and no password change.
 *
 * @param[in] obj   AFP object, unused
 * @param[in] path  path of the module
 * @returns 0, or -1 when the registration fails
 */
static int uam_setup(void *obj, const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "DHX2", passwd_login,
                     passwd_logincont, NULL, passwd_login_ext) < 0) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Unregister the DHX2 login of the passwd module
 */
static void uam_cleanup(void)
{
    uam_unregister(UAM_SERVER_LOGIN, "DHX2");
}


UAM_MODULE_EXPORT struct uam_export uams_dhx2 = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};


UAM_MODULE_EXPORT struct uam_export uams_dhx2_passwd = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};
