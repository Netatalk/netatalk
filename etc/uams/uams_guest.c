/*
 * $Id: uams_guest.c,v 1.4 2001-02-27 17:07:43 rufustfirefly Exp $
 *
 * (c) 2001 (see COPYING)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <syslog.h>

#include <atalk/afp.h>
#include <atalk/uam.h>

static int noauth_login(void *obj, struct passwd **uam_pwd,
			char *ibuf, int ibuflen, 
			char *rbuf, int *rbuflen)
{
    struct passwd *pwent;
    char *guest, *username;

    *rbuflen = 0;
    syslog( LOG_INFO, "login noauth" );

    if (uam_afpserver_option(obj, UAM_OPTION_GUEST, (void *) &guest,
			     NULL) < 0)
      return AFPERR_MISC;

    if (uam_afpserver_option(obj, UAM_OPTION_USERNAME, 
			     (void *) &username, NULL) < 0)
      return AFPERR_MISC;

    strcpy(username, guest);
    if ((pwent = getpwnam(guest)) == NULL) {
	syslog( LOG_ERR, "noauth_login: getpwnam( %s ): %m", guest);
	return( AFPERR_BADUAM );
    }

#ifdef AFS
    if ( setpag() < 0 ) {
	syslog( LOG_ERR, "noauth_login: setpag: %m" );
	return( AFPERR_BADUAM );
    }
#endif /* AFS */

    *uam_pwd = pwent;
    return( AFP_OK );
}


/* Printer NoAuthUAM Login */
int noauth_printer(start, stop, username, out)
	char	*start, *stop, *username;
	struct papfile	*out;
{
    char	*data, *p, *q;
    static const char *loginok = "0\r";

    data = (char *)malloc(stop - start + 1);
    strncpy(data, start, stop - start + 1);

    /* We are looking for the following format in data:
     * (username)
     *
     * Hopefully username doesn't contain a ")"
     */

    if ((p = strchr(data, '(' )) == NULL) {
	syslog(LOG_INFO,"Bad Login NoAuthUAM: username not found in string");
	free(data);
	return(-1);
    }
    p++;
    if ((q = strchr(data, ')' )) == NULL) {
	syslog(LOG_INFO,"Bad Login NoAuthUAM: username not found in string");
	free(data);
	return(-1);
    }
    strncpy(username, p, q - p);

    /* Done copying username, clean up */
    free(data);

    if (getpwnam(username) == NULL) {
	syslog(LOG_INFO, "Bad Login NoAuthUAM: %s: %m", username);
	return(-1);
    }

    /* Login successful */
    append(out, loginok, strlen(loginok));
    syslog(LOG_INFO, "Login NoAuthUAM: %s", username);
    return(0);
}


static int uam_setup(const char *path)
{
  if (uam_register(UAM_SERVER_LOGIN, path, "No User Authent",
	       noauth_login, NULL, NULL) < 0)
	return -1;
  if (uam_register(UAM_SERVER_PRINTAUTH, path, "NoAuthUAM",
		noauth_printer) < 0)
	return -1;

  return 0;
}

static void uam_cleanup()
{
  uam_unregister(UAM_SERVER_LOGIN, "No User Authent");
  uam_unregister(UAM_SERVER_PRINTAUTH, "NoAuthUAM");
}

UAM_MODULE_EXPORT struct uam_export uams_guest = {
  UAM_MODULE_SERVER,
  UAM_MODULE_VERSION,
  uam_setup, uam_cleanup
};
