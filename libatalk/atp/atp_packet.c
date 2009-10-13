/*
 * $Id: atp_packet.c,v 1.6 2009-10-13 22:55:37 didg Exp $
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
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <netinet/in.h>

#include <netatalk/at.h>
#include <netatalk/endian.h>

#include <atalk/netddp.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>
#include <atalk/util.h>

#include "atp_internals.h"

/* FIXME/SOCKLEN_T: socklen_t is a unix98 feature. */
#ifndef SOCKLEN_T
#define SOCKLEN_T unsigned int
#endif /* ! SOCKLEN_T */

#ifdef EBUG
#include <stdio.h>

static void print_func(u_int8_t ctrlinfo)
{
    switch ( ctrlinfo & ATP_FUNCMASK ) {
    case ATP_TREQ:
	printf( "TREQ" );
	break;
    case ATP_TRESP:
	printf( "TRESP" );
	break;
    case ATP_TREL:
	printf( "ANY/TREL" );
	break;
    case ATP_TIDANY:
	printf( "*" );
	break;
    default:
	printf( "%x", ctrlinfo & ATP_FUNCMASK );
    }
}

static void dump_packet(char *buf, int len)
{
    int		i;

    for ( i = 0; i < len; ++i ) {
	printf( "%x-%c ", buf[i], buf[i] );
    }
    putchar( '\n' );
}

void atp_print_addr(char *s, struct sockaddr_at *saddr)
{
    printf( "%s ", s );
    saddr->sat_family == AF_APPLETALK ? printf( "at." ) :
      printf( "%d.", saddr->sat_family );
    saddr->sat_addr.s_net == ATADDR_ANYNET ? printf( "*." ) :
      printf( "%d.", ntohs( saddr->sat_addr.s_net ));
    saddr->sat_addr.s_node == ATADDR_ANYNODE ? printf( "*." ) :
      printf( "%d.", saddr->sat_addr.s_node );
    saddr->sat_port == ATADDR_ANYPORT ? printf( "*" ) :
      printf( "%d", saddr->sat_port );
}
#endif /* EBUG */


void atp_build_req_packet( struct atpbuf *pktbuf,
			   u_int16_t tid,
			   u_int8_t ctrl,
			   struct atp_block *atpb )
{
    struct atphdr	hdr;

    /* fill in the packet fields
    */
    hdr.atphd_ctrlinfo = ctrl;
    hdr.atphd_bitmap = atpb->atp_bitmap;
    hdr.atphd_tid = htons( tid );
    *(pktbuf->atpbuf_info.atpbuf_data) = DDPTYPE_ATP;
    memcpy(pktbuf->atpbuf_info.atpbuf_data + 1, &hdr, sizeof( struct atphdr ));
    memcpy(pktbuf->atpbuf_info.atpbuf_data + ATP_HDRSIZE, 
	   atpb->atp_sreqdata, atpb->atp_sreqdlen ); 

    /* set length
    */
    pktbuf->atpbuf_dlen = ATP_HDRSIZE + (size_t) atpb->atp_sreqdlen;
}

void atp_build_resp_packet( struct atpbuf *pktbuf,
			    u_int16_t tid,
			    u_int8_t ctrl,
			    struct atp_block *atpb,
			    u_int8_t seqnum )
{
    struct atphdr	hdr;

    /* fill in the packet fields */
    *(pktbuf->atpbuf_info.atpbuf_data) = DDPTYPE_ATP;
    hdr.atphd_ctrlinfo = ctrl;
    hdr.atphd_bitmap = seqnum;
    hdr.atphd_tid = htons( tid );
    memcpy(pktbuf->atpbuf_info.atpbuf_data + 1, &hdr, 
	   sizeof( struct atphdr ));
    memcpy(pktbuf->atpbuf_info.atpbuf_data + ATP_HDRSIZE,
	  atpb->atp_sresiov[ seqnum ].iov_base,
	  atpb->atp_sresiov[ seqnum ].iov_len ); 

    /* set length
    */
    pktbuf->atpbuf_dlen = ATP_HDRSIZE + (size_t) atpb->atp_sresiov[ seqnum ].iov_len;
}


