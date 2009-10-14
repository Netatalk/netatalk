/*
 * $Id: atp_rsel.c,v 1.6 2009-10-14 01:38:28 didg Exp $
 *
 * Copyright (c) 1990,1997 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <signal.h>
#include <errno.h>

#include <netatalk/endian.h>
#include <netatalk/at.h>

#include <atalk/netddp.h>
#include <atalk/compat.h>
#include <atalk/atp.h>
#include <atalk/util.h>

#include "atp_internals.h"

#ifdef DROP_ATPTREL
static int	release_count = 0;
#endif /* DROP_ATPTREL */


static int
resend_request(ATP ah)
{
    /*
     * update bitmap and send request packet
     */
    struct atphdr	req_hdr;

#ifdef EBUG
    printf( "\n<%d> resend_request: resending %ld byte request packet",
	    getpid(), ah->atph_reqpkt->atpbuf_dlen );
    atp_print_addr( " to", &ah->atph_reqpkt->atpbuf_addr );
    putchar( '\n' );
    bprint( ah->atph_reqpkt->atpbuf_info.atpbuf_data,
	    ah->atph_reqpkt->atpbuf_dlen );
#endif /* EBUG */

    memcpy( &req_hdr, ah->atph_reqpkt->atpbuf_info.atpbuf_data + 1, 
	sizeof( struct atphdr ));
    req_hdr.atphd_bitmap = ah->atph_rbitmap;
    memcpy( ah->atph_reqpkt->atpbuf_info.atpbuf_data + 1, &req_hdr, 
	    sizeof( struct atphdr ));

    gettimeofday( &ah->atph_reqtv, (struct timezone *)0 );
    if ( netddp_sendto( ah->atph_socket,
	    ah->atph_reqpkt->atpbuf_info.atpbuf_data,
	    ah->atph_reqpkt->atpbuf_dlen, 0,
	    (struct sockaddr *) &ah->atph_reqpkt->atpbuf_addr,
	    sizeof( struct sockaddr_at )) != ah->atph_reqpkt->atpbuf_dlen ) {
	return( -1 );
    }

    if ( ah->atph_reqtries > 0 ) {
	--(ah->atph_reqtries);
    }

    return( 0 );
}

