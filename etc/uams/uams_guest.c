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

static int uam_setup(const char *path)
{
  if (uam_register(UAM_SERVER_LOGIN, path, "No User Authent",
	       noauth_login, NULL, NULL) < 0)
    return -1;
  /* uam_register(UAM_SERVER_PRINTAUTH, path,
     "No User Authent", noauth_printer); */

  return 0;
}

static void uam_cleanup()
{
  uam_unregister(UAM_SERVER_LOGIN, "No User Authent");
  /* uam_unregister(UAM_SERVER_PRINTAUTH, "No User Authent"); */
}

UAM_MODULE_EXPORT struct uam_export uams_guest = {
  UAM_MODULE_SERVER,
  UAM_MODULE_VERSION,
  uam_setup, uam_cleanup
};
