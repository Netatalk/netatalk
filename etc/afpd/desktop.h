/*
 * $Id: desktop.h,v 1.4 2005-04-28 20:49:41 bfernhomberg Exp $
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

#ifndef AFPD_DESKTOP_H
#define AFPD_DESKTOP_H 1

#include <sys/cdefs.h>
#include "globals.h"
#include "volume.h"

struct savedt {
    u_char	sdt_creator[ 4 ];
    int		sdt_fd;
    int		sdt_index;
    short	sdt_vid;
};

typedef unsigned char CreatorType[4];

extern char	*dtfile __P((const struct vol *, u_char [], char *));
extern char	*mtoupath __P((const struct vol *, char *, cnid_t, int utf8));
extern char	*utompath __P((const struct vol *, char *, cnid_t, int utf8));

/* FP functions */
extern int	afp_opendt __P((AFPObj *, char *, int, char *, int *));
extern int	afp_addcomment __P((AFPObj *, char *, int, char *, int *));
extern int	afp_getcomment __P((AFPObj *, char *, int, char *, int *));
extern int	afp_rmvcomment __P((AFPObj *, char *, int, char *, int *));
extern int	afp_addappl __P((AFPObj *, char *, int, char *, int *));
extern int	afp_rmvappl __P((AFPObj *, char *, int, char *, int *));
extern int	afp_getappl __P((AFPObj *, char *, int, char *, int *));
extern int      afp_closedt __P((AFPObj *, char *, int, char *, int *));
extern int	afp_addicon __P((AFPObj *, char *, int, char *, int *));
extern int	afp_geticoninfo __P((AFPObj *, char *, int, char *, int *));
extern int	afp_geticon __P((AFPObj *, char *, int, char *, int *));

#endif
