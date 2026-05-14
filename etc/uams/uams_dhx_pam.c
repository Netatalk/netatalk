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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#endif

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif

#include <gcrypt.h>

#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/uam.h>

#include "uam_common.h"

#define KEYSIZE 16
#define PASSWDLEN 64
#define CRYPTBUFLEN  (KEYSIZE*2)
#define CRYPT2BUFLEN (KEYSIZE + PASSWDLEN)
#define CHANGEPWBUFLEN (KEYSIZE + 2*PASSWDLEN)

/*! hash a number to a 16-bit quantity */
#define dhxhash(a) ((((unsigned long) (a) >> 8) ^ \
		     (unsigned long) (a)) & 0xffff)

/*! the secret key */
gcry_mpi_t K;

static struct passwd *dhxpwd;
static uint8_t randbuf[KEYSIZE];

/* diffie-hellman bits */
static unsigned char msg2_iv[] = "CJalbert";
static unsigned char msg3_iv[] = "LWallace";
static const unsigned char p_binary[] = {0xBA, 0x28, 0x73, 0xDF, 0xB0, 0x60, 0x57, 0xD4,
                                         0x3F, 0x20, 0x24, 0x74, 0x4C, 0xEE, 0xE7, 0x5B
                                        };
static const unsigned char g_binary[] = {0x07};


/* Username is captured during pam_login() and consulted again during the
 * later pam_logincont() round, so it has to survive across calls.  The
 * password lives only within a single function and is passed to PAM via
 * a stack-local struct uam_pam_ctx. */
static pam_handle_t *pamh = NULL;
static unsigned char *PAM_username;


static int dhx_setup(void *obj, const unsigned char *ibuf, size_t ibuflen _U_,
                     unsigned char *rbuf, size_t *rbuflen)
{
    uint16_t sessid;
    size_t i;

    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        LOG(log_error, logtype_uams,
            "uams_dhx_pam.c: libgcrypt versions mismatch. Needs: %s Has: %s",
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
    uam_mpi_to_padded_buf(K_binary, sizeof(K_binary), K);
    /* session id. it's just a hashed version of the object pointer. */
    sessid = dhxhash(obj);
    memcpy(rbuf, &sessid, sizeof(sessid));
    rbuf += sizeof(sessid);
    *rbuflen += sizeof(sessid);
    uam_mpi_to_padded_buf(rbuf, KEYSIZE, Mb);
    rbuf += KEYSIZE;
    *rbuflen += KEYSIZE;
    gcry_mpi_release(Mb);
    /* buffer to be encrypted */
    i = sizeof(randbuf);

    if (uam_afpserver_option(obj, UAM_OPTION_RANDNUM, (void *) randbuf,
                             &i) < 0) {
        *rbuflen = 0;
        /* Log Entry */
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Buffer Encryption Err. -- %s",
            strerror(errno));
        /* Log Entry */
        goto pam_fail;
    }

    memcpy(rbuf, &randbuf, sizeof(randbuf));
    /* get the signature. it's always 16 bytes. */
#if 0

    if (uam_afpserver_option(obj, UAM_OPTION_SIGNATURE,
                             (void *) &buf, NULL) < 0) {
        *rbuflen = 0;
        /* Log Entry */
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: Signature Retieval Failure -- %s",
            strerror(errno));
        /* Log Entry */
        goto pam_fail;
    }

    memcpy(rbuf + KEYSIZE, buf, KEYSIZE);
#else /* 0 */
    memset(rbuf + KEYSIZE, 0, KEYSIZE);
#endif /* 0 */

    if (uam_cast5_cbc_encrypt(K_binary, sizeof(K_binary),
                              msg2_iv, sizeof(msg2_iv),
                              rbuf, CRYPTBUFLEN,
                              NULL, 0) != 0) {
        goto pam_fail;
    }

    *rbuflen += CRYPTBUFLEN;
    return AFPERR_AUTHCONT;
pam_fail:
    gcry_mpi_release(K);
    K = NULL;
    /* Log Entry */
    LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Fail - Cast Encryption -- %s",
        strerror(errno));
    /* Log Entry */
    return AFPERR_PARAM;
}

/* -------------------------------- */
static int login(void *obj, unsigned char *username, int ulen,
                 struct passwd **uam_pwd _U_, const unsigned char *ibuf, size_t ibuflen,
                 unsigned char *rbuf, size_t *rbuflen)
{
    if ((dhxpwd = uam_getname(obj, (char *)username, ulen)) == NULL) {
        LOG(log_info, logtype_uams, "uams_dhx_pam.c: unknown username [%s]", username);
        return AFPERR_NOTAUTH;
    }

    PAM_username = username;
    LOG(log_info, logtype_uams, "dhx login: %s", username);
    return dhx_setup(obj, ibuf, ibuflen, rbuf, rbuflen);
}

