/*
 * $Id: asp_getreq.c,v 1.3 2001-06-29 14:14:46 rufustfirefly Exp $
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
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/asp.h>

int asp_getrequest(ASP asp)
{
    struct atp_block	atpb;
    u_int16_t		seq;

    asp->asp_sat.sat_port = ATADDR_ANYPORT;
    atpb.atp_saddr = &asp->asp_sat;
    atpb.atp_rreqdata = asp->cmdbuf;
    atpb.atp_rreqdlen = sizeof(asp->cmdbuf);

    if ( atp_rreq( asp->asp_atp, &atpb ) < 0 ) {
	return( -1 );
    }

    asp->cmdlen = atpb.atp_rreqdlen - 4;
    asp->read_count += asp->cmdlen;
    memcpy( &seq, asp->cmdbuf + 2, sizeof(seq));
    seq = ntohs( seq );

    if ((asp->cmdbuf[0] != ASPFUNC_CLOSE) && (seq != asp->asp_seq)) {
	return( -2 );
    }
    if ( asp->cmdbuf[1] != asp->asp_sid ) {
	return( -3 );
    }

    return( asp->cmdbuf[0] ); /* the command */
}
