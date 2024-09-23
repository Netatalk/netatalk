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

#define KEYSIZE 16
#define PASSWDLEN 64
#define CRYPTBUFLEN  (KEYSIZE*2)
#define CRYPT2BUFLEN (KEYSIZE + PASSWDLEN)
#define CHANGEPWBUFLEN (KEYSIZE + 2*PASSWDLEN)

/* hash a number to a 16-bit quantity */
#define dhxhash(a) ((((unsigned long) (a) >> 8) ^ \
		     (unsigned long) (a)) & 0xffff)

/* the secret key */
gcry_mpi_t K;

static struct passwd *dhxpwd;
static uint8_t randbuf[KEYSIZE];

/* diffie-hellman bits */
static unsigned char msg2_iv[] = "CJalbert";
static unsigned char msg3_iv[] = "LWallace";
static const unsigned char p_binary[] = {0xBA, 0x28, 0x73, 0xDF, 0xB0, 0x60, 0x57, 0xD4,
			     0x3F, 0x20, 0x24, 0x74, 0x4C, 0xEE, 0xE7, 0x5B};
static const unsigned char g_binary[] = {0x07};


/* Static variables used to communicate between the conversation function
 * and the server_login function
 */
static pam_handle_t *pamh = NULL;
static unsigned char *PAM_username;
static unsigned char *PAM_password;

/* PAM conversation function
 * Here we assume (for now, at least) that echo on means login name, and
 * echo off means password.
 */
static int PAM_conv (int num_msg,
#if !defined(__svr4__)
                     const struct pam_message **msg,
#else
                     struct pam_message **msg,
#endif
                     struct pam_response **resp,
                     void *appdata_ptr _U_) {
  int count = 0;
  struct pam_response *reply;

#define COPY_STRING(s) (s) ? strdup(s) : NULL

  errno = 0;

  if (num_msg < 1) {
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM DHX Conversation Err -- %s",
		  strerror(errno));
    /* Log Entry */
    return PAM_CONV_ERR;
  }

  reply = (struct pam_response *)
    calloc(num_msg, sizeof(struct pam_response));

  if (!reply) {
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM DHX Conversation Err -- %s",
		  strerror(errno));
    /* Log Entry */
    return PAM_CONV_ERR;
  }

  for (count = 0; count < num_msg; count++) {
    char *string = NULL;

    switch (msg[count]->msg_style) {
    case PAM_PROMPT_ECHO_ON:
      if (!(string = COPY_STRING((const char *)PAM_username))) {
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: username failure -- %s",
		  strerror(errno));
    /* Log Entry */
	goto pam_fail_conv;
      }
      break;
    case PAM_PROMPT_ECHO_OFF:
      if (!(string = COPY_STRING((const char *)PAM_password))) {
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: passwd failure: --: %s",
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
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Binary_Prompt -- %s",
		  strerror(errno));
    /* Log Entry */
      goto pam_fail_conv;
    }

    if (string) {
      reply[count].resp_retcode = 0;
      reply[count].resp = string;
      string = NULL;
    }
  }

  *resp = reply;
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM Success");
    /* Log Entry */
  return PAM_SUCCESS;

pam_fail_conv:
  for (count = 0; count < num_msg; count++) {
    if (!reply[count].resp)
	continue;
    switch (msg[count]->msg_style) {
    case PAM_PROMPT_ECHO_OFF:
    case PAM_PROMPT_ECHO_ON:
      free(reply[count].resp);
      break;
    }
  }
  free(reply);
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM DHX Conversation Err -- %s",
		  strerror(errno));
    /* Log Entry */
    return PAM_CONV_ERR;
}

static struct pam_conv PAM_conversation = {
  &PAM_conv,
  NULL
};


