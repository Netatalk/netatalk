---
type: C Source File
title: "etc/spotlight/xapian/sl_xapian.c"
description: "20 functions, 2 types, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/xapian/sl_xapian.c"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/cnid.h](../../../include/atalk/cnid.h.md)
* [atalk/errchk.h](../../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../../include/atalk/globals.h.md)
* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [atalk/spotlight.h](../../../include/atalk/spotlight.h.md)
* [atalk/unix.h](../../../include/atalk/unix.h.md)
* [atalk/util.h](../../../include/atalk/util.h.md)
* [atalk/volume.h](../../../include/atalk/volume.h.md)
* [etc/afpd/volume.h](../../afpd/volume.h.md)
* [etc/spotlight/spotlight_private.h](../spotlight_private.h.md)
* [etc/spotlight/xapian/sl_xapian.h](sl_xapian.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `inttypes.h`, `stdbool.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `strings.h`, `sys/stat.h`, `talloc.h`, `unistd.h`

# Function tables

### sl_xapian_ops

Initialized at line 597 as `struct sl_backend_ops`. Assigns:

* `sbo_close`: [sl_xapian_close](sl_xapian.c.md#sl_xapian_close)
* `sbo_close_query`: [sl_xapian_close_query](sl_xapian.c.md#sl_xapian_close_query)
* `sbo_fetch_results`: [sl_xapian_fetch_results](sl_xapian.c.md#sl_xapian_fetch_results)
* `sbo_index_event`: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)
* `sbo_init`: [sl_xapian_init](sl_xapian.c.md#sl_xapian_init)
* `sbo_open_query`: [sl_xapian_open_query](sl_xapian.c.md#sl_xapian_open_query)

# Functions

### sl_xapian_mkdir_state

```c
static int sl_xapian_mkdir_state(const char *path, mode_t mode, char *errbuf, size_t errlen)
```

Defined at lines 60 to 86.

Called by: [sl_xapian_ensure_state_root](sl_xapian.c.md#sl_xapian_ensure_state_root)

### sl_xapian_ensure_state_root

```c
static int sl_xapian_ensure_state_root(char *errbuf, size_t errlen)
```

Defined at lines 88 to 102.

Calls: [become_root](../../../libatalk/util/unix.c.md#become_root), [sl_xapian_mkdir_state](sl_xapian.c.md#sl_xapian_mkdir_state), [unbecome_root](../../../libatalk/util/unix.c.md#unbecome_root)

Called by: [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded), [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)

### cnid_comp_fn

```c
static int cnid_comp_fn(const void *p1, const void *p2)
```

Defined at lines 104 to 113.

Called by: [sl_xapian_fill_results](sl_xapian.c.md#sl_xapian_fill_results)

### sl_xapian_seeded

```c
static bool sl_xapian_seeded(const struct vol *vol)
```

Defined at lines 115 to 124.

Uses file-scope variables: `seeded_volumes`

### sl_xapian_mark_seeded

```c
static int sl_xapian_mark_seeded(const struct vol *vol)
```

Defined at lines 126 to 145.

Called by: [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded)

Uses file-scope variables: `seeded_volumes`

### sl_xapian_unmark_seeded

```c
static void sl_xapian_unmark_seeded(const struct vol *vol)
```

Defined at lines 147 to 167.

Called by: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)

Uses file-scope variables: `seeded_volumes`

### sl_xapian_db_id_char_safe

```c
static bool sl_xapian_db_id_char_safe(unsigned char c)
```

Defined at lines 169 to 175.

Called by: [sl_xapian_db_id_safe](sl_xapian.c.md#sl_xapian_db_id_safe)

### sl_xapian_fnv1a_update

```c
static uint64_t sl_xapian_fnv1a_update(uint64_t hash, const char *s)
```

Defined at lines 177 to 185.

Called by: [sl_xapian_db_id_hash](sl_xapian.c.md#sl_xapian_db_id_hash)

### sl_xapian_db_id_safe

```c
static bool sl_xapian_db_id_safe(const char *id)
```

Defined at lines 187 to 212.

Calls: [sl_xapian_db_id_char_safe](sl_xapian.c.md#sl_xapian_db_id_char_safe), [strnlen](../../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_xapian_db_path](sl_xapian.c.md#sl_xapian_db_path)

### sl_xapian_db_id_hash

```c
static uint64_t sl_xapian_db_id_hash(const struct vol *vol, const char *id)
```

Defined at lines 214 to 220.

Calls: [sl_xapian_fnv1a_update](sl_xapian.c.md#sl_xapian_fnv1a_update)

Called by: [sl_xapian_db_path](sl_xapian.c.md#sl_xapian_db_path)

### sl_xapian_db_path

```c
static char * sl_xapian_db_path(TALLOC_CTX *mem_ctx, const struct vol *vol)
```

Defined at lines 222 to 244.

Calls: [sl_xapian_db_id_hash](sl_xapian.c.md#sl_xapian_db_id_hash), [sl_xapian_db_id_safe](sl_xapian.c.md#sl_xapian_db_id_safe)

Called by: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event), [sl_xapian_open_query](sl_xapian.c.md#sl_xapian_open_query)

### sl_xapian_volume_uuid

```c
static const char * sl_xapian_volume_uuid(const struct vol *vol)
```

Defined at lines 246 to 261.

Called by: [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded)

### sl_xapian_ensure_seeded

```c
static int sl_xapian_ensure_seeded(slq_t *slq, const char *db_path)
```

Defined at lines 263 to 316.

Calls: [sl_xapian_ensure_state_root](sl_xapian.c.md#sl_xapian_ensure_state_root), [sl_xapian_index_ready](sl_xapian.h.md#sl_xapian_index_ready), [sl_xapian_mark_seeded](sl_xapian.c.md#sl_xapian_mark_seeded), [sl_xapian_reconcile](sl_xapian.h.md#sl_xapian_reconcile), [sl_xapian_volume_uuid](sl_xapian.c.md#sl_xapian_volume_uuid)

Called by: [sl_xapian_open_query](sl_xapian.c.md#sl_xapian_open_query)

### sl_xapian_fill_results

```c
static int sl_xapian_fill_results(slq_t *slq)
```

Defined at lines 318 to 408.

Calls: [add_filemeta](../../afpd/spotlight.c.md#add_filemeta), [cnid_comp_fn](sl_xapian.c.md#cnid_comp_fn), [cnid_for_path](../../../libatalk/util/cnid.c.md#cnid_for_path), [cnid_volume_reset](../../afpd/volume.c.md#cnid_volume_reset), [sl_path_in_scope](../../afpd/spotlight.c.md#sl_path_in_scope)

Called by: [sl_xapian_fetch_results](sl_xapian.c.md#sl_xapian_fetch_results), [sl_xapian_open_query](sl_xapian.c.md#sl_xapian_open_query)

### sl_xapian_init

```c
static int sl_xapian_init(AFPObj *obj)
```

Defined at lines 410 to 421.

Called through [`sl_backend_ops::sbo_init`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_close

```c
static void sl_xapian_close(AFPObj *obj)
```

Defined at lines 423 to 435.

Uses file-scope variables: `seeded_volumes`

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_open_query

```c
static int sl_xapian_open_query(slq_t *slq)
```

Defined at lines 437 to 487.

Calls: [sl_xapian_db_path](sl_xapian.c.md#sl_xapian_db_path), [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded), [sl_xapian_fill_results](sl_xapian.c.md#sl_xapian_fill_results)

Called through [`sl_backend_ops::sbo_open_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_fetch_results

```c
static int sl_xapian_fetch_results(slq_t *slq)
```

Defined at lines 489 to 502.

Calls: [sl_xapian_fill_results](sl_xapian.c.md#sl_xapian_fill_results)

Called through [`sl_backend_ops::sbo_fetch_results`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_fetchQueryResultsForContext](../../afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_close_query

```c
static void sl_xapian_close_query(slq_t *slq)
```

Defined at lines 504 to 515.

Calls: [sl_xapian_free_paths](sl_xapian.h.md#sl_xapian_free_paths)

Called through [`sl_backend_ops::sbo_close_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [slq_cancelled_cleanup](../../afpd/spotlight.c.md#slq_cancelled_cleanup), [slq_destroy](../../afpd/spotlight.c.md#slq_destroy)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_index_event

```c
static int sl_xapian_index_event(const AFPObj *obj, const struct vol *vol, sl_index_event_t event, const char *path, const char *oldpath)
```

Defined at lines 517 to 595.

Calls: [sl_xapian_db_path](sl_xapian.c.md#sl_xapian_db_path), [sl_xapian_delete_path](sl_xapian.h.md#sl_xapian_delete_path), [sl_xapian_delete_prefix](sl_xapian.h.md#sl_xapian_delete_prefix), [sl_xapian_ensure_state_root](sl_xapian.c.md#sl_xapian_ensure_state_root), [sl_xapian_mark_dirty](sl_xapian.h.md#sl_xapian_mark_dirty), [sl_xapian_reindex_subtree](sl_xapian.h.md#sl_xapian_reindex_subtree), [sl_xapian_unmark_seeded](sl_xapian.c.md#sl_xapian_unmark_seeded), [sl_xapian_upsert_path](sl_xapian.h.md#sl_xapian_upsert_path)

Called through [`sl_backend_ops::sbo_index_event`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_index_event](../../afpd/spotlight.c.md#sl_index_event)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

# Types

### struct sl_xapian_query

Defined at line 52.
* `char ** paths`
* `size_t count`
* `size_t pos`

### struct sl_xapian_seeded

Defined at line 47.
* `char * volpath`
* `struct sl_xapian_seeded * next`

# Macros

* Undocumented: `SL_XAPIAN_DB_ID_MAX`, `SL_XAPIAN_PAGE_SIZE`

# File-scope variables

`seeded_volumes`
