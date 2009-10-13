/*
 * $Id: asp_shutdown.c,v 1.4 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
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

int asp_shutdown(ASP asp)
{
    struct atp_block	atpb;
    struct iovec	iov;
    char		*p;
    u_int16_t		seq;
    u_int8_t		oport;


    p = asp->commands;
    *p++ = ASPFUNC_CLOSE;
    *p++ = asp->asp_sid;
    seq = 0;
    memcpy( p, &seq, sizeof(seq));
    p += sizeof(seq);

    oport = asp->asp_sat.sat_port;
    atpb.atp_saddr = &asp->asp_sat;
    atpb.atp_saddr->sat_port = asp->asp_wss;
    atpb.atp_sreqdata = asp->commands;
    atpb.atp_sreqdlen = p - asp->commands;
    atpb.atp_sreqto = 2;
    atpb.atp_sreqtries = 5;

    if ( atp_sreq( asp->asp_atp, &atpb, 1, ATP_XO ) < 0 ) {
	asp->asp_sat.sat_port = oport;
	return( -1 );
    }

    iov.iov_base = asp->commands;
    iov.iov_len = ASP_CMDSIZ;
    atpb.atp_rresiov = &iov;
    atpb.atp_rresiovcnt = 1;

    if ( atp_rresp( asp->asp_atp, &atpb ) < 0 ) {
	asp->asp_sat.sat_port = oport;
	return( -1 );
    }
    asp->asp_sat.sat_port = oport;

    return( 0 );
}
