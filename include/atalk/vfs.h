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

#ifndef ATALK_VFS_H
#define ATALK_VFS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_NFSv4_ACLS
#include <sys/acl.h>
#endif

#include <atalk/adouble.h>
#include <atalk/volume.h>

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

    /* Extended Attributes */
    int (*get_easize)(const struct vol * restrict, char * restrict rbuf, int * restrict rbuflen, const char * restrict uname, int oflag, const char * restrict attruname);
    int (*get_eacontent)(const struct vol * restrict , char * restrict rbuf, int * restrict rbuflen, const char * restrict uname, int oflag, const char * restrict attruname, int maxreply);
    int (*list_eas)(const struct vol * restrict , char * restrict attrnamebuf, int * restrict buflen, const char * restrict uname, int oflag);
    int (*set_ea)(const struct vol * restrict , const char * restrict uname, const char * restrict attruname, const char * restrict ibuf, size_t attrsize, int oflag);
    int (*remove_ea)(const struct vol * restrict , const char * restrict uname, const char * restrict attruname, int oflag);
};

extern void initvol_vfs(struct vol * restrict vol);

/* VFS inderected funcs ... : */
/* ...default adouble EAs */
extern int get_easize(const struct vol * restrict vol, char * restrict rbuf, int * restrict rbuflen, const char * restrict uname, int oflag, const char * restrict attruname);
extern int get_eacontent(const struct vol * restrict vol, char * restrict rbuf, int * restrict rbuflen, const char * restrict uname, int oflag, const char * restrict attruname, int maxreply);
extern int list_eas(const struct vol * restrict vol, char * restrict attrnamebuf, int * restrict buflen, const char * restrict uname, int oflag);
extern int set_ea(const struct vol * restrict vol, const char * restrict uname, const char * restrict attruname, const char * restrict ibuf, size_t attrsize, int oflag);
extern int remove_ea(const struct vol * restrict vol, const char * restrict uname, const char * restrict attruname, int oflag);

/* ... Solaris native EAs */
#ifdef HAVE_SOLARIS_EAS
extern int sol_get_easize(const struct vol * restrict vol, char * restrict rbuf, int * restrict rbuflen, const char * restrict uname, int oflag, const char * restrict attruname);
extern int sol_get_eacontent(const struct vol * restrict vol, char * restrict rbuf, int * restrict rbuflen, const char * restrict uname, int oflag, char * restrict attruname, int maxreply);
extern int sol_list_eas(const struct vol * restrict vol, char * restrict attrnamebuf, int * restrict buflen, const char * restrict uname, int oflag);
extern int sol_set_ea(const struct vol * restrict vol, const char * restrict uname, const char * restrict attruname, const char * restrict ibuf,size_t attrsize, int oflag);
extern int sol_remove_ea(const struct vol * restrict vol, const char * restrict uname, const char * restrict attruname, int oflag);
#endif /* HAVE_SOLARIS_EAS */

/* Solaris NFSv4 ACL stuff */
#ifdef HAVE_NFSv4_ACLS
extern int get_nfsv4_acl(const char *name, ace_t **retAces);
extern int remove_acl(const char *name);
#endif /* HAVE_NFSv4_ACLS */

#endif /* ATALK_VFS_H */
