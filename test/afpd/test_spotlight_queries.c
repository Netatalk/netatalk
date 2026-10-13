/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <talloc.h>

#include "etc/spotlight/spotlight_private.h"

static int close_calls;
static int freed_queries;

static int query_freed(slq_t *slq)
{
    assert(slq->slq_membership == SLQ_MEMBERSHIP_DETACHED);
    assert(slq->slq_next == NULL && slq->slq_prev == NULL);
    freed_queries++;
    return 0;
}

static void close_query(slq_t *slq)
{
    /* Deferred cancellation must retain backend resources until acknowledged. */
    assert(slq->slq_state != SLQ_STATE_CANCEL_PENDING);
    assert(slq->slq_backend_private != NULL);
    close_calls++;
}

static const sl_backend_ops backend = {.sbo_close_query = close_query};
static const struct vol vol = {.v_sl_backend = &backend};

static slq_t *new_query(uint64_t ctx, slq_state_t state)
{
    slq_t *q = talloc_zero(NULL, slq_t);
    assert(q != NULL);
    assert(q->slq_membership == SLQ_MEMBERSHIP_DETACHED);
    q->slq_vol = &vol;
    q->slq_ctx1 = ctx;
    q->slq_ctx2 = 42;
    q->slq_state = state;
    q->slq_time = time(NULL);
    q->slq_backend_private = talloc_zero(q, int);
    assert(q->slq_backend_private != NULL);
    talloc_set_destructor(q, query_freed);
    return q;
}

static void test_failed_open_and_lookup(void)
{
    slq_t *old = new_query(1, SLQ_STATE_DONE);
    slq_t *new = new_query(1, SLQ_STATE_DONE);
    slq_t *failed = new_query(1, SLQ_STATE_NEW);
    slq_add(old);
    slq_add(new);
    assert(slq_for_ctx(1, 42) == new);
    assert(slq_for_ctx(1, 43) == NULL);
    assert(slq_for_ctx(2, 42) == NULL);
    /* Failed opens are never published, even when their contexts match. */
    slq_destroy(failed);
    assert(slq_for_ctx(1, 42) == new);
    assert(old->slq_prev == new && new->slq_next == old);
    slq_destroy(new);
    assert(slq_for_ctx(1, 42) == old && old->slq_prev == NULL);
    slq_destroy(old);
    assert(slq_for_ctx(1, 42) == NULL);
    slq_destroy(NULL);
}

static void test_unlink_positions(void)
{
    slq_t *tail = new_query(2, SLQ_STATE_DONE);
    slq_t *middle = new_query(3, SLQ_STATE_DONE);
    slq_t *head = new_query(4, SLQ_STATE_DONE);
    slq_add(tail);
    slq_add(middle);
    slq_add(head);
    slq_destroy(middle);
    assert(head->slq_next == tail && tail->slq_prev == head);
    slq_destroy(tail);
    assert(head->slq_next == NULL);
    slq_destroy(head);
    assert(slq_for_ctx(2, 42) == NULL);
    assert(slq_for_ctx(3, 42) == NULL);
    assert(slq_for_ctx(4, 42) == NULL);
}

static void test_deferred_cancellation(void)
{
    slq_t *old = new_query(5, SLQ_STATE_RUNNING);
    slq_t *new = new_query(5, SLQ_STATE_RESULTS);
    slq_t *third = new_query(6, SLQ_STATE_RUNNING);
    int closed = close_calls;
    int freed = freed_queries;
    slq_add(old);
    slq_add(new);
    slq_add(third);
    slq_cancel(old);
    slq_cancel(new);
    slq_cancel(third);
    assert(slq_for_ctx(5, 42) == NULL && slq_for_ctx(6, 42) == NULL);
    assert(new->slq_membership == SLQ_MEMBERSHIP_CANCELLED);
    slq_cancelled_cleanup();
    assert(close_calls == closed && freed_queries == freed);
    assert(new->slq_backend_private != NULL);
    assert(new->slq_state == SLQ_STATE_CANCEL_PENDING);
    /* The backend callback acknowledges a middle entry before its neighbors. */
    new->slq_state = SLQ_STATE_CANCELLED;
    slq_cancelled_cleanup();
    assert(close_calls == closed + 1 && freed_queries == freed + 1);
    assert(third->slq_next == old && old->slq_prev == third);
    old->slq_state = SLQ_STATE_CANCELLED;
    third->slq_state = SLQ_STATE_CANCELLED;
    slq_cancelled_cleanup();
    assert(close_calls == closed + 3 && freed_queries == freed + 3);
    slq_cancelled_cleanup();
    assert(close_calls == closed + 3);
}

