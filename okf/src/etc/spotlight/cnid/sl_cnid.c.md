---
type: C Source File
title: "etc/spotlight/cnid/sl_cnid.c"
description: "17 functions, 2 types, includes 10 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/cnid/sl_cnid.c"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/cnid.h](../../../include/atalk/cnid.h.md)
* [atalk/directory.h](../../../include/atalk/directory.h.md)
* [atalk/errchk.h](../../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../../include/atalk/globals.h.md)
* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [atalk/spotlight.h](../../../include/atalk/spotlight.h.md)
* [atalk/util.h](../../../include/atalk/util.h.md)
* [atalk/volume.h](../../../include/atalk/volume.h.md)
* [etc/afpd/volume.h](../../afpd/volume.h.md)
* [etc/spotlight/spotlight_private.h](../spotlight_private.h.md)
* System headers: `ctype.h`, `inttypes.h`, `stdbool.h`, `stdint.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `talloc.h`, `unistd.h`

# Function tables

### sl_cnid_ops

Initialized at line 933 as `struct sl_backend_ops`. Assigns:

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

Defined at lines 42 to 51.

Called by: [sl_cnid_fill_results](sl_cnid.c.md#sl_cnid_fill_results)

### cnid32_comp_fn

```c
static int cnid32_comp_fn(const void *p1, const void *p2)
```

Defined at lines 58 to 67.

Called by: [sl_cnid_collect](sl_cnid.c.md#sl_cnid_collect)

### sl_cnid_quoted_value

```c
static char * sl_cnid_quoted_value(TALLOC_CTX *mem_ctx, const char *p, const char **endp)
```

Defined at lines 168 to 260.

Extract the quoted value starting at or after `p`

Scans to the first unescaped closing quote, strips unescaped leading/trailing '*' wildcards, drops one wrapping escaped-quote pair (Finder's exact-phrase delimiters; the substring search preserves adjacency by construction) and removes the remaining backslash escapes.

Parameters:
* `mem_ctx`: talloc context for the returned term
* `p`: query-string position the scan starts from
* `endp`: set to just past the closing quote when a quoted span was found (even if the term is rejected), NULL when no quoted span follows — the caller must stop scanning then

Returns: talloc'd term, or NULL when no quoted value follows or the result is shorter than SL_CNID_MIN_TERMLEN

Calls: [strnlen](../../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_cnid_extract_terms](sl_cnid.c.md#sl_cnid_extract_terms)

### sl_cnid_add_term

```c
static bool sl_cnid_add_term(char **terms, int *count, char *term)
```

Defined at lines 267 to 277.

Append `term` to `terms` unless already present.

Returns: true when the term was appended

Called by: [sl_cnid_extract_terms](sl_cnid.c.md#sl_cnid_extract_terms)

### sl_cnid_extract_terms

```c
static int sl_cnid_extract_terms(TALLOC_CTX *mem_ctx, const char *qstring, char **terms, int max_terms)
```

Defined at lines 296 to 389.

Extract all filename search terms from a Spotlight query.

Handles the common macOS patterns: kMDItemFSName = "foo*"cd → "foo" kMDItemDisplayName = "*foo*"cd → "foo" _kMDItemFileName = "foo*"cd → "foo" *=="foo*"cdw → "foo"

Finder splits a multi-word search into one predicate per word joined with ||, so every extracted term contributes to the result set. Named filename attributes take precedence; the *== "any attribute" form is scanned only when no named attribute matched. Duplicate terms are collapsed.

Returns: the number of talloc'd terms stored in `terms`

Calls: [sl_cnid_add_term](sl_cnid.c.md#sl_cnid_add_term), [sl_cnid_quoted_value](sl_cnid.c.md#sl_cnid_quoted_value), [strnlen](../../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_dcache_get

```c
static const char * sl_dcache_get(struct sl_cnid_query *csq, cnid_t did)
```

Defined at lines 395 to 404.

Called by: [sl_cnid_to_path](sl_cnid.c.md#sl_cnid_to_path)

### sl_dcache_put

```c
static void sl_dcache_put(struct sl_cnid_query *csq, cnid_t did, const char *path)
```

Defined at lines 406 to 430.

Called by: [sl_cnid_to_path](sl_cnid.c.md#sl_cnid_to_path)

### sl_cnid_to_path

```c
static char * sl_cnid_to_path(TALLOC_CTX *mem_ctx, const struct vol *vol, struct sl_cnid_query *csq, cnid_t cnid)
```

Defined at lines 447 to 500.

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

Defined at lines 506 to 510.

Called through [`sl_backend_ops::sbo_init`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_close

```c
static void sl_cnid_close(AFPObj *obj)
```

Defined at lines 512 to 515.

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_fill_results

```c
static int sl_cnid_fill_results(slq_t *slq)
```

Defined at lines 532 to 640.

Emit up to SL_CNID_PAGE_SIZE results from the private CNID buffer.

Iterates over the remaining entries in csq->cnids[csq->pos..csq->count-1], resolving each CNID to a filesystem path and adding it to query_results. Stops after SL_CNID_PAGE_SIZE accepted results or when the buffer is exhausted, whichever comes first.

Sets slq_state to: SLQ_STATE_FULL — page is full, more results remain; client must poll SLQ_STATE_DONE — all results have been delivered SLQ_STATE_ERROR — [add_filemeta()](../../afpd/spotlight.c.md#add_filemeta) failed

Returns: 0 on success, -1 on error

Calls: [add_filemeta](../../afpd/spotlight.c.md#add_filemeta), [cnid_comp_fn](sl_cnid.c.md#cnid_comp_fn), [sl_cnid_to_path](sl_cnid.c.md#sl_cnid_to_path), [sl_path_in_scope](../../afpd/spotlight.c.md#sl_path_in_scope)

Called by: [sl_cnid_fetch_results](sl_cnid.c.md#sl_cnid_fetch_results), [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_cap_for

```c
static int sl_cnid_cap_for(uint64_t want, int nterms)
```

Defined at lines 649 to 662.

Candidate capacity for `want` results across `nterms` search terms.

Every term needs CNID_FIND_MIN_RESULTS entries of room or [cnid_find()](../../../libatalk/cnid/cnid.c.md#cnid_find) refuses the call, so a small limit still allocates enough for all the terms; the surplus is trimmed from the results afterwards.

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_collect

```c
static int sl_cnid_collect(slq_t *slq, struct sl_cnid_query *csq, char **terms, int nterms, cnid_t scope_did)
```

Defined at lines 676 to 743.

Search every term into the candidate buffer and deduplicate.

Finder joins one predicate per typed word with ||, so the terms are alternatives: one [cnid_find()](../../../libatalk/cnid/cnid.c.md#cnid_find) per term into the shared buffer, then a sort and unique pass because a name can match more than one term.

Restartable: count and the truncation flag are reset on entry so the caller can re-run against a grown buffer.

Returns: 0 on success, -1 when a backend search failed

Calls: [cnid32_comp_fn](sl_cnid.c.md#cnid32_comp_fn), [cnid_find_scoped](../../../libatalk/cnid/cnid.c.md#cnid_find_scoped), [strnlen](../../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_scope_is_vol_root

```c
static bool sl_cnid_scope_is_vol_root(const char *scope, const char *vol_path)
```

Defined at lines 749 to 764.

True when `scope` names the volume root (ignoring trailing slashes).

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_open_query

```c
static int sl_cnid_open_query(slq_t *slq)
```

Defined at lines 766 to 904.

Calls: [cnid_for_path](../../../libatalk/util/cnid.c.md#cnid_for_path), [cnid_volume_reset](../../afpd/volume.c.md#cnid_volume_reset), [sl_cnid_cap_for](sl_cnid.c.md#sl_cnid_cap_for), [sl_cnid_collect](sl_cnid.c.md#sl_cnid_collect), [sl_cnid_extract_terms](sl_cnid.c.md#sl_cnid_extract_terms), [sl_cnid_fill_results](sl_cnid.c.md#sl_cnid_fill_results), [sl_cnid_scope_is_vol_root](sl_cnid.c.md#sl_cnid_scope_is_vol_root)

Called through [`sl_backend_ops::sbo_open_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_fetch_results

