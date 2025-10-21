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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*!
 * @file
 * vfs layer for afp
*/

#ifndef ATALK_VFS_H
#define ATALK_VFS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/acl.h>
#include <atalk/adouble.h>
#include <atalk/volume.h>

/*! Maximum number of VFS modules that can be chained */
#define VFS_MODULES_MAX 3

/* Function argument types for VFS operations */
typedef int (*vfs_validupath_fn)(const struct vol *vol, const char *name);
typedef int (*vfs_chown_fn)(const struct vol *vol, const char *path, uid_t uid,
                            gid_t gid);
typedef int (*vfs_renamedir_fn)(const struct vol *vol, int dirfd,
                                const char *oldpath, const char *newpath);
typedef int (*vfs_deletecurdir_fn)(const struct vol *vol);
typedef int (*vfs_setfilmode_fn)(const struct vol *vol, const char *name,
                                 mode_t mode, struct stat *st);
typedef int (*vfs_setdirmode_fn)(const struct vol *vol, const char *name,
                                 mode_t mode, struct stat *st);
typedef int (*vfs_setdirunixmode_fn)(const struct vol *vol, const char *name,
                                     mode_t mode, struct stat *st);
typedef int (*vfs_setdirowner_fn)(const struct vol *vol, const char *name,
                                  uid_t uid, gid_t gid);
typedef int (*vfs_deletefile_fn)(const struct vol *vol, int dirfd,
                                 const char *file);
typedef int (*vfs_renamefile_fn)(const struct vol *vol, int dirfd,
                                 const char *src, const char *dst);
typedef int (*vfs_copyfile_fn)(const struct vol *vol, int sfd, const char *src,
                               const char *dst);

#ifdef HAVE_NFSV4_ACLS
typedef int (*vfs_solaris_acl_fn)(const struct vol *vol, const char *path,
                                  int cmd, int count, void *aces);
#endif

#ifdef HAVE_POSIX_ACLS
typedef int (*vfs_posix_acl_fn)(const struct vol *vol, const char *path,
                                acl_type_t type, int count, acl_t acl);
#endif

typedef int (*vfs_remove_acl_fn)(const struct vol *vol, const char *path,
                                 int dir);

typedef int (*vfs_ea_getsize_fn)(const struct vol *vol, char *rbuf,
                                 size_t *rbuflen, const char *uname,
                                 int oflag, const char *attruname, int fd);

typedef int (*vfs_ea_getcontent_fn)(const struct vol *vol, char *rbuf,
                                    size_t *rbuflen, const char *uname,
                                    int oflag, const char *attruname,
                                    int maxreply, int fd);

typedef int (*vfs_ea_list_fn)(const struct vol *vol, char *attrnamebuf,
                              size_t *buflen, const char *uname, int oflag, int fd);

typedef int (*vfs_ea_set_fn)(const struct vol *vol, const char *uname,
                             const char *attruname, const char *ibuf,
                             size_t attrsize, int oflag, int fd);

typedef int (*vfs_ea_remove_fn)(const struct vol *vol, const char *uname,
                                const char *attruname, int oflag, int fd);

/*!
 * Forward declaration. We need it because of the circular inclusion of
 * of vfs.h <-> volume.h.
 */
struct vol;

struct vfs_ops {
    vfs_validupath_fn vfs_validupath;
    vfs_chown_fn vfs_chown;
    vfs_renamedir_fn vfs_renamedir;
    vfs_deletecurdir_fn vfs_deletecurdir;
    vfs_setfilmode_fn vfs_setfilmode;
    vfs_setdirmode_fn vfs_setdirmode;
    vfs_setdirunixmode_fn vfs_setdirunixmode;
    vfs_setdirowner_fn vfs_setdirowner;
    vfs_deletefile_fn vfs_deletefile;
    vfs_renamefile_fn vfs_renamefile;
    vfs_copyfile_fn vfs_copyfile;

#ifdef HAVE_ACLS
#ifdef HAVE_NFSV4_ACLS
    vfs_solaris_acl_fn vfs_solaris_acl;
#endif
#ifdef HAVE_POSIX_ACLS
    vfs_posix_acl_fn vfs_posix_acl;
#endif
    vfs_remove_acl_fn vfs_remove_acl;
#endif

    /* Extended Attributes */
    vfs_ea_getsize_fn vfs_ea_getsize;
    vfs_ea_getcontent_fn vfs_ea_getcontent;
    vfs_ea_list_fn vfs_ea_list;
    vfs_ea_set_fn vfs_ea_set;
    vfs_ea_remove_fn vfs_ea_remove;
};

extern void initvol_vfs(struct vol *vol);

#endif /* ATALK_VFS_H */
