/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <pwd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif /* ! HAVE_CRYPT_H */

#ifdef SHADOWPW
#include <shadow.h>
#endif /* SHADOWPW */

#include <gcrypt.h>

#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/uam.h>

#define KEYSIZE 16
#define PASSWDLEN 64
#define CRYPTBUFLEN  (KEYSIZE*2)
#define CRYPT2BUFLEN (KEYSIZE + PASSWDLEN)

/* hash a number to a 16-bit quantity */
#define dhxhash(a) ((((unsigned long) (a) >> 8) ^ \
		     (unsigned long) (a)) & 0xffff)

/* the secret key */
gcry_mpi_t K;
static struct passwd *dhxpwd;
static uint8_t randbuf[16];

/* dhx passwd */
static int pwd_login(void *obj, char *username, int ulen,
                     struct passwd **uam_pwd _U_,
                     char *ibuf, size_t ibuflen _U_,
                     char *rbuf, size_t *rbuflen)
{
    unsigned char iv[] = "CJalbert";
    static const unsigned char p_binary[] = {0xBA, 0x28, 0x73, 0xDF, 0xB0, 0x60, 0x57, 0xD4,
                                             0x3F, 0x20, 0x24, 0x74, 0x4C, 0xEE, 0xE7, 0x5B
                                            };
    static const unsigned char g_binary[] = { 0x07 };
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */
    uint16_t sessid;
    size_t nwritten;
    size_t i;

    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        LOG(log_error, logtype_uams,
            "uams_dhx_passwd.c: libgcrypt versions mismatch. Needs: %s Has: %s",
            UAM_NEED_LIBGCRYPT_VERSION, gcry_check_version(NULL));
        return AFPERR_MISC;
    }

    gcry_mpi_t p, g, Rb, Ma, Mb;
    p = gcry_mpi_new(0);
    g = gcry_mpi_new(0);
    Rb = gcry_mpi_new(0);
    Ma = gcry_mpi_new(0);
    Mb = gcry_mpi_new(0);
    K = gcry_mpi_new(0);
    unsigned char Rb_binary[32], K_binary[16];
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    *rbuflen = 0;

    if ((dhxpwd = uam_getname(obj, username, ulen)) == NULL) {
        return AFPERR_NOTAUTH;
    }

    LOG(log_info, logtype_uams, "dhx login: %s", username);

    if (uam_checkuser(dhxpwd) < 0) {
        return AFPERR_NOTAUTH;
    }

#ifdef SHADOWPW

    if ((sp = getspnam(dhxpwd->pw_name)) == NULL) {
        LOG(log_info, logtype_uams, "no shadow passwd entry for %s", username);
        return AFPERR_NOTAUTH;
    }

    dhxpwd->pw_passwd = sp->sp_pwdp;
#endif /* SHADOWPW */

    if (!dhxpwd->pw_passwd) {
        return AFPERR_NOTAUTH;
    }

    /* Extract Ma, client's "public" key */
    gcry_mpi_scan(&Ma, GCRYMPI_FMT_USG, ibuf, KEYSIZE, NULL);
    /* Get p and g into a form that libgcrypt can use */
    gcry_mpi_scan(&p, GCRYMPI_FMT_USG, p_binary, sizeof(p_binary), NULL);
    gcry_mpi_scan(&g, GCRYMPI_FMT_USG, g_binary, sizeof(g_binary), NULL);
    /* Get random bytes for Rb. */
    gcry_randomize(Rb_binary, sizeof(Rb_binary), GCRY_STRONG_RANDOM);
    /* Translate the binary form of Rb into libgcrypt's preferred form */
    gcry_mpi_scan(&Rb, GCRYMPI_FMT_USG, Rb_binary, sizeof(Rb_binary), NULL);
    /* Mb = g^Rb mod p <- This is our "public" key, which we exchange
     * with the client to help make K, the session key. */
    gcry_mpi_powm(Mb, g, Rb, p);
    /* K = Ma^Rb mod p <- This nets us the "session key", which we
     * actually use to encrypt and decrypt data. */
    gcry_mpi_powm(K, Ma, Rb, p);
    /* Clean up */
    gcry_mpi_release(p);
    gcry_mpi_release(g);
    gcry_mpi_release(Ma);
    gcry_mpi_release(Rb);
    gcry_mpi_print(GCRYMPI_FMT_USG, K_binary, sizeof(K_binary), &i, K);

    if (i < KEYSIZE) {
        memmove(K_binary + sizeof(K_binary) - i, K_binary, i);
        memset(K_binary, 0, sizeof(K_binary) - i);
    }

    /* session id. it's just a hashed version of the object pointer. */
    sessid = dhxhash(obj);
    memcpy(rbuf, &sessid, sizeof(sessid));
    rbuf += sizeof(sessid);
    *rbuflen += sizeof(sessid);
    gcry_mpi_print(GCRYMPI_FMT_USG, (unsigned char *) rbuf, KEYSIZE, &nwritten, Mb);

    if (nwritten < KEYSIZE) {
        memmove(rbuf + KEYSIZE - nwritten, rbuf, nwritten);
        memset(rbuf, 0, KEYSIZE - nwritten);
    }

    rbuf += KEYSIZE;
    *rbuflen += KEYSIZE;
    gcry_mpi_release(Mb);
    /* buffer to be encrypted */
    i = sizeof(randbuf);

    if (uam_afpserver_option(obj, UAM_OPTION_RANDNUM, (void *)randbuf,
                             &i) < 0) {
        *rbuflen = 0;
        goto passwd_fail;
    }

    memcpy(rbuf, &randbuf, sizeof(randbuf));
    /* get the signature. it's always 16 bytes. */
