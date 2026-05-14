/*
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
#include <atalk/logger.h>
#include <atalk/uam.h>

#include "uam_common.h"

#define PASSWDLEN 255

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

/* The initialization vectors for CAST128 are fixed by Apple. */
static unsigned char dhx_c2siv[] = { 'L', 'W', 'a', 'l', 'l', 'a', 'c', 'e' };
static unsigned char dhx_s2civ[] = { 'C', 'J', 'a', 'l', 'b', 'e', 'r', 't' };

/* Static variables used to communicate between the conversation function
 * and the server_login function */
static struct passwd *dhxpwd;

static int dhx2_setup(void *obj, char *ibuf _U_, size_t ibuflen _U_,
                      char *rbuf, size_t *rbuflen)
{
    int ret;
    gcry_mpi_t g = NULL, Ma;
    char *Ra_binary = NULL;
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */
    uint16_t uint16;
    *rbuflen = 0;
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
    Ra = gcry_mpi_new(0);
    Ma = gcry_mpi_new(0);
    /* Generate p and g for DH */
    ret = uam_dh_params_generate(&p, &g, PRIMEBITS);

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
    uam_mpi_to_padded_buf((unsigned char *)rbuf, 4, g);
    rbuf += 4;
    *rbuflen += 4;
    /* len = length of p = PRIMEBITS/8 */
    uint16 = htons((uint16_t) PRIMEBITS / 8);
    memcpy(rbuf, &uint16, sizeof(uint16_t));
    rbuf += 2;
    *rbuflen += 2;
    /* p */
    uam_mpi_to_padded_buf((unsigned char *)rbuf, PRIMEBITS / 8, p);
    rbuf += PRIMEBITS / 8;
    *rbuflen += PRIMEBITS / 8;
    /* Ma */
    uam_mpi_to_padded_buf((unsigned char *)rbuf, PRIMEBITS / 8, Ma);
    rbuf += PRIMEBITS / 8;
    *rbuflen += PRIMEBITS / 8;
    ret = AFPERR_AUTHCONT;
error:              /* We exit here anyway */
    /* We will only need p and Ra later, but mustn't forget to release it ! */
    gcry_mpi_release(g);
    gcry_mpi_release(Ma);
    return ret;
}

/* -------------------------------- */
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

/* -------------------------------- */
/*!
 * @brief dhx login
 * @note things are done in a slightly bizarre order to avoid
 * having to clean things up if there's an error.
 */
