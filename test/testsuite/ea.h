/*
   Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifndef ATALK_EA_H
#define ATALK_EA_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_ATTR_XATTR_H
#include <attr/xattr.h>
#elif HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#ifdef HAVE_SYS_EA_H
#include <sys/ea.h>
#endif

#ifdef HAVE_SYS_EXTATTR_H
#include <sys/extattr.h>
#endif

#ifdef HAVE_SOLARIS_ACLS
#include <sys/acl.h>
#endif

#ifndef ENOATTR
#define ENOATTR ENODATA
#endif

/*
 * This seems to be the current limit fo HFS+, we arbitrarily force that
 *  which also safes us from buffer overflows
 */
#define MAX_EA_SIZE 3802

/*
 * Library user must provide a static buffer of size ATTRNAMEBUFSIZ.
 * It's used when listing EAs as intermediate buffer. For afpd it's
 * defined in extattrs.c.
 */
#define ATTRNAMEBUFSIZ 4096

enum {
    kXAttrNoFollow = 0x1,
    kXAttrCreate = 0x2,
    kXAttrReplace = 0x4
};

#if !defined(HAVE_SETXATTR)
#define XATTR_CREATE  0x1       /* set value, fail if attr already exists */
#define XATTR_REPLACE 0x2       /* set value, fail if attr does not exist */
#endif

/* Names for our Extended Attributes adouble data */
#define AD_EA_META "org.netatalk.Metadata"
#ifdef __APPLE__
#define AD_EA_RESO "com.apple.ResourceFork"
#define EA_FINFO "com.apple.FinderInfo"
#define NOT_NETATALK_EA(a) (strcmp((a), AD_EA_META) != 0) && (strcmp((a), AD_EA_RESO) != 0) && (strcmp((a), EA_FINFO) != 0)
#else
#define AD_EA_RESO "org.netatalk.ResourceFork"
#define NOT_NETATALK_EA(a) (strcmp((a), AD_EA_META) != 0) && (strcmp((a), AD_EA_RESO) != 0)
#endif

/****************************************************************************************
 * Wrappers for native EA functions taken from Samba
 ****************************************************************************************/
ssize_t sys_getxattr (const char *path, const char *name, void *value, size_t size);
ssize_t sys_lgetxattr (const char *path, const char *name, void *value, size_t size);
ssize_t sys_fgetxattr (int filedes, const char *name, void *value, size_t size);
ssize_t sys_listxattr (const char *path, char *list, size_t size);
ssize_t sys_llistxattr (const char *path, char *list, size_t size);
ssize_t sys_flistxattr (int filedes, char *list, size_t size);
int sys_removexattr (const char *path, const char *name);
int sys_lremovexattr (const char *path, const char *name);
int sys_fremovexattr (int filedes, const char *name);
int sys_setxattr (const char *path, const char *name, const void *value, size_t size, int flags);
int sys_lsetxattr (const char *path, const char *name, const void *value, size_t size, int flags);
int sys_fsetxattr (int filedes, const char *name, const void *value, size_t size, int flags);
int sys_copyxattr (const char *src, const char *dst);
int sys_getxattrfd(const char *path, const char *uname, int oflag, ...);


#endif /* ATALK_EA_H */
