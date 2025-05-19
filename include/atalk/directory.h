/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#ifndef ATALK_DIRECTORY_H
#define ATALK_DIRECTORY_H 1

#include <arpa/inet.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/types.h>

#include <atalk/bstrlib.h>
#include <atalk/cnid.h>
#include <atalk/queue.h>
#include <atalk/unicode.h>

/* setgid directories */
#ifndef DIRBITS
#  define DIRBITS S_ISGID
#endif /* DIRBITS */

/* reserved directory id's */
/* parent directory of root */
#define DIRDID_ROOT_PARENT    htonl(1)
/* root directory */
#define DIRDID_ROOT           htonl(2)

/* struct dir.d_flags */
#define DIRF_FSMASK	   (3<<0)
#define DIRF_NOFS	   (0<<0)
#define DIRF_UFS	   (1<<1)
/* it's cached file, not a directory */
#define DIRF_ISFILE    (1<<3)
/* offsprings count is valid */
#define DIRF_OFFCNT    (1<<4)
/* renumerate id */
#define DIRF_CNID	   (1<<5)

struct dir {
    /* complete unix path to dir (or file) */
    bstring d_fullpath;
    /* mac name */
    bstring d_m_name;
    /* unix name                                          */
    /* be careful here! if d_m_name == d_u_name, d_u_name */
    /* will just point to the same storage as d_m_name !! */
    bstring d_u_name;
    /* mac name as UCS2 */
    ucs2_t *d_m_name_ucs2;
    /* pointer to position in queue index */
    qnode_t *qidx_node;
    /* inode ctime, used and modified by reenumeration */
    time_t d_ctime;
    /* directory flags */
    int d_flags;
    /* CNID of parent directory */
    cnid_t d_pdid;
    /* CNID of directory */
    cnid_t d_did;
    /* offspring count */
    uint32_t d_offcnt;
    /* only needed in the dircache, because
       we put all directories in one cache. */
    uint16_t d_vid;
    /* cached rights combinded from mode and possible ACL */
    uint32_t d_rights_cache;
    /* Stuff used in the dircache */
    /* inode ctime, used and modified by dircache */
    time_t dcache_ctime;
    /* inode number, used to detect changes in the dircache */
    ino_t dcache_ino;
};

struct path {
    /* mac name type (long name, unicode */
    int         m_type;
    /* mac name */
    char        *m_name;
    /* unix name */
    char        *u_name;
    /* file id (only for getmetadata) */
    cnid_t      id;
    struct dir  *d_dir;
    /* does st_errno and st set */
    int         st_valid;
    int         st_errno;
    struct stat st;
};

static inline int path_isadir(struct path *o_path)
{
    return o_path->d_dir != NULL;
#if 0
    return o_path->m_name == '\0' || /* we are in a it */
           !o_path->st_valid ||      /* in cache but we can't chdir in it */
           /* not in cache and can't chdir */
           (!o_path->st_errno && S_ISDIR(o_path->st.st_mode));
#endif
}

#endif /* ATALK_DIRECTORY_H */
