/*
 * $Id: asp_cmdreply.c,v 1.5 2009-10-13 22:55:37 didg Exp $
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
#include <sys/socket.h>

#include <atalk/atp.h>
#include <atalk/asp.h>

#if defined(BSD) || defined(BSD4_3)
#define memmove(a, b, n)   bcopy((b), (a), (n))
#endif /* BSD || BSD4_3 */

int asp_cmdreply(ASP asp, int result)
{
    struct iovec	iov[ ASP_MAXPACKETS ];
    struct atp_block	atpb;
    int			iovcnt, buflen;
    char                *buf;

    /* unpack data into a format that atp likes. it needs to get
     * 4-byte headers prepended before each ASP_CMDSIZ chunk. */
    buf = (char *) asp->data;
    buflen = asp->datalen;
    asp->write_count += buflen;
    result = htonl(result);

    iovcnt = 0;
    do {
	iov[ iovcnt ].iov_base = buf;
	memmove(buf + ASP_HDRSIZ, buf, buflen);

	if ( iovcnt == 0 ) {
	    memcpy( iov[ iovcnt ].iov_base, &result, ASP_HDRSIZ );
	} else {
	    memset( iov[ iovcnt ].iov_base, 0, ASP_HDRSIZ );
	}

	if ( buflen > ASP_CMDSIZ ) {
	  buf += ASP_CMDMAXSIZ;
	  buflen -= ASP_CMDSIZ;
	  iov[ iovcnt ].iov_len = ASP_CMDMAXSIZ;
	} else {
	  iov[ iovcnt ].iov_len = buflen + ASP_HDRSIZ;
	  buflen = 0;
	}
	iovcnt++;
    } while ( buflen > 0 );

    atpb.atp_saddr = &asp->asp_sat;
    atpb.atp_sresiov = iov;
    atpb.atp_sresiovcnt = iovcnt;
    if ( atp_sresp( asp->asp_atp, &atpb ) < 0 ) {
	return( -1 );
    }
    asp->asp_seq++;

    return( 0 );
}
