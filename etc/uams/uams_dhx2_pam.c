/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <gcrypt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif

#ifdef HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#endif

#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/constant_time.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/uam.h>

#define PASSWDBUFLEN 256

/*! Number of bits for p which we generate. Everybody out there uses 512, so we beat them */
#define PRIMEBITS 1024

/*! hash a number to a 16-bit quantity */
#define dhxhash(a) ((((unsigned long) (a) >> 8) ^   \
                     (unsigned long) (a)) & 0xffff)

/* Some parameters need be maintained across calls */
static gcry_mpi_t p, g, Ra;
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
 * The call releases Ra and the server nonce, frees the session key, clears
 * the session ID, and returns to DHX2_STATE_IDLE. The group p and g stay for
 * the next login.
 */
static void dhx2_clear_session(void)
{
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
static pam_handle_t *pamh = NULL;
static char *PAM_username;
static char *PAM_password;
static struct passwd *dhxpwd;

/*********************************************************
 * Crypto helper func to generate p and g for use in DH.
 * libgcrypt doesn't provide one directly.
 * Algorithm taken from GNUTLS:gnutls_dh_primes.c
 *********************************************************/

/*!
 * @brief Generate the prime p and the generator g of the Diffie-Hellman group
 *
 * The algorithm follows gnutls_dh_primes.c, since libgcrypt has no direct
 * call for it. The call generates up to 10 primes until one passes
 * gcry_prime_check. The bits value should be 768, 1024, 2048, 3072, or 4096.
 *
 * @param[in] bits  size of p in bits
 * @returns 0, or AFPERR_MISC for a libgcrypt version mismatch or a failed
 *          generation
 */
static int dh_params_generate(unsigned int bits)
{
    int result, times = 0, qbits;
    gcry_mpi_t *factors = NULL;
    gcry_error_t err;

    /* Version check should be the very first call because it
       makes sure that important subsystems are initialized. */
    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        LOG(log_error, logtype_uams,
            "PAM DHX2: libgcrypt versions mismatch. Needs: %s Has: %s",
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
            gcry_mpi_release(p);
            gcry_prime_release_factors(factors);
        }

        err = gcry_prime_generate(&p, bits, qbits, &factors, NULL, NULL,
                                  GCRY_STRONG_RANDOM, GCRY_PRIME_FLAG_SPECIAL_FACTOR);

        if (err != 0) {
            result = AFPERR_MISC;
            goto error;
        }

        err = gcry_prime_check(p, 0);
        times++;
    } while (err != 0 && times < 10);

    if (err != 0) {
        result = AFPERR_MISC;
        goto error;
    }

    /* generate the group generator. */
    err = gcry_prime_group_generator(&g, p, factors, NULL);

    if (err != 0) {
        result = AFPERR_MISC;
        goto error;
    }

    gcry_prime_release_factors(factors);
    return 0;
error:
    gcry_prime_release_factors(factors);
    return result;
}


/*!
 * @brief Answer PAM prompts with the user name and the password of the login
 *
 * An echo-on prompt gets PAM_username and an echo-off prompt PAM_password.
 * Info and binary prompts get no answer, and an error message or an unknown
 * style fails the conversation.
 *
 * @param[in]  num_msg      number of messages
 * @param[in]  msg          PAM messages
 * @param[out] resp         receives the responses
 * @param[in]  appdata_ptr  unused
 * @returns PAM_SUCCESS, or PAM_CONV_ERR
 */
