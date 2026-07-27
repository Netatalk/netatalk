/*
 * Copyright (c) 2026 Andy Lemin (andylemin)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>

#include <atalk/logger.h>
#include <atalk/queue.h>

#include "dircache.h"
#include "directory.h"
#include "idle_worker.h"

/* Self-wake interval for the idle worker thread. 10ms ~0.005% CPU overhead */
#define IW_WAKE_MS 10
_Static_assert(IW_WAKE_MS < 1000,
               "Use a while loop for nanosecond normalization if IW_WAKE_MS >= 1s");

/* iw_wait_release() phase 1: bounded cpu-relax spin iterations.
 * With per-removal iw_can_work checks the worker residual after a revoke is
 * ~one dir_remove_and_free (~1µs); 128 pause iterations (~5-13µs on modern
 * x86) cover it on multi-core without ever sleeping. */
#define IW_SPIN_ITERATIONS 128

/* iw_wait_release() phase 2: sleep quantum while waiting on a preempted or
 * single-core worker. 100µs bounds the single-core reclaim latency; an
 * empty spin here would burn a 1-10ms scheduler quantum starving the
 * worker it waits on. */
#define IW_WAIT_SLEEP_NS 100000L

#if defined(__x86_64__) || defined(__i386__)
#define IW_CPU_RELAX() __builtin_ia32_pause()
#elif defined(__aarch64__) || defined(__arm__)
#define IW_CPU_RELAX() __asm__ __volatile__("yield" ::: "memory")
#else
#define IW_CPU_RELAX() __asm__ __volatile__("" ::: "memory")
#endif

/*
 * Handshake flags — seq_cst everywhere (C11 default). Both sides store one
 * flag then load the other (StoreLoad); acquire/release does not order that
 * and races on store-buffer forwarding.
 *   iw_has_work   set by main at every enqueue (iw_note_work), cleared by
 *                 the worker only on full drain, before iw_is_working
 *   iw_can_work   main only: granted while blocked in poll(), then revoked
 *   iw_is_working worker only: published before re-checking iw_can_work
 *                 (Dekker store-then-check), cleared on abort/interrupt/done
 */
static atomic_int iw_has_work = 0;
static atomic_int iw_can_work = 0;
static atomic_int iw_is_working = 0;
static atomic_int iw_shutdown_flag = 0;

static pthread_t iw_tid;
static int       iw_started = 0;

/* Main-thread-only: whether the current poll() cycle granted iw_can_work.
 * Lets iw_revoke() skip all atomics when no grant was issued. */
static int iw_granted = 0;

/* Cumulative statistics. Relaxed atomics: written by both threads, read by
 * iw_log_stats() while the worker may still be alive; counters carry no
 * synchronization role. */
static struct {
    /* iw_note_work() calls (enqueues) */
    atomic_uint work_noted;
    /* poll cycles that granted */
    atomic_uint grants;
    /* validated grants acted on */
    atomic_uint cycles_started;
    /* full drains (iw_has_work cleared) */
    atomic_uint cycles_completed;
    /* revoked mid-drain */
    atomic_uint cycles_interrupted;
    /* Dekker abort before touching data */
    atomic_uint cycles_aborted;
    /* invalid queue entries freed */
    atomic_uint invalid_freed;
    /* dircache_process_deferred_chain calls */
    atomic_uint chains_processed;
} iw_stat;

#define IW_STAT_INC(field) \
    atomic_fetch_add_explicit(&iw_stat.field, 1, memory_order_relaxed)

/* Compile-time check for lock-free atomics.
 * iw_revoke_signal_safe() may run in signal-handler context where atomic
 * operations must be lock-free (no hidden mutex). On platforms where
 * ATOMIC_INT_LOCK_FREE != 2 the idle worker is disabled entirely and all
 * producers/consumers take the synchronous fallback paths. */
#if ATOMIC_INT_LOCK_FREE != 2

int iw_init(void)
{
    LOG(log_warning, logtype_afpd,
        "iw_init: lock-free atomics unavailable, using synchronous fallback");
    return -1;
}

void iw_grant(void) { }
void iw_revoke(void) { }
void iw_revoke_signal_safe(void) { }
void iw_shutdown(void) { }
void iw_note_work(void) { }
int  iw_grant_active(void)
{
    return 0;
}
int  iw_is_active(void)
{
    return 0;
}
void iw_log_stats(void) { }

#else /* ATOMIC_INT_LOCK_FREE == 2 */

/*!
 * @brief Work-pending predicate.
 *
 * Only valid on the worker thread while iw_is_working==1: inside that
 * window main is either blocked in poll() or waiting in iw_wait_release(),
 * so the non-atomic queue reads cannot race with producer writes.
 */
static int iw_work_pending(void)
{
    return (invalid_dircache_entries &&
            invalid_dircache_entries->next != invalid_dircache_entries)
           || dircache_has_deferred_work();
}