static int dhx_setup(void *obj, const unsigned char *ibuf, size_t ibuflen _U_,
		     unsigned char *rbuf, size_t *rbuflen)
{
    uint16_t sessid;
    size_t i;
    size_t nwritten;

    if (!gcry_check_version(GCRYPT_VERSION))
        LOG(log_info, logtype_uams, "uams_dhx_pam.c : libgcrypt versions mismatch. Need: %s", GCRYPT_VERSION);

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

    gcry_mpi_print(GCRYMPI_FMT_USG, rbuf, KEYSIZE, &nwritten, Mb);
    if (nwritten < KEYSIZE) {
        memmove(rbuf + KEYSIZE - nwritten, rbuf, nwritten);
        memset(rbuf, 0, KEYSIZE - nwritten);
    }
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
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Signature Retieval Failure -- %s",
		  strerror(errno));
    /* Log Entry */
      goto pam_fail;
    }
    memcpy(rbuf + KEYSIZE, buf, KEYSIZE);
#else /* 0 */
    memset(rbuf + KEYSIZE, 0, KEYSIZE);
#endif /* 0 */

    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
        GCRY_CIPHER_MODE_CBC, 0);
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        goto pam_fail;
    /* Set the binary form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_binary, sizeof(K_binary));
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        goto pam_fail;
    /* Set the initialization vector for server->client transfer. */
    ctxerror = gcry_cipher_setiv(ctx, msg2_iv, sizeof(msg2_iv));
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        goto pam_fail;
    /* Encrypt the ciphertext from the server. */
    ctxerror = gcry_cipher_encrypt(ctx, rbuf, CRYPTBUFLEN, NULL, 0);
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        goto pam_fail;
    *rbuflen += CRYPTBUFLEN;
    gcry_cipher_close(ctx);

    return AFPERR_AUTHCONT;

pam_fail:
    gcry_mpi_release(K);

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
    if (( dhxpwd = uam_getname(obj, (char *)username, ulen)) == NULL ) {
        LOG(log_info, logtype_uams, "uams_dhx_pam.c: unknown username [%s]", username);
        return AFPERR_NOTAUTH;
    }

    PAM_username = username;
    LOG(log_info, logtype_uams, "dhx login: %s", username);
    return dhx_setup(obj, ibuf, ibuflen, rbuf, rbuflen);
}

/* -------------------------------- */
/* dhx login: things are done in a slightly bizarre order to avoid
 * having to clean things up if there's an error. */
static int pam_login(void *obj, struct passwd **uam_pwd,
		     unsigned char *ibuf, size_t ibuflen,
		     unsigned char *rbuf, size_t *rbuflen)
{
    unsigned char *username;
    size_t len, ulen;

    *rbuflen = 0;

    /* grab some of the options */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username, &ulen) < 0) {
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: uam_afpserver_option didn't meet uam_option_username  -- %s",
		  strerror(errno));
        return AFPERR_PARAM;
    }

    len = *ibuf++;
    if ( len > ulen ) {
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Signature Retieval Failure -- %s",
		  strerror(errno));
	return AFPERR_PARAM;
    }

    memcpy(username, ibuf, len );
    ibuf += len;
    username[ len ] = '\0';

    if ((unsigned long) ibuf & 1) /* pad to even boundary */
      ++ibuf;

    return (login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen));
}

/* ----------------------------- */
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
		     const unsigned char *ibuf, size_t ibuflen,
		     unsigned char *rbuf, size_t *rbuflen)
{
    unsigned char *username;
    int len;
    size_t ulen;
    uint16_t temp16;

    *rbuflen = 0;

    /* grab some of the options */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username, &ulen) < 0) {
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: uam_afpserver_option didn't meet uam_option_username  -- %s",
		  strerror(errno));
        return AFPERR_PARAM;
    }

    if (*uname != 3)
        return AFPERR_PARAM;
    uname++;
    memcpy(&temp16, uname, sizeof(temp16));
    len = ntohs(temp16);

    if ( !len || len > ulen ) {
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Signature Retrieval Failure -- %s",
		  strerror(errno));
	return AFPERR_PARAM;
    }
    memcpy(username, uname +2, len );
    username[ len ] = '\0';

    return (login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen));
}

/* -------------------------------- */

