/*
 * $Id: multicast.h,v 1.5 2009-10-14 01:38:28 didg Exp $
 *
 * Copyright (c) 1990,1997 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifndef ATALKD_MULTICAST_H
#define ATALKD_MULTICAST_H 1

#include <sys/cdefs.h>
#include "zip.h"

int addmulti (const char *, const unsigned char *);
int zone_bcast (struct ziptab *);

#endif /* atalkd/multicast.h */
