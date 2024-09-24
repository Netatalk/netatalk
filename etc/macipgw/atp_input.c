/*
 * Add an AT packet to ATPs input queue
 *
 * $Id: atp_input.c,v 1.1.1.1 2001/10/28 15:01:48 stefanbethke Exp $
 *
 * (c) 1997 Stefan Bethke. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <string.h>
#undef s_net
#include <netatalk/at.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>

/*
 * atp_internals.h declares a few functions we require; they are exported in
 * libatalk, but not declared in any public header file.  atp_internals.h
 * has been copied over from netatalk-2.2.4.
 */
#include "atp_internals.h"

#ifdef EBUG
#include <stdio.h>
#endif

/*#define EBUG    
*/

int atp_input(ATP ah, struct sockaddr_at *faddr, char *rbuf, int recvlen) {
	struct atpbuf		*pq, *cq;
	struct atphdr		ahdr;
	uint16_t				rfunc;
	uint16_t				rtid;
	int					i;
	struct atpbuf		*inbuf;

	bcopy( rbuf + 1, (char *)&ahdr, sizeof( struct atphdr ));
	if ( recvlen >= ATP_HDRSIZE && *rbuf == DDPTYPE_ATP) {
		/* this is a valid ATP packet -- check for a match */
		rfunc = ahdr.atphd_ctrlinfo & ATP_FUNCMASK;
		rtid = ahdr.atphd_tid;
#ifdef EBUG
		printf( "<%d> got tid=%hu func=", getpid(), ntohs( rtid ));
		print_func( rfunc );
		print_addr( " from", faddr );
		putchar( '\n' );
		bprint( rbuf, recvlen );
#endif
		if ( rfunc == ATP_TREL ) {
			/* remove response from sent list */
			for ( pq = NULL, cq = ah->atph_sent; cq != NULL;
			  pq = cq, cq = cq->atpbuf_next ) {
				if ( at_addr_eq( faddr, &cq->atpbuf_addr ) &&
				  cq->atpbuf_info.atpbuf_xo.atpxo_tid == ntohs( rtid )) 
					break;
			}
			if ( cq != NULL ) {
#ifdef EBUG
		printf( "<%d> releasing transaction %hu\n", getpid(), ntohs( rtid ));
#endif
				if ( pq == NULL ) {
					ah->atph_sent = cq->atpbuf_next;
				} else {
					pq->atpbuf_next = cq->atpbuf_next;
				}
				for ( i = 0; i < 8; ++i ) {
					if (cq->atpbuf_info.atpbuf_xo.atpxo_packet[i] != NULL) {
						atp_free_buf(cq->atpbuf_info.atpbuf_xo.atpxo_packet[i]);
					}
				}
				atp_free_buf(cq);
			}
		} else {
			/* add packet to incoming queue */
#ifdef EBUG
		printf( "<%d> queuing incoming...\n", getpid() );
#endif
			if (( inbuf = atp_alloc_buf() ) == NULL ) {
#ifdef EBUG
		printf( "<%d> can't alloc buffer\n", getpid() );
#endif
				return -1;
			}
			bcopy( (char *)faddr, (char *)&inbuf->atpbuf_addr,
			  sizeof( struct sockaddr_at ));
			inbuf->atpbuf_next = ah->atph_queue;
			ah->atph_queue = inbuf;
			inbuf->atpbuf_dlen = recvlen;
			bcopy( (char *)rbuf,
				(char *)inbuf->atpbuf_info.atpbuf_data, recvlen );
		}
		return 0;
	}

	return -1; /* invalid packet */
}
