/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu) 
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif /* ! HAVE_CRYPT_H */
#include <pwd.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef SHADOWPW
#include <shadow.h>
#endif /* SHADOWPW */

#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/uam.h>
#include <atalk/util.h>
#include <atalk/standards.h>

#define PASSWDLEN 8

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif /* MIN */



/*XXX in etc/papd/file.h */
struct papfile;
extern UAM_MODULE_EXPORT void append(struct papfile *, const char *, int);

static int pwd_login(void *obj, char *username, int ulen, struct passwd **uam_pwd,
                        char *ibuf, size_t ibuflen,
                        char *rbuf _U_, size_t *rbuflen _U_)
{
    char  *p;
    struct passwd *pwd;
    int err = AFP_OK;
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */


    if (ibuflen <= PASSWDLEN) {
        return( AFPERR_PARAM );
    }
    ibuf[ PASSWDLEN ] = '\0';

    if (( pwd = uam_getname(obj, username, ulen)) == NULL ) {
        return AFPERR_NOTAUTH;
    }

    LOG(log_info, logtype_uams, "cleartext login: %s", username);

    if (uam_checkuser(pwd) < 0) {
        LOG(log_info, logtype_uams, "not a valid user");
        return AFPERR_NOTAUTH;
    }

#ifdef SHADOWPW
    if (( sp = getspnam( pwd->pw_name )) == NULL ) {
        LOG(log_info, logtype_uams, "no shadow passwd entry for %s", username);
        return AFPERR_NOTAUTH;
    }
    pwd->pw_passwd = sp->sp_pwdp;

    if (sp->sp_max != -1 && sp->sp_lstchg) {
        time_t now = time(NULL) / (60*60*24);
        int32_t expire_days = sp->sp_lstchg - now + sp->sp_max;
        if ( expire_days < 0 ) {
                LOG(log_info, logtype_uams, "Password for user %s expired", username);
		err = AFPERR_PWDEXPR;
        }
    }
#endif /* SHADOWPW */

    if (!pwd->pw_passwd) {
        return AFPERR_NOTAUTH;
    }

    *uam_pwd = pwd;

    p = crypt( ibuf, pwd->pw_passwd );
    if ( strcmp( p, pwd->pw_passwd ) == 0 )
        return err;

    return AFPERR_NOTAUTH;

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
                             (void *) &username, &ulen) < 0)
        return AFPERR_MISC;

    if (ibuflen < 2) {
        return( AFPERR_PARAM );
    }

    len = (unsigned char) *ibuf++;
    ibuflen--;
    if (!len || len > ibuflen || len > ulen ) {
        return( AFPERR_PARAM );
    }
    memcpy(username, ibuf, len );
    ibuf += len;
    ibuflen -=len;
    username[ len ] = '\0';

    if ((unsigned long) ibuf & 1) { /* pad character */
        ++ibuf;
        ibuflen--;
    }
    return (pwd_login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen));
    
}

/* cleartxt login ext 
 * uname format :
    byte      3
    2 bytes   len (network order)
    len bytes unicode name
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
                             (void *) &username, &ulen) < 0)
        return AFPERR_MISC;

    if (*uname != 3 || ibuflen < 2)
        return AFPERR_PARAM;
    uname++;
    memcpy(&temp16, uname, sizeof(temp16));
    len = ntohs(temp16);
    if (!len || len > ulen ) {
        return( AFPERR_PARAM );
    }
    memcpy(username, uname +2, len );
    username[ len ] = '\0';
    return (pwd_login(obj, username, ulen, uam_pwd, ibuf, ibuflen, rbuf, rbuflen));
}

/* Printer ClearTxtUAM login */
static int passwd_printer(char	*start, char *stop, char *username, struct papfile *out)
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
	LOG(log_info, logtype_uams,"Bad Login ClearTxtUAM: malloc");
	return(-1);
    }
    strlcpy(data, start, stop - start + 1);

    /* We are looking for the following format in data:
     * (username) (password)
     *
     * Let's hope username doesn't contain ") ("!
     */

    /* Parse input for username in () */
    if ((p = strchr(data, '(' )) == NULL) {
        LOG(log_info, logtype_uams,"Bad Login ClearTxtUAM: username not found in string");
        free(data);
        return(-1);
    }
    p++;
    if ((q = strstr(p, ") (" )) == NULL) {
        LOG(log_info, logtype_uams,"Bad Login ClearTxtUAM: username not found in string");
        free(data);
        return(-1);
    }
    memcpy(username, p,  MIN( UAM_USERNAMELEN, q - p ));

    /* Parse input for password in next () */
    p = q + 3;
    if ((q = strrchr(p , ')' )) == NULL) {
        LOG(log_info, logtype_uams,"Bad Login ClearTxtUAM: password not found in string");
        free(data);
        return(-1);
    }
    memcpy(password, p, MIN(PASSWDLEN, q - p) );

    /* Done copying username and password, clean up */
    free(data);

    ulen = strlen(username);

    if (( pwd = uam_getname(NULL, username, ulen)) == NULL ) {
        LOG(log_info, logtype_uams, "Bad Login ClearTxtUAM: ( %s ) not found ",
            username);
        return(-1);
    }

    if (uam_checkuser(pwd) < 0) {
        /* syslog of error happens in uam_checkuser */
        return(-1);
    }

#ifdef SHADOWPW
    if (( sp = getspnam( pwd->pw_name )) == NULL ) {
        LOG(log_info, logtype_uams, "Bad Login ClearTxtUAM: no shadow passwd entry for %s",
            username);
        return(-1);
    }
    pwd->pw_passwd = sp->sp_pwdp;

    if (sp->sp_max != -1 && sp->sp_lstchg) {
        time_t now = time(NULL) / (60*60*24);
        int32_t expire_days = sp->sp_lstchg - now + sp->sp_max;
        if ( expire_days < 0 ) {
                LOG(log_info, logtype_uams, "Password for user %s expired", username);
		return (-1);
        }
    }

#endif /* SHADOWPW */

    if (!pwd->pw_passwd) {
        LOG(log_info, logtype_uams, "Bad Login ClearTxtUAM: no password for %s",
            username);
        return(-1);
    }

    p = crypt(password, pwd->pw_passwd);
    if (strcmp(p, pwd->pw_passwd) != 0) {
        LOG(log_info, logtype_uams, "Bad Login ClearTxtUAM: %s: bad password", username);
        return(-1);
    }

    /* Login successful */
    append(out, loginok, strlen(loginok));
    LOG(log_info, logtype_uams, "Login ClearTxtUAM: %s", username);
    return(0);
}

static int uam_setup(const char *path)
{
    if (uam_register(UAM_SERVER_LOGIN_EXT, path, "Cleartxt Passwrd",
                     passwd_login, NULL, NULL, passwd_login_ext) < 0)
        return -1;
    if (uam_register(UAM_SERVER_PRINTAUTH, path, "ClearTxtUAM",
                     passwd_printer) < 0)
        return -1;

    return 0;
}

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
