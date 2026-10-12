/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief Cleartext UAM with Unix password authentication and the ClearTxtUAM
 * printer login
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif /* ! HAVE_CRYPT_H */

#ifdef SHADOWPW
#include <shadow.h>
#endif /* SHADOWPW */

#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/logger.h>
#include <atalk/uam.h>
#include <atalk/util.h>

#define PASSWDLEN 8

/*XXX in etc/papd/file.h */
struct papfile;
extern UAM_MODULE_EXPORT void append(struct papfile *, const char *, int);

/*!
 * @brief Check a cleartext password of up to 8 bytes against the password hash
 *
 * The user has to pass uam_checkuser. With shadow passwords an expired
 * password answers AFPERR_PWDEXPR before the password check. The password
 * goes to crypt_checkpass, or to crypt and a comparison, and a match wipes
 * it from the request.
 *
 * @param[in]     obj       session object
 * @param[in,out] username  user name from the request, receives the account
 *                          name of a match
 * @param[in]     ulen      size of the username buffer
 * @param[out]    uam_pwd   receives the user entry before the password check
 * @param[in,out] ibuf      password field of the request, cut after 8 bytes
 * @param[in]     ibuflen   length of ibuf, at least 8
 * @param[out]    rbuf      reply block, unused
 * @param[out]    rbuflen   unused
 * @returns AFP_OK, AFPERR_PWDEXPR, AFPERR_NOTAUTH, or AFPERR_PARAM
 */
