/* Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu) 
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef NO_CRYPT_H
#include <crypt.h>
#endif
#include <pwd.h>
#include <syslog.h>

#ifdef SOLARIS
#define SHADOWPW
#endif SOLARIS

#ifdef SHADOWPW
#include <shadow.h>
#endif SHADOWPW

#include <atalk/afp.h>
#include <atalk/uam.h>

#define PASSWDLEN 8

/* cleartxt login */
static int passwd_login(void *obj, struct passwd **uam_pwd,
			char *ibuf, int ibuflen,
			char *rbuf, int *rbuflen)
{
    struct passwd *pwd;
#ifdef SHADOWPW
    struct spwd *sp;
#endif
    char *username, *p;
    int len, ulen;

    *rbuflen = 0;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME,
			     (void *) &username, &ulen) < 0)
      return AFPERR_MISC;

    len = (unsigned char) *ibuf++;
    if ( len > ulen ) {
	return( AFPERR_PARAM );
    }

    memcpy(username, ibuf, len );
    ibuf += len;
    username[ len ] = '\0';
    if ((unsigned long) ibuf & 1) /* pad character */
      ++ibuf;
    ibuf[ PASSWDLEN ] = '\0';

    if (( pwd = uam_getname(username, ulen)) == NULL ) {
	return AFPERR_PARAM;
    }

    syslog(LOG_INFO, "cleartext login: %s", username);
    if (uam_checkuser(pwd) < 0)
      return AFPERR_NOTAUTH;

#ifdef SHADOWPW
    if (( sp = getspnam( pwd->pw_name )) == NULL ) {
	syslog( LOG_INFO, "no shadow passwd entry for %s", username);
	return AFPERR_NOTAUTH;
    }
    pwd->pw_passwd = sp->sp_pwdp;
#endif SHADOWPW

    if (!pwd->pw_passwd)
      return AFPERR_NOTAUTH;

    *uam_pwd = pwd;

    p = crypt( ibuf, pwd->pw_passwd );
    if ( strcmp( p, pwd->pw_passwd ) == 0 ) 
      return AFP_OK;

    return AFPERR_NOTAUTH;
}


#if 0
/* change passwd */
static int passwd_changepw(void *obj, char *username,
			   struct passwd *pwd, char *ibuf,
			   int ibuflen, char *rbuf, int *rbuflen)
{
#ifdef SHADOWPW
    struct spwd *sp;
#endif
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
	syslog( LOG_INFO, "no shadow passwd entry for %s", username);
	return AFPERR_PARAM;
    }
    pwd->pw_passwd = sp->sp_pwdp;
#endif SHADOWPW

    p = crypt(pw, pwd->pw_passwd );
    if (strcmp( p, pwd->pw_passwd )) {
      memset(pw, 0, sizeof(pw));
      return AFPERR_NOTAUTH;
    }

    /* new password */
    ibuf += PASSWDLEN;
    ibuf[PASSWDLEN] = '\0';
    
#ifdef SHADOWPW
#else
#endif
    return AFP_OK;
}
#endif


/* Printer ClearTxtUAM login */
static int passwd_printer(start, stop, username, out)
	char	*start, *stop, *username;
	struct papfile	*out;
{
    struct passwd *pwd;
#ifdef SHADOWPW
    struct spwd *sp;
#endif
    char *data, *p, *q;
    char	password[PASSWDLEN + 1] = "\0";
    static const char *loginok = "0\r";
    int ulen;

    data = (char *)malloc(stop - start + 1);
    strncpy(data, start, stop - start + 1);

    /* We are looking for the following format in data:
     * (username) (password)
     *
     * Let's hope username doesn't contain ") ("!
     */

    /* Parse input for username in () */
    if ((p = strchr(data, '(' )) == NULL) {
        syslog(LOG_INFO,"Bad Login ClearTxtUAM: username not found in string");
        free(data);
        return(-1);
    }
    p++;
    if ((q = strstr(data, ") (" )) == NULL) {
        syslog(LOG_INFO,"Bad Login ClearTxtUAM: username not found in string");
        free(data);
        return(-1);
    }
    strncpy(username, p, q - p);

    /* Parse input for password in next () */
    p = q + 3;
    if ((q = strrchr(data, ')' )) == NULL) {
        syslog(LOG_INFO,"Bad Login ClearTxtUAM: password not found in string");
	free(data);
        return(-1);
    }
    strncpy(password, p, q - p);

    /* Done copying username and password, clean up */
    free(data);

    ulen = strlen(username);

    if (( pwd = uam_getname(username, ulen)) == NULL ) {
	syslog(LOG_INFO, "Bad Login ClearTxtUAM: ( %s ) not found ",
			username);
	return(-1);
    }

    if (uam_checkuser(pwd) < 0) {
	/* syslog of error happens in uam_checkuser */
	return(-1);
    }

#ifdef SHADOWPW
    if (( sp = getspnam( pwd->pw_name )) == NULL ) {
	syslog(LOG_INFO, "Bad Login ClearTxtUAM: no shadow passwd entry for %s", 
			username);
	return(-1);
    }
    pwd->pw_passwd = sp->sp_pwdp;
#endif SHADOWPW

    if (!pwd->pw_passwd) {
	syslog(LOG_INFO, "Bad Login ClearTxtUAM: no password for %s",
			username);
	return(-1);
    }

#ifdef AFS
    if ( kcheckuser( pwd, password) == 0) 
      return(0);
#endif AFS

    p = crypt(password, pwd->pw_passwd);
    if (strcmp(p, pwd->pw_passwd) != 0) {
	syslog(LOG_INFO, "Bad Login ClearTxtUAM: %s: bad password", username);
	return(-1);
    }

    /* Login successful */
    append(out, loginok, strlen(loginok));
    syslog(LOG_INFO, "Login ClearTxtUAM: %s", username);
    return(0);
}


static int uam_setup(const char *path)
{
  if (uam_register(UAM_SERVER_LOGIN, path, "Cleartxt Passwrd", 
		passwd_login, NULL, NULL) < 0)
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

UAM_MODULE_EXPORT struct uam_export uams_passwd = {
  UAM_MODULE_SERVER,
  UAM_MODULE_VERSION,
  uam_setup, uam_cleanup
};
