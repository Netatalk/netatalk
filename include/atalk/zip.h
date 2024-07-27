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


#ifndef _ATALK_ZIP_H
#define _ATALK_ZIP_H 1

#ifndef NO_DDP

#include <netatalk/endian.h>

struct ziphdr {
    uint8_t    zh_op;
    uint8_t    zh_cnt;
#define zh_count	zh_cnt
#define zh_zero		zh_cnt
#define zh_flags	zh_cnt
};

struct zipreplent {
    uint16_t   zre_net;
    uint8_t    zre_zonelen;
};

#define ZIPOP_QUERY	1
#define ZIPOP_REPLY	2
#define ZIPOP_TAKEDOWN	3	/* XXX */
#define ZIPOP_BRINGUP	4	/* XXX */
#define ZIPOP_GNI	5
#define ZIPOP_GNIREPLY	6
#define ZIPOP_NOTIFY	7
#define ZIPOP_EREPLY	8
#define ZIPOP_GETMYZONE	7
#define ZIPOP_GETZONELIST 8
#define ZIPOP_GETLOCALZONES 9

#define ZIPGNI_INVALID	0x80
#define ZIPGNI_ONEZONE	0x20

#endif  /* NO_DDP */
#endif