/*!
 * @brief Worker thread main loop.
 *
 * Single tier: sleep IW_WAKE_MS, then act only on a granted window.
 * iw_can_work==1 implies work exists (main grants only with iw_has_work
 * set), so no discovery pass is needed.
 *
 * Dekker start: iw_is_working is published BEFORE the re-check of
 * iw_can_work. Either this thread sees the revoke and aborts before
 * touching shared data, or main sees iw_is_working==1 and waits in
 * iw_wait_release() — seq_cst guarantees at least one of the two.
 */
static void *iw_main(void *arg)
{
    (void)arg;
    /* Block all signals — worker thread must not receive process signals */
    sigset_t sigs;
    sigfillset(&sigs);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);
    const struct timespec sleep_ts = {
        .tv_sec = 0,
        .tv_nsec = IW_WAKE_MS * 1000000L
    };

    while (!atomic_load(&iw_shutdown_flag)) {
        /* nanosleep uses hrtimer internally; the thread is off-CPU.
         * EINTR is harmless: signals are blocked, an early wake just
         * re-checks and sleeps again. */
        nanosleep(&sleep_ts, NULL);

        if (!atomic_load(&iw_can_work)) {
            continue;
        }

        atomic_store(&iw_is_working, 1);

        if (!atomic_load(&iw_can_work)) {
            /* Revoked between our load and our publish — abort having
             * touched nothing. */
            atomic_store(&iw_is_working, 0);
            IW_STAT_INC(cycles_aborted);
            continue;
        }

        IW_STAT_INC(cycles_started);

        /* Grant validated: main is in poll(). Process jobs in priority
         * order, re-checking iw_can_work per unit of work.
         * Priority 1: free entries queued by dir_remove().
         * Priority 2: deferred dircache_remove_children chain scans
         *             (internally re-checks iw_can_work per removal). */
        while (atomic_load(&iw_can_work)) {
            struct dir *dir = (struct dir *)dequeue(invalid_dircache_entries);

            if (!dir) {
                break;
            }

            dir_free(dir);
            IW_STAT_INC(invalid_freed);
        }

        while (atomic_load(&iw_can_work) && dircache_has_deferred_work()) {
            dircache_process_deferred_chain();
            IW_STAT_INC(chains_processed);
        }

        if (iw_work_pending()) {
            /* Revoked mid-drain. iw_has_work stays 1 (sticky) so main
             * re-grants on a later poll cycle. */
            IW_STAT_INC(cycles_interrupted);
        } else {
            /* Full drain: clear iw_has_work BEFORE releasing — main
             * cannot produce until it observes iw_is_working==0, so this
             * store can never clobber a fresh iw_has_work=1. */
            atomic_store(&iw_has_work, 0);
            IW_STAT_INC(cycles_completed);
        }

        atomic_store(&iw_is_working, 0);
    }

    return NULL;
}

/*!
 * @brief Wait until the worker has released the shared-data window.
 *
 * Phase 0: single load — the expected case (worker finished during the
 *          poll block, or aborted) returns immediately, zero spin.
 * Phase 1: bounded spin with cpu-relax. Worker residual after a revoke is
 *          ~one removal (~1µs) thanks to per-unit checks, so on
 *          multi-core this phase almost always completes.
 * Phase 2: 100µs nanosleeps. On single-core the first sleep yields the CPU
 *          so the (preempted) worker can run, observe the revoke within one
 *          removal, and release. An empty spin here would burn a 1-10ms
 *          scheduler quantum starving the very worker it waits on.
 *          nanosleep EINTR (SIGALRM tickles etc.) is harmless: the loop
 *          re-checks and handlers only set flags.
 */
static void iw_wait_release(void)
{
    if (!atomic_load(&iw_is_working)) {
        return;
    }

    for (int i = 0; i < IW_SPIN_ITERATIONS; i++) {
        IW_CPU_RELAX();

        if (!atomic_load(&iw_is_working)) {
            return;
        }
    }

    const struct timespec sleep_ts = {
        .tv_sec = 0,
        .tv_nsec = IW_WAIT_SLEEP_NS
    };

    while (atomic_load(&iw_is_working)) {
        nanosleep(&sleep_ts, NULL);
    }
}

/*!
 * @brief Initialize the idle worker thread.
 *
 * Must be called once during session init, after dircache_init().
 * Must be called in the afpd child session process (post-fork).
 * The child process does not fork again.
 *
 * Returns -1 (triggering synchronous fallback) if pthread_create() fails.
 */
int iw_init(void)
{
    if (pthread_create(&iw_tid, NULL, iw_main, NULL) != 0) {
        LOG(log_error, logtype_afpd,
            "iw_init: pthread_create failed: %s", strerror(errno));
        return -1;
    }

    iw_started = 1;
    LOG(log_info, logtype_afpd, "iw_init: worker thread started");
    return 0;
}

/*!
 * @brief Producer hook: note that idle work was enqueued.
 *
 * MUST be called on the main thread only, immediately after enqueueing to
 * any idle-work queue, while the worker is dormant — which is every
 * AFP-command / cache-hint context, since those run outside the grant
 * window by construction.
 */