/* -------------------------------- */
/*!
 * @brief dhx login
 * @note things are done in a slightly bizarre order to avoid
 * having to clean things up if there's an error.
 */
static int pam_login(void *obj, struct passwd **uam_pwd,
                     unsigned char *ibuf, size_t ibuflen,
                     unsigned char *rbuf, size_t *rbuflen)
{
    unsigned char *username;
    size_t ulen;
    char *cibuf = (char *)ibuf;
    *rbuflen = 0;

    /* grab some of the options */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: uam_afpserver_option didn't meet uam_option_username -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    if (uam_extract_username_v1(&cibuf, &ibuflen,
                                (char *)username, ulen) < 0) {
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: malformed username in login packet");
        return AFPERR_PARAM;
    }

    return login(obj, username, ulen, uam_pwd,
                 (const unsigned char *)cibuf, ibuflen, rbuf, rbuflen);
}

/* ----------------------------- */
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
                         const unsigned char *ibuf, size_t ibuflen,
                         unsigned char *rbuf, size_t *rbuflen)
{
    unsigned char *username;
    size_t ulen;
    *rbuflen = 0;

    /* grab some of the options */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: uam_afpserver_option didn't meet uam_option_username -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    if (uam_extract_username_v2(uname, (char *)username, ulen) < 0) {
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: malformed username in ext login packet");
        return AFPERR_PARAM;
    }

    return login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/* -------------------------------- */

static int pam_logincont(void *obj, struct passwd **uam_pwd,
                         const unsigned char *ibuf, size_t ibuflen _U_,
                         unsigned char *rbuf, size_t *rbuflen)
{
    const char *hostname;
    gcry_mpi_t bn1, bn2, bn3;
    uint16_t sessid;
    int err, PAM_error;
    unsigned char K_binary[16];
    struct uam_pam_ctx pam_ctx = {0};
    struct pam_conv pam_conv_local;
    *rbuflen = 0;

    /* Make sure dhx_setup actually ran and established the shared key */
    if (K == NULL) {
        LOG(log_error, logtype_uams, "DHX: logincont called without completing login");
        return AFPERR_PARAM;
    }

    /* check for session id */
    memcpy(&sessid, ibuf, sizeof(sessid));

    if (sessid != dhxhash(obj)) {
        /* Log Entry */
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM Session ID - DHXHash Mismatch -- %s",
            strerror(errno));
        /* Log Entry */
        return AFPERR_PARAM;
    }

    ibuf += sizeof(sessid);

    if (uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME,
                             (void *) &hostname, NULL) < 0) {
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: unable to retrieve client hostname");
        hostname = NULL;
    }

    uam_mpi_to_padded_buf(K_binary, sizeof(K_binary), K);
    gcry_mpi_release(K);
    K = NULL;

    if (uam_cast5_cbc_decrypt(K_binary, sizeof(K_binary),
                              msg3_iv, sizeof(msg3_iv),
                              rbuf, CRYPT2BUFLEN,
                              ibuf, CRYPT2BUFLEN) != 0) {
        return AFPERR_PARAM;
    }

    bn1 = gcry_mpi_snew(KEYSIZE);
    gcry_mpi_scan(&bn1, GCRYMPI_FMT_STD, rbuf, KEYSIZE, NULL);
    bn2 = gcry_mpi_snew(sizeof(randbuf));
    gcry_mpi_scan(&bn2, GCRYMPI_FMT_STD, randbuf, sizeof(randbuf), NULL);
    /* zero out the random number */
    explicit_bzero(rbuf, sizeof(randbuf));
    explicit_bzero(randbuf, sizeof(randbuf));
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
    /* Set these things up for the conv function */
    rbuf[PASSWDLEN] = '\0';
    pam_ctx.username = (const char *)PAM_username;
    pam_ctx.password = (const char *)rbuf;
    pam_ctx.log_tag = "uams_dhx_pam";
    pam_conv_local.conv = uam_pam_conv;
    pam_conv_local.appdata_ptr = &pam_ctx;
    err = AFPERR_NOTAUTH;
    PAM_error = pam_start("netatalk", (const char *)PAM_username,
                          &pam_conv_local, &pamh);

    if (PAM_error != PAM_SUCCESS) {
        /* Log Entry */
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM_Error: %s",
            pam_strerror(pamh, PAM_error));
        /* Log Entry */
        goto logincont_err;
    }

    /* solaris craps out if PAM_TTY and PAM_RHOST aren't set. */
    pam_set_item(pamh, PAM_TTY, "afpd");
    pam_set_item(pamh, PAM_RHOST, hostname);
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
        if (sigchld_saved) {
            sigaction(SIGCHLD, &sa_old, NULL);
        }

        if (PAM_error == PAM_MAXTRIES) {
            err = AFPERR_PWDEXPR;
        }

        /* Log Entry */
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM_Error: %s",
            pam_strerror(pamh, PAM_error));
        /* Log Entry */
        goto logincont_err;
    }

    PAM_error = pam_acct_mgmt(pamh, 0);

    if (sigchld_saved) {
        sigaction(SIGCHLD, &sa_old, NULL);
    }

    if (PAM_error != PAM_SUCCESS) {
        /* Log Entry */
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM_Error: %s",
            pam_strerror(pamh, PAM_error));

        /* Log Entry */
        if (PAM_error == PAM_NEW_AUTHTOK_REQD) {
            /* password expired */
            err = AFPERR_PWDEXPR;
        }

