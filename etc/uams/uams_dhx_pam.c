/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief DHCAST128 UAM with PAM authentication
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
#include <atalk/compat.h>
#include <atalk/logger.h>
#include <atalk/uam.h>

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

/*!
 * @brief Release the session key K of a DHCAST128 exchange
 */
static void dhx_release_key(void)
{
    if (K != NULL) {
        gcry_mpi_release(K);
        K = NULL;
    }
}

static struct passwd *dhxpwd;
static uint8_t randbuf[KEYSIZE];

/* diffie-hellman bits */
static unsigned char msg2_iv[8] = "CJalbert";
static unsigned char msg3_iv[8] = "LWallace";
static const unsigned char p_binary[] = {0xBA, 0x28, 0x73, 0xDF, 0xB0, 0x60, 0x57, 0xD4,
                                         0x3F, 0x20, 0x24, 0x74, 0x4C, 0xEE, 0xE7, 0x5B
                                        };
static const unsigned char g_binary[] = {0x07};


/* Static variables used to communicate between the conversation function
 * and the server_login function
 */
static pam_handle_t *pamh = NULL;
static unsigned char *PAM_username;
static unsigned char *PAM_password;

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
    LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM DHX Conversation Err -- %s",
        strerror(errno));
    /* Log Entry */
    return PAM_CONV_ERR;
}

static struct pam_conv PAM_conversation = {
    &PAM_conv,
    NULL
};


/*!
 * @brief Answer the public key of the client in a DHCAST128 exchange with Mb
 * and a challenge
 *
 * The group is the fixed 128-bit prime of the protocol with generator 7. The
 * call picks a random Rb, computes Mb = g^Rb mod p and the session key
 * K = Ma^Rb mod p, and draws a 16-byte random challenge. The reply holds the
 * session ID, Mb, and under CAST5-CBC the challenge and 16 zero bytes in the
 * place of the server signature.
 *
 * @param[in]  obj      session object, which also seeds the session ID
 * @param[in]  ibuf     Ma of the client, 16 bytes
 * @param[in]  ibuflen  length of ibuf, unused
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFPERR_AUTHCONT, AFPERR_MISC for a libgcrypt version mismatch, or
 *          AFPERR_PARAM when a step fails
 */
static int dhx_setup(void *obj, const unsigned char *ibuf, size_t ibuflen _U_,
                     unsigned char *rbuf, size_t *rbuflen)
{
    uint16_t sessid;
    size_t i;
    size_t nwritten;
    /* A new exchange supersedes any unfinished challenge. */
    explicit_bzero(randbuf, sizeof(randbuf));

    if (!gcry_check_version(UAM_NEED_LIBGCRYPT_VERSION)) {
        LOG(log_error, logtype_uams,
            "uams_dhx_pam.c: libgcrypt versions mismatch. Needs: %s Has: %s",
            UAM_NEED_LIBGCRYPT_VERSION, gcry_check_version(NULL));
        return AFPERR_MISC;
    }

    gcry_mpi_t p, g, Rb, Ma, Mb;
    Mb = gcry_mpi_new(0);
    dhx_release_key();
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

    if (gcry_mpi_print(GCRYMPI_FMT_USG, K_binary, sizeof(K_binary), &i, K) != 0) {
        *rbuflen = 0;
        goto pam_fail;
    }

    if (i < KEYSIZE) {
        memmove(K_binary + sizeof(K_binary) - i, K_binary, i);
        memset(K_binary, 0, sizeof(K_binary) - i);
    }

    /* session id. it's just a hashed version of the object pointer. */
    sessid = dhxhash(obj);
    memcpy(rbuf, &sessid, sizeof(sessid));
    rbuf += sizeof(sessid);
    *rbuflen += sizeof(sessid);

    if (gcry_mpi_print(GCRYMPI_FMT_USG, rbuf, KEYSIZE, &nwritten, Mb) != 0) {
        *rbuflen = 0;
        goto pam_fail;
    }

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
    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
                                GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto pam_fail;
    }

    /* Set the binary form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_binary, sizeof(K_binary));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto pam_fail;
    }

    /* Set the initialization vector for server->client transfer. */
    ctxerror = gcry_cipher_setiv(ctx, msg2_iv, sizeof(msg2_iv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto pam_fail;
    }

    /* Encrypt the ciphertext from the server. */
    ctxerror = gcry_cipher_encrypt(ctx, rbuf, CRYPTBUFLEN, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        goto pam_fail;
    }

    *rbuflen += CRYPTBUFLEN;
    gcry_cipher_close(ctx);
    return AFPERR_AUTHCONT;
pam_fail:
    dhx_release_key();
    explicit_bzero(randbuf, sizeof(randbuf));
    /* Log Entry */
    LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Fail - Cast Encryption -- %s",
        strerror(errno));
    /* Log Entry */
    return AFPERR_PARAM;
}

