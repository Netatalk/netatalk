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

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
#ifdef HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#endif

#include <atalk/afp.h>
#include <atalk/constant_time.h>
#include <atalk/uam.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/compat.h>

#define PASSWDLEN 8

/* Static variables used to communicate between the conversation function
 * and the server_login function
 */
static pam_handle_t *pamh = NULL;
static const char *hostname;
static char *PAM_username;
static char *PAM_password;
static char *PAM_oldpassword;
static int PAM_chauthtok_mode;
static int PAM_chauthtok_count;

/*XXX in etc/papd/file.h */
struct papfile;
extern UAM_MODULE_EXPORT void append(struct papfile *, const char *, int);

/*!
 * @brief Answer PAM prompts with the user name and the password of the login
 *
 * An echo-on prompt gets PAM_username and an echo-off prompt PAM_password.
 * During a password change without root, the first echo-off prompt gets the
 * old password instead. Info and binary prompts get no answer, and an error
 * message or an unknown style fails the conversation.
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
    struct pam_response *reply;
    int count;
#define COPY_STRING(s) (s) ? strdup(s) : NULL

    if (num_msg < 1) {
        return PAM_CONV_ERR;
    }

    reply = (struct pam_response *)
            calloc(num_msg, sizeof(struct pam_response));

    if (!reply) {
        return PAM_CONV_ERR;
    }

    for (count = 0; count < num_msg; count++) {
        char *string = NULL;

        switch (msg[count]->msg_style) {
        case PAM_PROMPT_ECHO_ON:
            if (!(string = COPY_STRING(PAM_username))) {
                goto pam_fail_conv;
            }

            break;

        case PAM_PROMPT_ECHO_OFF:
            if (PAM_chauthtok_mode && PAM_chauthtok_count == 0) {
                string = COPY_STRING(PAM_oldpassword);
                PAM_chauthtok_count++;
            } else {
                string = COPY_STRING(PAM_password);
            }

            if (!string) {
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
            goto pam_fail_conv;
        }

        if (string) {
            reply[count].resp_retcode = 0;
            reply[count].resp = string;
            string = NULL;
        }
    }

    *resp = reply;
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
    return PAM_CONV_ERR;
}

static struct pam_conv PAM_conversation = {
    &PAM_conv,
    NULL
};

/*!
 * @brief Log a failed PAM step through uam_log_pam_failure()
 *
 * errno is read before pam_strerror(), whose message lookup can change it.
 * A failed pam_start(), and a failed account or session step of a printer
 * login, are errors; the other steps are informational.
 *
 * @param[in] level    log level of the line
 * @param[in] ph       PAM handle, NULL when pam_start() failed
 * @param[in] user     user being authenticated
 * @param[in] step     PAM function that failed
 * @param[in] pam_err  its return code
 */
static void clrtxt_log_pam_error(enum loglevels level, pam_handle_t *ph,
                                 const char *user, const char *step,
                                 int pam_err)
{
    int saved_errno = errno;
    uam_log_pam_failure(level, "ClearTxt", user, step,
                        pam_strerror(ph, pam_err), pam_err, saved_errno);
}

/*!
 * @brief Check a cleartext password of up to 8 bytes with PAM
 *
 * The user has to pass uam_checkuser. PAM then runs authenticate, acct_mgmt,
 * setcred, and open_session, with SIGCHLD at its default action during the
 * checks. A password that acct_mgmt reports as expired still opens the
 * session and answers AFPERR_PWDEXPR, so the client can change it.
 *
 * @param[in]     obj       session object
 * @param[in,out] username  user name from the request, receives the account
 *                          name of a match
 * @param[in]     ulen      size of the username buffer
 * @param[out]    uam_pwd   receives the user entry after a successful login
 * @param[in,out] ibuf      password field of the request, cut after 8 bytes
 * @param[in]     ibuflen   length of the request data, which must exceed 8
 * @param[out]    rbuf      reply block, unused
 * @param[out]    rbuflen   unused
 * @returns AFP_OK, AFPERR_PWDEXPR, AFPERR_NOTAUTH, or AFPERR_PARAM
 */
