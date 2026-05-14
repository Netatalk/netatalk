/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <gcrypt.h>
#include <signal.h>
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
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/uam.h>

#include "uam_common.h"

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

/* The initialization vectors for CAST128 are fixed by Apple. */
static unsigned char dhx_c2siv[] = { 'L', 'W', 'a', 'l', 'l', 'a', 'c', 'e' };
static unsigned char dhx_s2civ[] = { 'C', 'J', 'a', 'l', 'b', 'e', 'r', 't' };

/* PAM_username persists across rounds (login → logincont, or across the
 * three changepw packets).  The password is passed via a stack-local
 * struct uam_pam_ctx at each pam_start site. */
static pam_handle_t *pamh = NULL;
static char *PAM_username;
static struct passwd *dhxpwd;


static int dhx2_setup(void *obj, char *ibuf _U_, size_t ibuflen _U_,
                      char *rbuf, size_t *rbuflen)
{
    int ret;
    gcry_mpi_t Ma;
    char *Ra_binary = NULL;
    uint16_t uint16;
    *rbuflen = 0;
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
    /* We will need Ra later, but mustn't forget to release it ! */
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

    PAM_username = dhxpwd->pw_name;
    LOG(log_info, logtype_uams, "DHX2 login: %s", username);
    return dhx2_setup(obj, ibuf, ibuflen, rbuf, rbuflen);
}

/* -------------------------------- */
/*!
 * @brief dhx login
 * @note things are done in a slightly bizarre order to avoid
 * having to clean things up if there's an error.
 */
static int pam_login(void *obj, struct passwd **uam_pwd,
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
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
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
static int logincont1(void *obj _U_, char *ibuf, size_t ibuflen, char *rbuf,
                      size_t *rbuflen)
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
    gcry_mpi_release(clientNonce);
    return ret;
}


static int logincont2(void *obj_in, struct passwd **uam_pwd,
                      char *ibuf, size_t ibuflen,
                      char *rbuf _U_, size_t *rbuflen)
{
    AFPObj *obj = obj_in;
    int ret = AFPERR_MISC;
    int PAM_error;
    const char *hostname = NULL;
    gcry_mpi_t retServerNonce = NULL;
    char *utfpass = NULL;
    struct uam_pam_ctx pam_ctx = {0};
    struct pam_conv pam_conv_local;
    *rbuflen = 0;

    /* Make sure logincont1 actually ran and established the shared key */
    if (K_MD5hash == NULL) {
        LOG(log_error, logtype_uams,
            "DHX2: logincont2 called without completing logincont1");
        ret = AFPERR_PARAM;
        goto error_exit;
    }

    /* Packet size should be: Session ID + ServerNonce + Passwd buffer (evantually +10 extra bytes, see Apples Docs) */
    if ((ibuflen != 2 + 16 + 256) && (ibuflen != 2 + 16 + 256 + 10)) {
        LOG(log_error, logtype_uams,
            "DHX2: Packet length not correct: %u. Should be 274 or 284.", ibuflen);
        ret = AFPERR_PARAM;
        goto error_exit;
    }

    retServerNonce = gcry_mpi_new(0);
    /* For PAM */
    uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME, (void *) &hostname, NULL);
    /* Skip Session ID */
    ibuf += 2;

    /* Decrypt client's md5_K(serverNonce+1, password) inplace */
    if (uam_cast5_cbc_decrypt((unsigned char *)K_MD5hash, K_hash_len,
                              dhx_c2siv, sizeof(dhx_c2siv),
                              (unsigned char *)ibuf, 16 + 256,
                              NULL, 0) != 0) {
        ret = AFPERR_MISC;
        goto error_exit;
    }

    /* Pull out nonce. Should be serverNonce+1 */
    gcry_mpi_scan(&retServerNonce, GCRYMPI_FMT_USG, ibuf, 16, NULL);
    gcry_mpi_sub_ui(retServerNonce, retServerNonce, 1);

    if (gcry_mpi_cmp(serverNonce, retServerNonce) != 0) {
        /* We're hacked!  */
        ret = AFPERR_NOTAUTH;
        goto error_exit;
    }

    ibuf += 16;

    /* ---- Start authentication with PAM --- */

    /* The password is in legacy Mac encoding, convert it to host encoding */
    if (convert_string_allocate(CH_MAC, CH_UNIX, ibuf, -1,
                                &utfpass) == (size_t) -1) {
        LOG(log_error, logtype_uams, "DHX2: conversion error");
        goto error_exit;
    }

    /* Set these things up for the conv function */
    pam_ctx.username = PAM_username;
    pam_ctx.password = utfpass;
    pam_ctx.log_tag = "uams_dhx2_pam";
    pam_conv_local.conv = uam_pam_conv;
    pam_conv_local.appdata_ptr = &pam_ctx;
    ret = AFPERR_NOTAUTH;
    PAM_error = pam_start("netatalk", PAM_username, &pam_conv_local, &pamh);

    if (PAM_error != PAM_SUCCESS) {
        LOG(log_info, logtype_uams, "DHX2: PAM_Error: %s", pam_strerror(pamh,
                PAM_error));
        goto error_exit;
    }

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
        if (sigchld_saved) {
            sigaction(SIGCHLD, &sa_old, NULL);
        }

        if (PAM_error == PAM_MAXTRIES) {
            ret = AFPERR_PWDEXPR;
        }

        LOG(log_info, logtype_uams, "DHX2: PAM_Error: %s", pam_strerror(pamh,
                PAM_error));
        goto error_exit;
    }

    PAM_error = pam_acct_mgmt(pamh, 0);

    if (sigchld_saved) {
        sigaction(SIGCHLD, &sa_old, NULL);
    }

    if (PAM_error != PAM_SUCCESS) {
        LOG(log_info, logtype_uams, "DHX2: PAM_Error: %s",
            pam_strerror(pamh, PAM_error));

        if (PAM_error == PAM_NEW_AUTHTOK_REQD) {
            /* password expired */
            ret = AFPERR_PWDEXPR;
        }

#ifdef PAM_AUTHTOKEN_REQD
        else if (PAM_error == PAM_AUTHTOKEN_REQD) {
            ret = AFPERR_PWDCHNG;
        }

#endif
        goto error_exit;
    }

