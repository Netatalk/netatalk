/*
 * Copyright (c) 2026 Andy Lemin (@andylemin)
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

#include <atalk/logger.h>
#include <atalk/queue.h>

#include "dircache.h"
#include "directory.h"
#include "idle_worker.h"

/* Atomic coordination flags.
 * Using default memory_order_seq_cst for all atomic operations. Could use
 * acquire/release for ~25ns savings on ARM, but seq_cst is simpler and the
 * overhead is negligible relative to poll() syscall cost (~1-5μs). */
static atomic_int is_idle = 0;
static atomic_int bg_running = 0;

/* Condvar for sleep/wake — mutex only held during condvar wait */
static pthread_mutex_t sleep_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  sleep_cond  = PTHREAD_COND_INITIALIZER;
static atomic_int      shutdown_flag = 0;

static pthread_t worker_tid;
static int       worker_started = 0;

/* Worker thread metrics */
static struct {
    unsigned int cycles_started;      /* idle cycles entered */
    unsigned int cycles_completed;    /* idle cycles that finished all work */
    unsigned int cycles_interrupted;  /* idle cycles cut short by poll() return */
} worker_stat;

/*!
 * @brief Check if any idle work is pending (work-availability predicate).
 *
 * This function serves two purposes:
 * 1. POSIX spurious wakeup guard: the is_idle check ensures the worker
 *    does not start work if woken spuriously when the main thread is NOT
 *    in poll(). Also prevents work during the window between
 *    idle_worker_stop() and the next idle_worker_start().
 * 2. Work-availability predicate: returns true only if there is actual
 *    work to perform. If new work types are added to the idle worker
 *    (e.g., FCE event dispatch), their pending-work check MUST be added
 *    here — otherwise the worker will sleep through available work.
 */
static int idle_worker_has_work(void)
{
    if (!atomic_load(&is_idle)) {
        return 0;
    }

    /* Non-atomic read of queue sentinel pointers is safe here because:
     * 1. We hold sleep_mutex (provides memory barrier)
     * 2. is_idle==1 means main thread is in poll() (cannot modify queue)
     * 3. Only the condvar wait loop calls this, so mutex is always held */
    return (invalid_dircache_entries &&
            invalid_dircache_entries->next != invalid_dircache_entries)
           || dircache_has_deferred_work();
}

static void *idle_worker_main(void *arg)
{
    (void)arg;
    /* Block all Signals — worker thread must not receive process signals */
    sigset_t sigs;
    sigfillset(&sigs);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);
    pthread_mutex_lock(&sleep_mutex);

    while (!atomic_load(&shutdown_flag)) {
        /* Sleep until work exists AND main thread is idle */
        while (!atomic_load(&shutdown_flag) &&
                !idle_worker_has_work()) {
            pthread_cond_wait(&sleep_cond, &sleep_mutex);
        }

        if (atomic_load(&shutdown_flag)) {
            break;
        }

        /* Signal main thread we are active before releasing mutex.
         * Ordering is critical, idle_worker_stop() spins on bg_running. */
        atomic_store(&bg_running, 1);
        /* Release sleep mutex — not needed during work */
        pthread_mutex_unlock(&sleep_mutex);
        worker_stat.cycles_started++;

        /* Process jobs in priority order, checking is_idle per unit of work.
         * Priority 1: Free entries queued by AFP command handlers.
         * Priority 2: Deferred dircache_remove_children scans and cleaning. */

        /* Free invalidated dircache entries
         * queued by main thread's dir_remove() during AFP commands */
        while (atomic_load(&is_idle)) {
            struct dir *dir = (struct dir *)dequeue(invalid_dircache_entries);

            if (!dir) {
                break;
            }

            dir_free(dir);
        }

        /* Clean stale dircache entries — includes ARC ghost entries */
        while (atomic_load(&is_idle) && dircache_has_deferred_work()) {
            /* Process one hash chain per iteration */
            dircache_process_deferred_chain();
        }

        if (!atomic_load(&is_idle)) {
            worker_stat.cycles_interrupted++;
        } else {
            worker_stat.cycles_completed++;
        }

        /* Done or interrupted — signal dormant */
        atomic_store(&bg_running, 0);
        /* Re-acquire sleep mutex for condvar */
        pthread_mutex_lock(&sleep_mutex);
    }

    pthread_mutex_unlock(&sleep_mutex);
    return NULL;
}

/* Compile-time check for lock-free atomics — required for signal safety.
 * idle_worker_stop_signal_safe() is called from signal handler context where
 * atomic operations must be lock-free (no hidden mutex).
 * On exotic platforms (old MIPS, some RISC-V) where ATOMIC_INT_LOCK_FREE != 2,
 * the idle worker is disabled entirely at compile time. */
#if ATOMIC_INT_LOCK_FREE != 2

int idle_worker_init(void)
{
    LOG(log_warning, logtype_afpd,
        "idle_worker_init: lock-free atomics unavailable, "
        "using synchronous fallback");
    return -1;
}

void idle_worker_start(void) { }
void idle_worker_stop(void) { }
void idle_worker_stop_signal_safe(void) { }
void idle_worker_shutdown(void) { }
int  idle_worker_is_active(void)
{
    return 0;
}
void idle_worker_log_stats(void) { }

#else /* ATOMIC_INT_LOCK_FREE == 2 */