void iw_note_work(void)
{
    if (!iw_started) {
        return;
    }

    atomic_store(&iw_has_work, 1);
    IW_STAT_INC(work_noted);
}

/*!
 * @brief Conditionally grant the worker the shared-data window.
 *
 * Called immediately before blocking in poll(). Grants only if work is
 * pending: with no work, no grant is issued and the paired
 * iw_revoke() is a single plain-int check — the no-work hot path performs
 * one atomic load here and nothing after poll().
 * Main-loop context only (writes the plain-int iw_granted and stats).
 */
void iw_grant(void)
{
    if (!iw_started) {
        return;
    }

    if (!atomic_load(&iw_has_work)) {
        return;
    }

    atomic_store(&iw_can_work, 1);
    iw_granted = 1;
    IW_STAT_INC(grants);
}

/*!
 * @brief Revoke the grant and reclaim exclusive access after poll() returns.
 *
 * MUST be called before ANY shared-data access after poll() (including the
 * EINTR path). Not async-signal-safe (may nanosleep) — signal-context
 * paths use iw_revoke_signal_safe().
 *
 * Dekker: the iw_can_work=0 store precedes the iw_is_working load
 * inside iw_wait_release(); seq_cst makes the crossing with the worker's
 * publish-then-recheck safe.
 */
void iw_revoke(void)
{
    if (!iw_started || !iw_granted) {
        /* No grant this cycle: iw_is_working is provably 0 (worker can only
         * publish after observing iw_can_work==1, which was never set).
         * Zero atomics on this path. */
        return;
    }

    atomic_store(&iw_can_work, 0);
    iw_wait_release();
    iw_granted = 0;
}

/*!
 * @brief Signal-safe revoke for exit paths.
 *
 * Single lock-free store; does NOT wait for release. Safe because every
 * caller reaches exit() without returning to AFP command processing, and
 * at every call site the grant was already revoked by the main loop (the
 * store is belt-and-braces against future signal-context close paths).
 * Does not touch iw_granted (not async-signal-safe to reason about; the
 * process is exiting).
 */
void iw_revoke_signal_safe(void)
{
    if (!iw_started) {
        return;
    }

    atomic_store(&iw_can_work, 0);
}

/*!
 * @brief Shut down the idle worker thread, ABANDONING all queued work.
 *
 * The session is ending and every caller exits immediately afterwards
 * — outstanding dircache cleanup is redundant: the whole cache dies
 * with the process, and the OS reclaims queue memory at exit(). No drain
 * is performed.
 *
 * Join is bounded: all call sites run after the main loop revoked the
 * grant, so the worker is dormant in its 10ms tick — it observes
 * iw_shutdown_flag within IW_WAKE_MS and exits.
 *
 * WARNING: not signal-context-safe (pthread_join); callers are main-loop
 * paths only. Never call on a path that continues serving AFP commands.
 */
void iw_shutdown(void)
{
    if (!iw_started) {
        return;
    }

    atomic_store(&iw_can_work, 0);
    atomic_store(&iw_shutdown_flag, 1);
    pthread_join(iw_tid, NULL);
    /* Log stats while iw_started is still 1 */
    iw_log_stats();
    iw_started = 0;
    /* Queued work intentionally abandoned — caller exits immediately. */
}

/*!
 * @brief Revoke-check accessor for worker-side drain code outside this file
 *        (dircache_process_deferred_chain per-removal check).
 */
int iw_grant_active(void)
{
    return atomic_load(&iw_can_work);
}

/*!
 * @brief True when the worker thread is running (synchronous fallbacks
 *        apply when it is not).
 */
int iw_is_active(void)
{
    return iw_started;
}

/*!
 * @brief Log idle worker statistics.
 *
 * Called from session close (afp_dsi_close) and iw_shutdown(). Relaxed
 * reads; the worker may still be alive on die paths (no join there).
 */
void iw_log_stats(void)
{
    if (!iw_started) {
        return;
    }

    LOG(log_info, logtype_afpd,
        "iw stats: noted=%u grants=%u cycles=%u completed=%u "
        "interrupted=%u aborted=%u invalid_freed=%u chains=%u",
        atomic_load_explicit(&iw_stat.work_noted, memory_order_relaxed),
        atomic_load_explicit(&iw_stat.grants, memory_order_relaxed),
        atomic_load_explicit(&iw_stat.cycles_started, memory_order_relaxed),
        atomic_load_explicit(&iw_stat.cycles_completed, memory_order_relaxed),
        atomic_load_explicit(&iw_stat.cycles_interrupted, memory_order_relaxed),
        atomic_load_explicit(&iw_stat.cycles_aborted, memory_order_relaxed),
        atomic_load_explicit(&iw_stat.invalid_freed, memory_order_relaxed),
        atomic_load_explicit(&iw_stat.chains_processed, memory_order_relaxed));
}

#endif /* ATOMIC_INT_LOCK_FREE */
