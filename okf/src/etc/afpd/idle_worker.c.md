---
type: C Source File
title: "etc/afpd/idle_worker.c"
description: "9 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/idle_worker.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

### iw_init

```c
int iw_init(void)
```

Defined at lines 113 to 118.

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

### iw_grant

```c
void iw_grant(void)
```

Defined at lines 120 to 120.

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

### iw_revoke

```c
void iw_revoke(void)
```

Defined at lines 121 to 121.

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

### iw_revoke_signal_safe

```c
void iw_revoke_signal_safe(void)
```

Defined at lines 122 to 122.

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

### iw_shutdown

```c
void iw_shutdown(void)
```

Defined at lines 123 to 123.

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

### iw_note_work

```c
void iw_note_work(void)
```

Defined at lines 124 to 124.

Called by: [dir_remove](directory.c.md#dir_remove), [dircache_defer_free](dircache.c.md#dircache_defer_free), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer)

### iw_grant_active

```c
int iw_grant_active(void)
```

Defined at lines 125 to 128.

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

### iw_is_active

```c
int iw_is_active(void)
```

Defined at lines 129 to 132.

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer)

### iw_log_stats

```c
void iw_log_stats(void)
```

Defined at lines 133 to 133.

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

# Macros

* Undocumented: `IW_CPU_RELAX`, `IW_SPIN_ITERATIONS`, `IW_STAT_INC`, `IW_WAIT_SLEEP_NS`, `IW_WAKE_MS`

# File-scope variables

`chains_processed`, `cycles_aborted`, `cycles_completed`, `cycles_interrupted`, `cycles_started`, `grants`, `invalid_freed`, `iw_can_work`, `iw_granted`, `iw_has_work`, `iw_is_working`, `iw_shutdown_flag`, `iw_started`, `iw_stat`, `iw_tid`, `work_noted`
