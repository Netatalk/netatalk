/*
 * $Id: auth.c,v 1.9 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <netatalk/endian.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/util.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <atalk/logger.h>

#include "uam_auth.h"

static struct uam_mod uam_modules = {NULL, NULL, &uam_modules, &uam_modules};
static struct uam_obj uam_login = {"", "", 0, {{NULL, NULL, NULL}}, &uam_login,
				   &uam_login};
static struct uam_obj uam_changepw = {"", "", 0, {{NULL, NULL, NULL}}, &uam_changepw, 
				      &uam_changepw};
static struct uam_obj uam_printer = {"", "", 0, {{NULL, NULL, NULL}}, &uam_printer,
					&uam_printer};


/*
 * Return a list of names for loaded uams
 */
int getuamnames(const int type, char *uamnames)
{
    struct uam_obj *prev, *start;
  
    if (!(start = UAM_LIST(type)))
        return(-1);
  
    prev = start;
      
    while((prev = prev->uam_prev) != start) {
        strcat(uamnames, prev->uam_name);
        strcat(uamnames, "\n");
    }
 
    strcat(uamnames, "*\n");
    return(0);
}
  

/* just do a linked list search. this could be sped up with a hashed
 * list, but i doubt anyone's going to have enough uams to matter. */
struct uam_obj *auth_uamfind(const int type, const char *name, 
			     const int len)
{
  struct uam_obj *prev, *start;

  if (!name || !(start = UAM_LIST(type)))
    return NULL;

  prev = start;
  while ((prev = prev->uam_prev) != start) 
    if (strndiacasecmp(prev->uam_name, name, len) == 0)
      return prev;

  return NULL;
}

int auth_register(const int type, struct uam_obj *uam)
{
  struct uam_obj *start;

  if (!uam || !uam->uam_name || (*uam->uam_name == '\0'))
    return -1;

  if (!(start = UAM_LIST(type)))
    return 1; 

  uam_attach(start, uam);
  return 0;
}

/* load all of the modules */
int auth_load(const char *path, const char *list)
{
  char name[MAXPATHLEN + 1], buf[MAXPATHLEN + 1], *p; 
  struct uam_mod *mod;
  struct stat st;
  size_t len;
  
  if (!path || !list || (len = strlen(path)) > sizeof(name) - 2)
    return -1;

  strlcpy(buf, list, sizeof(buf));
  if ((p = strtok(buf, ",")) == NULL)
    return -1;

  strcpy(name, path);
  if (name[len - 1] != '/') {
    strcat(name, "/");
    len++;
  }

  while (p) {
    strlcpy(name + len, p, sizeof(name) - len);
    if ((stat(name, &st) == 0) && (mod = uam_load(name, p))) {
      uam_attach(&uam_modules, mod);
      LOG(log_info, logtype_papd, "uam: %s loaded", p);
    }
    p = strtok(NULL, ",");
  }
  return 0;
}

/* get rid of all of the uams */
void auth_unload(void)
{
  struct uam_mod *mod, *prev, *start = &uam_modules;

  prev = start->uam_prev;
  while ((mod = prev) != start) {
    prev = prev->uam_prev;
    uam_detach(mod);
    uam_unload(mod);
  }
}