```c
static int sl_cnid_fetch_results(slq_t *slq)
```

Defined at lines 906 to 923.

Calls: [sl_cnid_fill_results](sl_cnid.c.md#sl_cnid_fill_results)

Called through [`sl_backend_ops::sbo_fetch_results`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_fetchQueryResultsForContext](../../afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_close_query

```c
static void sl_cnid_close_query(slq_t *slq)
```

Defined at lines 925 to 931.

Called through [`sl_backend_ops::sbo_close_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [slq_cancelled_cleanup](../../afpd/spotlight.c.md#slq_cancelled_cleanup), [slq_destroy](../../afpd/spotlight.c.md#slq_destroy)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

# Types

### struct sl_cnid_query

Defined at line 130.
* `cnid_t * cnids`
* `int cap`
* `int count`
* `int pos`
* `bool more_available`
* `struct sl_did_cache_ent * dcache`
* `int dcache_n`

### struct sl_did_cache_ent

Defined at line 118.
* `cnid_t did`
* `char * path`

# Macros

* Undocumented: `SL_CNID_DCACHE_MAX`, `SL_CNID_GROWTH`, `SL_CNID_MAX_CAP`, `SL_CNID_MAX_DEPTH`, `SL_CNID_MAX_TERMS`, `SL_CNID_MIN_TERMLEN`, `SL_CNID_PAGE_SIZE`, `SL_CNID_START_RESULTS`