#if 0

    if (uam_afpserver_option(obj, UAM_OPTION_SIGNATURE,
                             (void *)&name, NULL) < 0) {
        *rbuflen = 0;
        goto passwd_fail;
    }

    memcpy(rbuf + KEYSIZE, name, KEYSIZE);
#else /* 0 */
    memset(rbuf + KEYSIZE, 0, KEYSIZE);
#endif /* 0 */
    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
                                GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto passwd_fail;
    }

    /* Set the binary form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_binary, sizeof(K_binary));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto passwd_fail;
    }

    /* Set the initialization vector for server->client transfer. */
    ctxerror = gcry_cipher_setiv(ctx, iv, sizeof(iv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto passwd_fail;
    }

    /* Encrypt the ciphertext from the server. */
    ctxerror = gcry_cipher_encrypt(ctx, rbuf, CRYPTBUFLEN, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto passwd_fail;
    }

    *rbuflen += CRYPTBUFLEN;
    gcry_cipher_close(ctx);
    return AFPERR_AUTHCONT;
passwd_fail:
    gcry_mpi_release(K);
    return AFPERR_PARAM;
}

/* cleartxt login */
static int passwd_login(void *obj, struct passwd **uam_pwd,
                        char *ibuf, size_t ibuflen,
                        char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t len, ulen;
    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *) &username, &ulen) < 0) {
        return AFPERR_MISC;
    }

    if (ibuflen < 2) {
        return AFPERR_PARAM;
    }

    len = (unsigned char) * ibuf++;
    ibuflen--;

    if (!len || len > ibuflen || len > ulen) {
        return AFPERR_PARAM;
    }

    memcpy(username, ibuf, len);
    ibuf += len;
    ibuflen -= len;
    username[len] = '\0';

    if ((unsigned long) ibuf & 1) { /* pad character */
        ++ibuf;
        ibuflen--;
    }

    return pwd_login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/* cleartxt login ext
 * uname format :
    byte      3
    2 bytes   len (network order)
    len bytes utf8 name
*/
static int passwd_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
                            char *ibuf, size_t ibuflen,
                            char *rbuf, size_t *rbuflen)
{
    char       *username;
    size_t     len, ulen;
    uint16_t  temp16;
    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *) &username, &ulen) < 0) {
        return AFPERR_MISC;
    }

    if (*uname != 3) {
        return AFPERR_PARAM;
    }

    uname++;
    memcpy(&temp16, uname, sizeof(temp16));
    len = ntohs(temp16);

    if (!len || len > ulen) {
        return AFPERR_PARAM;
    }

    memcpy(username, uname + 2, len);
    username[len] = '\0';
    return pwd_login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