int
atp_recv_atp( ATP ah,
	      struct sockaddr_at *fromaddr,
	      u_int8_t *func,
	      u_int16_t tid,
	      char *rbuf,
	      int wait )
{
/* 
  Receive a packet from address fromaddr of the correct function type
  and with the correct tid.  fromaddr = AT_ANY... and function == ATP_TYPEANY
  and tid == ATP_TIDANY can be used to wildcard match.
  
  recv_atp returns the length of the packet received (or -1 if error)
  The function code for the packet received is returned in *func (ATP_TREQ or
    ATP_TRESP).
*/
    struct atpbuf	*pq, *cq;
    struct atphdr	ahdr;
    u_int16_t		rfunc;
    u_int16_t		rtid;
    int			i;
    int			dlen = -1;
    int			recvlen;
    struct sockaddr_at	faddr;
    SOCKLEN_T		faddrlen;
    struct atpbuf	*inbuf;

    tid = htons( tid );

    /* first check the queue
    */
#ifdef EBUG
    atp_print_bufuse( ah, "recv_atp checking queue" );
#endif /* EBUG */
    for ( pq = NULL, cq = ah->atph_queue; cq != NULL;
      pq = cq, cq = cq->atpbuf_next ) {
	memcpy(&ahdr, cq->atpbuf_info.atpbuf_data + 1, 
	       sizeof( struct atphdr ));
	rfunc = ahdr.atphd_ctrlinfo & ATP_FUNCMASK;
#ifdef EBUG
	printf( "<%d> checking", getpid());
	printf( " tid=%hu func=", ntohs( ahdr.atphd_tid ));
	print_func( rfunc );
	atp_print_addr( " from", &cq->atpbuf_addr );
	putchar( '\n' );
#endif /* EBUG */
	if ((( tid & ahdr.atphd_tid ) == ahdr.atphd_tid ) &&
	    (( *func & rfunc ) == rfunc )
	    && at_addr_eq( fromaddr, &cq->atpbuf_addr )) {
	    break;
	}
    }
    if ( cq != NULL ) {
	/* we found one in the queue -- copy to rbuf
	*/
	dlen = (int) cq->atpbuf_dlen;
	*func = rfunc;
	memcpy( fromaddr, &cq->atpbuf_addr, sizeof( struct sockaddr_at ));
	memcpy( rbuf, cq->atpbuf_info.atpbuf_data, cq->atpbuf_dlen );

	/* remove packet from queue and free buffer
	*/
	if ( pq == NULL ) {
	    ah->atph_queue = NULL;
	} else {
	    pq->atpbuf_next = cq->atpbuf_next;
	}
	atp_free_buf( cq );
	return( dlen );
    }

    /* we need to get it the net -- call on ddp to receive a packet
    */
#ifdef EBUG
    printf( "<%d>", getpid());
    atp_print_addr( " waiting on address", &ah->atph_saddr );
    printf( "\nfor tid=%hu func=", ntohs( tid ));
    print_func( *func );
    atp_print_addr( " from", fromaddr );
    putchar( '\n' );
#endif /* EBUG */

    do {
#ifdef EBUG
    fflush( stdout );
#endif /* EBUG */
	faddrlen = sizeof( struct sockaddr_at );
	memset( &faddr, 0, sizeof( struct sockaddr_at ));

	if (( recvlen = netddp_recvfrom( ah->atph_socket, rbuf, 
					 ATP_BUFSIZ, 0,
					 (struct sockaddr *) &faddr,
					 &faddrlen )) < 0 ) {
	    return -1;
	}
	memcpy( &ahdr, rbuf + 1, sizeof( struct atphdr ));
	if ( recvlen >= ATP_HDRSIZE && *rbuf == DDPTYPE_ATP) {
	    /* this is a valid ATP packet -- check for a match */
	    rfunc = ahdr.atphd_ctrlinfo & ATP_FUNCMASK;
	    rtid = ahdr.atphd_tid;
#ifdef EBUG
	    printf( "<%d> got tid=%hu func=", getpid(), ntohs( rtid ));
	    print_func( rfunc );
	    atp_print_addr( " from", &faddr );
	    putchar( '\n' );
	    bprint( rbuf, recvlen );
#endif /* EBUG */
	    if ( rfunc == ATP_TREL ) {
		/* remove response from sent list */
		for ( pq = NULL, cq = ah->atph_sent; cq != NULL;
		  pq = cq, cq = cq->atpbuf_next ) {
		    if ( at_addr_eq( &faddr, &cq->atpbuf_addr ) &&
		      cq->atpbuf_info.atpbuf_xo.atpxo_tid == ntohs( rtid )) 
			break;
		}
		if ( cq != NULL ) {
#ifdef EBUG
	printf( "<%d> releasing transaction %hu\n", getpid(), ntohs( rtid ));
#endif /* EBUG */
		    if ( pq == NULL ) {
			ah->atph_sent = cq->atpbuf_next;
		    } else {
			pq->atpbuf_next = cq->atpbuf_next;
		    }
		    for ( i = 0; i < 8; ++i ) {
			if ( cq->atpbuf_info.atpbuf_xo.atpxo_packet[ i ]
				    != NULL ) {
			    atp_free_buf ( cq->atpbuf_info.atpbuf_xo.atpxo_packet[ i ] );
			}
		    }
		    atp_free_buf( cq );
		}

	    } else if ((( tid & rtid ) == rtid ) &&
		    (( *func & rfunc ) == rfunc ) &&
		    at_addr_eq( fromaddr, &faddr )) { /* got what we wanted */
		*func = rfunc;
		dlen = recvlen;
		memcpy( fromaddr, &faddr, sizeof( struct sockaddr_at ));

	    } else {
		/* add packet to incoming queue */
#ifdef EBUG
	printf( "<%d> queuing incoming...\n", getpid() );
#endif /* EBUG */
		if (( inbuf = atp_alloc_buf()) == NULL ) {
		    return -1;
		}
		memcpy( &inbuf->atpbuf_addr, &faddr, 
			sizeof( struct sockaddr_at ));
		inbuf->atpbuf_next = ah->atph_queue;
		inbuf->atpbuf_dlen = (size_t) recvlen;
		memcpy( inbuf->atpbuf_info.atpbuf_data, rbuf, recvlen );
	    }
	}
	if ( !wait && dlen < 0 ) {
	    return( 0 );
	}

    } while ( dlen < 0 );

    return( dlen );
}


int at_addr_eq( 
    struct sockaddr_at	*paddr,		/* primary address */
    struct sockaddr_at	*saddr)		/* secondary address */
{
/* compare two atalk addresses -- only check the non-zero fields
    of paddr against saddr.
   return zero if not equal, non-zero if equal
*/
    return (( paddr->sat_port == ATADDR_ANYPORT || paddr->sat_port == saddr->sat_port )
	&&  ( paddr->sat_addr.s_net == ATADDR_ANYNET ||
	      paddr->sat_addr.s_net == saddr->sat_addr.s_net )
	&&  ( paddr->sat_addr.s_node == ATADDR_ANYNODE ||
	      paddr->sat_addr.s_node == saddr->sat_addr.s_node ));
}

