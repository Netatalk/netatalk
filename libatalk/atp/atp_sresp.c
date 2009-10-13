/*
 * $Id: atp_sresp.c,v 1.6 2009-10-13 22:55:37 didg Exp $
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
#include <errno.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netatalk/at.h>
#include <netatalk/endian.h>

#include <atalk/netddp.h>
#include <atalk/atp.h>
#include <atalk/util.h>

#include "atp_internals.h"

/* send a transaction response
*/
int atp_sresp(
    ATP			ah,		/* open atp handle */
    struct atp_block	*atpb)		/* parameter block */
{
    int			i;
    u_int8_t		ctrlinfo;
    struct atpbuf	*resp_buf;
    struct atpbuf	*save_buf; /* uninitialized, OK 310105 */

#ifdef EBUG
    atp_print_bufuse( ah, "atp_sresp" );
#endif /* EBUG */

    /* check parameters
    */
    for ( i = atpb->atp_sresiovcnt - 1; i >= 0; --i ) {
	if ( atpb->atp_sresiov[ i ].iov_len > ATP_MAXDATA ) 
	    break;
    }
    if ( i >= 0 || atpb->atp_sresiovcnt < 1 || atpb->atp_sresiovcnt > 8 ) {
	errno = EINVAL;
	return -1;
    }

    /* allocate a new buffer for reponse packet construction
    */
    if (( resp_buf = atp_alloc_buf()) == NULL ) {
	return -1;
    }

    /* send all the response packets
     * also need to attach list to ah for dup. detection (if XO)
    */
#ifdef EBUG
    printf( "<%d> preparing to send %s response tid=%hu ", getpid(),
	    ah->atph_rxo ? "XO" : "", ah->atph_rtid );
    atp_print_addr( " to", atpb->atp_saddr );
    putchar( '\n' );
#endif /* EBUG */
    if ( ah->atph_rxo ) {
	if (( save_buf = atp_alloc_buf()) == NULL ) {
	    return -1;
	}
	for ( i = 0; i < 8;
		save_buf->atpbuf_info.atpbuf_xo.atpxo_packet[ i++ ] = NULL );
    }
    for ( i = 0; i < atpb->atp_sresiovcnt; ++i ) {
	ctrlinfo = ATP_TRESP;
#ifdef STS_RESPONSES
	ctrlinfo |= ATP_STS;
#endif /* STS_RESPONSES */
	if ( i == atpb->atp_sresiovcnt-1 ) {
	    ctrlinfo |= ATP_EOM;
	}
	atp_build_resp_packet( resp_buf, ah->atph_rtid, ctrlinfo, atpb, i );

	if ( ah->atph_rxo ) {
	    save_buf->atpbuf_info.atpbuf_xo.atpxo_packet[i] = resp_buf;
	}
#ifdef DROPPACKETS
if (( random() % 3 ) != 2 ) {
#endif /* DROPPACKETS */
#ifdef EBUG
printf( "<%d> sending packet tid=%hu serial no.=%d\n", getpid(),
  ah->atph_rtid, i );
bprint( resp_buf->atpbuf_info.atpbuf_data, resp_buf->atpbuf_dlen );
#endif /* EBUG */
	if ( netddp_sendto( ah->atph_socket, resp_buf->atpbuf_info.atpbuf_data,
	  resp_buf->atpbuf_dlen, 0, (struct sockaddr *) atpb->atp_saddr,
	  sizeof( struct sockaddr_at )) != resp_buf->atpbuf_dlen ) {
	    if ( ah->atph_rxo ) {
		for ( ; i >= 0; --i ) {
		    atp_free_buf( save_buf->atpbuf_info.atpbuf_xo.atpxo_packet[ i ] );
		}
		atp_free_buf( save_buf );
	    }
	    return -1;
	}
#ifdef DROPPACKETS
} else printf( "<%d> atp_sresp: dropped serial no. %d\n", getpid(),  i );
#endif /* DROPPACKETS */
	/* allocate a buffer for next packet (if XO mode)
	*/
	if ( ah->atph_rxo && ( resp_buf = atp_alloc_buf()) == NULL ) {
	    return -1;
	}
    }
    atp_free_buf( resp_buf );
    if ( ah->atph_rxo ) {
	/* record timestamp, tid, release time, and destination address
	*/
	gettimeofday( &save_buf->atpbuf_info.atpbuf_xo.atpxo_tv,
		(struct timezone *) 0 );
	save_buf->atpbuf_info.atpbuf_xo.atpxo_tid = ah->atph_rtid;
	save_buf->atpbuf_info.atpbuf_xo.atpxo_reltime = ah->atph_rreltime;
	memcpy( &save_buf->atpbuf_addr, atpb->atp_saddr, 
		sizeof( struct sockaddr_at ));

	/* add to list of packets we have sent
	*/
	save_buf->atpbuf_next = ah->atph_sent;
	ah->atph_sent = save_buf;
#ifdef EBUG
printf( "<%d> saved XO response\n", getpid());
#endif /* EBUG */
    }
    return 0;
}
