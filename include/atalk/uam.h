/* Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef UAM_H
#define UAM_H 1

#include <sys/cdefs.h>
#include <pwd.h>
#include <stdarg.h>

#ifdef TRU64
#include <sia.h>
#include <siad.h>
#endif /* TRU64 */

/* just a label for exported bits */
#define UAM_MODULE_EXPORT

/* type of uam */
#define UAM_MODULE_SERVER   	 1
#define UAM_MODULE_CLIENT  	 2

/* in case something drastic has to change */
#define UAM_MODULE_VERSION       1

/* things for which we can have uams */
#define UAM_SERVER_LOGIN         (1 << 0)
#define UAM_SERVER_CHANGEPW      (1 << 1)
#define UAM_SERVER_PRINTAUTH     (1 << 2) 

/* options */
#define UAM_OPTION_USERNAME     (1 << 0) /* get space for username */
#define UAM_OPTION_GUEST        (1 << 1) /* get guest user */
#define UAM_OPTION_PASSWDOPT    (1 << 2) /* get the password file */
#define UAM_OPTION_SIGNATURE    (1 << 3) /* get server signature */
#define UAM_OPTION_RANDNUM      (1 << 4) /* request a random number */
#define UAM_OPTION_HOSTNAME     (1 << 5) /* get host name */
#define UAM_OPTION_COOKIE       (1 << 6) /* cookie handle */
#define UAM_OPTION_PROTOCOL	(1 << 7) /* DSI or ASP */
#define UAM_OPTION_CLIENTNAME   (1 << 8) /* get client IP address */

/* some password options. you pass these in the length parameter and
 * get back the corresponding option. not all of these are implemented. */
#define UAM_PASSWD_FILENAME     (1 << 0)
#define UAM_PASSWD_MINLENGTH    (1 << 1)
#define UAM_PASSWD_MAXFAIL      (1 << 2) /* not implemented yet. */
#define UAM_PASSWD_EXPIRETIME   (1 << 3) /* not implemented yet. */

/* i'm doing things this way because os x server's dynamic linker
 * support is braindead. it also allows me to do a little versioning. */
struct uam_export {
  int uam_type, uam_version;
  int (*uam_setup)(const char *);
  void (*uam_cleanup)(void);
};

/* register and unregister uams with these functions */
extern int uam_register __P((const int, const char *, const char *, ...));
extern void uam_unregister __P((const int, const char *));

/* helper functions */
extern struct passwd *uam_getname __P((char *, const int));
extern int uam_checkuser __P((const struct passwd *));

/* afp helper functions */
extern int uam_afp_read __P((void *, char *, int *,
			     int (*)(void *, void *, const int)));
extern int uam_afpserver_option __P((void *, const int, void *, int *));
#ifdef TRU64
extern void uam_afp_getcmdline __P((int *, char ***));
extern int uam_sia_validate_user __P((sia_collect_func_t *, int, char **,
                                     char *, char *, char *, int, char *,
                                     char *));
#endif /* TRU64 */

/* switch.c */
#define UAM_AFPSERVER_PREAUTH  (0)
#define UAM_AFPSERVER_POSTAUTH (1 << 0)

extern int uam_afpserver_action __P((const int /*id*/, const int /*switch*/, 
				     int (**)(), int (**)()));

#endif
