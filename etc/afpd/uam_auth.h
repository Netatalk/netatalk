/*
 * $Id: uam_auth.h,v 1.2 2001-06-20 18:33:04 rufustfirefly Exp $
 *
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved.  See COPYRIGHT.
 *
 * interface between uam.c and auth.c
 */

#ifndef AFPD_UAM_AUTH_H
#define AFPD_UAM_AUTH_H 1

#include <sys/cdefs.h>
#include <pwd.h>

#include <atalk/uam.h>
#include "globals.h"

struct uam_mod {
  void *uam_module;
  struct uam_export *uam_fcn;
  struct uam_mod *uam_prev, *uam_next;
};

struct uam_obj {
  const char *uam_name; /* authentication method */
  char *uam_path; /* where it's located */
  int uam_count;
  union {
    struct {
      int (*login) __P((void *, struct passwd **, 
			char *, int, char *, int *));
      int (*logincont) __P((void *, struct passwd **, char *,
			    int, char *, int *));
      void (*logout) __P((void));
    } uam_login;
    int (*uam_changepw) __P((void *, char *, struct passwd *, char *,
			     int, char *, int *));
  } u;
  struct uam_obj *uam_prev, *uam_next;
};

#define uam_attach(a, b) do { \
    (a)->uam_prev->uam_next = (b); \
    (b)->uam_prev = (a)->uam_prev; \
    (b)->uam_next = (a); \
    (a)->uam_prev = (b); \
} while (0)				     

#define uam_detach(a) do { \
    (a)->uam_prev->uam_next = (a)->uam_next; \
    (a)->uam_next->uam_prev = (a)->uam_prev; \
} while (0)

extern struct uam_mod *uam_load __P((const char *, const char *));
extern void uam_unload __P((struct uam_mod *));

/* auth.c */
int auth_load __P((const char *, const char *));
int auth_register __P((const int, struct uam_obj *));
#define auth_unregister(a) uam_detach(a)
struct uam_obj *auth_uamfind __P((const int, const char *, const int));
void auth_unload __P((void));

#endif /* uam_auth.h */
