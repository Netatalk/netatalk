/*
 * $Id: file.h,v 1.6 2002-03-24 17:43:39 jmarcus Exp $
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

#ifndef AFPD_FILE_H 
#define AFPD_FILE_H 1

/*#include <sys/stat.h>*/ /* including it here causes some confusion */
#include <sys/param.h>
#include <sys/cdefs.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>

#include "globals.h"
#include "volume.h"
#include "directory.h"

extern const u_char	ufinderi[];

#define FILPBIT_ATTR	0
#define FILPBIT_PDID	1
#define FILPBIT_CDATE	2
#define FILPBIT_MDATE	3
#define FILPBIT_BDATE	4
#define FILPBIT_FINFO	5
#define FILPBIT_LNAME	6
#define FILPBIT_SNAME	7
#define FILPBIT_FNUM	8
#define FILPBIT_DFLEN	9
#define FILPBIT_RFLEN	10
#define FILPBIT_PDINFO  13    /* ProDOS Info */

/* attribute bits. (d) = directory attribute bit as well. */
#define ATTRBIT_INVISIBLE (1<<0)  /* invisible (d) */
#define ATTRBIT_MULTIUSER (1<<1)  /* multiuser */
#define ATTRBIT_SYSTEM    (1<<2)  /* system (d) */
#define ATTRBIT_DOPEN     (1<<3)  /* data fork already open */
#define ATTRBIT_ROPEN     (1<<4)  /* resource fork already open */
#define ATTRBIT_NOWRITE   (1<<5)  /* write inhibit(v2)/read-only(v1) bit */
#define ATTRBIT_BACKUP    (1<<6)  /* backup needed (d) */
#define ATTRBIT_NORENAME  (1<<7)  /* rename inhibit (d) */
#define ATTRBIT_NODELETE  (1<<8)  /* delete inhibit (d) */
#define ATTRBIT_NOCOPY    (1<<10) /* copy protect */
#define ATTRBIT_SETCLR	  (1<<15) /* set/clear bits (d) */

struct extmap {
    struct extmap	*em_next;
    char		em_ext[ MAXPATHLEN + 1];
    char		em_creator[ 4 ];
    char		em_type[ 4 ];
};

extern struct extmap	*getextmap __P((const char *));

extern int getfilparams __P((struct vol *, u_int16_t, char *,
                                 struct dir *, struct stat *, char *buf,
                                 int *));
extern int setfilparams __P((struct vol *, char *, u_int16_t, char *));
extern int renamefile   __P((char *, char *, char *, const int));
extern int copyfile     __P((char *, char *, char *, const int));
extern int deletefile   __P((char *, int));

/* FP functions */
extern int      afp_exchangefiles __P((AFPObj *, char *, int, char *, int *));
extern int	afp_setfilparams __P((AFPObj *, char *, int, char *, int *));
extern int	afp_copyfile __P((AFPObj *, char *, int, char *, int *));
extern int	afp_createfile __P((AFPObj *, char *, int, char *, int *));
#ifdef CNID_DB
extern int      afp_createid __P((AFPObj *, char *, int, char *, int *));
extern int      afp_resolveid __P((AFPObj *, char *, int, char *, int *));
extern int      afp_deleteid __P((AFPObj *, char *, int, char *, int *));
#else /* CNID_DB */
#define afp_createid      afp_null
#define afp_resolveid     afp_null
#define afp_deleteid      afp_null
#endif /* AD_VERSION > AD_VERSION1 */

#endif
