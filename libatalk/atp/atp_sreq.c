/*
 * $Id: atp_sreq.c,v 1.5 2002-01-17 06:08:55 srittau Exp $
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/uio.h>
#include <signal.h>

#include <netinet/in.h>
#include <netatalk/at.h>
#include <netatalk/endian.h>

#include <atalk/netddp.h>
#include <atalk/atp.h>
#include <atalk/util.h>

#include "atp_internals.h"

/*
 * ah:        open atp handle
 * atpb:      parameter block
 * respcount: buffers available for response
 * flags:     ATP_XO, ATP_TREL
 */
int
atp_sreq( ATP ah, struct atp_block *atpb, int respcount, u_int8_t flags )
{
    struct atpbuf	*req_buf;
    int			i;

#ifdef EBUG
    atp_print_bufuse( ah, "atp_sreq" );
#endif /* EBUG */

    /* check parameters
    */
    if ( atpb->atp_sreqdlen < 4 || atpb->atp_sreqdlen > ATP_MAXDATA
	    || ( respcount < 0 ) || ( respcount > 8 )
	    || ( atpb->atp_sreqto < 0 ) || (( atpb->atp_sreqtries < 1 )
	    && ( atpb->atp_sreqtries != ATP_TRIES_INFINITE ))) {
	errno = EINVAL;
	return -1;
    }
    /* clean up any packet fragments left from last request
    */
    for ( i = 0; i < 8; ++i ) {
	if ( ah->atph_resppkt[ i ] != NULL ) {
	    atp_free_buf( ah->atph_resppkt[ i ] );
	    ah->atph_resppkt[ i ] = NULL;
	}
    }

    /* generate bitmap, tid and ctrlinfo
    */
    atpb->atp_bitmap = ( 1 << respcount ) - 1;

    /* allocate a new buffer and build request packet
    */
    if (( req_buf = atp_alloc_buf()) == NULL ) {
	return( -1 );
    }
    atp_build_req_packet( req_buf, ah->atph_tid++, flags | ATP_TREQ, atpb );
    memcpy( &req_buf->atpbuf_addr, atpb->atp_saddr,
	    sizeof( struct sockaddr_at ));

    /* send the initial request
    */
#ifdef EBUG
    printf( "\n<%d> atp_sreq: sending a %ld byte packet ", getpid(),
	    req_buf->atpbuf_dlen );
    atp_print_addr( " to", atpb->atp_saddr );
    putchar( '\n' );
    bprint( req_buf->atpbuf_info.atpbuf_data, req_buf->atpbuf_dlen );
#endif /* EBUG */

    gettimeofday( &ah->atph_reqtv, (struct timezone *)0 );
#ifdef DROPPACKETS
if (( random() % 3 ) != 2 ) {
#endif /* DROPPACKETS */
    if ( netddp_sendto( ah->atph_socket, req_buf->atpbuf_info.atpbuf_data,
	    req_buf->atpbuf_dlen, 0, (struct sockaddr *) atpb->atp_saddr,
	    sizeof( struct sockaddr_at )) != req_buf->atpbuf_dlen ) {
	atp_free_buf( req_buf );
	return( -1 );
    }
#ifdef DROPPACKETS
} else printf( "<%d> atp_sreq: dropped request\n", getpid() );
#endif /* DROPPACKETS */

    if ( atpb->atp_sreqto != 0 ) {
	if ( ah->atph_reqpkt != NULL ) {
	    atp_free_buf( ah->atph_reqpkt );
	}
	ah->atph_reqto = atpb->atp_sreqto;
	if ( atpb->atp_sreqtries == ATP_TRIES_INFINITE ) {
	    ah->atph_reqtries = ATP_TRIES_INFINITE;
	} else {
	    /* we already sent one */
	    ah->atph_reqtries = atpb->atp_sreqtries - 1;
	}
	ah->atph_reqpkt = req_buf;
	ah->atph_rbitmap = ( 1 << respcount ) - 1;
	ah->atph_rrespcount = respcount;
    } else {
	atp_free_buf( req_buf );
	ah->atph_rrespcount = 0;
    }

    return( 0 );
}
