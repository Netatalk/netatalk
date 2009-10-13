/*
 * $Id: atp_bufs.c,v 1.5 2009-10-13 22:55:37 didg Exp $
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

/* 
 * Our own memory maintenance for atp
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netatalk/at.h>
#include <atalk/atp.h>
#include "atp_internals.h"

#define			N_MORE_BUFS		10

static struct atpbuf 	*free_list = NULL;	/* free buffers */

#ifdef EBUG
static int		numbufs = 0;
#endif /* EBUG */

/* only call this when the free_list is empty...
 * N_MORE_BUFS must be >= one
*/
static int more_bufs(void)
{
    int			i;
    char		*mem;
    struct atpbuf	*bp;

    /* get the whole chunk in one malloc call
    */
    if (( mem = malloc( N_MORE_BUFS * sizeof( struct atpbuf ))) == NULL ) {
	errno = ENOBUFS;
	return -1;
    }
    /* now split into separate bufs
    */
    bp = free_list = (struct atpbuf *) mem;
    for ( i = 1; i < N_MORE_BUFS; ++i ) {
	bp->atpbuf_next = (struct atpbuf *) ( mem += sizeof( struct atpbuf ));
	bp = bp->atpbuf_next;
    }
    bp->atpbuf_next = NULL;

    return 0;
}


#ifdef EBUG
void atp_print_bufuse(ATP ah, char *s)
{
    struct atpbuf	*bp;
    int			i, sentcount, incount, respcount;

    sentcount = 0;
    for ( bp = ah->atph_sent; bp != NULL; bp = bp->atpbuf_next ) {
	++sentcount;
	for ( i = 0; i < 8; ++i ) {
	    if ( bp->atpbuf_info.atpbuf_xo.atpxo_packet[ i ] != NULL ) {
		++sentcount;
	    }
	}
    }

    if ( ah->atph_reqpkt != NULL ) {
	++sentcount;
    }


    incount = 0;
    for ( bp = ah->atph_queue; bp != NULL; bp = bp->atpbuf_next, ++incount );

    respcount = 0;
    for ( i = 0; i < 8; ++i ) {
        if ( ah->atph_resppkt[ i ] != NULL ) {
	    ++respcount;
	}
    }

    printf( "<%d> %s: bufs total %d  sent %d  incoming %d  req %d  resp %d\n",
	getpid(), s, numbufs, sentcount, incount,
	( ah->atph_reqpkt != NULL ) ? 1: 0, respcount );
}
#endif /* EBUG */


struct atpbuf *atp_alloc_buf(void)
{
    struct atpbuf *bp;

    if ( free_list == NULL && more_bufs() ) return NULL;

    bp = free_list;
    free_list = free_list->atpbuf_next;
#ifdef EBUG
    ++numbufs;
#endif /* EBUG */
    return bp;
}


int atp_free_buf(struct atpbuf *bp)
{
    if ( bp == NULL ) {
	return -1;
    }
    bp->atpbuf_next = free_list;
    free_list = bp;
#ifdef EBUG
    --numbufs;
#endif /* EBUG */
    return 0;
}