/*!
 * @brief Look up the user of a DHCAST128 login and start the Diffie-Hellman
 * exchange
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
 * @returns AFPERR_AUTHCONT, AFPERR_NOTAUTH for an unknown user, AFPERR_PARAM,
 *          or AFPERR_MISC
 */
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

/*!
 * @brief Read the user name of an FPLogin request and start a DHCAST128 login
 *
 * The request holds the name as a Pascal string, padded to an even address.
 * The checks run before any state exists, so a refusal needs no cleanup.
 *
 * @param[in]  obj      session object
 * @param[out] uam_pwd  unused until the continuation
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFPERR_AUTHCONT, AFPERR_PARAM for a name longer than the buffer,
 *          AFPERR_NOTAUTH, or AFPERR_MISC
 */
static int pam_login(void *obj, struct passwd **uam_pwd,
                     unsigned char *ibuf, size_t ibuflen,
                     unsigned char *rbuf, size_t *rbuflen)
{
    unsigned char *username;
    size_t len, ulen;
    *rbuflen = 0;

    /* grab some of the options */
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: uam_afpserver_option didn't meet uam_option_username -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    len = *ibuf++;

    if (len > ulen) {
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: Signature Retieval Failure -- %s",
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
 * @brief Read the user name of an FPLoginExt request and start a DHCAST128
 * login
 *
 * The name comes as an AFP name of type 3, a UTF-8 string with a 2-byte
 * length.
 *
 * @param[in]  obj      session object
 * @param[in]  uname    user name field of the request
 * @param[out] uam_pwd  unused until the continuation
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block
 * @param[out] rbuflen  reply length on return
 * @returns AFPERR_AUTHCONT, AFPERR_PARAM for another name type or a bad length,
 *          AFPERR_NOTAUTH, or AFPERR_MISC
 */
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
    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: uam_afpserver_option didn't meet uam_option_username -- %s",
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
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: Signature Retrieval Failure -- %s",
            strerror(errno));
        return AFPERR_PARAM;
    }

    memcpy(username, uname + 2, len);
    username[len] = '\0';
    return login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief Log a failed PAM step through uam_log_pam_failure()
 *
 * errno is read before pam_strerror(), whose message lookup can change it.
 *
 * @param[in] ph       PAM handle, NULL when pam_start() failed
 * @param[in] user     user being authenticated
 * @param[in] step     PAM function that failed
 * @param[in] pam_err  its return code
 */
static void dhx_log_pam_error(pam_handle_t *ph, const char *user,
                              const char *step, int pam_err)
{
    int saved_errno = errno;
    uam_log_pam_failure(log_info, "DHCAST128", user, step,
                        pam_strerror(ph, pam_err), pam_err, saved_errno);
}

/*!
 * @brief Check the challenge answer and the password of a DHCAST128 login with
 * PAM
 *
 * The request holds the session ID and, under CAST5-CBC, the challenge plus 1
 * and a 64-byte password field. PAM runs authenticate, acct_mgmt, setcred,
 * and open_session. A password that acct_mgmt reports as expired still opens
 * the session and answers AFPERR_PWDEXPR, so the client can change it. The
 * call wipes the challenge and the password.
 *
 * @param[in]  obj      session object
 * @param[out] uam_pwd  receives the user entry after a successful login
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf, at least 2 + CRYPT2BUFLEN
 * @param[out] rbuf     reply block, which the call uses as a work buffer and
 *                      clears
 * @param[out] rbuflen  0 on return
 * @returns AFP_OK, AFPERR_PWDEXPR, AFPERR_NOTAUTH, or AFPERR_PARAM
 */