static int passwd_login(void *obj, struct passwd **uam_pwd,
                        char *ibuf, size_t ibuflen,
                        char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t ulen;
    *rbuflen = 0;

    /* grab some of the options */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
        LOG(log_info, logtype_uams,
            "DHX2: uam_afpserver_option didn't meet uam_option_username -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    if (uam_extract_username_v1(&ibuf, &ibuflen, username, ulen) < 0) {
        LOG(log_info, logtype_uams,
            "DHX2: malformed username in login packet");
        return AFPERR_PARAM;
    }

    return login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/* ----------------------------- */
static int passwd_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
                            char *ibuf, size_t ibuflen,
                            char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t ulen;
    *rbuflen = 0;

    /* grab some of the options */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
        LOG(log_info, logtype_uams,
            "DHX2: uam_afpserver_option didn't meet uam_option_username -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    if (uam_extract_username_v2(uname, username, ulen) < 0) {
        LOG(log_info, logtype_uams,
            "DHX2: malformed username in ext login packet");
        return AFPERR_PARAM;
    }

    return login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/* -------------------------------- */

static int logincont1(void *obj _U_, struct passwd **uam_pwd _U_,
                      char *ibuf, size_t ibuflen,
                      char *rbuf, size_t *rbuflen)
{
    int ret;
    gcry_mpi_t Mb, K, clientNonce;
    unsigned char *K_bin = NULL;
    char serverNonce_bin[16];
    uint16_t uint16;
    *rbuflen = 0;
    Mb = gcry_mpi_new(0);
    K = gcry_mpi_new(0);
    clientNonce = gcry_mpi_new(0);
    serverNonce = gcry_mpi_new(0);

    /* Packet size should be: Session ID + Ma + Encrypted client nonce */
    if (ibuflen != 2 + PRIMEBITS / 8 + 16) {
        LOG(log_error, logtype_uams, "DHX2: Packet length not correct");
        ret = AFPERR_PARAM;
        goto error_release;
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
        goto error_release;
    }

    uam_mpi_to_padded_buf(K_bin, PRIMEBITS / 8, K);
    /* ... generate the MD5 hash of K. K_MD5hash is what we actually use ! */
    K_MD5hash = calloc(1, K_hash_len = gcry_md_get_algo_dlen(GCRY_MD_MD5));

    if (K_MD5hash == NULL) {
        ret = AFPERR_MISC;
        free(K_bin);
        K_bin = NULL;
        goto error_release;
    }

    gcry_md_hash_buffer(GCRY_MD_MD5, K_MD5hash, K_bin, PRIMEBITS / 8);
    free(K_bin);
    K_bin = NULL;
    /* FIXME: To support the Reconnect UAM, we need to store this key somewhere */

    /* Decrypt client's md5_K(client nonce, C2SIV) inplace */
    if (uam_cast5_cbc_decrypt((unsigned char *)K_MD5hash, K_hash_len,
                              dhx_c2siv, sizeof(dhx_c2siv),
                              (unsigned char *)ibuf, 16,
                              NULL, 0) != 0) {
        ret = AFPERR_MISC;
        goto error_release;
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
    uam_mpi_to_padded_buf((unsigned char *)rbuf, 16, clientNonce);
    /* Server nonce */
    memcpy(rbuf + 16, serverNonce_bin, 16);

    /* Encrypt md5_K(clientNonce+1, serverNonce) inplace */
    if (uam_cast5_cbc_encrypt((unsigned char *)K_MD5hash, K_hash_len,
                              dhx_s2civ, sizeof(dhx_s2civ),
                              (unsigned char *)rbuf, 32,
                              NULL, 0) != 0) {
        ret = AFPERR_MISC;
        goto error_release;
    }

    rbuf += 32;
    *rbuflen += 32;
    ret = AFPERR_AUTHCONT;
    goto exit;
error_release:
    gcry_mpi_release(serverNonce);
    free(K_MD5hash);
    K_MD5hash = NULL;
exit:
    gcry_mpi_release(K);
    gcry_mpi_release(Mb);
    gcry_mpi_release(Ra);
    gcry_mpi_release(p);
    gcry_mpi_release(clientNonce);
    return ret;
}

static int logincont2(void *obj _U_, struct passwd **uam_pwd,
                      char *ibuf, size_t ibuflen,
                      char *rbuf _U_, size_t *rbuflen)
{
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */
    int ret;
    char *p;
    gcry_mpi_t retServerNonce;
    *rbuflen = 0;
    retServerNonce = gcry_mpi_new(0);

    /* Make sure logincont1 actually ran and established the shared key */
    if (K_MD5hash == NULL) {
        LOG(log_error, logtype_uams,
            "DHX2: logincont2 called without completing logincont1");
        ret = AFPERR_PARAM;
        goto exit;
    }

    /* Packet size should be: Session ID + ServerNonce + Passwd buffer (evantually +10 extra bytes, see Apples Docs)*/
    if ((ibuflen != 2 + 16 + 256) && (ibuflen != 2 + 16 + 256 + 10)) {
        LOG(log_error, logtype_uams,
            "DHX2: Packet length not correct: %d. Should be 274 or 284.", ibuflen);
        ret = AFPERR_PARAM;
        goto exit;
    }

    /* Skip Session ID */
    ibuf += 2;

    /* Decrypt client's md5_K(serverNonce+1, password) inplace */
    if (uam_cast5_cbc_decrypt((unsigned char *)K_MD5hash, K_hash_len,
                              dhx_c2siv, sizeof(dhx_c2siv),
                              (unsigned char *)ibuf, 16 + 256,
                              NULL, 0) != 0) {
        ret = AFPERR_MISC;
        goto exit;
    }

    /* Pull out nonce. Should be serverNonce+1 */
    gcry_mpi_scan(&retServerNonce, GCRYMPI_FMT_USG, ibuf, 16, NULL);
    gcry_mpi_sub_ui(retServerNonce, retServerNonce, 1);

    if (gcry_mpi_cmp(serverNonce, retServerNonce) != 0) {
        /* We're hacked!  */
        ret = AFPERR_NOTAUTH;
        goto exit;
    }

    ibuf += 16;         /* ibuf now point to passwd in cleartext */
    /* ---- Start authentication --- */
    ret = AFPERR_NOTAUTH;
#ifdef HAVE_CRYPT_CHECKPASS

    if (crypt_checkpass(ibuf, dhxpwd->pw_passwd) == 0) {
#else
    p = crypt(ibuf, dhxpwd->pw_passwd);

    if (strcmp(p, dhxpwd->pw_passwd) == 0) {
#endif
        *uam_pwd = dhxpwd;
        ret = AFP_OK;
    }

    explicit_bzero(ibuf, PASSWDLEN);
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
exit:
    free(K_MD5hash);
    K_MD5hash = NULL;
    gcry_mpi_release(serverNonce);
    gcry_mpi_release(retServerNonce);
    return ret;
}

static int passwd_logincont(void *obj, struct passwd **uam_pwd,
                            char *ibuf, size_t ibuflen,
                            char *rbuf, size_t *rbuflen)
{
    uint16_t retID;
    int ret;
    /* check for session id */
    memcpy(&retID, ibuf, sizeof(uint16_t));
    retID = ntohs(retID);

    if (retID == ID) {
        ret = logincont1(obj, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
    } else if (retID == ID + 1) {
        ret = logincont2(obj, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
    } else {
        LOG(log_info, logtype_uams, "DHX2: Session ID Mismatch");
        ret = AFPERR_PARAM;
    }

    return ret;
}

static int uam_setup(void *obj, const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "DHX2", passwd_login,
                     passwd_logincont, NULL, passwd_login_ext) < 0) {
        return -1;
    }

    return 0;
}

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
