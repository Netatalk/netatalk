/*
 * $Id: directory.h,v 1.2 2001-06-20 18:33:04 rufustfirefly Exp $
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

#ifndef AFPD_DIRECTORY_H
#define AFPD_DIRECTORY_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
/*#include <sys/stat.h>*/ /* including it here causes some confusion */
#include <netatalk/endian.h>

/* sys/types.h usually snarfs in major/minor macros. if they don't
 * try this file. */
#ifndef major
#include <sys/sysmacros.h>
#endif

#include "globals.h"
#include "volume.h"

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
    char	*d_name;
};

/* child addition/removal macros */
#define dirchildadd(a, b) do { \
	if (!(a)->d_child) \
		(a)->d_child = (b); \
	else { \
		(b)->d_next = (a)->d_child; \
		(b)->d_prev = (b)->d_next->d_prev; \
		(b)->d_next->d_prev = (b); \
		(b)->d_prev->d_next = (b); \
	} \
} while (0)

#define dirchildremove(a,b) do { \
	if ((a)->d_child == (b)) \
		(a)->d_child = ((b) == (b)->d_next) ? NULL : (b)->d_next; \
	(b)->d_next->d_prev = (b)->d_prev; \
	(b)->d_prev->d_next = (b)->d_next; \
        (b)->d_next = (b)->d_prev = (b); \
} while (0)

#define DIRTREE_COLOR_RED    0
#define DIRTREE_COLOR_BLACK  1

/* setgid directories */
#ifndef DIRBITS
#define DIRBITS S_ISGID
#endif /* DIRBITS */

#define DIRF_FSMASK	(3<<0)
#define DIRF_NOFS	(0<<0)
#define DIRF_AFS	(1<<0)
#define DIRF_UFS	(2<<0)

#define AFPDIR_READ	(1<<0)

/* directory bits */
#define DIRPBIT_ATTR	0
#define DIRPBIT_PDID	1
#define DIRPBIT_CDATE	2
#define DIRPBIT_MDATE	3
#define DIRPBIT_BDATE	4
#define DIRPBIT_FINFO	5
#define DIRPBIT_LNAME	6
#define DIRPBIT_SNAME	7
#define DIRPBIT_DID	8
#define DIRPBIT_OFFCNT	9
#define DIRPBIT_UID	10
#define DIRPBIT_GID	11
#define DIRPBIT_ACCESS	12
#define DIRPBIT_PDINFO  13         /* ProDOS Info */

/* directory attribute bits (see file.h for other bits) */
#define ATTRBIT_EXPFOLDER   (1 << 1) /* shared point */
#define ATTRBIT_MOUNTED     (1 << 3) /* mounted share point by non-admin */
#define ATTRBIT_INEXPFOLDER (1 << 4) /* folder in a shared area */

#define FILDIRBIT_ISDIR        (1 << 7) /* is a directory */
#define FILDIRBIT_ISFILE       (0)      /* is a file */

/* reserved directory id's */
#define DIRDID_ROOT_PARENT    htonl(1)  /* parent directory of root */
#define DIRDID_ROOT           htonl(2)  /* root directory */

/* file/directory ids. what a mess. we scramble things in a vain attempt
 * to get something meaningful */
#ifndef AFS
#define CNID_XOR(a)  (((a) >> 16) ^ (a))
#define CNID_DEV(a)   ((((CNID_XOR(major((a)->st_dev)) & 0xf) << 3) | \
	(CNID_XOR(minor((a)->st_dev)) & 0x7)) << 24)
#define CNID_INODE(a) (((a)->st_ino ^ (((a)->st_ino & 0xff000000) >> 8)) \
				       & 0x00ffffff)
#define CNID_FILE(a)  (((a) & 0x1) << 31)
#define CNID(a,b)     (CNID_DEV(a) | CNID_INODE(a) | CNID_FILE(b))
#else /* AFS */
#define CNID(a,b)     (((a)->st_ino & 0x7fffffff) | CNID_FILE(b))
#endif /* AFS */


struct maccess {
    u_char	ma_user;
    u_char	ma_world;
    u_char	ma_group;
    u_char	ma_owner;
};

#define	AR_USEARCH	(1<<0)
#define	AR_UREAD	(1<<1)
#define	AR_UWRITE	(1<<2)
#define	AR_UOWN		(1<<7)

extern struct dir       *dirnew __P((const int));
extern void             dirfree __P((struct dir *));
extern struct dir	*dirsearch __P((const struct vol *, u_int32_t));
extern struct dir	*adddir __P((struct vol *, struct dir *, char *,
				     int, char *, int, struct stat *));
extern struct dir       *dirinsert __P((struct vol *, struct dir *));
extern int              movecwd __P((const struct vol *, struct dir *)); 
extern int              deletecurdir __P((const struct vol *, char *, int));
extern char		*cname __P((const struct vol *, struct dir *,
				    char **));
extern mode_t           mtoumode __P((struct maccess *));
extern void             utommode __P((struct stat *, struct maccess *));
extern int getdirparams __P((const struct vol *, u_int16_t, char *,
			     struct dir *, struct stat *, char *, int *));
extern int setdirparams __P((const struct vol *, char *, u_int16_t, char *));
extern int renamedir __P((char *, char *, struct dir *, 
			  struct dir *, char *, const int));


/* FP functions */
extern int	afp_createdir __P((AFPObj *, char *, int, char *, int *));
extern int      afp_opendir __P((AFPObj *, char *, int, char *, int *));
extern int	afp_setdirparams __P((AFPObj *, char *, int, char *, int *));
extern int      afp_closedir __P((AFPObj *, char *, int, char *, int *));
extern int	afp_mapid __P((AFPObj *, char *, int, char *, int *));
extern int	afp_mapname __P((AFPObj *, char *, int, char *, int *));

/* from enumerate.c */
extern int	afp_enumerate __P((AFPObj *, char *, int, char *, int *));
extern int	afp_catsearch __P((AFPObj *, char *, int, char *, int *));

#endif