static void test_close(void)
{
    const slq_state_t states[] = {
        SLQ_STATE_RUNNING, SLQ_STATE_RESULTS, SLQ_STATE_FULL,
        SLQ_STATE_DONE, SLQ_STATE_ERROR
    };
    slq_t *queries[5];
    slq_t *unexpected = new_query(35, SLQ_STATE_NEW);
    int closed = close_calls;
    int freed = freed_queries;
    assert(slq_close(NULL) == 0);

    for (size_t i = 0; i < 5; i++) {
        queries[i] = new_query(30 + i, states[i]);
        slq_add(queries[i]);
        /* Packaging failures may leave a pending query without result buffers. */
        assert(queries[i]->query_results == NULL);
        assert(slq_close(queries[i]) == 0);
        assert(slq_for_ctx(30 + i, 42) == NULL);
    }

    assert(close_calls == closed + 3 && freed_queries == freed + 3);
    slq_cancelled_cleanup();
    assert(close_calls == closed + 3 && freed_queries == freed + 3);

    for (size_t i = 0; i < 2; i++) {
        assert(queries[i]->slq_state == SLQ_STATE_CANCEL_PENDING);
        assert(queries[i]->slq_membership == SLQ_MEMBERSHIP_CANCELLED);
        assert(queries[i]->slq_backend_private != NULL);
        queries[i]->slq_state = SLQ_STATE_CANCELLED;
    }

    slq_cancelled_cleanup();
    assert(close_calls == closed + 5 && freed_queries == freed + 5);
    slq_add(unexpected);
    assert(slq_close(unexpected) == -1);
    assert(slq_for_ctx(35, 42) == unexpected);
    assert(unexpected->slq_state == SLQ_STATE_NEW);
    slq_destroy(unexpected);
    assert(close_calls == closed + 6 && freed_queries == freed + 6);
}

static void test_idle_cleanup(void)
{
    const slq_state_t states[] = {
        SLQ_STATE_RUNNING, SLQ_STATE_DONE, SLQ_STATE_RESULTS,
        SLQ_STATE_FULL, SLQ_STATE_ERROR, SLQ_STATE_NEW
    };
    slq_t *queries[6];
    slq_t *recent = new_query(20, SLQ_STATE_DONE);
    slq_t *future = new_query(21, SLQ_STATE_RUNNING);
    slq_t *retained = new_query(22, SLQ_STATE_FULL);
    int closed = close_calls;

    for (size_t i = 0; i < 6; i++) {
        queries[i] = new_query(10 + i, states[i]);
        queries[i]->slq_time -= 1000;
        slq_add(queries[i]);
    }

    future->slq_time += 1000;
    retained->slq_time -= 120; /* terminal queries have a longer idle limit */
    slq_add(recent);
    slq_add(future);
    slq_add(retained);
    slq_idle_cleanup();
    assert(close_calls == closed + 3);
    assert(slq_for_ctx(10, 42) == NULL && slq_for_ctx(12, 42) == NULL);
    assert(queries[0]->slq_state == SLQ_STATE_CANCEL_PENDING);
    assert(queries[2]->slq_state == SLQ_STATE_CANCEL_PENDING);
    assert(slq_for_ctx(11, 42) == NULL);
    assert(slq_for_ctx(13, 42) == NULL);
    assert(slq_for_ctx(14, 42) == NULL);
    assert(slq_for_ctx(15, 42) == queries[5]);
    assert(slq_for_ctx(20, 42) == recent);
    assert(slq_for_ctx(21, 42) == future);
    assert(slq_for_ctx(22, 42) == retained);
    queries[0]->slq_state = SLQ_STATE_CANCELLED;
    queries[2]->slq_state = SLQ_STATE_CANCELLED;
    slq_cancelled_cleanup();
    slq_destroy(queries[5]);
    slq_destroy(recent);
    slq_destroy(future);
    slq_destroy(retained);
}

int main(void)
{
    slq_idle_cleanup();
    slq_cancelled_cleanup();
    test_failed_open_and_lookup();
    test_unlink_positions();
    test_deferred_cancellation();
    test_close();
    test_idle_cleanup();
    slq_idle_cleanup();
    slq_cancelled_cleanup();
    assert(close_calls == 24 && freed_queries == 24);
    puts("Spotlight query lifecycle tests passed");
    return 0;
}