static int login(void *obj, char *username, int ulen,  struct passwd **uam_pwd,
                 char *ibuf, size_t ibuflen _U_,
                 char *rbuf _U_, size_t *rbuflen _U_)
{
    struct passwd *pwd;
    int err, PAM_error;

    if (uam_afpserver_option(obj, UAM_OPTION_CLIENTNAME,
                             (void *) &hostname, NULL) < 0) {
        LOG(log_error, logtype_uams,
            "uams_pam.c :PAM: unable to retrieve client hostname");
        hostname = NULL;
    }

    if (ibuflen <= PASSWDLEN) {
        return AFPERR_PARAM;
    }

    ibuf[PASSWDLEN] = '\0';

    if ((pwd = uam_getname(obj, username, ulen)) == NULL) {
        return AFPERR_NOTAUTH;
    }

    if (uam_checkuser(NULL, pwd) < 0) {
        free(PAM_password);
        PAM_password = NULL;
        return AFPERR_NOTAUTH;
    }

    LOG(log_warning, logtype_uams, "cleartext login: %s (INSECURE)", username);
    PAM_username = username;
    PAM_password = ibuf; /* Set these things up for the conv function */
    err = AFPERR_NOTAUTH;
    LOG(log_debug, logtype_uams, "PAM: calling pam_start for %s", username);
    PAM_error = pam_start("netatalk", username, &PAM_conversation,
                          &pamh);

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_error, NULL, username, "pam_start", PAM_error);
        /* a failed pam_start() leaves the handle undefined */
        pamh = NULL;
        goto login_err;
    }

    LOG(log_debug, logtype_uams, "PAM: pam_start OK, calling pam_set_item");
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
    /* use PAM_DISALLOW_NULL_AUTHTOK if passwdminlen > 0 */
    LOG(log_debug, logtype_uams, "PAM: calling pam_authenticate");
    PAM_error = pam_authenticate(pamh, 0);
    LOG(log_debug, logtype_uams, "PAM: pam_authenticate returned: %d", PAM_error);

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_info, pamh, username, "pam_authenticate",
                             PAM_error);

        if (sigchld_saved) {
            sigaction(SIGCHLD, &sa_old, NULL);
        }

        if (PAM_error == PAM_MAXTRIES) {
            err = AFPERR_PWDEXPR;
        }

        goto login_err;
    }

    LOG(log_debug, logtype_uams, "PAM: calling pam_acct_mgmt");
    PAM_error = pam_acct_mgmt(pamh, 0);
    LOG(log_debug, logtype_uams, "PAM: pam_acct_mgmt returned: %d", PAM_error);

    if (sigchld_saved) {
        sigaction(SIGCHLD, &sa_old, NULL);
    }

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_info, pamh, username, "pam_acct_mgmt", PAM_error);

        /* Password change required */
        if (PAM_error == PAM_NEW_AUTHTOK_REQD) {
            err = AFPERR_PWDEXPR;
        }

#ifdef PAM_AUTHTOKEN_REQD
        else if (PAM_error == PAM_AUTHTOKEN_REQD) {
            err = AFPERR_PWDCHNG;
        }

#endif /* PAM_AUTHTOKEN_REQD */
        else {
            goto login_err;
        }
    }

#ifndef PAM_CRED_ESTABLISH
#define PAM_CRED_ESTABLISH PAM_ESTABLISH_CRED
#endif /* PAM_CRED_ESTABLISH */
    LOG(log_debug, logtype_uams, "PAM: calling pam_setcred");
    PAM_error = pam_setcred(pamh, PAM_CRED_ESTABLISH);
    LOG(log_debug, logtype_uams, "PAM: pam_setcred returned: %d", PAM_error);

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_info, pamh, username, "pam_setcred", PAM_error);
        goto login_err;
    }

    LOG(log_debug, logtype_uams, "PAM: calling pam_open_session");
    PAM_error = pam_open_session(pamh, 0);
    LOG(log_debug, logtype_uams, "PAM: pam_open_session returned: %d", PAM_error);

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_info, pamh, username, "pam_open_session",
                             PAM_error);
        goto login_err;
    }

    *uam_pwd = pwd;
    LOG(log_debug, logtype_uams, "PAM: login complete for %s", username);

    if (err == AFPERR_PWDEXPR) {
        return err;
    }

    return AFP_OK;
login_err:

    if (pamh != NULL) {
        pam_end(pamh, PAM_error);
        pamh = NULL;
    }

    return err;
}

/*!
 * @brief Read the user name of a cleartext FPLogin request and check its
 * password with PAM
 *
 * The request holds the name as a Pascal string, padded to an even address,
 * and then the password.
 *
 * @param[in]  obj      session object
 * @param[out] uam_pwd  receives the user entry after a successful login
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns the result of login, AFPERR_PARAM for a name longer than the buffer,
 *          or AFPERR_MISC
 */