static int passwd_logincont(void *obj, struct passwd **uam_pwd,
                            char *ibuf, size_t ibuflen _U_,
                            char *rbuf, size_t *rbuflen)
{
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */
    unsigned char iv[] = "LWallace";
    gcry_mpi_t bn1, bn2, bn3;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    unsigned char K_binary[16];
    size_t i;
    uint16_t sessid;
    char *p;
    int err = AFPERR_NOTAUTH;
    *rbuflen = 0;
    /* check for session id */
    memcpy(&sessid, ibuf, sizeof(sessid));

    if (sessid != dhxhash(obj)) {
        /* Log Entry */
        LOG(log_info, logtype_uams,
            "uams_dhx_passwd.c :passwd Session ID - DHXHash Mismatch -- %s",
            strerror(errno));
        /* Log Entry */
        return AFPERR_PARAM;
    }

    ibuf += sizeof(sessid);
    gcry_mpi_print(GCRYMPI_FMT_USG, K_binary, sizeof(K_binary), &i, K);

    if (i < KEYSIZE) {
        memmove(K_binary + sizeof(K_binary) - i, K_binary, i);
        memset(K_binary, 0, sizeof(K_binary) - i);
    }

    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
                                GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        return AFPERR_PARAM;
    }

    /* Set the binary form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_binary, sizeof(K_binary));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        return AFPERR_PARAM;
    }

    /* Set the initialization vector for client->server transfer. */
    ctxerror = gcry_cipher_setiv(ctx, iv, sizeof(iv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        return AFPERR_PARAM;
    }

    /* Decrypt the ciphertext from the client. */
    ctxerror = gcry_cipher_decrypt(ctx, rbuf, CRYPT2BUFLEN, ibuf, CRYPT2BUFLEN);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        return AFPERR_PARAM;
    }

    gcry_cipher_close(ctx);
    bn1 = gcry_mpi_snew(KEYSIZE);
    gcry_mpi_scan(&bn1, GCRYMPI_FMT_STD, rbuf, KEYSIZE, NULL);
    bn2 = gcry_mpi_snew(sizeof(randbuf));
    gcry_mpi_scan(&bn2, GCRYMPI_FMT_STD, randbuf, sizeof(randbuf), NULL);
    /* zero out the random number */
    memset(rbuf, 0, sizeof(randbuf));
    memset(randbuf, 0, sizeof(randbuf));
    rbuf += KEYSIZE;
    bn3 = gcry_mpi_snew(0);
    gcry_mpi_sub(bn3, bn1, bn2);
    gcry_mpi_release(bn2);
    gcry_mpi_release(bn1);

    if (gcry_mpi_cmp_ui(bn3, 1UL) != 0) {
        gcry_mpi_release(bn3);
        return AFPERR_PARAM;
    }

    gcry_mpi_release(bn3);
    rbuf[PASSWDLEN] = '\0';
#ifdef HAVE_CRYPT_CHECKPASS

    if (crypt_checkpass(rbuf, dhxpwd->pw_passwd) == 0) {
#else
    p = crypt(rbuf, dhxpwd->pw_passwd);

    if (strcmp(p, dhxpwd->pw_passwd) == 0) {
#endif
        *uam_pwd = dhxpwd;
        err = AFP_OK;
    }

    memset(rbuf, 0, PASSWDLEN);
#ifdef SHADOWPW

    if ((sp = getspnam(dhxpwd->pw_name)) == NULL) {
        LOG(log_info, logtype_uams, "no shadow passwd entry for %s", dhxpwd->pw_name);
        return AFPERR_NOTAUTH;
    }

    /* check for expired password */
    if (sp && sp->sp_max != -1 && sp->sp_lstchg) {
        time_t now = time(NULL) / (60 * 60 * 24);
        int32_t expire_days = sp->sp_lstchg - now + sp->sp_max;

        if (expire_days < 0) {
            LOG(log_info, logtype_uams, "password for user %s expired", dhxpwd->pw_name);
            err = AFPERR_PWDEXPR;
        }
    }

#endif /* SHADOWPW */
    return err;
    return AFPERR_NOTAUTH;
}


static int uam_setup(void *obj, const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "DHCAST128",
                     passwd_login, passwd_logincont, NULL, passwd_login_ext) < 0) {
        return -1;
    }

    /*uam_register(UAM_SERVER_PRINTAUTH, path, "DHCAST128",
      passwd_printer);*/
    return 0;
}

static void uam_cleanup(void)
{
    uam_unregister(UAM_SERVER_LOGIN, "DHCAST128");
    /*uam_unregister(UAM_SERVER_PRINTAUTH, "DHCAST128"); */
}

UAM_MODULE_EXPORT struct uam_export uams_dhx = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};

UAM_MODULE_EXPORT struct uam_export uams_dhx_passwd = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};
