---
type: C Source File
title: "etc/spotlight/xapian/sl_xapian.c"
description: "Spotlight Xapian backend: index state, queries, and results."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/xapian/sl_xapian.c"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T06:55:24+02:00 }
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

Initialized at line 778 as `struct sl_backend_ops`. Assigns:

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

Defined at lines 78 to 104.

Create a state directory, or check an existing one, so that it carries every bit of mode.

An existing directory that lacks a bit of mode gets mode through chmod, and extra bits stay. A path that is not a directory fails.

Parameters:
* `path`: directory
* `mode`: mode bits the directory must carry
* `errbuf`: receives the error text
* `errlen`: size of errbuf

Returns: 0, or -1 with the reason in errbuf

Called by: [sl_xapian_ensure_state_root](sl_xapian.c.md#sl_xapian_ensure_state_root)

### sl_xapian_ensure_state_root

```c
static int sl_xapian_ensure_state_root(char *errbuf, size_t errlen)
```

Defined at lines 117 to 131.

Create the Spotlight state directories as root, spotlight with mode 0755 and spotlight/xapian with mode 01777.

The sticky, world-writable xapian directory lets sessions under any user create their entries there.

Parameters:
* `errbuf`: receives the error text
* `errlen`: size of errbuf

Returns: 0, or -1 with the reason in errbuf

Calls: [become_root](../../../libatalk/util/unix.c.md#become_root), [sl_xapian_mkdir_state](sl_xapian.c.md#sl_xapian_mkdir_state), [unbecome_root](../../../libatalk/util/unix.c.md#unbecome_root)

Called by: [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded), [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)

### cnid_comp_fn

```c
static int cnid_comp_fn(const void *p1, const void *p2)
```

Defined at lines 140 to 149.

Order two 64-bit host-order CNIDs, for bsearch over the client filter.

Parameters:
* `p1`: first CNID
* `p2`: second CNID

Returns: -1, 0, or 1

Called by: [sl_xapian_fill_results](sl_xapian.c.md#sl_xapian_fill_results)

### sl_xapian_seeded

```c
static bool sl_xapian_seeded(const struct vol *vol)
```

Defined at lines 157 to 166.

Tell whether this process has the Xapian index of a volume ready.

Parameters:
* `vol`: volume

Returns: true when the volume path is in the seeded list

Called by: [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded)

Uses file-scope variables: `seeded_volumes`

### sl_xapian_mark_seeded

```c
static int sl_xapian_mark_seeded(const struct vol *vol)
```

Defined at lines 175 to 194.

Add a volume to the seeded list, which holds the volumes whose Xapian index is ready.

Parameters:
* `vol`: volume

Returns: 0, or -1 when an allocation fails

Called by: [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded)

Uses file-scope variables: `seeded_volumes`

### sl_xapian_unmark_seeded

```c
static void sl_xapian_unmark_seeded(const struct vol *vol)
```

Defined at lines 201 to 221.

Remove a volume from the seeded list.

Parameters:
* `vol`: volume, or NULL

Called by: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event)

Uses file-scope variables: `seeded_volumes`

### sl_xapian_db_id_char_safe

```c
static bool sl_xapian_db_id_char_safe(unsigned char c)
```

Defined at lines 229 to 235.

Tell whether a byte may appear in a Xapian database ID.

Parameters:
* `c`: byte

Returns: true for an ASCII letter, a digit, '-', '_', or '.'

Called by: [sl_xapian_db_id_safe](sl_xapian.c.md#sl_xapian_db_id_safe)

### sl_xapian_fnv1a_update

```c
static uint64_t sl_xapian_fnv1a_update(uint64_t hash, const char *s)
```

Defined at lines 244 to 252.

Fold the bytes of a string into a 64-bit FNV-1a hash.

Parameters:
* `hash`: running hash
* `s`: string, or NULL to add nothing

Returns: the updated hash

Called by: [sl_xapian_db_id_hash](sl_xapian.c.md#sl_xapian_db_id_hash)

### sl_xapian_db_id_safe

```c
static bool sl_xapian_db_id_safe(const char *id)
```

Defined at lines 263 to 288.

Tell whether an ID can name a Xapian database directory as it is.

The ID needs 1 to SL_XAPIAN_DB_ID_MAX bytes that sl_xapian_db_id_char_safe accepts, and "." and ".." fail.

Parameters:
* `id`: candidate ID, or NULL

Returns: true for a safe ID

Calls: [sl_xapian_db_id_char_safe](sl_xapian.c.md#sl_xapian_db_id_char_safe), [strnlen](../../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_xapian_db_path](sl_xapian.c.md#sl_xapian_db_path)

### sl_xapian_db_id_hash

```c
static uint64_t sl_xapian_db_id_hash(const struct vol *vol, const char *id)
```

Defined at lines 299 to 305.

Hash an unsafe ID and the volume path into a 64-bit FNV-1a value.

A 0xff byte separates the ID from the path.

Parameters:
* `vol`: volume, or NULL
* `id`: ID

Returns: the hash

Calls: [sl_xapian_fnv1a_update](sl_xapian.c.md#sl_xapian_fnv1a_update)

Called by: [sl_xapian_db_path](sl_xapian.c.md#sl_xapian_db_path)

### sl_xapian_db_path

```c
static char * sl_xapian_db_path(TALLOC_CTX *mem_ctx, const struct vol *vol)
```

Defined at lines 319 to 341.

Return the path of the Xapian database of a volume under the Spotlight state directory.

The directory takes the volume UUID as its name, or the local name without one, or "unknown" without either. A name that sl_xapian_db_id_safe refuses becomes unsafe- and the hex of sl_xapian_db_id_hash.

Parameters:
* `mem_ctx`: talloc context for the path
* `vol`: volume

Returns: the path, or NULL when talloc fails

Calls: [sl_xapian_db_id_hash](sl_xapian.c.md#sl_xapian_db_id_hash), [sl_xapian_db_id_safe](sl_xapian.c.md#sl_xapian_db_id_safe)

Called by: [sl_xapian_index_event](sl_xapian.c.md#sl_xapian_index_event), [sl_xapian_open_query](sl_xapian.c.md#sl_xapian_open_query)

### sl_xapian_volume_uuid

```c
static const char * sl_xapian_volume_uuid(const struct vol *vol)
```

Defined at lines 350 to 365.

Return the volume UUID that the index metadata records, or the local name without one.

Parameters:
* `vol`: volume, or NULL

Returns: the UUID, the local name, or ""

Called by: [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded)

### sl_xapian_ensure_seeded

```c
static int sl_xapian_ensure_seeded(slq_t *slq, const char *db_path)
```

Defined at lines 381 to 434.

Make sure the Xapian index of the query volume is ready, once per process.

The call prepares the state directories and checks the index metadata against the volume path and UUID. An index that is not ready, or whose check fails, goes through sl_xapian_reconcile. Either way the volume then joins the seeded list.

Parameters:
* `slq`: query with the volume
* `db_path`: path of the Xapian database

Returns: 0, or -1 when the state directories, the reconcile, or an allocation fails

Calls: [sl_xapian_ensure_state_root](sl_xapian.c.md#sl_xapian_ensure_state_root), [sl_xapian_index_ready](sl_xapian.h.md#sl_xapian_index_ready), [sl_xapian_mark_seeded](sl_xapian.c.md#sl_xapian_mark_seeded), [sl_xapian_reconcile](sl_xapian.h.md#sl_xapian_reconcile), [sl_xapian_seeded](sl_xapian.c.md#sl_xapian_seeded), [sl_xapian_volume_uuid](sl_xapian.c.md#sl_xapian_volume_uuid)

Called by: [sl_xapian_open_query](sl_xapian.c.md#sl_xapian_open_query)

### sl_xapian_fill_results

```c
static int sl_xapian_fill_results(slq_t *slq)
```

Defined at lines 448 to 538.

Add up to SL_XAPIAN_PAGE_SIZE results from the candidate paths to the result page.

The call skips a path out of scope, a stale or unreadable file, a file without a CNID, and a CNID outside the client filter. A CNID reset during a lookup marks the volume reset and fails the query. Candidates left after a full page set SLQ_STATE_FULL, and none left set SLQ_STATE_DONE.

Parameters:
* `slq`: query with the result page and the candidate paths

Returns: 0, or -1 on error

Calls: [add_filemeta](../../afpd/spotlight.c.md#add_filemeta), [cnid_comp_fn](sl_xapian.c.md#cnid_comp_fn), [cnid_for_path](../../../libatalk/util/cnid.c.md#cnid_for_path), [cnid_volume_reset](../../afpd/volume.c.md#cnid_volume_reset), [sl_path_in_scope](../../afpd/spotlight.c.md#sl_path_in_scope)

Called by: [sl_xapian_fetch_results](sl_xapian.c.md#sl_xapian_fetch_results), [sl_xapian_open_query](sl_xapian.c.md#sl_xapian_open_query)

### sl_xapian_init

```c
static int sl_xapian_init(AFPObj *obj)
```

Defined at lines 546 to 557.

Start the Xapian backend, which only logs once per process.

Parameters:
* `obj`: AFP object, unused

Returns: 0

Called through [`sl_backend_ops::sbo_init`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_close

```c
static void sl_xapian_close(AFPObj *obj)
```

Defined at lines 564 to 576.

Stop the Xapian backend and empty the seeded list.

Parameters:
* `obj`: AFP object, unused

Uses file-scope variables: `seeded_volumes`

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_open_query

```c
static int sl_xapian_open_query(slq_t *slq)
```

Defined at lines 588 to 638.

Run a Spotlight query on the Xapian index of the volume.

The call seeds the index first when this process has not. The search returns candidate paths up to the result limit, and a truncated result logs a hint to raise the spotlight results limit. The first page fills at once.

Parameters:
* `slq`: query

Returns: 0, or -1 on error

Calls: [sl_xapian_db_path](sl_xapian.c.md#sl_xapian_db_path), [sl_xapian_ensure_seeded](sl_xapian.c.md#sl_xapian_ensure_seeded), [sl_xapian_fill_results](sl_xapian.c.md#sl_xapian_fill_results), [sl_xapian_query](sl_xapian.h.md#sl_xapian_query)

Called through [`sl_backend_ops::sbo_open_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_fetch_results

```c
static int sl_xapian_fetch_results(slq_t *slq)
```

Defined at lines 649 to 662.

Fill the next result page of a Xapian query.

A page that holds results already goes out first, since open_query fills the first one. A finished query adds nothing.

Parameters:
* `slq`: query

Returns: 0, or -1 without a result set or when the fill fails

Calls: [sl_xapian_fill_results](sl_xapian.c.md#sl_xapian_fill_results)

Called through [`sl_backend_ops::sbo_fetch_results`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_fetchQueryResultsForContext](../../afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_close_query

```c
static void sl_xapian_close_query(slq_t *slq)
```

Defined at lines 669 to 680.

Free the candidate paths and the backend state of a Xapian query.

Parameters:
* `slq`: query

Calls: [sl_xapian_free_paths](sl_xapian.h.md#sl_xapian_free_paths)

Called through [`sl_backend_ops::sbo_close_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [slq_destroy](../../afpd/spotlight_queries.c.md#slq_destroy)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

### sl_xapian_index_event

```c
static int sl_xapian_index_event(const AFPObj *obj, const struct vol *vol, sl_index_event_t event, const char *path, const char *oldpath)
```

Defined at lines 698 to 776.

Apply a file system change of a volume to its Xapian index.

Creates and modifications upsert the path, and a file delete drops it. A directory delete drops every path under it. A file move drops the old path and upserts the new one, and a directory move reindexes the subtree. A failed update marks the index dirty and drops the volume from the seeded list, so the next query reconciles the index.

Parameters:
* `obj`: AFP object, unused
* `vol`: volume
* `event`: SL_INDEX_* change
* `path`: new path
* `oldpath`: old path of a move, or NULL

Returns: 0, or -1 when the update fails

Calls: [sl_xapian_db_path](sl_xapian.c.md#sl_xapian_db_path), [sl_xapian_delete_path](sl_xapian.h.md#sl_xapian_delete_path), [sl_xapian_delete_prefix](sl_xapian.h.md#sl_xapian_delete_prefix), [sl_xapian_ensure_state_root](sl_xapian.c.md#sl_xapian_ensure_state_root), [sl_xapian_mark_dirty](sl_xapian.h.md#sl_xapian_mark_dirty), [sl_xapian_reindex_subtree](sl_xapian.h.md#sl_xapian_reindex_subtree), [sl_xapian_unmark_seeded](sl_xapian.c.md#sl_xapian_unmark_seeded), [sl_xapian_upsert_path](sl_xapian.h.md#sl_xapian_upsert_path)

Called through [`sl_backend_ops::sbo_index_event`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_index_event](../../afpd/spotlight.c.md#sl_index_event)

Dispatched via: [sl_xapian_ops](sl_xapian.c.md#sl_xapian_ops)

# Types

### struct sl_xapian_query

Defined at line 57.
* `char ** paths`
* `size_t count`
* `size_t pos`

### struct sl_xapian_seeded

Defined at line 52.
* `char * volpath`
* `struct sl_xapian_seeded * next`

# Macros

* Undocumented: `SL_XAPIAN_DB_ID_MAX`, `SL_XAPIAN_PAGE_SIZE`

# File-scope variables

`seeded_volumes`
