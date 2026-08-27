/*
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

/*!
 * @file
 * Our own memory maintenance for atp
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netatalk/at.h>
#include <atalk/atp.h>
#include "atp_internals.h"

#ifdef EBUG
#include <stdio.h>
#endif /* EBUG */

#define			N_MORE_BUFS		10

static struct atpbuf 	*free_list = NULL;	/*!< free buffers */
/* Bases of the malloc'd chunks free_list is carved from. free_list holds
 * interior pointers in hand-back order, so the bases cannot be recovered from
 * it and have to be recorded to be freed. */
static char		**chunks = NULL;
static size_t		nchunks = 0;
static size_t		chunkcap = 0;

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
    if ((mem = malloc(N_MORE_BUFS * sizeof(struct atpbuf))) == NULL) {
        errno = ENOBUFS;
        return -1;
    }

    if (nchunks == chunkcap) {
        size_t newcap = (chunkcap == 0) ? 4 : chunkcap * 2;
        char **grown = realloc(chunks, newcap * sizeof(*chunks));

        if (grown == NULL) {
            free(mem);
            errno = ENOBUFS;
            return -1;
        }

        chunks = grown;
        chunkcap = newcap;
    }

    chunks[nchunks++] = mem;
    /* now split into separate bufs
    */
    bp = free_list = (struct atpbuf *) mem;

    for (i = 1; i < N_MORE_BUFS; ++i) {
        bp->atpbuf_next = (struct atpbuf *)(mem += sizeof(struct atpbuf));
        bp = bp->atpbuf_next;
    }

    bp->atpbuf_next = NULL;
    return 0;
}


#ifdef EBUG
void atp_print_bufuse(ATP ah, char *s)
{
    struct atpbuf *bp;
    int i;
    int sentcount;
    int incount;
    int respcount;
    sentcount = 0;

    for (bp = ah->atph_sent; bp != NULL; bp = bp->atpbuf_next) {
        ++sentcount;

        for (i = 0; i < 8; ++i) {
            if (bp->atpbuf_info.atpbuf_xo.atpxo_packet[i] != NULL) {
                ++sentcount;
            }
        }
    }

    if (ah->atph_reqpkt != NULL) {
        ++sentcount;
    }

    incount = 0;

    for (bp = ah->atph_queue; bp != NULL; bp = bp->atpbuf_next, ++incount);

    respcount = 0;

    for (i = 0; i < 8; ++i) {
        if (ah->atph_resppkt[i] != NULL) {
            ++respcount;
        }
    }

    printf("<%d> %s: bufs total %d  sent %d  incoming %d  req %d  resp %d\n",
           getpid(), s, numbufs, sentcount, incount,
           (ah->atph_reqpkt != NULL) ? 1 : 0, respcount);
}
#endif /* EBUG */


struct atpbuf *atp_alloc_buf(void)
{
    struct atpbuf *bp;

    if (free_list == NULL && more_bufs()) {
        return NULL;
    }

    bp = free_list;
    free_list = free_list->atpbuf_next;
#ifdef EBUG
    ++numbufs;
#endif /* EBUG */
    return bp;
}


/*
 * Release every chunk the pool has taken. Only valid once no buffer is still
 * in use anywhere, since it frees the memory the handed-out buffers live in;
 * a session process simply exits instead of calling this.
 */
void atp_bufs_release(void)
{
    for (size_t i = 0; i < nchunks; i++) {
        free(chunks[i]);
    }

    free(chunks);
    chunks = NULL;
    nchunks = chunkcap = 0;
    free_list = NULL;
}

int atp_free_buf(struct atpbuf *bp)
{
    if (bp == NULL) {
        return -1;
    }

    bp->atpbuf_next = free_list;
    free_list = bp;
#ifdef EBUG
    --numbufs;
#endif /* EBUG */
    return 0;
}


