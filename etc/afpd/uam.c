/*
 * $Id: uam.c,v 1.10 2001-06-25 15:18:01 rufustfirefly Exp $
 *
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <ctype.h>
#include <syslog.h>
#include <sys/param.h>
#include <sys/time.h>
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif /* HAVE_DLFCN_H */

#ifdef SHADOWPW
#include <shadow.h>
#endif /* SHADOWPW */

#include <netatalk/endian.h>
#include <atalk/asp.h>
#include <atalk/dsi.h>
#include <atalk/afp.h>
#include <atalk/util.h>

#include "globals.h"
#include "afp_config.h"
#include "auth.h"
#include "uam_auth.h"

#ifdef TRU64
#include <netdb.h>
#include <arpa/inet.h>
#endif /* TRU64 */

/* --- server uam functions -- */

/* uam_load. uams must have a uam_setup function. */
struct uam_mod *uam_load(const char *path, const char *name)
{
  char buf[MAXPATHLEN + 1], *p;
  struct uam_mod *mod;
  void *module;

  if ((module = mod_open(path)) == NULL) {
    syslog(LOG_ERR, "uam_load(%s): failed to load: %s", name, mod_error());
    return NULL;
  }

  if ((mod = (struct uam_mod *) malloc(sizeof(struct uam_mod))) == NULL) {
    syslog(LOG_ERR, "uam_load(%s): malloc failed", name);
    goto uam_load_fail;
  }

  strncpy(buf, name, sizeof(buf));
  if ((p = strchr(buf, '.')))
    *p = '\0';
  if ((mod->uam_fcn = mod_symbol(module, buf)) == NULL) {
    syslog(LOG_ERR, "uam_load(%s): mod_symbol error for symbol %s",
	   name,
	   buf);
    goto uam_load_err;
  }

  if (mod->uam_fcn->uam_type != UAM_MODULE_SERVER) {
    syslog(LOG_ERR, "uam_load(%s): attempted to load a non-server module",
	   name);
    goto uam_load_err;
  }

  /* version check would go here */

  if (!mod->uam_fcn->uam_setup || 
      ((*mod->uam_fcn->uam_setup)(name) < 0)) {
    syslog(LOG_ERR, "uam_load(%s): uam_setup failed", name);
    goto uam_load_err;
  }

  mod->uam_module = module;
  return mod;

uam_load_err:
  free(mod);
uam_load_fail:
  mod_close(module);
  return NULL;
}

/* unload the module. we check for a cleanup function, but we don't
 * die if one doesn't exist. however, things are likely to leak without one.
 */
void uam_unload(struct uam_mod *mod)
{
  if (mod->uam_fcn->uam_cleanup)
    (*mod->uam_fcn->uam_cleanup)();
  mod_close(mod->uam_module);
  free(mod);
}

/* -- client-side uam functions -- */

/* set up stuff for this uam. */
int uam_register(const int type, const char *path, const char *name, ...)
{
  va_list ap;
  struct uam_obj *uam;

  if (!name)
    return -1;

  /* see if it already exists. */
  if ((uam = auth_uamfind(type, name, strlen(name)))) {
    if (strcmp(uam->uam_path, path)) {
      /* it exists, but it's not the same module. */
      syslog(LOG_ERR, "uam_register: \"%s\" already loaded by %s",
	     name, path);
      return -1;
    }
    uam->uam_count++;
    return 0;
  }
  
  /* allocate space for uam */
  if ((uam = calloc(1, sizeof(struct uam_obj))) == NULL)
    return -1;

  uam->uam_name = name;
  uam->uam_path = strdup(path);
  uam->uam_count++;

  va_start(ap, name);
  switch (type) {
  case UAM_SERVER_LOGIN: /* expect three arguments */
    uam->u.uam_login.login = va_arg(ap, void *);
    uam->u.uam_login.logincont = va_arg(ap, void *);
    uam->u.uam_login.logout = va_arg(ap, void *);
    break;
  case UAM_SERVER_CHANGEPW: /* one argument */
    uam->u.uam_changepw = va_arg(ap, void *);
    break;
  case UAM_SERVER_PRINTAUTH: /* x arguments */
  default:
    break;
  }
  va_end(ap);

  /* attach to other uams */
  if (auth_register(type, uam) < 0) {
    free(uam->uam_path);
    free(uam);
    return -1;
  }

  return 0;
}

void uam_unregister(const int type, const char *name)
{
  struct uam_obj *uam;

  if (!name)
    return;

  uam = auth_uamfind(type, name, strlen(name));
  if (!uam || --uam->uam_count > 0)
    return;

  auth_unregister(uam);
  free(uam->uam_path);
  free(uam);
}

/* --- helper functions for plugin uams --- */

struct passwd *uam_getname(char *name, const int len)
{
  struct passwd *pwent;
  char *user;
  int i;

  if ((pwent = getpwnam(name)))
    return pwent;

#ifndef NO_REAL_USER_NAME
  for (i = 0; i < len; i++)
    name[i] = tolower(name[i]);

  setpwent();
  while ((pwent = getpwent())) {
    if ((user = strchr(pwent->pw_gecos, ',')))
      *user = '\0';
    user = pwent->pw_gecos;

    /* check against both the gecos and the name fields. the user
     * might have just used a different capitalization. */
    if ((strncasecmp(user, name, len) == 0) ||
	(strncasecmp(pwent->pw_name, name, len) == 0)) {
      strncpy(name, pwent->pw_name, len);
      break;
    }
  }
  endpwent();
#endif /* ! NO_REAL_USER_NAME */

