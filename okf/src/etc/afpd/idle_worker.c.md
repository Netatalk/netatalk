---
type: C Source File
title: "etc/afpd/idle_worker.c"
description: "Idle worker thread that runs deferred directory cache work between requests."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/idle_worker.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/queue.h](../../include/atalk/queue.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [idle_worker.h](idle_worker.h.md)
* System headers: `errno.h`, `pthread.h`, `signal.h`, `stdatomic.h`, `string.h`, `time.h`

# Functions

### iw_work_pending

```c
static int iw_work_pending(void)
```

Defined at lines 153 to 158.

Tell whether work for the idle worker is still in its queues.

Only the worker thread calls it, while iw_is_working is 1. The main thread then blocks in poll or waits in iw_wait_release, so the plain queue reads cannot race with a producer.

Returns: nonzero while invalid dircache entries or deferred chain scans remain

Calls: [dircache_has_deferred_work](dircache.c.md#dircache_has_deferred_work)

Called by: [iw_main](idle_worker.c.md#iw_main)

Uses file-scope variables: `invalid_dircache_entries` in [etc/afpd/directory.c](directory.c.md)

### iw_main

```c
static void * iw_main(void *arg)
```

Defined at lines 180 to 257.

Run the idle worker thread.

The thread blocks the asynchronous signals, which belong to the main thread, and keeps SIGSEGV and SIGBUS deliverable for the fault handler. It wakes every IW_WAKE_MS ms and works only inside a granted window. A grant implies queued work, so the thread needs no discovery pass.

The start follows Dekker: the thread publishes iw_is_working before it checks iw_can_work again. Either it sees a revoke and aborts before it touches shared data, or the main thread sees iw_is_working and waits in iw_wait_release. Sequential consistency guarantees one of the two.

The thread frees the invalid dircache entries first and then runs the deferred chain scans, checking iw_can_work before each unit. Only a full drain clears iw_has_work.

Parameters:
* `arg`: unused

Returns: NULL once iw_shutdown sets the shutdown flag

Calls: [dequeue](../../libatalk/util/queue.c.md#dequeue), [dir_free](directory.c.md#dir_free), [dircache_has_deferred_work](dircache.c.md#dircache_has_deferred_work), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain), [fault_setup_thread](../../libatalk/util/fault.c.md#fault_setup_thread), [iw_work_pending](idle_worker.c.md#iw_work_pending)

Called by: [iw_init](idle_worker.c.md#iw_init)

Uses file-scope variables: `chains_processed`, `cycles_aborted`, `cycles_completed`, `cycles_interrupted`, `cycles_started`, `invalid_dircache_entries` in [etc/afpd/directory.c](directory.c.md), `invalid_freed`, `iw_can_work`, `iw_has_work`, `iw_is_working`, `iw_shutdown_flag`

### iw_wait_release

```c
static void iw_wait_release(void)
```

Defined at lines 274 to 296.

Wait until the worker has released the shared-data window.

Phase 0: single load — the expected case (worker finished during the poll block, or aborted) returns immediately, zero spin. Phase 1: bounded spin with cpu-relax. Worker residual after a revoke is ~one removal (~1µs) thanks to per-unit checks, so on multi-core this phase almost always completes. Phase 2: 100µs nanosleeps. On single-core the first sleep yields the CPU so the (preempted) worker can run, observe the revoke within one removal, and release. An empty spin here would burn a 1-10ms scheduler quantum starving the very worker it waits on. nanosleep EINTR (SIGALRM tickles etc.) is harmless: the loop re-checks and handlers only set flags.

Called by: [iw_revoke](idle_worker.c.md#iw_revoke)

Uses file-scope variables: `iw_is_working`

Mentioned in the documentation of: [iw_revoke](idle_worker.c.md#iw_revoke)

### iw_init

```c
int iw_init(void)
```

Defined at lines 307 to 318.

Start the idle worker thread of a session.

The session child calls it once after dircache_init and never forks again. A failed start leaves the synchronous fallback paths in use. A build without lock-free int atomics has no worker and always returns -1.

Returns: 0, or -1 when pthread_create fails

Calls: [iw_main](idle_worker.c.md#iw_main)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `iw_started`, `iw_tid`

### iw_note_work

```c
void iw_note_work(void)
```

Defined at lines 328 to 336.

Producer hook: note that idle work was enqueued.

MUST be called on the main thread only, immediately after enqueueing to any idle-work queue, while the worker is dormant — which is every AFP-command / cache-hint context, since those run outside the grant window by construction.

Called by: [dir_remove](directory.c.md#dir_remove), [dircache_defer_free](dircache.c.md#dircache_defer_free), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer)

Uses file-scope variables: `iw_has_work`, `iw_started`, `work_noted`

### iw_grant

```c
void iw_grant(void)
```

Defined at lines 347 to 360.

Conditionally grant the worker the shared-data window.

Called immediately before blocking in poll(). Grants only if work is pending: with no work, no grant is issued and the paired [iw_revoke()](idle_worker.c.md#iw_revoke) is a single plain-int check — the no-work hot path performs one atomic load here and nothing after poll(). Main-loop context only (writes the plain-int iw_granted and stats).

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `grants`, `iw_can_work`, `iw_granted`, `iw_has_work`, `iw_started`

### iw_revoke

```c
void iw_revoke(void)
```

Defined at lines 373 to 385.

Revoke the grant and reclaim exclusive access after poll() returns.

MUST be called before ANY shared-data access after poll() (including the EINTR path). Not async-signal-safe (may nanosleep) — signal-context paths use [iw_revoke_signal_safe()](idle_worker.c.md#iw_revoke_signal_safe).

Dekker: the iw_can_work=0 store precedes the iw_is_working load inside [iw_wait_release()](idle_worker.c.md#iw_wait_release); seq_cst makes the crossing with the worker's publish-then-recheck safe.

Calls: [iw_wait_release](idle_worker.c.md#iw_wait_release)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

Uses file-scope variables: `iw_can_work`, `iw_granted`, `iw_started`

Mentioned in the documentation of: [iw_grant](idle_worker.c.md#iw_grant)

### iw_revoke_signal_safe

```c
void iw_revoke_signal_safe(void)
```

Defined at lines 397 to 404.

Signal-safe revoke for exit paths.

Single lock-free store; does NOT wait for release. Safe because every caller reaches exit() without returning to AFP command processing, and at every call site the grant was already revoked by the main loop (the store is belt-and-braces against future signal-context close paths). Does not touch iw_granted (not async-signal-safe to reason about; the process is exiting).

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

Uses file-scope variables: `iw_can_work`, `iw_started`

Mentioned in the documentation of: [iw_revoke](idle_worker.c.md#iw_revoke)

### iw_shutdown

```c
void iw_shutdown(void)
```

Defined at lines 421 to 434.

Shut down the idle worker thread, ABANDONING all queued work.

The session is ending and every caller exits immediately afterwards — outstanding dircache cleanup is redundant: the whole cache dies with the process, and the OS reclaims queue memory at exit(). No drain is performed.

Join is bounded: all call sites run after the main loop revoked the grant, so the worker is dormant in its 10ms tick — it observes iw_shutdown_flag within IW_WAKE_MS and exits.

WARNING: not signal-context-safe (pthread_join); callers are main-loop paths only. Never call on a path that continues serving AFP commands.

Calls: [iw_log_stats](idle_worker.c.md#iw_log_stats)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

Uses file-scope variables: `iw_can_work`, `iw_shutdown_flag`, `iw_started`, `iw_tid`

Mentioned in the documentation of: [iw_log_stats](idle_worker.c.md#iw_log_stats)

### iw_grant_active

```c
int iw_grant_active(void)
```

Defined at lines 443 to 446.

Tell worker-side code outside this file whether the grant still holds.

The deferred chain scan of the dircache checks it before each removal.

Returns: nonzero while the main thread grants the window

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `iw_can_work`

### iw_is_active

```c
int iw_is_active(void)
```

Defined at lines 455 to 458.

Tell whether the idle worker thread runs.

Without the thread the callers take their synchronous fallback paths.

Returns: nonzero after a successful iw_init and before iw_shutdown ends

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer)

Uses file-scope variables: `iw_started`

### iw_log_stats

```c
void iw_log_stats(void)
```

Defined at lines 466 to 483.

Log idle worker statistics.

Called from session close (afp_dsi_close) and [iw_shutdown()](idle_worker.c.md#iw_shutdown). Relaxed reads; the worker may still be alive on die paths (no join there).

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [iw_shutdown](idle_worker.c.md#iw_shutdown)

Uses file-scope variables: `iw_started`, `iw_stat`

# Macros

* Undocumented: `IW_CPU_RELAX`, `IW_SPIN_ITERATIONS`, `IW_STAT_INC`, `IW_WAIT_SLEEP_NS`, `IW_WAKE_MS`

# File-scope variables

`chains_processed`, `cycles_aborted`, `cycles_completed`, `cycles_interrupted`, `cycles_started`, `grants`, `invalid_freed`, `iw_can_work`, `iw_granted`, `iw_has_work`, `iw_is_working`, `iw_shutdown_flag`, `iw_started`, `iw_stat`, `iw_tid`, `work_noted`
