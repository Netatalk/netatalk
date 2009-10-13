/*
 * $Id: atp_close.c,v 1.5 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1990,1997 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netatalk/at.h>
#include <atalk/netddp.h>
#include <atalk/atp.h>
#include "atp_internals.h"
#ifdef EBUG
#include <stdio.h>
#endif /* EBUG */

int atp_close(ATP ah)
{
    struct atpbuf	*cq;
    int			i;

    /* remove from list of open atp sockets & discard queued data
    */
#ifdef EBUG
    print_bufuse( ah, "atp_close");
#endif /* EBUG */

    while ( ah->atph_queue != NULL ) {
	cq = ah->atph_queue;
	ah->atph_queue = cq->atpbuf_next;
	atp_free_buf( cq );
    }

    while ( ah->atph_sent != NULL ) {
	cq = ah->atph_sent;
	for ( i = 0; i < 8; ++i ) {
	    if ( cq->atpbuf_info.atpbuf_xo.atpxo_packet[ i ] != NULL ) {
		atp_free_buf( cq->atpbuf_info.atpbuf_xo.atpxo_packet[ i ] );
	    }
	}
	ah->atph_sent = cq->atpbuf_next;
	atp_free_buf( cq );
    }

    if ( ah->atph_reqpkt != NULL ) {
	atp_free_buf( ah->atph_reqpkt );
	ah->atph_reqpkt = NULL;
    }

    for ( i = 0; i < 8; ++i ) {
	if ( ah->atph_resppkt[ i ] != NULL ) {
	    atp_free_buf( ah->atph_resppkt[ i ] );
	    ah->atph_resppkt[ i ] = NULL;
	}
    }

#ifdef EBUG
    print_bufuse( ah, "atp_close end");
#endif /* EBUG */

    i = ah->atph_socket;
    atp_free_buf( (struct atpbuf *) ah );

    if (netddp_close(i) < 0)
      return -1;

    return 0;
}
