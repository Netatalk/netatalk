/*
  Copyright (c) 2026 Andy Lemin (andylemin)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

/*
 * ATP unmatched-receive queue tests.
 *
 * The queue holds packets no request has claimed. It is bounded, so a full
 * queue has to sacrifice something, and which packet it sacrifices decides
 * whether a session survives: the packet just received is the only one a
 * caller may still be waiting for, so it must never be the victim. Dropping
 * it instead of an older entry wedges the session permanently, because the
 * entries already there are the ones nothing can claim.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef NO_DDP

#include <string.h>

#include <netatalk/at.h>
#include <atalk/atp.h>

#include "atp_internals.h"
#include "subtests_atp.h"

/*
 * Buffers come from the library pool, so the tests exercise the same ownership
 * the transport does — atp_queue_push() releases what it evicts through
 * atp_free_buf(), which returns it to that pool rather than freeing it. Each
 * test calls atp_bufs_release() when it is done so the pool's chunks are freed
 * and nothing of this file's is left on the free list for a later test to be
 * handed.
 */
static struct atpbuf *atp_test_alloc(char tag)
{
    struct atpbuf *buf = atp_alloc_buf();

    if (buf == NULL) {
        return NULL;
    }

    memset(buf, 0, sizeof(*buf));
    buf->atpbuf_dlen = 1;
    buf->atpbuf_info.atpbuf_data[0] = tag;      /* recognise it after the push */
    return buf;
}

static int atp_test_queue_len(struct atp_handle *ah)
{
    int n = 0;

    for (struct atpbuf *q = ah->atph_queue; q != NULL; q = q->atpbuf_next) {
        n++;
    }

    return n;
}

static void atp_test_queue_free(struct atp_handle *ah)
{
    while (ah->atph_queue != NULL) {
        struct atpbuf *next = ah->atph_queue->atpbuf_next;
        atp_free_buf(ah->atph_queue);
        ah->atph_queue = next;
    }

    ah->atph_queue = NULL;
    atp_bufs_release();
}

/*
 * Fill the queue to the bound, then push one more. The queue must stay at the
 * bound, and the entry that goes is the oldest — the tail, since pushes are at
 * the head.
 */
int utest_atp_queue_push_evicts_oldest(void)
{
    struct atp_handle ah;
    struct atpbuf *buf;
    struct atpbuf *tail;
    int rc = 0;
    memset(&ah, 0, sizeof(ah));

    /* 'a' is pushed first, so it is the oldest and should be the one evicted */
    for (int i = 0; i < ATP_MAXQUEUE; i++) {
        if ((buf = atp_test_alloc((char)('a' + i))) == NULL) {
            rc = 1;
            goto done;
        }

        atp_queue_push(&ah, buf);
    }

    if (atp_test_queue_len(&ah) != ATP_MAXQUEUE) {
        rc = 2;
        goto done;
    }

    /* the oldest is the tail */
    for (tail = ah.atph_queue; tail->atpbuf_next != NULL;
            tail = tail->atpbuf_next) {
        /* walk */
    }

    if (tail->atpbuf_info.atpbuf_data[0] != 'a') {
        rc = 3;
        goto done;
    }

    if ((buf = atp_test_alloc('Z')) == NULL) {
        rc = 4;
        goto done;
    }

    atp_queue_push(&ah, buf);

    if (atp_test_queue_len(&ah) != ATP_MAXQUEUE) {
        rc = 5;                 /* bound not held */
        goto done;
    }

    for (struct atpbuf *q = ah.atph_queue; q != NULL; q = q->atpbuf_next) {
        if (q->atpbuf_info.atpbuf_data[0] == 'a') {
            rc = 6;             /* oldest was not the victim */
            goto done;
        }
    }

done:
    atp_test_queue_free(&ah);
    return rc;
}

/*
 * The packet just pushed must be present and reachable afterwards. A full
 * queue that discards the incoming packet instead loses every subsequent
 * request, which no retransmission can recover from because the queue never
 * drains on its own.
 */
int utest_atp_queue_push_keeps_newest(void)
{
    struct atp_handle ah;
    struct atpbuf *buf;
    int rc = 0;
    memset(&ah, 0, sizeof(ah));

    /* deliberately overfill: every push must still surface its own packet */
    for (int i = 0; i < ATP_MAXQUEUE * 3; i++) {
        if ((buf = atp_test_alloc('Z')) == NULL) {
            rc = 1;
            goto done;
        }

        atp_queue_push(&ah, buf);

        if (ah.atph_queue != buf) {
            rc = 2;             /* newest not at the head */
            goto done;
        }

        if (ah.atph_queue->atpbuf_info.atpbuf_data[0] != 'Z') {
            rc = 3;
            goto done;
        }

        if (atp_test_queue_len(&ah) > ATP_MAXQUEUE) {
            rc = 4;             /* bound exceeded */
            goto done;
        }
    }

done:
    atp_test_queue_free(&ah);
    return rc;
}

#endif /* NO_DDP */