int
atp_rsel( 
    ATP			ah,		/* open atp handle */
    struct sockaddr_at	*faddr,		/* address to receive from */
    int			func)		/* which function(s) to wait for;
					   0 means request or response */
{
    struct atpbuf	*abuf, *pb, *cb;
    struct atphdr	req_hdr, resp_hdr;
    fd_set		fds;
    int			i, recvlen, requesting, mask, c;
    u_int8_t		rfunc;
    u_int16_t		tid;
    struct timeval	tv;
    struct sockaddr_at	saddr;

#ifdef EBUG
    atp_print_bufuse( ah, "atp_rsel at top" );
#endif /* EBUG */
    if ( func == 0 ) {
	func = ATP_FUNCANY;
    }

    requesting = ( func & ATP_TRESP ) && ah->atph_rrespcount > 0 &&
	( ah->atph_reqtries > 0 || ah->atph_reqtries == ATP_TRIES_INFINITE );

    if ( requesting && ah->atph_rbitmap == 0 ) {
	/*
	 * we already have a complete atp response; just return
	 */
	return( ATP_TRESP );
    }

    if (( abuf = atp_alloc_buf()) == NULL ) {
	return( -1 );
    }

    if ( requesting ) {
#ifdef EBUG
	printf( "<%d> atp_rsel: request pending\n", getpid());
#endif /* EBUG */
	gettimeofday( &tv, (struct timezone *)0 );
	if ( tv.tv_sec - ah->atph_reqtv.tv_sec > ah->atph_reqto ) {
	    if ( resend_request( ah ) < 0 ) {
		atp_free_buf( abuf );
		return( -1 );
	    }
	}
    }

    for ( ;; ) {
	rfunc = func;
	if ( requesting ) {
	    FD_ZERO( &fds );
	    FD_SET( ah->atph_socket, &fds );
	    tv.tv_sec = ah->atph_reqto;
	    tv.tv_usec = 0;
	    if (( c = select( ah->atph_socket + 1, &fds, NULL, NULL,
		    &tv )) < 0 ) {
		atp_free_buf( abuf );
		return( -1 );
	    }
	    if ( c == 0 || FD_ISSET( ah->atph_socket, &fds ) == 0 ) {
		recvlen = -1;
		errno = EINTR;
		goto timeout;
	    }
	}
	memcpy( &saddr, faddr, sizeof( struct sockaddr_at ));
#ifdef EBUG
	printf( "<%d> atp_rsel calling recv_atp,", getpid());
	atp_print_addr( " accepting from: ", &saddr );
	putchar( '\n' );
#endif /* EBUG */
	if (( recvlen = atp_recv_atp( ah, &saddr, &rfunc, ATP_TIDANY,
		abuf->atpbuf_info.atpbuf_data, 0 )) >= 0 ) {
	    break;	/* we received something */
	}

timeout :
	if ( !requesting || errno != EINTR ) {
	    break;	/* error */
	}
	    
	if ( ah->atph_reqtries <= 0 &&
		ah->atph_reqtries != ATP_TRIES_INFINITE ) {
	    errno = ETIMEDOUT;
	    break;
	}
	
	if ( resend_request( ah ) < 0 ) {
	    break;	/* error */
	}
    }

    if ( recvlen <= 0 ) {	/* error */
	atp_free_buf( abuf );
	return( recvlen );
    }

#ifdef EBUG
    printf( "<%d> atp_rsel: rcvd %d bytes", getpid(), recvlen );
    atp_print_addr( " from: ", &saddr );
    putchar( '\n' );
    bprint( abuf->atpbuf_info.atpbuf_data, recvlen );
#endif /* EBUG */

    abuf->atpbuf_dlen = (size_t) recvlen;
    memcpy( &resp_hdr, abuf->atpbuf_info.atpbuf_data + 1,
	    sizeof( struct atphdr ));

    if ( rfunc == ATP_TREQ ) {
	/*
	 * we got a request: check to see if it is a duplicate (XO)
	 * while we are at it, we expire old XO responses from sent list
	 */
	memcpy( &req_hdr, abuf->atpbuf_info.atpbuf_data + 1, 
		sizeof( struct atphdr ));
	tid = ntohs( req_hdr.atphd_tid );
	gettimeofday( &tv, (struct timezone *)0 );
	for ( pb = NULL, cb = ah->atph_sent; cb != NULL;
		pb = cb, cb = cb->atpbuf_next ) {
#ifdef EBUG
	    printf( "<%d>", getpid());
	    atp_print_addr( " examining", &cb->atpbuf_addr );
	    printf( " %hu", cb->atpbuf_info.atpbuf_xo.atpxo_tid );
	    atp_print_addr( " (looking for", &saddr );
	    printf( " %hu)\n", tid );
#endif /* EBUG */
	    if ( tv.tv_sec - cb->atpbuf_info.atpbuf_xo.atpxo_tv.tv_sec
		    > cb->atpbuf_info.atpbuf_xo.atpxo_reltime ) {
		/* discard expired response */
#ifdef EBUG
		printf( "<%d> expiring tid %hu\n", getpid(),
			cb->atpbuf_info.atpbuf_xo.atpxo_tid );
#endif /* EBUG */
		if ( pb == NULL ) {
		    ah->atph_sent = cb->atpbuf_next;
		} else {
		    pb->atpbuf_next = cb->atpbuf_next;
		}

		for ( i = 0; i < 8; ++i ) {
		    if ( cb->atpbuf_info.atpbuf_xo.atpxo_packet[ i ]
			    != NULL ) {
			atp_free_buf( cb->atpbuf_info.atpbuf_xo.atpxo_packet[ i ] );
		    }
		}
		atp_free_buf( cb );

		if (( cb = pb ) == NULL )
		    break;

	    } else if ( at_addr_eq( &saddr, &cb->atpbuf_addr )) {
		if ( cb->atpbuf_info.atpbuf_xo.atpxo_tid == tid ) {
		    break;
		}
	    }
	}

	if ( cb != NULL ) {
#ifdef EBUG
	    printf( "<%d> duplicate request -- re-sending XO resp\n",
		    getpid());
#endif /* EBUG */
	    /* matches an old response -- just re-send and reset expire */
	    cb->atpbuf_info.atpbuf_xo.atpxo_tv = tv;
	    for ( i = 0; i < 8; ++i ) {
		if ( cb->atpbuf_info.atpbuf_xo.atpxo_packet[i] != NULL &&
			req_hdr.atphd_bitmap & ( 1 << i )) {
		    netddp_sendto( ah->atph_socket,
			    cb->atpbuf_info.atpbuf_xo.atpxo_packet[i]->atpbuf_info.atpbuf_data,
			    cb->atpbuf_info.atpbuf_xo.atpxo_packet[i]->atpbuf_dlen,
			    0, (struct sockaddr *) &saddr, sizeof( struct sockaddr_at));
		}
	    }
	}

	if ( cb == NULL ) {
	    /* new request -- queue it and return */
	    memcpy( &abuf->atpbuf_addr, &saddr, sizeof( struct sockaddr_at ));
	    memcpy( faddr, &saddr, sizeof( struct sockaddr_at ));
	    abuf->atpbuf_next = ah->atph_queue;
	    ah->atph_queue = abuf;
	    return( ATP_TREQ );
	} else {
	    atp_free_buf( abuf );
	    return( 0 );
	}
    }

    /*
     * we got a response: update bitmap
     */
    memcpy( &req_hdr, ah->atph_reqpkt->atpbuf_info.atpbuf_data + 1, 
	    sizeof( struct atphdr ));
    if ( requesting && ah->atph_rbitmap & ( 1<<resp_hdr.atphd_bitmap )
		&& req_hdr.atphd_tid == resp_hdr.atphd_tid ) {
	ah->atph_rbitmap &= ~( 1<<resp_hdr.atphd_bitmap );

	if ( ah->atph_resppkt[ resp_hdr.atphd_bitmap ] != NULL ) {
	    atp_free_buf( ah->atph_resppkt[ resp_hdr.atphd_bitmap ] );
	}
	ah->atph_resppkt[ resp_hdr.atphd_bitmap ] = abuf;

	/* if End Of Message, clear all higher bitmap bits
	*/
	if ( resp_hdr.atphd_ctrlinfo & ATP_EOM ) {
#ifdef EBUG
	    printf( "<%d> EOM -- seq num %d  current bitmap %d\n",
		getpid(), resp_hdr.atphd_bitmap, ah->atph_rbitmap );
#endif /* EBUG */
	    mask = 1 << resp_hdr.atphd_bitmap;
	    ah->atph_rbitmap &= ( mask | (mask-1) );
	}

	/* if Send Trans. Status, send updated request
	*/
	if ( resp_hdr.atphd_ctrlinfo & ATP_STS ) {
#ifdef EBUG
	    puts( "STS" );
#endif /* EBUG */
	    req_hdr.atphd_bitmap = ah->atph_rbitmap;
	    memcpy(ah->atph_reqpkt->atpbuf_info.atpbuf_data + 1,
		   &req_hdr, sizeof( struct atphdr ));
	    if ( netddp_sendto( ah->atph_socket,
		    ah->atph_reqpkt->atpbuf_info.atpbuf_data,
		    ah->atph_reqpkt->atpbuf_dlen, 0,
		    (struct sockaddr *) &ah->atph_reqpkt->atpbuf_addr,
		    sizeof( struct sockaddr_at )) !=
		    ah->atph_reqpkt->atpbuf_dlen ) {
		atp_free_buf( abuf );
		return( -1 );
	    }
	}
    } else {
	/*
	 * we are not expecting this response -- toss it
	 */
	atp_free_buf( abuf );
#ifdef EBUG
	printf( "atp_rsel: ignoring resp bm=%x tid=%d (expected %x/%d)\n",
		resp_hdr.atphd_bitmap, ntohs( resp_hdr.atphd_tid ),
		ah->atph_rbitmap, ah->atph_tid );
#endif /* EBUG */
    }

    if ( !ah->atph_rbitmap && ( req_hdr.atphd_ctrlinfo & ATP_XO )) {
	/*
	 * successful completion - send release
	 * the release consists of DDP type byte + ATP header + 4 user bytes
	 */
	req_hdr.atphd_ctrlinfo = ATP_TREL;
	memcpy( ah->atph_reqpkt->atpbuf_info.atpbuf_data + 1, &req_hdr, 
	    sizeof( struct atphdr ));
	memset( ah->atph_reqpkt->atpbuf_info.atpbuf_data + ATP_HDRSIZE, 0, 4 );
	ah->atph_reqpkt->atpbuf_dlen = sizeof( struct atphdr ) + ATP_HDRSIZE;
#ifdef EBUG
	printf( "<%d> sending TREL", getpid() );
	bprint( ah->atph_reqpkt->atpbuf_info.atpbuf_data,
		ah->atph_reqpkt->atpbuf_dlen );
#endif /* EBUG */
#ifdef DROP_ATPTREL
	if (( ++release_count % 10 ) != 0 ) {
#endif /* DROP_ATPTREL */
	netddp_sendto( ah->atph_socket, 
		       ah->atph_reqpkt->atpbuf_info.atpbuf_data,
		       ah->atph_reqpkt->atpbuf_dlen, 0,
		       (struct sockaddr *) &ah->atph_reqpkt->atpbuf_addr,
		       sizeof( struct sockaddr_at));
#ifdef DROP_ATPTREL
	}
#endif /* DROP_ATPTREL */
    }

    if ( ah->atph_rbitmap != 0 ) {
	if ( ah->atph_reqtries > 0
		|| ah->atph_reqtries == ATP_TRIES_INFINITE ) {
	    return( 0 );
	} else {
	    errno = ETIMEDOUT;
	    return( -1 );
	}
    }

    memcpy( faddr, &saddr, sizeof( struct sockaddr_at ));
    return( ATP_TRESP );
}
