/*
 * Copyright (c) 2012-2014 Ralph Boehme <sloowfranklin@gmail.com>
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

/*!
 * @file
 * @brief Session-local Spotlight query ownership and deferred cancellation
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <inttypes.h>
#include <time.h>
#include <talloc.h>
#include <atalk/logger.h>

#include "etc/spotlight/spotlight_private.h"

#define MAX_SL_QUERY_IDLE_TIME_ACTIVE 60
#define MAX_SL_QUERY_IDLE_TIME_TERMINAL 300

/*! Active queries in newest-first client lookup order. */
static slq_t *sl_queries;
/*! Queries retained while an asynchronous callback still owns them. */
static slq_t *sl_cancelled_queries;

/*!
 * @brief Return the log name of a backend execution state
 *
 * @param[in] state  valid backend execution state
 * @returns the corresponding static log string
 */
const char *slq_state_name(slq_state_t state)
{
    static const char *const names[] = {
        "SLQ_STATE_NEW", "SLQ_STATE_RUNNING", "SLQ_STATE_RESULTS",
        "SLQ_STATE_FULL", "SLQ_STATE_DONE", "SLQ_STATE_CANCEL_PENDING",
        "SLQ_STATE_CANCELLED", "SLQ_STATE_ERROR"
    };
    assert(state >= SLQ_STATE_NEW && state <= SLQ_STATE_ERROR);
    return names[state];
}

/*!
 * @brief Unlink a query from its owner in constant time
 *
 * Detached queries, including failed opens, are safe to remove. Membership
 * identifies the owning head independently of the backend execution state.
 *
 * @param[in,out] slq  query to detach
 */
static void slq_remove(slq_t *slq)
{
    slq_t **head;

    switch (slq->slq_membership) {
    case SLQ_MEMBERSHIP_ACTIVE:
        head = &sl_queries;
        break;

    case SLQ_MEMBERSHIP_CANCELLED:
        head = &sl_cancelled_queries;
        break;

    case SLQ_MEMBERSHIP_DETACHED:
        return;

    default:
        assert(0);
        return;
    }

    if (slq->slq_prev) {
        slq->slq_prev->slq_next = slq->slq_next;
    } else {
        assert(*head == slq);
        *head = slq->slq_next;
    }

    if (slq->slq_next) {
        slq->slq_next->slq_prev = slq->slq_prev;
    }

    slq->slq_next = NULL;
    slq->slq_prev = NULL;
    slq->slq_membership = SLQ_MEMBERSHIP_DETACHED;
}

/*!
 * @brief Publish a detached query for client lookup, newest first
 *
 * Repeated context IDs resolve to the most recently published query.
 *
 * @param[in,out] slq  detached query to publish
 */
void slq_add(slq_t *slq)
{
    assert(slq->slq_membership == SLQ_MEMBERSHIP_DETACHED);
    slq->slq_prev = NULL;
    slq->slq_next = sl_queries;

    if (sl_queries) {
        sl_queries->slq_prev = slq;
    }

    sl_queries = slq;
    slq->slq_membership = SLQ_MEMBERSHIP_ACTIVE;
}

/*!
 * @brief Find the newest active query with a pair of client context IDs
 *
 * @param[in] ctx1  first context ID
 * @param[in] ctx2  second context ID
 * @returns the query, or NULL when no active query matches
 */
slq_t *slq_for_ctx(uint64_t ctx1, uint64_t ctx2)
{
    for (slq_t *q = sl_queries; q; q = q->slq_next) {
        if (q->slq_ctx1 == ctx1 && q->slq_ctx2 == ctx2) {
            return q;
        }
    }

    return NULL;
}

/*!
 * @brief Close the backend, detach, and free a query, or ignore NULL
 *
 * The caller must ensure no asynchronous callback still owns the query.
 * A failed open is detached; cancellation requires backend acknowledgement
 * before this function may release the allocation.
 *
 * @param[in] slq  query to release, or NULL
 */
void slq_destroy(slq_t *slq)
{
    if (slq == NULL) {
        return;
    }

    if (slq->slq_vol && slq->slq_vol->v_sl_backend
            && slq->slq_vol->v_sl_backend->sbo_close_query) {
        slq->slq_vol->v_sl_backend->sbo_close_query(slq);
    }

    slq_remove(slq);
    talloc_free(slq);
}

