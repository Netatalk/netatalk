---
type: C Source File
title: "etc/spotlight/cnid/sl_cnid.c"
description: "Spotlight CNID backend: file name searches in the CNID database."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/cnid/sl_cnid.c"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-11-22T14:39:48+01:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/cnid.h](../../../include/atalk/cnid.h.md)
* [atalk/directory.h](../../../include/atalk/directory.h.md)
* [atalk/errchk.h](../../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../../include/atalk/globals.h.md)
* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [atalk/sl_names.h](../../../include/atalk/sl_names.h.md)
* [atalk/spotlight.h](../../../include/atalk/spotlight.h.md)
* [atalk/util.h](../../../include/atalk/util.h.md)
* [atalk/volume.h](../../../include/atalk/volume.h.md)
* [etc/afpd/volume.h](../../afpd/volume.h.md)
* [etc/spotlight/spotlight_private.h](../spotlight_private.h.md)
* System headers: `ctype.h`, `inttypes.h`, `stdbool.h`, `stdint.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `talloc.h`, `unistd.h`

# Function tables

### sl_cnid_ops

Initialized at line 699 as `struct sl_backend_ops`. Assigns:

* `sbo_close`: [sl_cnid_close](sl_cnid.c.md#sl_cnid_close)
* `sbo_close_query`: [sl_cnid_close_query](sl_cnid.c.md#sl_cnid_close_query)
* `sbo_fetch_results`: [sl_cnid_fetch_results](sl_cnid.c.md#sl_cnid_fetch_results)
* `sbo_init`: [sl_cnid_init](sl_cnid.c.md#sl_cnid_init)
* `sbo_open_query`: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

# Functions

### cnid_comp_fn

```c
static int cnid_comp_fn(const void *p1, const void *p2)
```

Defined at lines 56 to 65.

Order two 64-bit host-order CNIDs, for bsearch over the client filter.

Parameters:
* `p1`: first CNID
* `p2`: second CNID

Returns: -1, 0, or 1

Called by: [sl_cnid_fill_results](sl_cnid.c.md#sl_cnid_fill_results)

### sl_cnid_query_free

```c
static int sl_cnid_query_free(struct sl_cnid_query *csq)
```

Defined at lines 152 to 156.

Free a query's name comparisons as talloc frees its state.

Parameters:
* `csq`: query state

Returns: 0, so talloc goes on to free the state

Calls: [sl_name_query_free](../../../libatalk/util/sl_names.c.md#sl_name_query_free)

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_dcache_get

```c
static const char * sl_dcache_get(struct sl_cnid_query *csq, cnid_t did)
```

Defined at lines 169 to 178.

Return the memoized path of a directory in the query cache.

Parameters:
* `csq`: query state
* `did`: directory CNID

Returns: the path, or NULL without an entry

Called by: [sl_cnid_to_path](sl_cnid.c.md#sl_cnid_to_path)

### sl_dcache_put

```c
static void sl_dcache_put(struct sl_cnid_query *csq, cnid_t did, const char *path)
```

Defined at lines 190 to 214.

Memoize a directory path for the rest of the query.

The cache allocates its SL_CNID_DCACHE_MAX entries on first use. A full cache or a failed allocation skips the entry.

Parameters:
* `csq`: query state
* `did`: directory CNID
* `path`: full path of the directory

Called by: [sl_cnid_to_path](sl_cnid.c.md#sl_cnid_to_path)

### sl_cnid_to_path

```c
static char * sl_cnid_to_path(TALLOC_CTX *mem_ctx, const struct vol *vol, struct sl_cnid_query *csq, cnid_t cnid)
```

Defined at lines 231 to 284.

Reconstruct the full filesystem path for a CNID.

Walks the DID chain upward via repeated [cnid_resolve()](../../../libatalk/cnid/cnid.c.md#cnid_resolve) calls, prepending path components, until reaching DIRDID_ROOT or a directory whose path is already memoized in the query's cache. Ancestor directory paths discovered along the way are memoized, so results sharing a directory resolve it once per query.

Parameters:
* `mem_ctx`: talloc context for the returned string
* `vol`: volume whose CNID database to query
* `csq`: per-query state carrying the directory memo
* `cnid`: network-byte-order CNID (as returned by cnid_find)

Returns: talloc-allocated full path, or NULL on error

Calls: [cnid_resolve](../../../libatalk/cnid/cnid.c.md#cnid_resolve), [sl_dcache_get](sl_cnid.c.md#sl_dcache_get), [sl_dcache_put](sl_cnid.c.md#sl_dcache_put)

Called by: [sl_cnid_fill_results](sl_cnid.c.md#sl_cnid_fill_results)

### sl_cnid_init

```c
static int sl_cnid_init(AFPObj *obj)
```

Defined at lines 296 to 300.

Start the CNID backend, which keeps no global state.

Parameters:
* `obj`: AFP object, unused

Returns: 0

Called through [`sl_backend_ops::sbo_init`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_close

```c
static void sl_cnid_close(AFPObj *obj)
```

Defined at lines 307 to 310.

Stop the CNID backend, which has nothing to release.

Parameters:
* `obj`: AFP object, unused

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_fill_results

```c
static int sl_cnid_fill_results(slq_t *slq)
```

Defined at lines 327 to 442.

Add up to SL_CNID_PAGE_SIZE results from the CNID buffer to the result page.

The call resolves each remaining CNID to a path. It skips a CNID outside the client filter, a path that does not resolve or lies out of scope, and a name the query's comparisons do not match. It also skips a file that fails stat or a read access check. A full page with CNIDs left sets SLQ_STATE_FULL, so the client polls for the next page. With no CNIDs left the call sets SLQ_STATE_DONE, and a failed add_filemeta sets SLQ_STATE_ERROR.

Parameters:
* `slq`: query with the result page and the backend state

Returns: 0, or -1 on error

Calls: [add_filemeta](../../afpd/spotlight.c.md#add_filemeta), [cnid_comp_fn](sl_cnid.c.md#cnid_comp_fn), [sl_cnid_to_path](sl_cnid.c.md#sl_cnid_to_path), [sl_name_query_match](../../../libatalk/util/sl_names.c.md#sl_name_query_match), [sl_path_in_scope](../../afpd/spotlight.c.md#sl_path_in_scope)

Called by: [sl_cnid_fetch_results](sl_cnid.c.md#sl_cnid_fetch_results), [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_cap_for

```c
static int sl_cnid_cap_for(uint64_t want)
```

Defined at lines 454 to 465.

Size the candidate buffer for the wanted results.

A limit under CNID_FIND_MIN_RESULTS still gets that much room, and the caller trims the surplus results afterwards. The capacity stops at SL_CNID_MAX_CAP.

Parameters:
* `want`: number of results wanted

Returns: the capacity in entries

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_scope_is_vol_root

```c
static bool sl_cnid_scope_is_vol_root(const char *scope, const char *vol_path)
```

Defined at lines 475 to 490.

Tell whether a search scope names the volume root, trailing slashes aside.

Parameters:
* `scope`: scope path of the query
* `vol_path`: path of the volume

Returns: true when both paths match without their trailing slashes

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_open_query

```c
static int sl_cnid_open_query(slq_t *slq)
```

Defined at lines 508 to 656.

Run a filename search in the CNID database for a Spotlight query.

A query without a name comparison, or with a scope that does not resolve, returns no results. A scope below the volume root restricts the search to its subtree inside the database. A query without a result limit starts with SL_CNID_START_RESULTS entries. While the database reports further matches, the search reruns on a buffer of SL_CNID_GROWTH times the size. A bounded query keeps up to its limit of results. The first page fills at once.

A scope lookup that trips a CNID reset marks the volume reset and fails the query.

Parameters:
* `slq`: query

Returns: 0, or -1 on error

Calls: [cnid_find_scoped](../../../libatalk/cnid/cnid.c.md#cnid_find_scoped), [cnid_for_path](../../../libatalk/util/cnid.c.md#cnid_for_path), [cnid_volume_reset](../../afpd/volume.c.md#cnid_volume_reset), [sl_cnid_cap_for](sl_cnid.c.md#sl_cnid_cap_for), [sl_cnid_fill_results](sl_cnid.c.md#sl_cnid_fill_results), [sl_cnid_query_free](sl_cnid.c.md#sl_cnid_query_free), [sl_cnid_scope_is_vol_root](sl_cnid.c.md#sl_cnid_scope_is_vol_root), [sl_name_query_free](../../../libatalk/util/sl_names.c.md#sl_name_query_free), [sl_name_query_parse_in](../../../libatalk/util/sl_names.c.md#sl_name_query_parse_in)

Called through [`sl_backend_ops::sbo_open_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_fetch_results

