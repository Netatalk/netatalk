/*
 * $Id: desktop.h,v 1.5 2009-10-13 22:55:36 didg Exp $
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

extern char	*dtfile (const struct vol *, u_char [], char *);
extern char	*mtoupath (const struct vol *, char *, cnid_t, int utf8);
extern char	*utompath (const struct vol *, char *, cnid_t, int utf8);

/* FP functions */
extern int	afp_opendt (AFPObj *, char *, int, char *, int *);
extern int	afp_addcomment (AFPObj *, char *, int, char *, int *);
extern int	afp_getcomment (AFPObj *, char *, int, char *, int *);
extern int	afp_rmvcomment (AFPObj *, char *, int, char *, int *);
extern int	afp_addappl (AFPObj *, char *, int, char *, int *);
extern int	afp_rmvappl (AFPObj *, char *, int, char *, int *);
extern int	afp_getappl (AFPObj *, char *, int, char *, int *);
extern int      afp_closedt (AFPObj *, char *, int, char *, int *);
extern int	afp_addicon (AFPObj *, char *, int, char *, int *);
extern int	afp_geticoninfo (AFPObj *, char *, int, char *, int *);
extern int	afp_geticon (AFPObj *, char *, int, char *, int *);

#endif
