/*
 * $Id: multicast.h,v 1.3 2003-03-18 23:34:51 srittau Exp $
 *
 * Copyright (c) 1990,1997 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifndef ATALKD_MULTICAST_H
#define ATALKD_MULTICAST_H 1

#include <sys/cdefs.h>
#include "zip.h"

extern unsigned char	ethermultitab[ 253 ][ 6 ];
extern unsigned char	ethermulti[ 6 ];

int addmulti __P((const char *, const unsigned char *));
int zone_bcast __P((struct ziptab *));

#endif /* atalkd/multicast.h */