static int pam_login(void *obj, struct passwd **uam_pwd,
                     char *ibuf, size_t ibuflen,
                     char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t  len, ulen;
    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
        return AFPERR_MISC;
    }

    len = (unsigned char) * ibuf++;

    if (len > ulen) {
        return AFPERR_PARAM;
    }

    memcpy(username, ibuf, len);
    ibuf += len;
    username[len] = '\0';

    if ((unsigned long) ibuf & 1) { /* pad character */
        ++ibuf;
    }

    return login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
}

/*!
 * @brief Read the user name of a cleartext FPLoginExt request and check its
 * password with PAM
 *
 * The name comes as an AFP name of type 3, a UTF-8 string with a 2-byte
 * length.
 *
 * @param[in]  obj      session object
 * @param[in]  uname    user name field of the request
 * @param[out] uam_pwd  receives the user entry after a successful login
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns the result of login, AFPERR_PARAM for another name type or a bad
 *          length, or AFPERR_MISC
 */
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd,
                         char *ibuf, size_t ibuflen,
                         char *rbuf, size_t *rbuflen)
{
    char *username;
    size_t  len, ulen;
    uint16_t  temp16;
    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, (void *) &username,
                             &ulen) < 0) {
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
    return login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen);
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
 * @brief Change a cleartext password with PAM
 *
 * The request holds the old and the new password, 8 bytes each, and the call
 * clears them from the request at once. Equal passwords fail with
 * AFPERR_PWDSAME. PAM authenticates the old password and runs acct_mgmt and
 * chauthtok, as root when seteuid allows it. Without root, the first echo-off
 * prompt of chauthtok gets the old password.
 *
 * @param[in]     obj       session object, unused
 * @param[in]     username  user name
 * @param[in]     pwd       user entry, unused
 * @param[in,out] ibuf      UAM data of the request, cleared on return
 * @param[in]     ibuflen   length of ibuf, unused
 * @param[out]    rbuf      reply block, unused
 * @param[out]    rbuflen   unused
 * @returns AFP_OK, AFPERR_PWDSAME, AFPERR_NOTAUTH, AFPERR_ACCESS when chauthtok
 *          fails, or AFPERR_PARAM
 */
static int pam_changepw(void *obj _U_, char *username,
                        struct passwd *pwd _U_, char *ibuf, size_t ibuflen _U_,
                        char *rbuf _U_, size_t *rbuflen _U_)
{
    char oldpw[PASSWDLEN + 1];
    char newpw[PASSWDLEN + 1];
    pam_handle_t *lpamh;
    uid_t uid;
    int PAM_error;
    int ret;
    int is_root;
    /* Copy both passwords into stack buffers upfront */
    memcpy(oldpw, ibuf, PASSWDLEN);
    oldpw[PASSWDLEN] = '\0';
    memcpy(newpw, ibuf + PASSWDLEN, PASSWDLEN);
    newpw[PASSWDLEN] = '\0';
    /* Clear passwords from the network buffer immediately */
    explicit_bzero(ibuf, PASSWDLEN * 2);

    /* Quick check for the same password */
    if (atalk_ct_memcmp(oldpw, newpw, PASSWDLEN) == 0) {
        explicit_bzero(oldpw, PASSWDLEN);
        explicit_bzero(newpw, PASSWDLEN);
        return AFPERR_PWDSAME;
    }

    /* Set up for pam_authenticate conv: needs old password */
    PAM_username = username;
    PAM_password = oldpw;
    PAM_error = pam_start("netatalk", username, &PAM_conversation,
                          &lpamh);

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_error, NULL, username, "pam_start", PAM_error);
        ret = AFPERR_PARAM;
        goto changepw_done;
    }

    pam_set_item(lpamh, PAM_TTY, "afpd");
    pam_set_item(lpamh, PAM_RHOST, hostname);
    /* We might need to do this as root */
    uid = geteuid();
    is_root = (seteuid(0) == 0);

    if (!is_root) {
        LOG(log_error, logtype_uams, "pam_changepw: could not seteuid(%i)", 0);
    }

    struct sigaction sa_dfl2 = {0}, sa_old2;
    int sigchld_saved2;
    sa_dfl2.sa_handler = SIG_DFL;
    sigemptyset(&sa_dfl2.sa_mask);
    sigchld_saved2 = (sigaction(SIGCHLD, &sa_dfl2, &sa_old2) == 0);
    PAM_error = pam_authenticate(lpamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_info, lpamh, username, "pam_authenticate",
                             PAM_error);

        if (sigchld_saved2) {
            sigaction(SIGCHLD, &sa_old2, NULL);
        }

        ret = AFPERR_NOTAUTH;
        goto changepw_restore_uid;
    }

    PAM_error = pam_acct_mgmt(lpamh, 0);

    if (sigchld_saved2) {
        sigaction(SIGCHLD, &sa_old2, NULL);
    }

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_info, lpamh, username, "pam_acct_mgmt", PAM_error);
        ret = AFPERR_NOTAUTH;
        goto changepw_restore_uid;
    }

    /*
     * Set up for pam_chauthtok conv: needs new password, and
     * possibly old password if PAM prompts for the current password.
     *
     * As root, pam_unix won't prompt for the current password,
     * so all ECHO_OFF prompts should return the new password.
     *
     * Not as root, pam_unix prompts for the current password first,
     * so the first ECHO_OFF returns old, subsequent return new.
     */
    PAM_password = newpw;

    if (!is_root) {
        PAM_oldpassword = oldpw;
        PAM_chauthtok_mode = 1;
        PAM_chauthtok_count = 0;
    }

    PAM_error = pam_chauthtok(lpamh, 0);
    PAM_chauthtok_mode = 0;

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_info, lpamh, username, "pam_chauthtok", PAM_error);
        ret = AFPERR_ACCESS;
        goto changepw_restore_uid;
    }

    ret = AFP_OK;
