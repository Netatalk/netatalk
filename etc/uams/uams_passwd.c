/*
 * $Id: uams_passwd.c,v 1.21 2003-06-11 07:16:14 srittau Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu) 
 * All Rights Reserved.  See COPYRIGHT.
 */

#define _XOPEN_SOURCE /* for crypt() */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>

/* STDC check */
#if STDC_HEADERS
#include <string.h>
#else /* STDC_HEADERS */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr index
#endif /* HAVE_STRCHR */
char *strchr (), *strrchr ();
#ifndef HAVE_MEMCPY
#define memcpy(d,s,n) bcopy ((s), (d), (n))
#define memmove(d,s,n) bcopy ((s), (d), (n))
#endif /* ! HAVE_MEMCPY */
#endif /* STDC_HEADERS */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifndef NO_CRYPT_H
#include <crypt.h>
#endif /* ! NO_CRYPT_H */
#include <pwd.h>
#include <atalk/logger.h>

#ifdef SOLARIS
#define SHADOWPW
#endif /* SOLARIS */

#ifdef SHADOWPW
#include <shadow.h>
#endif /* SHADOWPW */

#include <atalk/afp.h>
#include <atalk/uam.h>

#define PASSWDLEN 8

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif /* MIN */


#ifdef TRU64
#include <sia.h>
#include <siad.h>

static char *clientname;
#endif /* TRU64 */

static int pwd_login(void *obj, char *username, int ulen, struct passwd **uam_pwd,
                        char *ibuf, int ibuflen,
                        char *rbuf, int *rbuflen)
{
    char  *p;
    struct passwd *pwd;
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */

#ifdef TRU64
    if( uam_afpserver_option( obj, UAM_OPTION_CLIENTNAME,
                              (void *) &clientname, NULL ) < 0 )
        return AFPERR_MISC;
#endif /* TRU64 */

    if (ibuflen < PASSWDLEN) {
        return( AFPERR_PARAM );
    }
    ibuf[ PASSWDLEN ] = '\0';

    if (( pwd = uam_getname(username, ulen)) == NULL ) {
        return AFPERR_PARAM;
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
#endif /* SHADOWPW */

    if (!pwd->pw_passwd) {
        return AFPERR_NOTAUTH;
    }

    *uam_pwd = pwd;

#ifdef TRU64
    {
        int ac;
        char **av;
        char hostname[256];

        uam_afp_getcmdline( &ac, &av );
        sprintf( hostname, "%s@%s", username, clientname );

        if( uam_sia_validate_user( NULL, ac, av, hostname, username,
                                   NULL, FALSE, NULL, ibuf ) != SIASUCCESS )
            return AFPERR_NOTAUTH;

        return AFP_OK;
    }
#else /* TRU64 */
    p = crypt( ibuf, pwd->pw_passwd );
    if ( strcmp( p, pwd->pw_passwd ) == 0 )
        return AFP_OK;
#endif /* TRU64 */

    return AFPERR_NOTAUTH;

}

/* cleartxt login */
static int passwd_login(void *obj, struct passwd **uam_pwd,
                        char *ibuf, int ibuflen,
                        char *rbuf, int *rbuflen)
{
    char *username;
    int len, ulen;

    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *) &username, &ulen) < 0)
        return AFPERR_MISC;

    if (ibuflen <= 1) {
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
                        char *ibuf, int ibuflen,
                        char *rbuf, int *rbuflen)
{
    char       *username;
    int        len, ulen;
    u_int16_t  temp16;

    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
                             (void *) &username, &ulen) < 0)
        return AFPERR_MISC;

    if (*uname != 3)
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
                        

#if 0
/* change passwd */
static int passwd_changepw(void *obj, char *username,
                           struct passwd *pwd, char *ibuf,
                           int ibuflen, char *rbuf, int *rbuflen)
{
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */
    char pw[PASSWDLEN + 1], *p;
    uid_t uid = geteuid();

    if (uam_checkuser(pwd) < 0)
        return AFPERR_ACCESS;

    /* old password */
    memcpy(pw, ibuf, PASSWDLEN);
    memset(ibuf, 0, PASSWDLEN);
    pw[PASSWDLEN] = '\0';

#ifdef SHADOWPW
    if (( sp = getspnam( pwd->pw_name )) == NULL ) {
        LOG(log_info, logtype_uams, "no shadow passwd entry for %s", username);
        return AFPERR_PARAM;
    }
    pwd->pw_passwd = sp->sp_pwdp;
#endif /* SHADOWPW */

    p = crypt(pw, pwd->pw_passwd );
    if (strcmp( p, pwd->pw_passwd )) {
        memset(pw, 0, sizeof(pw));
        return AFPERR_NOTAUTH;
    }

    /* new password */
    ibuf += PASSWDLEN;
    ibuf[PASSWDLEN] = '\0';

#ifdef SHADOWPW
#else /* SHADOWPW */
#endif /* SHADOWPW */
    return AFP_OK;
}
#endif /* 0 */


/* Printer ClearTxtUAM login */
static int passwd_printer(start, stop, username, out)
char	*start, *stop, *username;
struct papfile	*out;
{
    struct passwd *pwd;
#ifdef SHADOWPW
    struct spwd *sp;
#endif /* SHADOWPW */
    char *data, *p, *q;
    char	password[PASSWDLEN + 1] = "\0";
    static const char *loginok = "0\r";
    int ulen;

    data = (char *)malloc(stop - start + 2);
    if (!data) {
	LOG(log_info, logtype_uams,"Bad Login ClearTxtUAM: malloc");
	return(-1);
    }
    strncpy(data, start, stop - start + 1);
    data[stop - start + 2] = 0;

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
    if ((q = strstr(data, ") (" )) == NULL) {
        LOG(log_info, logtype_uams,"Bad Login ClearTxtUAM: username not found in string");
        free(data);
        return(-1);
    }
    strncpy(username, p, MIN( UAM_USERNAMELEN, (q - p)) );
    username[ UAM_USERNAMELEN+1] = '\0';


    /* Parse input for password in next () */
    p = q + 3;
    if ((q = strrchr(data, ')' )) == NULL) {
        LOG(log_info, logtype_uams,"Bad Login ClearTxtUAM: password not found in string");
        free(data);
        return(-1);
    }
    strncpy(password, p, MIN(PASSWDLEN, q - p) );
    password[ PASSWDLEN+1] = '\0';


    /* Done copying username and password, clean up */
    free(data);

    ulen = strlen(username);

    if (( pwd = uam_getname(username, ulen)) == NULL ) {
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
#endif /* SHADOWPW */

    if (!pwd->pw_passwd) {
        LOG(log_info, logtype_uams, "Bad Login ClearTxtUAM: no password for %s",
            username);
        return(-1);
    }

#ifdef AFS
    if ( kcheckuser( pwd, password) == 0)
        return(0);
#endif /* AFS */

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

#ifdef ATACC
int uam_setup(const char *path)
{
    if (uam_register_fn(UAM_SERVER_LOGIN_EXT, path, "Cleartxt Passwrd",
                     passwd_login, NULL, NULL, passwd_login_ext) < 0)
        return -1;
    if (uam_register_fn(UAM_SERVER_PRINTAUTH, path, "ClearTxtUAM",
                     passwd_printer) < 0)
        return -1;

    return 0;
}
#else 
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

#endif

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