static int pam_logincont(void *obj, struct passwd **uam_pwd,
			 const unsigned char *ibuf, size_t ibuflen _U_,
			 unsigned char *rbuf, size_t *rbuflen)
{
    const char *hostname;
    gcry_mpi_t bn1, bn2, bn3;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    uint16_t sessid;
    int err, PAM_error;
    unsigned char K_binary[16];
    size_t i;

    *rbuflen = 0;

    /* check for session id */
    memcpy(&sessid, ibuf, sizeof(sessid));
    if (sessid != dhxhash(obj)) {
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM Session ID - DHXHash Mismatch -- %s",
		  strerror(errno));
    /* Log Entry */
      return AFPERR_PARAM;
    }
    ibuf += sizeof(sessid);

    if (uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME,
			     (void *) &hostname, NULL) < 0)
	{
	LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: unable to retrieve client hostname");
	hostname = NULL;
	}

    gcry_mpi_print(GCRYMPI_FMT_USG, K_binary, sizeof(K_binary), &i, K);
    if (i < KEYSIZE) {
        memmove(K_binary + sizeof(K_binary) - i, K_binary, i);
        memset(K_binary, 0, sizeof(K_binary) - i);
    }

    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
        GCRY_CIPHER_MODE_CBC, 0);
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        return AFPERR_PARAM;
    /* Set the binary form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_binary, sizeof(K_binary));
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        return AFPERR_PARAM;
    /* Set the initialization vector for client->server transfer. */
    ctxerror = gcry_cipher_setiv(ctx, msg3_iv, sizeof(msg3_iv));
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        return AFPERR_PARAM;

    /* Decrypt the ciphertext from the client. */
    ctxerror = gcry_cipher_decrypt(ctx, rbuf, CRYPT2BUFLEN, ibuf, CRYPT2BUFLEN);
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        return AFPERR_PARAM;

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

    /* Set these things up for the conv function */
    rbuf[PASSWDLEN] = '\0';
    PAM_password = rbuf;

    err = AFPERR_NOTAUTH;
    PAM_error = pam_start("netatalk", (const char *)PAM_username,
			&PAM_conversation, &pamh);
    if (PAM_error != PAM_SUCCESS) {
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM_Error: %s",
		  pam_strerror(pamh,PAM_error));
    /* Log Entry */
      goto logincont_err;
    }

    /* solaris craps out if PAM_TTY and PAM_RHOST aren't set. */
    pam_set_item(pamh, PAM_TTY, "afpd");
    pam_set_item(pamh, PAM_RHOST, hostname);
    PAM_error = pam_authenticate(pamh,0);
    if (PAM_error != PAM_SUCCESS) {
      if (PAM_error == PAM_MAXTRIES)
	err = AFPERR_PWDEXPR;
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM_Error: %s",
		  pam_strerror(pamh, PAM_error));
    /* Log Entry */
      goto logincont_err;
    }

    PAM_error = pam_acct_mgmt(pamh, 0);
    if (PAM_error != PAM_SUCCESS ) {
      /* Log Entry */
      LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM_Error: %s",
	  pam_strerror(pamh, PAM_error));
      /* Log Entry */
      if (PAM_error == PAM_NEW_AUTHTOK_REQD)	/* password expired */
	err = AFPERR_PWDEXPR;
#ifdef PAM_AUTHTOKEN_REQD
      else if (PAM_error == PAM_AUTHTOKEN_REQD)
	err = AFPERR_PWDCHNG;
#endif
      else
        goto logincont_err;
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

    memset(rbuf, 0, PASSWDLEN); /* zero out the password */
    *uam_pwd = dhxpwd;
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM Auth OK!");
    /* Log Entry */
    if ( err == AFPERR_PWDEXPR)
	return err;
    return AFP_OK;

logincont_err:
    pam_end(pamh, PAM_error);
    pamh = NULL;
    memset(rbuf, 0, CRYPT2BUFLEN);
    return err;
}

/* logout */
static void pam_logout(void) {
    pam_close_session(pamh, 0);
    pam_end(pamh, 0);
    pamh = NULL;
}


/* change pw for dhx needs a couple passes to get everything all
 * right. basically, it's like the login/logincont sequence */