changepw_restore_uid:

    if (is_root && seteuid(uid) < 0) {
        LOG(log_error, logtype_uams, "pam_changepw: could not seteuid(%i)", uid);
    }

    pam_end(lpamh, PAM_error);
changepw_done:
    PAM_password = NULL;
    PAM_oldpassword = NULL;
    explicit_bzero(oldpw, PASSWDLEN);
    explicit_bzero(newpw, PASSWDLEN);
    return ret;
}


/*!
 * @brief Check a ClearTxtUAM printer login with PAM and close its session at
 * once
 *
 * The login string has the form `(username) (password)`, and the password
 * keeps up to 8 bytes. The user has to pass uam_checkuser, and PAM runs
 * authenticate, acct_mgmt, and open_session. A success writes the "0" answer
 * to out and closes the session right away.
 *
 * @param[in]  start     start of the login string
 * @param[in]  stop      end of the login string
 * @param[out] username  receives the user name, up to UAM_USERNAMELEN bytes
 *                       without a NUL
 * @param[out] out       PAP reply, receives the answer
 * @returns 0, or -1 for a malformed string, a refused user, or a failed PAM
 *          step
 */
static int pam_printer(char *start, char *stop, char *username,
                       struct papfile *out)
{
    int PAM_error;
    char *data;
    const char *p;
    const char *q;
    static const char *loginok = "0\r";
    struct passwd *pwd;
    data = (char *)malloc(stop - start + 1);

    if (!data) {
        LOG(log_error, logtype_uams, "Bad Login ClearTxtUAM: malloc");
        return -1;
    }

    strlcpy(data, start, stop - start + 1);

    /* We are looking for the following format in data:
     * (username) (password)
     *
     * Let's hope username doesn't contain ") ("!
     */

    /* Parse input for username in () */
    if ((p = strchr(data, '(')) == NULL) {
        LOG(log_error, logtype_uams,
            "Bad Login ClearTxtUAM: username not found in string");
        free(data);
        return -1;
    }

    p++;

    if ((q = strstr(p, ") (")) == NULL) {
        LOG(log_error, logtype_uams,
            "Bad Login ClearTxtUAM: username not found in string");
        free(data);
        return -1;
    }

    memcpy(username, p, MIN(UAM_USERNAMELEN, q - p));
    /* Parse input for password in next () */
    p = q + 3;

    if ((q = strrchr(p, ')')) == NULL) {
        LOG(log_error, logtype_uams,
            "Bad Login ClearTxtUAM: password not found in string");
        free(data);
        return -1;
    }

    /* Allocate memory for the global PAM_password */
    PAM_password = (char *)malloc(PASSWDLEN + 1);

    if (!PAM_password) {
        LOG(log_error, logtype_uams,
            "Bad Login ClearTxtUAM: malloc failed for password");
        free(data);
        return -1;
    }

