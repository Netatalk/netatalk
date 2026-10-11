---
type: C Source File
title: "etc/spotlight/cnid/sl_cnid.c"
description: "17 functions, 2 types, includes 10 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/cnid/sl_cnid.c"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Initialized at line 1030 as `struct sl_backend_ops`. Assigns:

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

Defined at lines 49 to 58.

Order two 64-bit host-order CNIDs, for bsearch over the client filter.

Parameters:
* `p1`: first CNID
* `p2`: second CNID

Returns: -1, 0, or 1

Called by: [sl_cnid_fill_results](sl_cnid.c.md#sl_cnid_fill_results)

### cnid32_comp_fn

```c
static int cnid32_comp_fn(const void *p1, const void *p2)
```

Defined at lines 72 to 81.

Order two 32-bit CNIDs in network byte order, for qsort over the result buffer.

The order serves deduplication only, so the call compares the raw network-order values. The cnid_comp_fn call compares the 64-bit host-order values of the client filter.

Parameters:
* `p1`: first CNID
* `p2`: second CNID

Returns: -1, 0, or 1

Called by: [sl_cnid_collect](sl_cnid.c.md#sl_cnid_collect)

### sl_cnid_quoted_value

```c
static char * sl_cnid_quoted_value(TALLOC_CTX *mem_ctx, const char *p, const char **endp)
```

Defined at lines 182 to 274.

Extract the quoted value starting at or after `p`.

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

Defined at lines 286 to 296.

Append a term to the term list unless the list holds it already.

The caller keeps count below the size of the list.

Parameters:
* `terms`: term list
* `count`: number of terms, incremented on an append
* `term`: term to append

Returns: true after an append, false for a term that the list holds

Called by: [sl_cnid_extract_terms](sl_cnid.c.md#sl_cnid_extract_terms)

### sl_cnid_extract_terms

```c
static int sl_cnid_extract_terms(TALLOC_CTX *mem_ctx, const char *qstring, char **terms, int max_terms)
```

Defined at lines 321 to 414.

Extract the filename search terms from a Spotlight query string.

The named attributes kMDItemFSName, kMDItemDisplayName, and _kMDItemFileName come first, each with "=" after the name. Only when none matches does the call read the any-attribute form *==. Finder splits a multi-word search into one predicate per word joined with ||, so every term adds to the results. A repeated term counts once.

Patterns and their terms:

```
kMDItemFSName = "foo*"cd       -> "foo"
kMDItemDisplayName = "*foo*"cd -> "foo"
_kMDItemFileName = "foo*"cd    -> "foo"
*=="foo*"cdw                   -> "foo"
```

Parameters:
* `mem_ctx`: talloc context for the terms
* `qstring`: query string, or NULL
* `terms`: receives the terms
* `max_terms`: size of terms

Returns: the number of terms

Calls: [sl_cnid_add_term](sl_cnid.c.md#sl_cnid_add_term), [sl_cnid_quoted_value](sl_cnid.c.md#sl_cnid_quoted_value), [strnlen](../../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_dcache_get

```c
static const char * sl_dcache_get(struct sl_cnid_query *csq, cnid_t did)
```

Defined at lines 427 to 436.

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

Defined at lines 448 to 472.

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

Defined at lines 489 to 542.

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

Defined at lines 554 to 558.

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

Defined at lines 565 to 568.

Stop the CNID backend, which has nothing to release.

Parameters:
* `obj`: AFP object, unused

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_fill_results

```c
static int sl_cnid_fill_results(slq_t *slq)
```

Defined at lines 584 to 692.

Add up to SL_CNID_PAGE_SIZE results from the CNID buffer to the result page.

The call resolves each remaining CNID to a path. It skips a CNID outside the client filter and a path that does not resolve or lies out of scope. It also skips a file that fails stat or a read access check. A full page with CNIDs left sets SLQ_STATE_FULL, so the client polls for the next page. With no CNIDs left the call sets SLQ_STATE_DONE, and a failed add_filemeta sets SLQ_STATE_ERROR.

Parameters:
* `slq`: query with the result page and the backend state

Returns: 0, or -1 on error

Calls: [add_filemeta](../../afpd/spotlight.c.md#add_filemeta), [cnid_comp_fn](sl_cnid.c.md#cnid_comp_fn), [sl_cnid_to_path](sl_cnid.c.md#sl_cnid_to_path), [sl_path_in_scope](../../afpd/spotlight.c.md#sl_path_in_scope)

Called by: [sl_cnid_fetch_results](sl_cnid.c.md#sl_cnid_fetch_results), [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_cap_for

```c
static int sl_cnid_cap_for(uint64_t want, int nterms)
```

Defined at lines 706 to 719.

Size the candidate buffer for the wanted results and the search terms.

Every term needs CNID_FIND_MIN_RESULTS entries of room, or cnid_find refuses the call. So a small limit still gets room for all the terms, and the caller trims the surplus results afterwards. The capacity stops at SL_CNID_MAX_CAP.

Parameters:
* `want`: number of results wanted
* `nterms`: number of search terms

Returns: the capacity in entries

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_collect

```c
static int sl_cnid_collect(slq_t *slq, struct sl_cnid_query *csq, char **terms, int nterms, cnid_t scope_did)
```

Defined at lines 739 to 806.

Search every term into the candidate buffer, then drop the duplicates.

Finder joins one predicate per typed word with ||, so the terms are alternatives. The call runs one cnid_find_scoped per term into the shared buffer, and it holds back CNID_FIND_MIN_RESULTS entries for each term still to come. A sort and a unique pass follow, since a name can match more than one term. The call resets the count and the truncation flag on entry, so the caller can run it again on a grown buffer.

Parameters:
* `slq`: query with the volume
* `csq`: backend state with the candidate buffer
* `terms`: search terms
* `nterms`: number of terms
* `scope_did`: CNID of the scope directory, or CNID_INVALID without a scope

Returns: 0, or -1 when a search fails

Calls: [cnid32_comp_fn](sl_cnid.c.md#cnid32_comp_fn), [cnid_find_scoped](../../../libatalk/cnid/cnid.c.md#cnid_find_scoped), [strnlen](../../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_cnid_open_query](sl_cnid.c.md#sl_cnid_open_query)

### sl_cnid_scope_is_vol_root

```c
static bool sl_cnid_scope_is_vol_root(const char *scope, const char *vol_path)
```

Defined at lines 816 to 831.

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

Defined at lines 849 to 987.

Run a filename search in the CNID database for a Spotlight query.

A query without a filename term, or with a scope that does not resolve, returns no results. A scope below the volume root restricts the search to its subtree inside the database. A query without a result limit starts with SL_CNID_START_RESULTS entries. While the database reports further matches, the search reruns on a buffer of SL_CNID_GROWTH times the size. A bounded query keeps up to its limit of results. The first page fills at once.

A scope lookup that trips a CNID reset marks the volume reset and fails the query.

Parameters:
* `slq`: query

Returns: 0, or -1 on error

Calls: [cnid_for_path](../../../libatalk/util/cnid.c.md#cnid_for_path), [cnid_volume_reset](../../afpd/volume.c.md#cnid_volume_reset), [sl_cnid_cap_for](sl_cnid.c.md#sl_cnid_cap_for), [sl_cnid_collect](sl_cnid.c.md#sl_cnid_collect), [sl_cnid_extract_terms](sl_cnid.c.md#sl_cnid_extract_terms), [sl_cnid_fill_results](sl_cnid.c.md#sl_cnid_fill_results), [sl_cnid_scope_is_vol_root](sl_cnid.c.md#sl_cnid_scope_is_vol_root)

Called through [`sl_backend_ops::sbo_open_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

### sl_cnid_fetch_results

```c
static int sl_cnid_fetch_results(slq_t *slq)
```

Defined at lines 998 to 1015.

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

Defined at lines 1022 to 1028.

Free the backend state of a CNID query.

Parameters:
* `slq`: query

Called through [`sl_backend_ops::sbo_close_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [slq_cancelled_cleanup](../../afpd/spotlight.c.md#slq_cancelled_cleanup), [slq_destroy](../../afpd/spotlight.c.md#slq_destroy)

Dispatched via: [sl_cnid_ops](sl_cnid.c.md#sl_cnid_ops)

# Types

### struct sl_cnid_query

Defined at line 144.
* `cnid_t * cnids`
* `int cap`
* `int count`
* `int pos`
* `bool more_available`
* `struct sl_did_cache_ent * dcache`
* `int dcache_n`

### struct sl_did_cache_ent

Defined at line 132.
* `cnid_t did`
* `char * path`

# Macros

* Undocumented: `SL_CNID_DCACHE_MAX`, `SL_CNID_GROWTH`, `SL_CNID_MAX_CAP`, `SL_CNID_MAX_DEPTH`, `SL_CNID_MAX_TERMS`, `SL_CNID_MIN_TERMLEN`, `SL_CNID_PAGE_SIZE`, `SL_CNID_START_RESULTS`