static int PAM_conv(int num_msg,
#ifdef HAVE_PAM_CONV_CONST_PAM_MESSAGE
                    const struct pam_message **msg,
#else
                    struct pam_message **msg,
#endif
                    struct pam_response **resp,
                    void *appdata_ptr _U_)
{
    int count = 0;
    struct pam_response *reply;
#define COPY_STRING(s) (s) ? strdup(s) : NULL
    errno = 0;

    if (num_msg < 1) {
        /* Log Entry */
        LOG(log_info, logtype_uams, "PAM DHX2 Conversation Err -- %s",
            strerror(errno));
        /* Log Entry */
        return PAM_CONV_ERR;
    }

    reply = (struct pam_response *)
            calloc(num_msg, sizeof(struct pam_response));

    if (!reply) {
        /* Log Entry */
        LOG(log_info, logtype_uams, "PAM DHX2: Conversation Err -- %s",
            strerror(errno));
        /* Log Entry */
        return PAM_CONV_ERR;
    }

    for (count = 0; count < num_msg; count++) {
        char *string = NULL;

        switch (msg[count]->msg_style) {
        case PAM_PROMPT_ECHO_ON:
            if (!(string = COPY_STRING(PAM_username))) {
                /* Log Entry */
                LOG(log_info, logtype_uams, "PAM DHX2: username failure -- %s",
                    strerror(errno));
                /* Log Entry */
                goto pam_fail_conv;
            }

            break;

        case PAM_PROMPT_ECHO_OFF:
            if (!(string = COPY_STRING(PAM_password))) {
                /* Log Entry */
                LOG(log_info, logtype_uams, "PAM DHX2: passwd failure: --: %s",
                    strerror(errno));
                /* Log Entry */
                goto pam_fail_conv;
            }

            break;

        case PAM_TEXT_INFO:
#ifdef PAM_BINARY_PROMPT
        case PAM_BINARY_PROMPT:
#endif /* PAM_BINARY_PROMPT */
            /* ignore it... */
            break;

        case PAM_ERROR_MSG:
        default:
            LOG(log_info, logtype_uams, "PAM DHX2: Binary_Prompt -- %s", strerror(errno));
            goto pam_fail_conv;
        }

        if (string) {
            reply[count].resp_retcode = 0;
            reply[count].resp = string;
            string = NULL;
        }
    }

    *resp = reply;
    LOG(log_info, logtype_uams, "PAM DHX2: password passed to PAM");
    return PAM_SUCCESS;
pam_fail_conv:

    for (count = 0; count < num_msg; count++) {
        if (!reply[count].resp) {
            continue;
        }

        switch (msg[count]->msg_style) {
        case PAM_PROMPT_ECHO_OFF:
        case PAM_PROMPT_ECHO_ON:
            free(reply[count].resp);
            break;
        }
    }

    free(reply);
    /* Log Entry */
    LOG(log_info, logtype_uams, "PAM DHX2: Conversation Err -- %s",
        strerror(errno));
    /* Log Entry */
    return PAM_CONV_ERR;
}

static struct pam_conv PAM_conversation = {
    &PAM_conv,
    NULL
};


/*!
 * @brief Start a DHX2 exchange with the public key of the server
 *
 * The call picks a random Ra of PRIMEBITS bits and computes Ma = g^Ra mod p.
 * The reply holds the session ID, g in 4 bytes, the length of p, p, and Ma,
 * each number padded to its full length. The next step is the first login
 * continuation.
 *
 * @param[in]  obj      session object, which also seeds the session ID
 * @param[in]  ibuf     UAM data of the request, unused
 * @param[in]  ibuflen  length of ibuf, unused
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return, 0 on failure
 * @returns AFPERR_AUTHCONT, or AFPERR_MISC
 */