    explicit_bzero(PAM_password, PASSWDLEN + 1);
    memcpy(PAM_password, p, MIN(PASSWDLEN, (q - p)));
    /* Done copying username and password, clean up */
    free(data);

    if ((pwd = uam_getname(NULL, username, strlen(username))) == NULL) {
        LOG(log_error, logtype_uams, "Bad Login ClearTxtUAM: ( %s ) not found ",
            username);
        free(PAM_password);
        PAM_password = NULL;
        return -1;
    }

    if (uam_checkuser(NULL, pwd) < 0) {
        /* syslog of error happens in uam_checkuser */
        free(PAM_password);
        PAM_password = NULL;
        return -1;
    }

    PAM_username = username;
    PAM_error = pam_start("netatalk", username, &PAM_conversation,
                          &pamh);

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_error, NULL, username, "pam_start", PAM_error);
        pamh = NULL;
        free(PAM_password);
        PAM_password = NULL;
        return -1;
    }

    pam_set_item(pamh, PAM_TTY, "papd");
    pam_set_item(pamh, PAM_RHOST, hostname);
    struct sigaction sa_dfl3 = {0}, sa_old3;
    int sigchld_saved3;
    sa_dfl3.sa_handler = SIG_DFL;
    sigemptyset(&sa_dfl3.sa_mask);
    sigchld_saved3 = (sigaction(SIGCHLD, &sa_dfl3, &sa_old3) == 0);
    PAM_error = pam_authenticate(pamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        if (sigchld_saved3) {
            sigaction(SIGCHLD, &sa_old3, NULL);
        }

        clrtxt_log_pam_error(log_info, pamh, username, "pam_authenticate",
                             PAM_error);
        pam_end(pamh, PAM_error);
        pamh = NULL;
        free(PAM_password);
        PAM_password = NULL;
        return -1;
    }

    PAM_error = pam_acct_mgmt(pamh, 0);

    if (sigchld_saved3) {
        sigaction(SIGCHLD, &sa_old3, NULL);
    }

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_error, pamh, username, "pam_acct_mgmt", PAM_error);
        pam_end(pamh, PAM_error);
        pamh = NULL;
        free(PAM_password);
        PAM_password = NULL;
        return -1;
    }

    PAM_error = pam_open_session(pamh, 0);

    if (PAM_error != PAM_SUCCESS) {
        clrtxt_log_pam_error(log_error, pamh, username, "pam_open_session",
                             PAM_error);
        pam_end(pamh, PAM_error);
        pamh = NULL;
        free(PAM_password);
        PAM_password = NULL;
        return -1;
    }

    /* Login successful, but no need to hang onto it,
       so logout immediately */
    append(out, loginok, strlen(loginok));
    LOG(log_warning, logtype_uams, "Login ClearTxtUAM: %s (INSECURE)", username);
    pam_close_session(pamh, 0);
    pam_end(pamh, 0);
    pamh = NULL;
    /* Before returning, free the allocated password */
    free(PAM_password);
    PAM_password = NULL;
    return 0;
}


/*!
 * @brief Register the Cleartxt Passwrd login and password change and the
 * ClearTxtUAM printer login
 *
 * A failed password change registration removes the login again.
 *
 * @param[in] obj   AFP object, unused
 * @param[in] path  path of the module
 * @returns 0, or -1 when a registration fails
 */
static int uam_setup(void *obj _U_, const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "Cleartxt Passwrd",
                     pam_login, NULL, pam_logout, pam_login_ext) < 0) {
        return -1;
    }

    if (uam_register(UAM_SERVER_CHANGEPW, path, "Cleartxt Passwrd",
                     pam_changepw) < 0) {
        uam_unregister(UAM_SERVER_LOGIN, "Cleartxt Passwrd");
        return -1;
    }

    if (uam_register(UAM_SERVER_PRINTAUTH, path, "ClearTxtUAM",
                     pam_printer) < 0) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Unregister the Cleartxt Passwrd login and password change and the
 * ClearTxtUAM printer login
 */
static void uam_cleanup(void)
{
    uam_unregister(UAM_SERVER_LOGIN, "Cleartxt Passwrd");
    uam_unregister(UAM_SERVER_CHANGEPW, "Cleartxt Passwrd");
    uam_unregister(UAM_SERVER_PRINTAUTH, "ClearTxtUAM");
}

UAM_MODULE_EXPORT struct uam_export uams_clrtxt = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};

UAM_MODULE_EXPORT struct uam_export uams_pam = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};