```c
static int sl_cnid_fetch_results(slq_t *slq)
```

Defined at lines 667 to 684.

Fill the next result page of a CNID query.

A page that holds results already goes out first, since open_query fills the first one. A finished query adds nothing.

Parameters:
* `slq`: query

Returns: 0, or -1 without a result set or when the fill fails

Calls: [sl_cnid_fill_results](sl_cnid.c.md#sl_cnid_fill_results)

Called through [`sl_backend_ops::sbo_fetch_results`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_fetchQueryResultsForContext](../../afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_close_query

```c
static void sl_cnid_close_query(slq_t *slq)
```

Defined at lines 691 to 697.

Free the backend state of a CNID query.

Parameters:
* `slq`: query

Called through [`sl_backend_ops::sbo_close_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [slq_destroy](../../afpd/spotlight_queries.c.md#slq_destroy)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

# Types

### struct sl_cnid_query

Defined at line 124.
* `cnid_t * cnids`
* `int cap`
* `int count`
* `int pos`
* `bool more_available`
* `struct sl_did_cache_ent * dcache`
* `int dcache_n`
* `struct sl_name_query * names`

### struct sl_did_cache_ent

Defined at line 112.
* `cnid_t did`
* `char * path`

# Macros

* Undocumented: `SL_CNID_DCACHE_MAX`, `SL_CNID_GROWTH`, `SL_CNID_MAX_CAP`, `SL_CNID_MAX_DEPTH`, `SL_CNID_MAX_TERMS`, `SL_CNID_MIN_TERMLEN`, `SL_CNID_PAGE_SIZE`, `SL_CNID_START_RESULTS`