/*!
 * @brief Initialize the idle worker thread.
 *
 * Must be called once during session init, after dircache_init().
 * Must be called in the afpd child session process (post-fork).
 * The child process does not fork again.
 *
 * Returns -1 (triggering synchronous fallback) if:
 * - pthread_create() fails
 */
int idle_worker_init(void)
{
    if (pthread_create(&worker_tid, NULL, idle_worker_main, NULL) != 0) {
        LOG(log_error, logtype_afpd,
            "idle_worker_init: pthread_create failed: %s", strerror(errno));
        return -1;
    }

    worker_started = 1;
    LOG(log_info, logtype_afpd, "idle_worker_init: worker thread started");
    return 0;
}

/*!
 * @brief Signal the idle worker that the main thread is about to enter poll().
 *
 * Sets is_idle=1 and wakes the worker via condvar signal.
 * MUST be called with SIGURG/SIGALRM/SIGTERM/SIGQUIT blocked by the caller
 * (the DSI loop blocks these signals around the start/poll/stop sequence).
 * This prevents siglongjmp() or afp_dsi_die() from skipping
 * pthread_mutex_unlock(), which would leave the mutex in a corrupt state.
 */
void idle_worker_start(void)
{
    if (!worker_started) {
        return;
    }

    atomic_store(&is_idle, 1);

    /* Only signal condvar if work is pending — avoids futex/try_to_wake_up
     * on every poll(). Non-atomic queue check is safe here: work enqueued
     * by main thread during AFP command processing (before this point).
     * If no work exists, worker stays sleeping */
    if ((invalid_dircache_entries &&
            invalid_dircache_entries->next != invalid_dircache_entries) ||
            dircache_has_deferred_work()) {
        pthread_mutex_lock(&sleep_mutex);
        pthread_cond_signal(&sleep_cond);
        pthread_mutex_unlock(&sleep_mutex);
    }
}

/*!
 * @brief Reclaim exclusive access from the idle worker after poll() returns.
 *
 * Sets is_idle=0 and spins until the worker finishes its current unit of
 * work (bg_running==0). On multi-core systems this completes in nanoseconds.
 * On single-core systems, the OS scheduler will preempt the spinning main
 * thread within one time quantum (~1-10ms), allowing the worker to observe
 * is_idle==0 and clear bg_running.
 *
 * NOT async-signal-safe due to the spin loop — use idle_worker_stop_signal_safe()
 * from signal handler context instead.
 */
void idle_worker_stop(void)
{
    if (!worker_started) {
        return;
    }

    atomic_store(&is_idle, 0);

    /* Spin until worker finishes current unit of work.
     * No sched_yield() — not async-signal-safe on all platforms, and the
     * OS preemptive scheduler handles single-core fairness adequately.
     * On multi-core: completes in nanoseconds (worker checks is_idle per chain).
     * On single-core: OS preempts within one scheduler quantum (~1-10ms). */
    while (atomic_load(&bg_running)) {
        /* empty spin */
    }
}

/*!
 * @brief Signal-safe variant of idle_worker_stop() for signal handler context.
 *
 * Sets is_idle=0 but does NOT spin on bg_running. This is safe because all
 * code paths that call this function (via afp_dsi_die() → afp_dsi_close())
 * end in exit(), which terminates the worker thread via process exit.
 * The worker will observe is_idle==0 at its next check and stop modifying
 * shared data, but we do not wait for that — exit() handles cleanup.
 *
 * Uses only atomic_store() which is async-signal-safe when
 * ATOMIC_INT_LOCK_FREE==2 (verified at compile time).
 */
void idle_worker_stop_signal_safe(void)
{
    if (!worker_started) {
        return;
    }

    atomic_store(&is_idle, 0);
    /* No spin — caller ends in exit(). Worker thread terminated by kernel. */
}

/*!
 * @brief Shut down the idle worker thread cleanly.
 *
 * Sets shutdown flag, signals condvar, joins thread.
 * WARNING: Must NOT be called from signal handler context —
 * pthread_mutex_lock/pthread_cond_signal/pthread_join are
 * async-signal-unsafe. Use idle_worker_stop_signal_safe() in signal
 * handlers instead.
 */
void idle_worker_shutdown(void)
{
    if (!worker_started) {
        return;
    }

    atomic_store(&shutdown_flag, 1);
    /* shutdown_flag=1 alone exits the condvar while loop (!shutdown_flag → false).
     * No need to set is_idle=1 — the outer while catches shutdown first. */
    pthread_mutex_lock(&sleep_mutex);
    pthread_cond_signal(&sleep_cond);
    pthread_mutex_unlock(&sleep_mutex);
    pthread_join(worker_tid, NULL);
    worker_started = 0;
    /* Drain any entries added after the worker's last idle cycle */
    dir_free_invalid_q();
}

int idle_worker_is_active(void)
{
    return worker_started;
}

/*!
 * @brief Log idle worker statistics.
 *
 * Called from session close (afp_dsi_close).
 */
void idle_worker_log_stats(void)
{
    if (!worker_started) {
        return;
    }

    LOG(log_info, logtype_afpd,
        "idle_worker stats: cycles=%u completed=%u interrupted=%u",
        worker_stat.cycles_started,
        worker_stat.cycles_completed,
        worker_stat.cycles_interrupted);
}

#endif /* ATOMIC_INT_LOCK_FREE */
