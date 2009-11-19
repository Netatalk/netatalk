/*
 * $Id: directory.h,v 1.2 2009-11-19 10:37:44 franklahm Exp $
 *
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

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netatalk/endian.h>
#include <dirent.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>

/* setgid directories */
#ifndef DIRBITS
# ifdef AFS
#  define DIRBITS 0
# else /* AFS */
#  define DIRBITS S_ISGID
# endif /* AFS */
#endif /* DIRBITS */

/* the did tree is now a red-black tree while the parent/child
 * tree is a circular doubly-linked list. how exciting. */
struct dir {
    struct dir	*d_left, *d_right, *d_back; /* for red-black tree */
    int		d_color;
    struct dir  *d_parent, *d_child; /* parent-child */
    struct dir  *d_prev, *d_next;    /* siblings */
    void        *d_ofork;            /* oforks using this directory. */
    u_int32_t   d_did;
    int	        d_flags;

    time_t      ctime;                /* inode ctime */
    u_int32_t   offcnt;               /* offspring count */

    char	*d_m_name;            /* mac name */
    char        *d_u_name;            /* unix name */
    size_t      d_u_name_len;         /* Length of unix name
                                         MUST be initialized from d_u_name !!*/
    ucs2_t	*d_m_name_ucs2;	      /* mac name as UCS2 */
};

struct path {
    int         m_type;             /* mac name type (long name, unicode */
    char	*m_name;            /* mac name */
    char        *u_name;            /* unix name */
    cnid_t	id;                 /* file id (only for getmetadata) */
    struct dir  *d_dir;             /* */
    int         st_valid;           /* does st_errno and st set */
    int         st_errno;
    struct stat st;
};

static inline int path_isadir(struct path *o_path)
{
    return o_path->d_dir != NULL;
#if 0
    return o_path->m_name == '\0' || /* we are in a it */
           !o_path->st_valid ||      /* in cache but we can't chdir in it */ 
           (!o_path->st_errno && S_ISDIR(o_path->st.st_mode)); /* not in cache an can't chdir */
#endif
}


#endif /* ATALK_DIRECTORY_H */