/*!
 * @brief Hide an active query while its asynchronous backend stops
 *
 * Retain the query and its backend resources until a callback sets
 * SLQ_STATE_CANCELLED. Execution state and list membership are separate.
 *
 * @param[in,out] slq  active query to cancel
 */
void slq_cancel(slq_t *slq)
{
    assert(slq->slq_membership == SLQ_MEMBERSHIP_ACTIVE);
    slq->slq_state = SLQ_STATE_CANCEL_PENDING;
    slq_remove(slq);
    slq->slq_next = sl_cancelled_queries;

    if (sl_cancelled_queries) {
        sl_cancelled_queries->slq_prev = slq;
    }

    sl_cancelled_queries = slq;
    slq->slq_membership = SLQ_MEMBERSHIP_CANCELLED;
}

/*!
 * @brief End an active query, retaining it while an asynchronous callback owns it
 *
 * RUNNING and RESULTS require cancellation acknowledgement. FULL, DONE and
 * ERROR have no pending callback and can be destroyed immediately. Callers
 * must preserve backend execution state when result packaging fails.
 *
 * @param[in,out] slq  active query to close, or NULL
 * @returns 0, or -1 for an unexpected execution state
 */
int slq_close(slq_t *slq)
{
    if (slq == NULL) {
        return 0;
    }

    switch (slq->slq_state) {
    case SLQ_STATE_FULL:
    case SLQ_STATE_DONE:
    case SLQ_STATE_ERROR:
        LOG(log_debug, logtype_sl, "close: destroying query: state %s",
            slq_state_name(slq->slq_state));
        slq_destroy(slq);
        return 0;

    case SLQ_STATE_RUNNING:
    case SLQ_STATE_RESULTS:
        LOG(log_debug, logtype_sl, "close: cancel query: state %s",
            slq_state_name(slq->slq_state));
        slq_cancel(slq);
        return 0;

    default:
        LOG(log_error, logtype_sl, "Unexpected state %d", slq->slq_state);
        return -1;
    }
}

/*!
 * @brief Release queries whose asynchronous callbacks acknowledged cancellation
 */
void slq_cancelled_cleanup(void)
{
    slq_t *next;

    for (slq_t *q = sl_cancelled_queries; q; q = next) {
        next = q->slq_next;
        LOG(log_debug, logtype_sl,
            "ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": %s",
            q->slq_ctx1, q->slq_ctx2, slq_state_name(q->slq_state));

        if (q->slq_state == SLQ_STATE_CANCELLED) {
            slq_destroy(q);
        }
    }
}

/*!
 * @brief Cancel idle running queries and destroy idle terminal queries
 *
 * Backend close hooks release only the current query's resources and do not
 * pump callbacks or remove other queries, so the saved successor stays valid.
 * NEW queries are left alone and do not restart traversal.
 */
void slq_idle_cleanup(void)
{
    time_t now = time(NULL);
    slq_t *next;

    for (slq_t *q = sl_queries; q; q = next) {
        time_t idle_limit;
        next = q->slq_next;

        switch (q->slq_state) {
        case SLQ_STATE_DONE:
        case SLQ_STATE_FULL:
        case SLQ_STATE_ERROR:
            idle_limit = MAX_SL_QUERY_IDLE_TIME_TERMINAL;
            break;

        default:
            idle_limit = MAX_SL_QUERY_IDLE_TIME_ACTIVE;
            break;
        }

        if (q->slq_time > now || now - q->slq_time < idle_limit) {
            continue;
        }

        LOG(log_debug, logtype_sl,
            "ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": idle timeout, state: %s",
            q->slq_ctx1, q->slq_ctx2, slq_state_name(q->slq_state));
        slq_close(q);
    }
}

/*!
 * @brief Log every active query in client lookup order
 */
void slq_dump(void)
{
    int i = 0;

    for (const slq_t *q = sl_queries; q; q = q->slq_next) {
        LOG(log_debug, logtype_sl,
            "query[%d]: ctx1: %" PRIx64 ", ctx2: %" PRIx64 ", state: %s",
            i++, q->slq_ctx1, q->slq_ctx2, slq_state_name(q->slq_state));
    }
}
