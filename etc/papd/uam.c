/*
 * $Id: uam.c,v 1.11 2009-10-15 11:39:48 didg Exp $
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
#include <fcntl.h>
#include <ctype.h>
#include <atalk/logger.h>
#include <sys/param.h>
#include <sys/time.h>

#include <netatalk/endian.h>
#include <atalk/asp.h>
#include <atalk/dsi.h>
#include <atalk/afp.h>
#include <atalk/util.h>

#include "uam_auth.h"

/* --- server uam functions -- */

/* uam_load. uams must have a uam_setup function. */
struct uam_mod *uam_load(const char *path, const char *name)
{
  char buf[MAXPATHLEN + 1], *p;
  struct uam_mod *mod;
  void *module;

  if ((module = mod_open(path)) == NULL) {
    LOG(log_error, logtype_papd, "uam_load(%s): failed to load: %s", name, mod_error());
    return NULL;
  }

  if ((mod = (struct uam_mod *) malloc(sizeof(struct uam_mod))) == NULL) {
    LOG(log_error, logtype_papd, "uam_load(%s): malloc failed", name);
    goto uam_load_fail;
  }

  strlcpy(buf, name, sizeof(buf));
  if ((p = strchr(buf, '.')))
    *p = '\0';
  if ((mod->uam_fcn = mod_symbol(module, buf)) == NULL) {
    goto uam_load_err;
  }

  if (mod->uam_fcn->uam_type != UAM_MODULE_SERVER) {
    LOG(log_error, logtype_papd, "uam_load(%s): attempted to load a non-server module",
	   name);
    goto uam_load_err;
  }

  /* version check would go here */

  if (!mod->uam_fcn->uam_setup || 
      ((*mod->uam_fcn->uam_setup)(name) < 0)) {
    LOG(log_error, logtype_papd, "uam_load(%s): uam_setup failed", name);
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
  int ret;

  if (!name)
    return -1;

  /* see if it already exists. */
  if ((uam = auth_uamfind(type, name, strlen(name)))) {
    if (strcmp(uam->uam_path, path)) {
      /* it exists, but it's not the same module. */
      LOG(log_error, logtype_papd, "uam_register: \"%s\" already loaded by %s",
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
    uam->u.uam_printer = va_arg(ap, void *);
    break;
  default:
    break;
  }
  va_end(ap);

  /* attach to other uams */
  ret = auth_register(type, uam);
  if (ret) {
    free(uam->uam_path);
    free(uam);
  }

  return ret;
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

/* Crap to support uams which call this afpd function */
int uam_afpserver_option(void *private _U_, const int what _U_, void *option _U_,
                         size_t *len _U_)
{
	return(0);
}

/* --- helper functions for plugin uams --- */

struct passwd *uam_getname(void *dummy _U_, char *name, const int len)
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
    if ((user = strchr(pwent->pw_gecos, ','))) *user = '\0';
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
#endif /* NO_REAL_USER_NAME */

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

#ifndef DISABLE_SHELLCHECK
  if (!p) {
    LOG(log_info, logtype_papd, "illegal shell %s for %s",pwd->pw_shell,pwd->pw_name);
    return -1;
  }
#endif /* DISABLE_SHELLCHECK */

  return 0;
}