#ifndef PAM_CRED_ESTABLISH
#define PAM_CRED_ESTABLISH PAM_ESTABLISH_CRED
#endif
    PAM_error = pam_setcred(pamh, PAM_CRED_ESTABLISH);

    if (PAM_error != PAM_SUCCESS) {
        LOG(log_info, logtype_uams, "DHX2: PAM_Error: %s",
            pam_strerror(pamh, PAM_error));
        goto error_exit;
    }

    PAM_error = pam_open_session(pamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        LOG(log_info, logtype_uams, "DHX2: PAM_Error: %s",
            pam_strerror(pamh, PAM_error));
        goto error_exit;
    }

    explicit_bzero(ibuf, 256); /* zero out the password */

    if (utfpass) {
        explicit_bzero(utfpass, strlen(utfpass));
    }

    *uam_pwd = dhxpwd;
    LOG(log_info, logtype_uams, "DHX2: PAM Auth OK!");
    ret = AFP_OK;
error_exit:

    if (utfpass) {
        free(utfpass);
    }

    free(K_MD5hash);
    K_MD5hash = NULL;
    gcry_mpi_release(serverNonce);
    gcry_mpi_release(retServerNonce);
    return ret;
}

static int pam_logincont(void *obj, struct passwd **uam_pwd,
                         char *ibuf, size_t ibuflen,
                         char *rbuf, size_t *rbuflen)
{
    uint16_t retID;
    int ret;
    /* check for session id */
    memcpy(&retID, ibuf, sizeof(uint16_t));
    retID = ntohs(retID);

    if (retID == ID) {
        ret = logincont1(obj, ibuf, ibuflen, rbuf, rbuflen);
    } else if (retID == ID + 1) {
        ret = logincont2(obj, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
    } else {
        LOG(log_info, logtype_uams, "DHX2: Session ID Mismatch");
        ret = AFPERR_PARAM;
    }

    return ret;
}


/* logout */
static void pam_logout(void)
{
    pam_close_session(pamh, 0);
    pam_end(pamh, 0);
    pamh = NULL;
}

/****************************
 * --- Change pwd stuff --- */

static int changepw_1(void *obj, char *uname,
                      char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    *rbuflen = 0;
    /* Remember it now, use it in changepw_3 */
    PAM_username = uname;
    return dhx2_setup(obj, ibuf, ibuflen, rbuf, rbuflen);
}

static int changepw_2(void *obj,
                      char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    return logincont1(obj, ibuf, ibuflen, rbuf, rbuflen);
}

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
    struct uam_pam_ctx pam_ctx = {0};
    struct pam_conv pam_conv_local;
    *rbuflen = 0;
    LOG(log_error, logtype_uams, "DHX2 ChangePW: packet 3 processing");

    /* Make sure changepw_2 actually ran and established the shared key */
    if (K_MD5hash == NULL) {
        LOG(log_error, logtype_uams,
            "DHX2 ChangePW: called without completing key exchange");
        ret = AFPERR_PARAM;
        goto error_exit;
    }

    /* Packet size should be: Session ID + ServerNonce + 2*Passwd buffer */
    if (ibuflen != 2 + 16 + 2 * 256) {
        LOG(log_error, logtype_uams, "DHX2: Packet length not correct");
        ret = AFPERR_PARAM;
        goto error_exit;
    }

    retServerNonce = gcry_mpi_new(0);
    /* For PAM */
    uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME, (void *) &hostname, NULL);
    /* Skip Session ID */
    ibuf += 2;

    /* Decrypt client's md5_K(serverNonce+1, 2*password) inplace */
    if (uam_cast5_cbc_decrypt((unsigned char *)K_MD5hash, K_hash_len,
                              dhx_c2siv, sizeof(dhx_c2siv),
                              (unsigned char *)ibuf, 16 + 2 * 256,
                              NULL, 0) != 0) {
        ret = AFPERR_MISC;
        goto error_exit;
    }

    /* Pull out nonce. Should be serverNonce+1 */
    gcry_mpi_scan(&retServerNonce, GCRYMPI_FMT_USG, ibuf, 16, NULL);
    gcry_mpi_sub_ui(retServerNonce, retServerNonce, 1);

    if (gcry_mpi_cmp(serverNonce, retServerNonce) != 0) {
        /* We're hacked!  */
        ret = AFPERR_NOTAUTH;
        goto error_exit;
    }

    ibuf += 16;
    /* ---- Start pwd changing with PAM --- */
    ibuf[255] = '\0';       /* For safety */
    ibuf[511] = '\0';

    /* check if new and old password are equal */
    if (memcmp(ibuf, ibuf + 256, 255) == 0) {
        LOG(log_info, logtype_uams, "DHX2 Chgpwd: new and old password are equal");
        ret = AFPERR_PWDSAME;
        goto error_exit;
    }

    /* Set these things up for the conv function. PAM_username was set in changepw_1.
     * The conv function answers ECHO_OFF with pam_ctx.password — we point it at
     * the old password for authenticate, then swap to the new password for
     * chauthtok further down. */
    pam_ctx.username = PAM_username;
    pam_ctx.password = ibuf + 256;    /* old password */
    pam_ctx.log_tag = "uams_dhx2_pam";
    pam_conv_local.conv = uam_pam_conv;
    pam_conv_local.appdata_ptr = &pam_ctx;
    PAM_error = pam_start("netatalk", PAM_username, &pam_conv_local, &lpamh);

    if (PAM_error != PAM_SUCCESS) {
        LOG(log_info, logtype_uams, "DHX2 Chgpwd: PAM error in pam_start");
        ret = AFPERR_PARAM;
        goto error_exit;
    }

    pam_set_item(lpamh, PAM_TTY, "afpd");
    uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME, (void *) &hostname, NULL);
    pam_set_item(lpamh, PAM_RHOST, hostname);
    uid = geteuid();

    if (seteuid(0) < 0) {
        LOG(log_error, logtype_uams, "DHX2 Chgpwd: could not seteuid(%i)", 0);
        pam_end(lpamh, PAM_SUCCESS);
        ret = AFPERR_MISC;
        goto error_exit;
    }

    PAM_error = pam_authenticate(lpamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        LOG(log_info, logtype_uams, "DHX2 Chgpwd: error authenticating with PAM");

        if (seteuid(uid) < 0) {
            LOG(log_error, logtype_uams, "DHX2 Chgpwd: could not seteuid(%i)", uid);
            pam_end(lpamh, PAM_error);
            ret = AFPERR_MISC;
            goto error_exit;
        }

        pam_end(lpamh, PAM_error);
        ret = AFPERR_NOTAUTH;
        goto error_exit;
    }

    PAM_error = pam_acct_mgmt(lpamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        LOG(log_info, logtype_uams, "DHX2 Chgpwd: error validating PAM account");

        if (seteuid(uid) < 0) {
            LOG(log_error, logtype_uams, "DHX2 Chgpwd: could not seteuid(%i)", uid);
            pam_end(lpamh, PAM_error);
            ret = AFPERR_MISC;
            goto error_exit;
        }

        pam_end(lpamh, PAM_error);
        ret = AFPERR_NOTAUTH;
        goto error_exit;
    }

    pam_ctx.password = ibuf;          /* new password for chauthtok */
    PAM_error = pam_chauthtok(lpamh, 0);

    if (seteuid(uid) < 0) {
        LOG(log_error, logtype_uams, "DHX2 Chgpwd: could not seteuid(%i)", uid);
        explicit_bzero(ibuf, 512);
        pam_end(lpamh, PAM_SUCCESS);
        ret = AFPERR_MISC;
        goto error_exit;
    }

    explicit_bzero(ibuf, 512);

    if (PAM_error != PAM_SUCCESS) {
        LOG(log_info, logtype_uams, "DHX2 Chgpwd: error changing pw with PAM");
        pam_end(lpamh, PAM_error);
        ret = AFPERR_ACCESS;
        goto error_exit;
    }

    pam_end(lpamh, 0);
    ret = AFP_OK;
error_exit:
    free(K_MD5hash);
    K_MD5hash = NULL;
    gcry_mpi_release(serverNonce);
    gcry_mpi_release(retServerNonce);
    return ret;
}

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
    if (uam_dh_params_generate(&p, &g, PRIMEBITS) != 0) {
        LOG(log_error, logtype_uams, "DHX2: Couldn't generate p and g");
        return -1;
    }

    return 0;
}

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