static int pam_logincont(void *obj, struct passwd **uam_pwd,
                         const unsigned char *ibuf, size_t ibuflen,
                         unsigned char *rbuf, size_t *rbuflen)
{
    const char *hostname;
    gcry_mpi_t bn1, bn2, bn3;
    gcry_cipher_hd_t ctx;
    gcry_error_t ctxerror;
    uint16_t sessid;
    int err, PAM_error;
    unsigned char K_binary[16];
    unsigned char password[PASSWDLEN + 1];
    size_t i;
    *rbuflen = 0;

    /* Make sure dhx_setup actually ran and established the shared key */
    if (K == NULL) {
        LOG(log_error, logtype_uams, "DHX: logincont called without completing login");
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    /* Require the session ID and complete encrypted nonce/password block. */
    if (ibuflen < sizeof(sessid) + CRYPT2BUFLEN) {
        LOG(log_info, logtype_uams,
            "DHX: short logincont packet: %zu bytes", ibuflen);
        dhx_release_key();
        explicit_bzero(randbuf, sizeof(randbuf));
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
        dhx_release_key();
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    ibuf += sizeof(sessid);

    if (uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME,
                             (void *) &hostname, NULL) < 0) {
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: unable to retrieve client hostname");
        hostname = NULL;
    }

    if (gcry_mpi_print(GCRYMPI_FMT_USG, K_binary, sizeof(K_binary), &i, K) != 0) {
        dhx_release_key();
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    dhx_release_key();

    if (i < KEYSIZE) {
        memmove(K_binary + sizeof(K_binary) - i, K_binary, i);
        memset(K_binary, 0, sizeof(K_binary) - i);
    }

    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
                                GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    /* Set the binary form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_binary, sizeof(K_binary));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    /* Set the initialization vector for client->server transfer. */
    ctxerror = gcry_cipher_setiv(ctx, msg3_iv, sizeof(msg3_iv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    /* Decrypt the ciphertext from the client. */
    ctxerror = gcry_cipher_decrypt(ctx, rbuf, CRYPT2BUFLEN, ibuf, CRYPT2BUFLEN);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    gcry_cipher_close(ctx);
    gcry_mpi_scan(&bn1, GCRYMPI_FMT_STD, rbuf, KEYSIZE, NULL);
    gcry_mpi_scan(&bn2, GCRYMPI_FMT_STD, randbuf, sizeof(randbuf), NULL);
    /* zero out the random number */
    explicit_bzero(rbuf, sizeof(randbuf));
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
    /* The wire password fills its field; terminate it in a separate buffer. */
    memcpy(password, rbuf + KEYSIZE, PASSWDLEN);
    password[PASSWDLEN] = '\0';
    explicit_bzero(rbuf, CRYPT2BUFLEN);
    /* Set these things up for the conv function */
    PAM_password = password;
    err = AFPERR_NOTAUTH;
    PAM_error = pam_start("netatalk", (const char *)PAM_username,
                          &PAM_conversation, &pamh);

    if (PAM_error != PAM_SUCCESS) {
        dhx_log_pam_error(NULL, (const char *)PAM_username, "pam_start",
                          PAM_error);
        /* a failed pam_start() leaves the handle undefined */
        pamh = NULL;
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

        dhx_log_pam_error(pamh, (const char *)PAM_username, "pam_authenticate",
                          PAM_error);
        goto logincont_err;
    }

    PAM_error = pam_acct_mgmt(pamh, 0);

    if (sigchld_saved) {
        sigaction(SIGCHLD, &sa_old, NULL);
    }

    if (PAM_error != PAM_SUCCESS) {
        dhx_log_pam_error(pamh, (const char *)PAM_username, "pam_acct_mgmt",
                          PAM_error);

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
        dhx_log_pam_error(pamh, (const char *)PAM_username, "pam_setcred",
                          PAM_error);
        goto logincont_err;
    }

    PAM_error = pam_open_session(pamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        dhx_log_pam_error(pamh, (const char *)PAM_username, "pam_open_session",
                          PAM_error);
        goto logincont_err;
    }

    explicit_bzero(password, sizeof(password));
    PAM_password = NULL;
    *uam_pwd = dhxpwd;
    /* Log Entry */
    LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: PAM Auth OK!");

    /* Log Entry */
    if (err == AFPERR_PWDEXPR) {
        return err;
    }

    return AFP_OK;
logincont_err:

    if (pamh != NULL) {
        pam_end(pamh, PAM_error);
        pamh = NULL;
    }

    explicit_bzero(password, sizeof(password));
    PAM_password = NULL;
    return err;
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


/*!
 * @brief Change the password of a DHCAST128 user in two requests
 *
 * A request with session ID 0 starts the exchange through dhx_setup. The next
 * request holds the session ID and, under CAST5-CBC, the challenge plus 1,
 * the new password, and the old password, 64 bytes each. As root PAM
 * authenticates the old password and runs acct_mgmt and chauthtok with the
 * new one. The call wipes the old password before chauthtok and the new one
 * after it.
 *
 * @param[in]     obj       session object
 * @param[in]     username  user name
 * @param[in]     pwd       user entry, unused
 * @param[in,out] ibuf      UAM data of the request, decrypted in place
 * @param[in]     ibuflen   length of ibuf
 * @param[out]    rbuf      reply block
 * @param[out]    rbuflen   reply length on return
 * @returns AFPERR_AUTHCONT after the first request, AFP_OK, AFPERR_NOTAUTH,
 *          AFPERR_ACCESS when chauthtok fails, AFPERR_PARAM, or AFPERR_MISC
 */
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
        explicit_bzero(randbuf, sizeof(randbuf));
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
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    /* check out the session id */
    if (sessid != dhxhash(obj)) {
        /* Log Entry */
        LOG(log_info, logtype_uams,
            "uams_dhx_pam.c :PAM: Session ID not Equal to DHX Hash -- %s",
            strerror(errno));
        /* Log Entry */
        dhx_release_key();
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    /* we need this for pam */
    if (uam_afpserver_option(obj, UAM_OPTION_HOSTNAME,
                             (void *) &hostname, NULL) < 0) {
        /* Log Entry */
        LOG(log_info, logtype_uams, "uams_dhx_pam.c :PAM: Hostname Null?? -- %s",
            strerror(errno));
        /* Log Entry */
        dhx_release_key();
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_MISC;
    }

    if (gcry_mpi_print(GCRYMPI_FMT_USG, K_binary, sizeof(K_binary), &i, K) != 0) {
        dhx_release_key();
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    dhx_release_key();

    if (i < KEYSIZE) {
        memmove(K_binary + sizeof(K_binary) - i, K_binary, i);
        memset(K_binary, 0, sizeof(K_binary) - i);
    }

    /* Set up our encryption context. */
    ctxerror = gcry_cipher_open(&ctx, GCRY_CIPHER_CAST5,
                                GCRY_CIPHER_MODE_CBC, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    /* Set the binary form of K as our key for this encryption context. */
    ctxerror = gcry_cipher_setkey(ctx, K_binary, sizeof(K_binary));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    /* Set the initialization vector for server->client transfer. */
    ctxerror = gcry_cipher_setiv(ctx, msg3_iv, sizeof(msg3_iv));

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    /* Decrypt the ciphertext from the server. */
    ctxerror = gcry_cipher_decrypt(ctx, ibuf, CHANGEPWBUFLEN, NULL, 0);

    if (gcry_err_code(ctxerror) != GPG_ERR_NO_ERROR) {
        explicit_bzero(randbuf, sizeof(randbuf));
        return AFPERR_PARAM;
    }

    gcry_cipher_close(ctx);
    gcry_mpi_scan(&bn1, GCRYMPI_FMT_STD, ibuf, KEYSIZE, NULL);
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
    PAM_password = ibuf + PASSWDLEN;
    PAM_error = pam_start("netatalk", (char *)username, &PAM_conversation,
                          &lpamh);

    if (PAM_error != PAM_SUCCESS) {
        dhx_log_pam_error(NULL, (const char *)username, "pam_start", PAM_error);
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
        dhx_log_pam_error(lpamh, (const char *)username, "pam_authenticate",
                          PAM_error);

        if (seteuid(uid) < 0) {
            LOG(log_error, logtype_uams, "pam_changepw: could not seteuid(%i)", uid);
        }

        pam_end(lpamh, PAM_error);
        return AFPERR_NOTAUTH;
    }

    PAM_error = pam_acct_mgmt(lpamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        dhx_log_pam_error(lpamh, (const char *)username, "pam_acct_mgmt",
                          PAM_error);

        if (seteuid(uid) < 0) {
            LOG(log_error, logtype_uams, "pam_changepw: could not seteuid(%i)", uid);
        }

        pam_end(lpamh, PAM_error);
        return AFPERR_NOTAUTH;
    }

    /* clear out old passwd */
    explicit_bzero(ibuf + PASSWDLEN, PASSWDLEN);
    /* new password */
    PAM_password = ibuf;

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
        dhx_log_pam_error(lpamh, (const char *)username, "pam_chauthtok",
                          PAM_error);
        pam_end(lpamh, PAM_error);
        return AFPERR_ACCESS;
    }

    pam_end(lpamh, 0);
    return AFP_OK;
}


/*!
 * @brief Register the DHCAST128 login and password change of the PAM module
 *
 * A failed password change registration removes the login again.
 *
 * @param[in] obj   AFP object, unused
 * @param[in] path  path of the module
 * @returns 0, or -1 when a registration fails
 */
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

/*!
 * @brief Release the session key and unregister the DHCAST128 login and
 * password change
 */
static void uam_cleanup(void)
{
    dhx_release_key();
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