  /* os x server doesn't keep anything useful if we do getpwent */
  return pwent ? getpwnam(name) : NULL;
}

int uam_checkuser(const struct passwd *pwd)
{
  char *p;

  if (!pwd || !pwd->pw_shell || (*pwd->pw_shell == '\0')) 
    return -1;

  while ((p = getusershell())) {
    if ( strcmp( p, pwd->pw_shell ) == 0 ) 
      break;
  }
  endusershell();

#ifdef DISABLE_SHELLCHECK
  if (!p) {
    syslog( LOG_INFO, "illegal shell %s for %s", pwd->pw_shell, pwd->pw_name);
    return -1;
  }
#endif /* DISABLE_SHELLCHECK */

  return 0;
}

/* afp-specific functions */
int uam_afpserver_option(void *private, const int what, void *option,
			 int *len)
{
  AFPObj *obj = private;
  char **buf = (char **) option; /* most of the options are this */
  int32_t result;
  int fd;

  if (!obj || !option)
    return -1;

  switch (what) {
  case UAM_OPTION_USERNAME:
    *buf = (void *) obj->username;
    if (len) 
      *len = sizeof(obj->username) - 1;
    break;

  case UAM_OPTION_GUEST:
    *buf = (void *) obj->options.guest;
    if (len) 
      *len = strlen(obj->options.guest);
    break;

  case UAM_OPTION_PASSWDOPT:
    if (!len)
      return -1;

    switch (*len) {
    case UAM_PASSWD_FILENAME:
      *buf = (void *) obj->options.passwdfile;
      *len = strlen(obj->options.passwdfile);
      break;

    case UAM_PASSWD_MINLENGTH:
      *((int *) option) = obj->options.passwdminlen;
      *len = sizeof(obj->options.passwdminlen);
      break;

    case UAM_PASSWD_MAXFAIL: 
      *((int *) option) = obj->options.loginmaxfail;
      *len = sizeof(obj->options.loginmaxfail);
      break;
      
    case UAM_PASSWD_EXPIRETIME: /* not implemented */
    default:
      return -1;
    break;
    }
    break;

  case UAM_OPTION_SIGNATURE:
    *buf = (void *) (((AFPConfig *)obj->config)->signature);
    if (len)
      *len = 16;
    break;

  case UAM_OPTION_RANDNUM: /* returns a random number in 4-byte units. */
    if (!len || (*len < 0) || (*len % sizeof(result)))
      return -1;

    /* construct a random number */
    if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
      struct timeval tv;
      struct timezone tz;
      char *randnum = (char *) option;
      int i;
    
      if (gettimeofday(&tv, &tz) < 0)
	return -1;
      srandom(tv.tv_sec + (unsigned long) obj + (unsigned long) obj->handle);
      for (i = 0; i < *len; i += sizeof(result)) {
	result = random();
	memcpy(randnum + i, &result, sizeof(result));
      }
    } else {
      result = read(fd, option, *len);
      close(fd);
      if (result < 0)
	return -1;
    }
    break;

  case UAM_OPTION_HOSTNAME:
    *buf = (void *) obj->options.hostname;
    if (len) 
      *len = strlen(obj->options.hostname);
    break;

  case UAM_OPTION_PROTOCOL:
    *buf = (void *) obj->proto;
    break;
#ifdef TRU64
  case UAM_OPTION_CLIENTNAME:
    {
      struct DSI *dsi = obj->handle;
      struct hostent *hp;

      hp = gethostbyaddr( (char *) &dsi->client.sin_addr,
                          sizeof( struct in_addr ),
                          dsi->client.sin_family );
      if( hp )
          *buf = (void *) hp->h_name;
      else
          *buf = (void *) inet_ntoa( dsi->client.sin_addr );
    }
    break;
#endif /* TRU64 */
  case UAM_OPTION_COOKIE: 
    /* it's up to the uam to actually store something useful here.
     * this just passes back a handle to the cookie. the uam side
     * needs to do something like **buf = (void *) cookie to store
     * the cookie. */
    *buf = (void *) &obj->uam_cookie;
    break;

  default:
    return -1;
    break;
  }

  return 0;
}

/* if we need to maintain a connection, this is how we do it. 
 * because an action pointer gets passed in, we can stream 
 * DSI connections */
int uam_afp_read(void *handle, char *buf, int *buflen, 
		 int (*action)(void *, void *, const int))
{
  AFPObj *obj = handle;
  int len;

  if (!obj)
    return AFPERR_PARAM;

  switch (obj->proto) {
    case AFPPROTO_ASP:
      if ((len = asp_wrtcont(obj->handle, buf, buflen )) < 0) 
	goto uam_afp_read_err;
      return action(handle, buf, *buflen);
      break;

    case AFPPROTO_DSI:
      len = dsi_writeinit(obj->handle, buf, *buflen);
      if (!len || ((len = action(handle, buf, len)) < 0)) {
	dsi_writeflush(obj->handle);
	goto uam_afp_read_err;
      }
	
      while ((len = (dsi_write(obj->handle, buf, *buflen)))) {
	if ((len = action(handle, buf, len)) < 0) {
	  dsi_writeflush(obj->handle);
	  goto uam_afp_read_err;
	}
      }
      break;
  }
  return 0;

uam_afp_read_err:
  *buflen = 0;
  return len;
}

#ifdef TRU64
void uam_afp_getcmdline( int *ac, char ***av )
{
    afp_get_cmdline( ac, av );
}
#endif /* TRU64 */

/* --- papd-specific functions (just placeholders) --- */
void append(void *pf, char *data, int len)
{
	return;
}