#ifdef PAM_AUTHTOKEN_REQD
        else if (PAM_error == PAM_AUTHTOKEN_REQD) {
            err = AFPERR_PWDCHNG;
        }

#endif
        else {
            goto logincont_err;
        }
    }

#ifndef PAM_CRED_ESTABLISH
#define PAM_CRED_ESTABLISH PAM_ESTABLISH_CRED
#endif
    PAM_error = pam_setcred(pamh, PAM_CRED_ESTABLISH);

    if (PAM_error != PAM_SUCCESS) {
        /* Log Entry */
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM_Error: %s",
            pam_strerror(pamh, PAM_error));
        /* Log Entry */
        goto logincont_err;
    }

    PAM_error = pam_open_session(pamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        /* Log Entry */
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM_Error: %s",
            pam_strerror(pamh, PAM_error));
        /* Log Entry */
        goto logincont_err;
    }

    explicit_bzero(rbuf, PASSWDLEN); /* zero out the password */
    *uam_pwd = dhxpwd;
    /* Log Entry */
    LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM Auth OK!");

    /* Log Entry */
    if (err == AFPERR_PWDEXPR) {
        return err;
    }

    return AFP_OK;
logincont_err:
    pam_end(pamh, PAM_error);
    pamh = NULL;
    explicit_bzero(rbuf, CRYPT2BUFLEN);
    return err;
}

/* logout */
static void pam_logout(void)
{
    pam_close_session(pamh, 0);
    pam_end(pamh, 0);
    pamh = NULL;
}


/*!
 * @brief change pw for dhx
 * @note needs a couple passes to get everything all right.
 * basically, it's like the login/logincont sequence
 */
