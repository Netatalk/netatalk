/*
   Copyright (c) 2004 Didier Gautheron
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   vfs layer for afp
*/

#ifndef _AFP_VFS_H
#define _AFP_VFS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_NFSv4_ACLS
#include <sys/acl.h>
#endif

#include <atalk/adouble.h>
struct vol;

struct vfs_ops {
    /* low level adouble fn */
    char *(*ad_path)(const char *, int);

    /* */
    int (*validupath)(const struct vol *, const char *);
    int (*rf_chown)(const struct vol *, const char *path, uid_t owner, gid_t group);
    int (*rf_renamedir)(const struct vol *, const char *oldpath, const char *newpath);
    int (*rf_deletecurdir)(const struct vol *);
    int (*rf_setfilmode)(const struct vol *, const char * name, mode_t mode, struct stat *st);
    int (*rf_setdirmode)(const struct vol *, const char * name, mode_t mode, struct stat *st);
    int (*rf_setdirunixmode)(const struct vol *, const char * name, mode_t mode, struct stat *st);

    int (*rf_setdirowner)(const struct vol *, const char *path, uid_t owner, gid_t group);

    int (*rf_deletefile)(const struct vol *, const char * );
    int (*rf_renamefile)(const struct vol *, const char *oldpath, const char *newpath);
#ifdef HAVE_NFSv4_ACLS
    int (*rf_acl)(const struct vol *, const char *path, int cmd, int count, ace_t *aces);
    int (*rf_remove_acl)(const struct vol *, const char *path, int dir);
#endif
};

void initvol_vfs(struct vol *vol);

#endif