static int dhx2_setup(void *obj, char *ibuf _U_, size_t ibuflen _U_,
                      char *rbuf, size_t *rbuflen)
{
    int ret;
    size_t nwritten;
    gcry_mpi_t Ma;
    char *Ra_binary = NULL;
    uint16_t uint16;
    *rbuflen = 0;
    dhx2_clear_session();
    Ra = gcry_mpi_new(0);
    Ma = gcry_mpi_new(0);
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
    /* We will need Ra later, but mustn't forget to release it ! */
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
 * @returns AFPERR_AUTHCONT, AFPERR_NOTAUTH for an unknown user, or AFPERR_MISC
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

    PAM_username = dhxpwd->pw_name;
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
static int pam_login(void *obj, struct passwd **uam_pwd,
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
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
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
 * encrypted, the client nonce plus 1 and a new server nonce. The next step is
 * the second continuation.
 *
 * @param[in]     obj      session object, unused
 * @param[in,out] ibuf     UAM data of the request, decrypted in place
 * @param[in]     ibuflen  length of ibuf, which must be 2 + PRIMEBITS / 8 + 16
 * @param[out]    rbuf     reply block
 * @param[out]    rbuflen  reply length on return
 * @returns AFPERR_AUTHCONT, AFPERR_PARAM out of sequence or for a bad length,
 *          or AFPERR_MISC
 */
static int logincont1(void *obj _U_, char *ibuf, size_t ibuflen, char *rbuf,
                      size_t *rbuflen)
{
    int ret;
    size_t nwritten;
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
 * @brief Log a failed PAM step through uam_log_pam_failure()
 *
 * errno is read before pam_strerror(), whose message lookup can change it.
 *
 * @param[in] ph       PAM handle, NULL when pam_start() failed
 * @param[in] step     PAM function that failed
 * @param[in] pam_err  its return code
 */
static void dhx2_log_pam_error(pam_handle_t *ph, const char *step, int pam_err)
{
    int saved_errno = errno;
    uam_log_pam_failure(log_info, "DHX2", PAM_username, step,
                        pam_strerror(ph, pam_err), pam_err, saved_errno);
}

/*!
 * @brief Check the server nonce and the password of a DHX2 login with PAM
 *
 * The request holds the session ID and, under CAST5-CBC, the server nonce
 * plus 1 and a 256-byte password buffer. Some clients add 10 more bytes. The
 * password goes from the Mac charset to the Unix charset. PAM then runs
 * authenticate, acct_mgmt, setcred, and open_session, with SIGCHLD at its
 * default action during the checks. The call wipes the password, and a
 * refused login calls pam_end.
 *
 * @param[in]     obj_in   session object
 * @param[out]    uam_pwd  receives the user entry after a successful login
 * @param[in,out] ibuf     UAM data of the request, decrypted in place
 * @param[in]     ibuflen  length of ibuf, which must be 274 or 284
 * @param[out]    rbuf     reply block, unused
 * @param[out]    rbuflen  0 on return
 * @returns AFP_OK, AFPERR_NOTAUTH, AFPERR_PWDEXPR, AFPERR_PWDCHNG,
 *          AFPERR_PARAM, or AFPERR_MISC
 */
static int logincont2(void *obj_in, struct passwd **uam_pwd,
                      char *ibuf, size_t ibuflen,
                      char *rbuf _U_, size_t *rbuflen)
{
    AFPObj *obj = obj_in;
    int ret = AFPERR_MISC;
    int PAM_error;
    const char *hostname = NULL;
    gcry_mpi_t retServerNonce = NULL;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    char *password = NULL;
    const char *password_end;
    char *utfpass = NULL;
    size_t password_len;
    size_t utfpass_len = 0;
    bool pam_started = false;
    *rbuflen = 0;

    if (dhx2_state != DHX2_STATE_EXPECT_CONT2 ||
            K_MD5hash == NULL || serverNonce == NULL) {
        LOG(log_info, logtype_uams,
            "DHX2: unexpected second login continuation");
        dhx2_clear_session();
        return AFPERR_PARAM;
    }

    /* Packet size should be: Session ID + ServerNonce + Passwd buffer (evantually +10 extra bytes, see Apples Docs) */
    if ((ibuflen != 2 + 16 + PASSWDBUFLEN) &&
            (ibuflen != 2 + 16 + PASSWDBUFLEN + 10)) {
        LOG(log_error, logtype_uams,
            "DHX2: Packet length not correct: %u. Should be 274 or 284.", ibuflen);
        ret = AFPERR_PARAM;
        goto error_noctx;
    }

    /* For PAM */
    uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME, (void *) &hostname, NULL);
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
        LOG(log_info, logtype_uams, "DHX2: server nonce mismatch, login refused");
        ret = AFPERR_NOTAUTH;
        goto error_ctx;
    }

    /* ---- Start authentication with PAM --- */
    password_end = memchr(password, '\0', PASSWDBUFLEN);

    if (password_end == NULL) {
        ret = AFPERR_NOTAUTH;
        goto error_ctx;
    }

    password_len = (size_t)(password_end - password);
    ret = AFPERR_NOTAUTH;
    /* The password is in legacy Mac encoding, convert it to host encoding */
    utfpass_len = convert_string_allocate(CH_MAC, CH_UNIX, password,
                                          password_len, &utfpass);

    if (utfpass_len == (size_t) -1) {
        LOG(log_error, logtype_uams, "DHX2: conversion error");
        goto error_ctx;
    }

    PAM_password = utfpass;
    /* Set these things up for the conv function */
    PAM_error = pam_start("netatalk", PAM_username, &PAM_conversation, &pamh);

    if (PAM_error != PAM_SUCCESS) {
        dhx2_log_pam_error(NULL, "pam_start", PAM_error);
        /* a failed pam_start() leaves the handle undefined */
        pamh = NULL;
        goto error_ctx;
    }

    pam_started = true;
    /* solaris craps out if PAM_TTY and PAM_RHOST aren't set. */
    pam_set_item(pamh, PAM_TTY, "afpd");
    pam_set_item(pamh, PAM_RHOST, hostname);
    pam_set_item(pamh, PAM_RUSER, PAM_username);
    /* Reset SIGCHLD to default during PAM auth: PAM modules like
     * pam_bsdauth fork helper processes and waitpid for them.
     * afpd sets SA_NOCLDWAIT which causes the kernel to auto-reap
     * children, making waitpid fail with ECHILD. */
    struct sigaction sa_dfl = {0};
    struct sigaction sa_old;
    int sigchld_saved;
    sa_dfl.sa_handler = SIG_DFL;
    sigemptyset(&sa_dfl.sa_mask);
    sigchld_saved = (sigaction(SIGCHLD, &sa_dfl, &sa_old) == 0);
    PAM_error = pam_authenticate(pamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        dhx2_log_pam_error(pamh, "pam_authenticate", PAM_error);

        if (sigchld_saved) {
            sigaction(SIGCHLD, &sa_old, NULL);
        }

        if (PAM_error == PAM_MAXTRIES) {
            ret = AFPERR_PWDEXPR;
        }

        goto error_ctx;
    }

    PAM_error = pam_acct_mgmt(pamh, 0);

    if (sigchld_saved) {
        sigaction(SIGCHLD, &sa_old, NULL);
    }

    if (PAM_error != PAM_SUCCESS) {
        dhx2_log_pam_error(pamh, "pam_acct_mgmt", PAM_error);

        if (PAM_error == PAM_NEW_AUTHTOK_REQD) {
            /* password expired */
            ret = AFPERR_PWDEXPR;
        }

#ifdef PAM_AUTHTOKEN_REQD
        else if (PAM_error == PAM_AUTHTOKEN_REQD) {
            ret = AFPERR_PWDCHNG;
        }

#endif
        goto error_ctx;
    }

#ifndef PAM_CRED_ESTABLISH
#define PAM_CRED_ESTABLISH PAM_ESTABLISH_CRED
#endif
    PAM_error = pam_setcred(pamh, PAM_CRED_ESTABLISH);

    if (PAM_error != PAM_SUCCESS) {
        dhx2_log_pam_error(pamh, "pam_setcred", PAM_error);
        goto error_ctx;
    }

    PAM_error = pam_open_session(pamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        dhx2_log_pam_error(pamh, "pam_open_session", PAM_error);
        goto error_ctx;
    }

    *uam_pwd = dhxpwd;
    LOG(log_info, logtype_uams, "DHX2: PAM Auth OK!");
    ret = AFP_OK;
error_ctx:

    if (password != NULL) {
        explicit_bzero(password, PASSWDBUFLEN);
    }

    gcry_cipher_close(ctx);

    /* a refused login leaves no handle behind; pam_logout() ends a kept one */
    if (pam_started && ret != AFP_OK) {
        pam_end(pamh, PAM_error);
        pamh = NULL;
    }

error_noctx:

    if (utfpass != NULL) {
        explicit_bzero(utfpass, utfpass_len + 1);
        free(utfpass);
    }

    PAM_password = NULL;
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
static int pam_logincont(void *obj, struct passwd **uam_pwd,
                         char *ibuf, size_t ibuflen,
                         char *rbuf, size_t *rbuflen)
{
    uint16_t retID;
    int ret;
    /* check for session id */
    memcpy(&retID, ibuf, sizeof(uint16_t));
    retID = ntohs(retID);

    if (retID == ID && dhx2_state == DHX2_STATE_EXPECT_CONT1) {
        ret = logincont1(obj, ibuf, ibuflen, rbuf, rbuflen);
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
 * @brief Close the PAM session of the login and call pam_end
 */
static void pam_logout(void)
{
    pam_close_session(pamh, 0);
    pam_end(pamh, 0);
    pamh = NULL;
}

/****************************
 * --- Change pwd stuff --- */

/*!
 * @brief Start the Diffie-Hellman exchange of a DHX2 password change and
 * remember the user
 *
 * @param[in]  obj      session object
 * @param[in]  uname    user name, kept for changepw_3
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns the result of dhx2_setup
 */
static int changepw_1(void *obj, char *uname,
                      char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    *rbuflen = 0;
    /* Remember it now, use it in changepw_3 */
    PAM_username = uname;
    return dhx2_setup(obj, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief Derive the session key of a DHX2 password change, as logincont1 does
 * for a login
 *
 * @param[in]     obj      session object
 * @param[in,out] ibuf     UAM data of the request
 * @param[in]     ibuflen  length of ibuf
 * @param[out]    rbuf     reply block
 * @param[out]    rbuflen  reply length on return
 * @returns the result of logincont1
 */
static int changepw_2(void *obj,
                      char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    return logincont1(obj, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief Check the server nonce and the old password, then set the new password
 * with PAM
 *
 * The request holds the session ID and, under CAST5-CBC, the server nonce
 * plus 1, the new password, and the old password, 256 bytes each. Equal
 * passwords fail with AFPERR_PWDSAME. As root PAM authenticates the old
 * password and runs acct_mgmt and chauthtok with the new one. The call wipes
 * both passwords after chauthtok.
 *
 * @param[in]     obj      session object, for the client host name
 * @param[in,out] ibuf     UAM data of the request, decrypted in place
 * @param[in]     ibuflen  length of ibuf, which must be 2 + 16 + 2 * 256
 * @param[out]    rbuf     reply block, unused
 * @param[out]    rbuflen  0 on return
 * @returns AFP_OK, AFPERR_PWDSAME, AFPERR_NOTAUTH, AFPERR_ACCESS when chauthtok
 *          fails, AFPERR_PARAM, or AFPERR_MISC
 */
static int changepw_3(void *obj _U_,
                      char *ibuf, size_t ibuflen _U_,
                      char *rbuf _U_, size_t *rbuflen _U_)
{
    int ret;
    int PAM_error;
    uid_t uid;
    pam_handle_t *lpamh;
    const char *hostname = NULL;
    gcry_mpi_t retServerNonce = NULL;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    *rbuflen = 0;
    LOG(log_error, logtype_uams, "DHX2 ChangePW: packet 3 processing");

    if (dhx2_state != DHX2_STATE_EXPECT_CONT2 ||
            K_MD5hash == NULL || serverNonce == NULL) {
        LOG(log_info, logtype_uams,
            "DHX2 ChangePW: unexpected password-change continuation");
        ret = AFPERR_PARAM;
        goto error_noctx;
    }

    /* Packet size should be: Session ID + ServerNonce + 2*Passwd buffer */
    if (ibuflen != 2 + 16 + 2 * 256) {
        LOG(log_error, logtype_uams, "DHX2: Packet length not correct");
        ret = AFPERR_PARAM;
        goto error_noctx;
    }

    /* For PAM */
    uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME, (void *) &hostname, NULL);
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
    /* Finally: decrypt client's md5_K(serverNonce+1, 2*password, C2SIV) inplace */
    ctxerror = gcry_cipher_decrypt(ctx, ibuf, 16 + 2 * 256, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        ret = AFPERR_MISC;
        goto error_ctx;
    }

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

    ibuf += 16;
    /* ---- Start pwd changing with PAM --- */
    ibuf[255] = '\0';       /* For safety */
    ibuf[511] = '\0';

    /* check if new and old password are equal */
    if (atalk_ct_memcmp(ibuf, ibuf + 256, 255) == 0) {
        LOG(log_info, logtype_uams, "DHX2 Chgpwd: new and old password are equal");
        ret = AFPERR_PWDSAME;
        goto error_ctx;
    }

    /* Set these things up for the conv function. PAM_username was set in changepw_1 */
    PAM_password = ibuf + 256;
    PAM_error = pam_start("netatalk", PAM_username, &PAM_conversation, &lpamh);

    if (PAM_error != PAM_SUCCESS) {
        dhx2_log_pam_error(NULL, "pam_start", PAM_error);
        ret = AFPERR_PARAM;
        goto error_ctx;
    }

    pam_set_item(lpamh, PAM_TTY, "afpd");
    uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME, (void *) &hostname, NULL);
    pam_set_item(lpamh, PAM_RHOST, hostname);
    uid = geteuid();

    if (seteuid(0) < 0) {
        LOG(log_error, logtype_uams, "DHX2 Chgpwd: could not seteuid(%i)", 0);
        pam_end(lpamh, PAM_SUCCESS);
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    PAM_error = pam_authenticate(lpamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        dhx2_log_pam_error(lpamh, "pam_authenticate", PAM_error);

        if (seteuid(uid) < 0) {
            LOG(log_error, logtype_uams, "DHX2 Chgpwd: could not seteuid(%i)", uid);
            pam_end(lpamh, PAM_error);
            ret = AFPERR_MISC;
            goto error_ctx;
        }

        pam_end(lpamh, PAM_error);
        ret = AFPERR_NOTAUTH;
        goto error_ctx;
    }

    PAM_error = pam_acct_mgmt(lpamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        dhx2_log_pam_error(lpamh, "pam_acct_mgmt", PAM_error);

        if (seteuid(uid) < 0) {
            LOG(log_error, logtype_uams, "DHX2 Chgpwd: could not seteuid(%i)", uid);
            pam_end(lpamh, PAM_error);
            ret = AFPERR_MISC;
            goto error_ctx;
        }

        pam_end(lpamh, PAM_error);
        ret = AFPERR_NOTAUTH;
        goto error_ctx;
    }

    PAM_password = ibuf;
    PAM_error = pam_chauthtok(lpamh, 0);

    if (seteuid(uid) < 0) {
        LOG(log_error, logtype_uams, "DHX2 Chgpwd: could not seteuid(%i)", uid);
        explicit_bzero(ibuf, 512);
        pam_end(lpamh, PAM_SUCCESS);
        ret = AFPERR_MISC;
        goto error_ctx;
    }

    explicit_bzero(ibuf, 512);

    if (PAM_error != PAM_SUCCESS) {
        dhx2_log_pam_error(lpamh, "pam_chauthtok", PAM_error);
        pam_end(lpamh, PAM_error);
        ret = AFPERR_ACCESS;
        goto error_ctx;
    }

    pam_end(lpamh, 0);
    ret = AFP_OK;
error_ctx:
    gcry_cipher_close(ctx);
error_noctx:
    dhx2_clear_session();
    gcry_mpi_release(retServerNonce);
    return ret;
}

/*!
 * @brief Run the three FPChangePassword requests of a DHX2 password change in
 * order
 *
 * A static step counter picks changepw_1, changepw_2, or changepw_3. A
 * failed second step and every third step restart the sequence.
 *
 * @param[in]     obj      session object
 * @param[in]     uname    user name
 * @param[in]     pwd      user entry, unused
 * @param[in,out] ibuf     UAM data of the request
 * @param[in]     ibuflen  length of ibuf
 * @param[out]    rbuf     reply block
 * @param[out]    rbuflen  reply length on return
 * @returns the result of the step
 */
static int dhx2_changepw(void *obj _U_, char *uname,
                         struct passwd *pwd _U_, char *ibuf, size_t ibuflen _U_,
                         char *rbuf _U_, size_t *rbuflen _U_)
{
    /* We use this to serialize the three incoming FPChangePassword calls */
    static int dhx2_changepw_status = 1;
    int ret = AFPERR_NOTAUTH;  /* gcc can't figure out it's always initialized */

    switch (dhx2_changepw_status) {
    case 1:
        ret = changepw_1(obj, uname, ibuf, ibuflen, rbuf, rbuflen);

        if (ret == AFPERR_AUTHCONT) {
            dhx2_changepw_status = 2;
        }

        break;

    case 2:
        ret = changepw_2(obj, ibuf, ibuflen, rbuf, rbuflen);

        if (ret == AFPERR_AUTHCONT) {
            dhx2_changepw_status = 3;
        } else {
            dhx2_changepw_status = 1;
        }

        break;

    case 3:
        ret = changepw_3(obj, ibuf, ibuflen, rbuf, rbuflen);
        dhx2_changepw_status = 1; /* Whether is was succesfull or not: we
                                     restart anyway !*/
        break;
    }

    return ret;
}

/*!
 * @brief Register the DHX2 login and password change, and generate the
 * Diffie-Hellman group
 *
 * The group has a prime of PRIMEBITS bits, generated once per process.
 *
 * @param[in] obj   AFP object, unused
 * @param[in] path  path of the module
 * @returns 0, or -1 when a registration or the group generation fails
 */
static int uam_setup(void *obj _U_, const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "DHX2", pam_login,
                     pam_logincont, pam_logout, pam_login_ext) < 0) {
        return -1;
    }

    if (uam_register(UAM_SERVER_CHANGEPW, path, "DHX2", dhx2_changepw) < 0) {
        return -1;
    }

    LOG(log_debug, logtype_uams, "DHX2: generating mersenne primes");

    /* Generate p and g for DH */
    if (dh_params_generate(PRIMEBITS) != 0) {
        LOG(log_error, logtype_uams, "DHX2: Couldn't generate p and g");
        return -1;
    }

    return 0;
}

/*!
 * @brief Unregister the DHX2 login and password change, and release the
 * Diffie-Hellman group
 */
static void uam_cleanup(void)
{
    uam_unregister(UAM_SERVER_LOGIN, "DHX2");
    uam_unregister(UAM_SERVER_CHANGEPW, "DHX2");
    LOG(log_debug, logtype_uams, "DHX2: uam_cleanup");
    gcry_mpi_release(p);
    gcry_mpi_release(g);
}

UAM_MODULE_EXPORT struct uam_export uams_dhx2 = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};


UAM_MODULE_EXPORT struct uam_export uams_dhx2_pam = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};
