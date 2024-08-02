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

#ifndef _ATALK_RTMP_H
#define _ATALK_RTMP_H 1


#include <netatalk/endian.h>

#define RTMPROP_REQUEST	1

struct rtmpent {
    u_int16_t   re_net;
    u_int8_t    re_hops;
};

#define RTMPHOPS_MAX	15
#define RTMPHOPS_POISON	31

struct rtmprdhdr {
    u_int16_t   rrdh_snet;
    u_int8_t    rrdh_idlen;
    u_int8_t    rrdh_id;
};

#endif