static int pwd_login(void *obj, char *username, int ulen,
                     struct passwd **uam_pwd,
                     char *ibuf, size_t ibuflen,
                     char *rbuf _U_, size_t *rbuflen _U_)
{
    char  *p;
    struct passwd *pwd;
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */

    if (ibuflen < PASSWDLEN) {
        return AFPERR_PARAM;
    }

    ibuf[PASSWDLEN] = '\0';

    if ((pwd = uam_getname(obj, username, ulen)) == NULL) {
        return AFPERR_NOTAUTH;
    }

    LOG(log_warning, logtype_uams, "cleartext login: %s (INSECURE)", username);

    if (uam_checkuser(obj, pwd) < 0) {
        LOG(log_info, logtype_uams, "not a valid user");
        return AFPERR_NOTAUTH;
    }

#ifdef SHADOWPW

    if ((sp = getspnam(pwd->pw_name)) == NULL) {
        LOG(log_info, logtype_uams, "no shadow passwd entry for %s", username);
        return AFPERR_NOTAUTH;
    }

    pwd->pw_passwd = sp->sp_pwdp;

    if (sp->sp_max != -1 && sp->sp_lstchg) {
        time_t now = time(NULL) / (60 * 60 * 24);
        int32_t expire_days = sp->sp_lstchg - now + sp->sp_max;

        if (expire_days < 0) {
            LOG(log_info, logtype_uams, "Password for user %s expired", username);
            return AFPERR_PWDEXPR;
        }
    }

#endif /* SHADOWPW */

    if (!pwd->pw_passwd) {
        return AFPERR_NOTAUTH;
    }

    *uam_pwd = pwd;
#ifdef HAVE_CRYPT_CHECKPASS

    if (crypt_checkpass(ibuf, pwd->pw_passwd) == 0) {
#else
    p = crypt(ibuf, pwd->pw_passwd);

    if (strcmp(p, pwd->pw_passwd) == 0) {
#endif
        explicit_bzero(ibuf, PASSWDLEN);
        return AFP_OK;
    }

    return AFPERR_NOTAUTH;
}

/*!
 * @brief Read the user name of a cleartext FPLogin request and check its
 * password against the hash
 *
 * The request holds the name as a Pascal string, padded to an even address,
 * and then the password.
 *
 * @param[in]  obj      session object
 * @param[out] uam_pwd  receives the user entry
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns the result of pwd_login, AFPERR_PARAM for an empty or oversized
 *          name, or AFPERR_MISC
 */
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

/*!
 * @brief Read the user name of a cleartext FPLoginExt request and check its
 * password against the hash
 *
 * The user name field has this form:
 * @code
 *   byte      3
 *   2 bytes   len (network order)
 *   len bytes unicode name
 * @endcode
 *
 * @param[in]  obj      session object
 * @param[in]  uname    user name field of the request
 * @param[out] uam_pwd  receives the user entry
 * @param[in]  ibuf     UAM data of the request
 * @param[in]  ibuflen  length of ibuf, at least 2
 * @param[out] rbuf     reply block, unused
 * @param[out] rbuflen  0 on return
 * @returns the result of pwd_login, AFPERR_PARAM for another name type or a bad
 *          length, or AFPERR_MISC
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

    if (*uname != 3 || ibuflen < 2) {
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

/*!
 * @brief Check a ClearTxtUAM printer login against the password hash
 *
 * The login string has the form `(username) (password)`, and the password
 * keeps up to 8 bytes. The user has to pass uam_checkuser, and an expired
 * shadow password fails. A match writes the "0" answer to out.
 *
 * @param[in]  start     start of the login string
 * @param[in]  stop      end of the login string
 * @param[out] username  receives the user name, up to UAM_USERNAMELEN bytes
 *                       without a NUL
 * @param[out] out       PAP reply, receives the answer
 * @returns 0, or -1 for a malformed string, a refused user, or a wrong password
 */
static int passwd_printer(char	*start, char *stop, char *username,
                          struct papfile *out)
{
    struct passwd *pwd;
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */
    char *data, *p, *q;
    char	password[PASSWDLEN + 1] = "\0";
    static const char *loginok = "0\r";
    int ulen;
    data = (char *)malloc(stop - start + 1);

    if (!data) {
        LOG(log_info, logtype_uams, "Bad Login ClearTxtUAM: malloc");
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
        LOG(log_info, logtype_uams,
            "Bad Login ClearTxtUAM: username not found in string");
        free(data);
        return -1;
    }

    p++;

    if ((q = strstr(p, ") (")) == NULL) {
        LOG(log_info, logtype_uams,
            "Bad Login ClearTxtUAM: username not found in string");
        free(data);
        return -1;
    }

    memcpy(username, p,  MIN(UAM_USERNAMELEN, q - p));
    /* Parse input for password in next () */
    p = q + 3;

    if ((q = strrchr(p, ')')) == NULL) {
        LOG(log_info, logtype_uams,
            "Bad Login ClearTxtUAM: password not found in string");
        free(data);
        return -1;
    }

    memcpy(password, p, MIN(PASSWDLEN, q - p));
    /* Done copying username and password, clean up */
    free(data);
    ulen = strlen(username);

    if ((pwd = uam_getname(NULL, username, ulen)) == NULL) {
        LOG(log_info, logtype_uams, "Bad Login ClearTxtUAM: ( %s ) not found ",
            username);
        return -1;
    }

    if (uam_checkuser(NULL, pwd) < 0) {
        /* syslog of error happens in uam_checkuser */
        return -1;
    }

#ifdef SHADOWPW

    if ((sp = getspnam(pwd->pw_name)) == NULL) {
        LOG(log_info, logtype_uams,
            "Bad Login ClearTxtUAM: no shadow passwd entry for %s",
            username);
        return -1;
    }

    pwd->pw_passwd = sp->sp_pwdp;

    if (sp->sp_max != -1 && sp->sp_lstchg) {
        time_t now = time(NULL) / (60 * 60 * 24);
        int32_t expire_days = sp->sp_lstchg - now + sp->sp_max;

        if (expire_days < 0) {
            LOG(log_info, logtype_uams, "Password for user %s expired", username);
            return -1;
        }
    }

#endif /* SHADOWPW */

    if (!pwd->pw_passwd) {
        LOG(log_info, logtype_uams, "Bad Login ClearTxtUAM: no password for %s",
            username);
        return -1;
    }

#ifdef HAVE_CRYPT_CHECKPASS

    if (crypt_checkpass(password, pwd->pw_passwd) != 0) {
#else
    p = crypt(password, pwd->pw_passwd);

    if (strcmp(p, pwd->pw_passwd) != 0) {
#endif
        LOG(log_info, logtype_uams, "Bad Login ClearTxtUAM: %s: bad password",
            username);
        explicit_bzero(password, sizeof(password));
        return -1;
    }

    /* Login successful */
    explicit_bzero(password, sizeof(password));
    append(out, loginok, strlen(loginok));
    LOG(log_warning, logtype_uams, "Login ClearTxtUAM: %s (INSECURE)", username);
    return 0;
}

/*!
 * @brief Register the Cleartxt Passwrd login and the ClearTxtUAM printer login
 * of the passwd module
 *
 * The module has no logout and no password change.
 *
 * @param[in] obj   AFP object, unused
 * @param[in] path  path of the module
 * @returns 0, or -1 when a registration fails
 */
static int uam_setup(void *obj, const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "Cleartxt Passwrd",
                     passwd_login, NULL, NULL, passwd_login_ext) < 0) {
        return -1;
    }

    if (uam_register(UAM_SERVER_PRINTAUTH, path, "ClearTxtUAM",
                     passwd_printer) < 0) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Unregister the Cleartxt Passwrd login and the ClearTxtUAM printer
 * login of the passwd module
 */
static void uam_cleanup(void)
{
    uam_unregister(UAM_SERVER_LOGIN, "Cleartxt Passwrd");
    uam_unregister(UAM_SERVER_PRINTAUTH, "ClearTxtUAM");
}

UAM_MODULE_EXPORT struct uam_export uams_clrtxt = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};

UAM_MODULE_EXPORT struct uam_export uams_passwd = {
    UAM_MODULE_SERVER,
    UAM_MODULE_VERSION,
    uam_setup, uam_cleanup
};
