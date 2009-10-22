/*
 * $Id: asp_write.c,v 1.4 2009-10-22 12:35:39 franklahm Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/asp.h>

#if defined(BSD) || defined(BSD4_3)
#define memmove(a, b, n)   bcopy((b), (a), (n))
#endif /* BSD || BSD4_3 */

int asp_wrtcont(ASP asp, char *buf, size_t *buflen)
{
    struct iovec	iov[ ASP_MAXPACKETS ];
    struct atp_block	atpb;
    char	        *p;
    int			iovcnt = ASP_MAXPACKETS;
    u_int16_t		blen, seq;
    u_int8_t		oport;

    p = buf;
    *p++ = ASPFUNC_WRTCONT;
    *p++ = asp->asp_sid;
    seq = htons( asp->asp_seq );
    memcpy( p, &seq, sizeof(seq));
    p += sizeof(seq);
    blen = htons(*buflen);
    memcpy( p, &blen, sizeof(blen));
    p += sizeof(blen);

    for ( iovcnt = 0; iovcnt < ASP_MAXPACKETS; iovcnt++ ) {
        iov[iovcnt].iov_base = buf + iovcnt*ASP_CMDMAXSIZ;
	iov[ iovcnt ].iov_len = ASP_CMDMAXSIZ;
    }

    oport = asp->asp_sat.sat_port;
    atpb.atp_saddr = &asp->asp_sat;
    atpb.atp_saddr->sat_port = asp->asp_wss;
    atpb.atp_sreqdata = buf;
    atpb.atp_sreqdlen = p - buf;
    atpb.atp_sreqto = 2;
    atpb.atp_sreqtries = 5;

    if ( atp_sreq( asp->asp_atp, &atpb, iovcnt, ATP_XO ) < 0 ) {
	asp->asp_sat.sat_port = oport;
	return( -1 );
    }
    asp->write_count += atpb.atp_sreqdlen;

    atpb.atp_rresiov = iov;
    atpb.atp_rresiovcnt = iovcnt;
    if ( atp_rresp( asp->asp_atp, &atpb ) < 0 ) {
	asp->asp_sat.sat_port = oport;
	return( -1 );
    }

    asp->asp_sat.sat_port = oport;

    /* get rid of the 4-byte headers */
    p = buf;
    for ( iovcnt = 0; iovcnt < atpb.atp_rresiovcnt; iovcnt++ ) {
   	memmove(p, (char *) iov[ iovcnt ].iov_base + ASP_HDRSIZ, 
		iov[ iovcnt ].iov_len - ASP_HDRSIZ );
	p += ( iov[ iovcnt ].iov_len - ASP_HDRSIZ );
    }

    *buflen = p - buf;
    asp->read_count += *buflen;
    return 0;
}