static int pam_changepw(void *obj, unsigned char *username,
                        const struct passwd *pwd _U_, unsigned char *ibuf,
                        size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
{
    gcry_mpi_t bn1, bn2, bn3;
    unsigned char K_binary[16];
    char *hostname;
    pam_handle_t *lpamh;
    uid_t uid;
    uint16_t sessid;
    int PAM_error;
    struct uam_pam_ctx pam_ctx = {0};
    struct pam_conv pam_conv_local;

    if (ibuflen < sizeof(sessid)) {
        return AFPERR_PARAM;
    }

    /* grab the id */
    memcpy(&sessid, ibuf, sizeof(sessid));
    ibuf += sizeof(sessid);

    if (!sessid) {  /* no sessid -> initialization phase */
        PAM_username = username;
        ibuflen -= sizeof(sessid);
        return dhx_setup(obj, ibuf, ibuflen, rbuf, rbuflen);
    }

    /* otherwise, it's like logincont but different. */

    /* Make sure dhx_setup actually ran and established the shared key */
    if (K == NULL) {
        LOG(log_error, logtype_uams,
            "DHX: changepw called without completing key exchange");
        return AFPERR_PARAM;
    }

    /* check out the session id */
    if (sessid != dhxhash(obj)) {
        /* Log Entry */
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: Session ID not Equal to DHX Hash -- %s",
            strerror(errno));
        /* Log Entry */
        return AFPERR_PARAM;
    }

    /* we need this for pam */
    if (uam_afpserver_option(obj, UAM_OPTION_HOSTNAME,
                             (void *) &hostname, NULL) < 0) {
        /* Log Entry */
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Hostname Null?? -- %s",
            strerror(errno));
        /* Log Entry */
        return AFPERR_MISC;
    }

    uam_mpi_to_padded_buf(K_binary, sizeof(K_binary), K);
    gcry_mpi_release(K);
    K = NULL;

    if (uam_cast5_cbc_decrypt(K_binary, sizeof(K_binary),
                              msg3_iv, sizeof(msg3_iv),
                              ibuf, CHANGEPWBUFLEN,
                              NULL, 0) != 0) {
        return AFPERR_PARAM;
    }

    bn1 = gcry_mpi_snew(KEYSIZE);
    gcry_mpi_scan(&bn1, GCRYMPI_FMT_STD, ibuf, KEYSIZE, NULL);
    bn2 = gcry_mpi_snew(sizeof(randbuf));
    gcry_mpi_scan(&bn2, GCRYMPI_FMT_STD, randbuf, sizeof(randbuf), NULL);
    /* zero out the random number */
    explicit_bzero(ibuf, sizeof(randbuf));
    explicit_bzero(randbuf, sizeof(randbuf));
    bn3 = gcry_mpi_snew(0);
    gcry_mpi_sub(bn3, bn1, bn2);
    gcry_mpi_release(bn2);
    gcry_mpi_release(bn1);

    if (gcry_mpi_cmp_ui(bn3, 1UL) != 0) {
        gcry_mpi_release(bn3);
        return AFPERR_PARAM;
    }

    gcry_mpi_release(bn3);
    /* Set these things up for the conv function. the old password
     * is at the end. */
    ibuf += KEYSIZE;

    if (ibuflen <= PASSWDLEN + PASSWDLEN) {
        return AFPERR_PARAM;
    }

    ibuf[PASSWDLEN + PASSWDLEN] = '\0';
    pam_ctx.username = (const char *)username;
    pam_ctx.password = (const char *)(ibuf + PASSWDLEN); /* old password */
    pam_ctx.log_tag = "uams_dhx_pam";
    pam_conv_local.conv = uam_pam_conv;
    pam_conv_local.appdata_ptr = &pam_ctx;
    PAM_error = pam_start("netatalk", (char *)username, &pam_conv_local,
                          &lpamh);

    if (PAM_error != PAM_SUCCESS) {
        /* Log Entry */
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: Needless to say, PAM_error is != to PAM_SUCCESS -- %s",
            strerror(errno));
        /* Log Entry */
        return AFPERR_PARAM;
    }

    pam_set_item(lpamh, PAM_TTY, "afpd");
    pam_set_item(lpamh, PAM_RHOST, hostname);
    /* we might need to do this as root */
    uid = geteuid();

    if (seteuid(0) < 0) {
        LOG(log_error, logtype_uams, "pam_changepw: could not seteuid(%i)", 0);
    }

    PAM_error = pam_authenticate(lpamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        if (seteuid(uid) < 0) {
            LOG(log_error, logtype_uams, "pam_changepw: could not seteuid(%i)", uid);
        }

        pam_end(lpamh, PAM_error);
        return AFPERR_NOTAUTH;
    }

    PAM_error = pam_acct_mgmt(lpamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        if (seteuid(uid) < 0) {
            LOG(log_error, logtype_uams, "pam_changepw: could not seteuid(%i)", uid);
        }

        pam_end(lpamh, PAM_error);
        return AFPERR_NOTAUTH;
    }

    /* clear out old passwd */
    explicit_bzero(ibuf + PASSWDLEN, PASSWDLEN);
    /* new password — appdata_ptr is still &pam_ctx, so updating
     * pam_ctx.password is what the conv function sees for chauthtok. */
    pam_ctx.password = (const char *)ibuf;

    if (ibuflen <= PASSWDLEN) {
        return AFPERR_PARAM;
    }

    ibuf[PASSWDLEN] = '\0';
    /* this really does need to be done as root */
    PAM_error = pam_chauthtok(lpamh, 0);

    if (seteuid(uid) < 0) {
        LOG(log_error, logtype_uams, "pam_changepw: could not seteuid(%i)", uid);
    }

    explicit_bzero(ibuf, PASSWDLEN);

    if (PAM_error != PAM_SUCCESS) {
        pam_end(lpamh, PAM_error);
        return AFPERR_ACCESS;
    }

    pam_end(lpamh, 0);
    return AFP_OK;
}


static int uam_setup(void *obj _U_, const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "DHCAST128", pam_login,
                     pam_logincont, pam_logout, pam_login_ext) < 0) {
        return -1;
    }

    if (uam_register(UAM_SERVER_CHANGEPW, path, "DHCAST128",
                     pam_changepw) < 0) {
        uam_unregister(UAM_SERVER_LOGIN, "DHCAST128");
        return -1;
    }

#if 0
    uam_register(UAM_SERVER_PRINTAUTH, path, "DHCAST128", pam_printer);
#endif
    return 0;
}

static void uam_cleanup(void)
{
    uam_unregister(UAM_SERVER_LOGIN, "DHCAST128");
    uam_unregister(UAM_SERVER_CHANGEPW, "DHCAST128");
#if 0
    uam_unregister(UAM_SERVER_PRINTAUTH, "DHCAST128");
#endif
}

UAM_MODULE_EXPORT struct uam_export uams_dhx = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};

UAM_MODULE_EXPORT struct uam_export uams_dhx_pam = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};
