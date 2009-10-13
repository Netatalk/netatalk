/*
 * $Id: multicast.h,v 1.4 2009-10-13 22:55:37 didg Exp $
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

int addmulti (const char *, const unsigned char *);
int zone_bcast (struct ziptab *);

#endif /* atalkd/multicast.h */