static int pam_changepw(void *obj, unsigned char *username,
			const struct passwd *pwd _U_, unsigned char *ibuf,
			size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
{
    gcry_mpi_t bn1, bn2, bn3;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    unsigned char K_binary[16];
    size_t i;
    char *hostname;
    pam_handle_t *lpamh;
    uid_t uid;
    uint16_t sessid;
    int PAM_error;

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

    /* check out the session id */
    if (sessid != dhxhash(obj)) {
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Session ID not Equal to DHX Hash -- %s",
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

    gcry_mpi_print(GCRYMPI_FMT_USG, K_binary, sizeof(K_binary), &i, K);
    if (i < KEYSIZE) {
        memmove(K_binary + sizeof(K_binary) - i, K_binary, i);
        memset(K_binary, 0, sizeof(K_binary) - i);
    }

    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
        GCRY_CIPHER_MODE_CBC, 0);
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        return AFPERR_PARAM;

    /* Set the binary form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_binary, sizeof(K_binary));
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        return AFPERR_PARAM;

    /* Set the initialization vector for server->client transfer. */
    ctxerror = gcry_cipher_setiv(ctx, msg3_iv, sizeof(msg3_iv));
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        return AFPERR_PARAM;

    /* Decrypt the ciphertext from the server. */
    ctxerror = gcry_cipher_decrypt(ctx, ibuf, CHANGEPWBUFLEN, NULL, 0);
    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR)
        return AFPERR_PARAM;

    gcry_cipher_close(ctx);

    bn1 = gcry_mpi_snew(KEYSIZE);
    gcry_mpi_scan(&bn1, GCRYMPI_FMT_STD, ibuf, KEYSIZE, NULL);
    bn2 = gcry_mpi_snew(sizeof(randbuf));
    gcry_mpi_scan(&bn2, GCRYMPI_FMT_STD, randbuf, sizeof(randbuf), NULL);

    /* zero out the random number */
    memset(ibuf, 0, sizeof(randbuf));
    memset(randbuf, 0, sizeof(randbuf));

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
    if (ibuflen <= PASSWDLEN + PASSWDLEN)
        return AFPERR_PARAM;
    ibuf[PASSWDLEN + PASSWDLEN] = '\0';
    PAM_password = ibuf + PASSWDLEN;

    PAM_error = pam_start("netatalk", (char *)username, &PAM_conversation,
			  &lpamh);
    if (PAM_error != PAM_SUCCESS) {
    /* Log Entry */
           LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Needless to say, PAM_error is != to PAM_SUCCESS -- %s",
		  strerror(errno));
    /* Log Entry */
      return AFPERR_PARAM;
    }
    pam_set_item(lpamh, PAM_TTY, "afpd");
    pam_set_item(lpamh, PAM_RHOST, hostname);

    /* we might need to do this as root */
    uid = geteuid();
    seteuid(0);
    PAM_error = pam_authenticate(lpamh, 0);
    if (PAM_error != PAM_SUCCESS) {
      seteuid(uid);
      pam_end(lpamh, PAM_error);
      return AFPERR_NOTAUTH;
    }

    /* clear out old passwd */
    memset(ibuf + PASSWDLEN, 0, PASSWDLEN);

    /* new password */
    PAM_password = ibuf;
    if (ibuflen <= PASSWDLEN)
        return AFPERR_PARAM;
    ibuf[PASSWDLEN] = '\0';

    /* this really does need to be done as root */
    PAM_error = pam_chauthtok(lpamh, 0);
    seteuid(uid); /* un-root ourselves. */
    memset(ibuf, 0, PASSWDLEN);
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
		   pam_logincont, pam_logout, pam_login_ext) < 0)
    return -1;

  if (uam_register(UAM_SERVER_CHANGEPW, path, "DHCAST128",
		   pam_changepw) < 0) {
    uam_unregister(UAM_SERVER_LOGIN, "DHCAST128");
    return -1;
  }

  /*uam_register(UAM_SERVER_PRINTAUTH, path, "DHCAST128",
    pam_printer);*/

  return 0;
}

static void uam_cleanup(void)
{
  uam_unregister(UAM_SERVER_LOGIN, "DHCAST128");
  uam_unregister(UAM_SERVER_CHANGEPW, "DHCAST128");
  /*uam_unregister(UAM_SERVER_PRINTAUTH, "DHCAST128"); */
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
