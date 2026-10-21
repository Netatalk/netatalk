---
type: C Source File
title: "etc/afpd/spotlight.c"
description: "FPSpotlightRPC: Spotlight queries and attribute requests over the search backends."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/spotlight.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T15:29:27+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/byteorder.h](../../include/atalk/byteorder.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dalloc.h](../../include/atalk/dalloc.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/iniparser_util.h](../../include/atalk/iniparser_util.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/spotlight.h](../../include/atalk/spotlight.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [directory.h](directory.h.md)
* [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md)
* System headers: `errno.h`, `glib.h`, `inttypes.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `strings.h`, `talloc.h`, `time.h`, `utime.h`

# Functions

### spotlight_get_be32

```c
static uint32_t spotlight_get_be32(const char *buf, size_t offset)
```

Defined at lines 68 to 73.

Read a big-endian 32-bit value at any offset of a buffer.

Parameters:
* `buf`: buffer
* `offset`: byte offset, no alignment needed

Returns: the value in host order

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### spotlight_put_be32

```c
static void spotlight_put_be32(char *buf, size_t offset, uint32_t val)
```

Defined at lines 82 to 86.

Write a 32-bit value in big-endian order at any offset of a buffer.

Parameters:
* `buf`: buffer
* `offset`: byte offset, no alignment needed
* `val`: value in host order

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### tab_level

```c
static char * tab_level(TALLOC_CTX *mem_ctx, int level)
```

Defined at lines 99 to 110.

Build a string of tab characters for the indentation of a dump.

Parameters:
* `mem_ctx`: talloc parent
* `level`: number of tabs

Returns: the string, allocated on mem_ctx

Called by: [dd_dump](spotlight.c.md#dd_dump)

### sl_index_event

```c
int sl_index_event(const AFPObj *obj, const struct vol *vol, sl_index_event_t event, const char *path, const char *oldpath)
```

Defined at lines 125 to 138.

Pass a file change to the Spotlight backend of a volume for its index.

A volume without Spotlight, or a backend without an index hook, ignores the event.

Parameters:
* `obj`: session object
* `vol`: volume of the change, or NULL
* `event`: change type
* `path`: path of the changed object
* `oldpath`: source path of a move, or NULL

Returns: the result of the backend hook, or 0 without a hook

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [moveandrename](filedir.c.md#moveandrename), [of_closefork](ofork.c.md#of_closefork)

Calls through [`sl_backend_ops::sbo_index_event`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_xapian_index_event](../spotlight/xapian/sl_xapian.c.md#sl_xapian_index_event)

### dd_dump

```c
static char * dd_dump(DALLOC_CTX *dd, int nestinglevel)
```

Defined at lines 150 to 311.

Render a Spotlight dalloc tree as indented text for the log.

Arrays, dictionaries, and file metadata nest 1 tab deeper, and CNID lists 2 tabs. Each scalar shows its type and value.

Parameters:
* `dd`: tree to render, also the talloc parent of the text
* `nestinglevel`: indentation of the outermost line

Returns: the text, or NULL when an allocation fails

Calls: [tab_level](spotlight.c.md#tab_level)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### cnid_comp_fn

```c
static int cnid_comp_fn(const void *p1, const void *p2)
```

Defined at lines 320 to 333.

Compare two 64-bit CNIDs for qsort and bsearch.

Parameters:
* `p1`: first CNID
* `p2`: second CNID

Returns: -1, 0, or 1 for ascending order

Called by: [sl_createCNIDArray](spotlight.c.md#sl_createcnidarray)

### sl_createCNIDArray

```c
int sl_createCNIDArray(slq_t *slq, const DALLOC_CTX *p)
```

Defined at lines 346 to 392.

Store the CNID filter of a query as a sorted array.

Every element of p must be a uint64_t. The backends look each result up in the sorted array through bsearch. An empty list stores no filter.

Parameters:
* `slq`: query that receives slq_cnids and slq_cnids_num
* `p`: CNID list from the client

Returns: 0, or -1 for a NULL argument, an invalid element, or a failed allocation

Calls: [cnid_comp_fn](spotlight.c.md#cnid_comp_fn)

Called by: [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery)

### sl_reqinfo_contains

```c
static bool sl_reqinfo_contains(sl_array_t *reqinfo, const char *attr)
```

Defined at lines 401 to 410.

Tell whether a list of requested attribute names holds a name.

Parameters:
* `reqinfo`: attribute names
* `attr`: name to find

Returns: true when the list holds attr

Called by: [sl_sanitize_reqinfo](spotlight.c.md#sl_sanitize_reqinfo)

### sl_sanitize_reqinfo

```c
static sl_array_t * sl_sanitize_reqinfo(TALLOC_CTX *mem_ctx, const sl_array_t *reqinfo)
```

Defined at lines 422 to 462.

Rebuild the requested attribute list of a query without duplicates.

The result keeps the first MAX_SL_REQINFO_ATTRS distinct names. An element that is not a string rejects the list.

Parameters:
* `mem_ctx`: talloc parent of the result
* `reqinfo`: attribute names from the client

Returns: the new list, or NULL for an invalid element or a failed allocation

Calls: [dalloc_strdup](../../libatalk/dalloc/dalloc.c.md#dalloc_strdup), [sl_reqinfo_contains](spotlight.c.md#sl_reqinfo_contains)

Called by: [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery)

### sl_path_in_scope

```c
bool sl_path_in_scope(const char *path, const char *scope)
```

Defined at lines 475 to 479.

Tell whether a path equals a scope or lies under it.

The test respects component boundaries, so /srv/afp2 does not cover /srv/afp22/x. An empty scope covers every path, and "/" every absolute one.

Parameters:
* `path`: path to test
* `scope`: scope path

Returns: true when path is inside scope, false for a NULL argument

Calls: [path_at_or_below](../../libatalk/util/unix.c.md#path_at_or_below)

Called by: [sl_cnid_fill_results](../spotlight/cnid/sl_cnid.c.md#sl_cnid_fill_results), [sl_xapian_fill_results](../spotlight/xapian/sl_xapian.c.md#sl_xapian_fill_results)

### sl_cnids_get_first

```c
static bool sl_cnids_get_first(const sl_cnids_t *cnids, uint64_t *cnid)
```

Defined at lines 491 to 500.

Read the first CNID of an unpacked CNIDs element.

The client builds the element, and an element without entries has no ca_cnids array.

Parameters:
* `cnids`: CNIDs element, or NULL
* `cnid`: receives the first CNID

Returns: true, or false when the element carries no CNID

Called by: [sl_rpc_fetchAttributeNamesForOIDArray](spotlight.c.md#sl_rpc_fetchattributenamesforoidarray), [sl_rpc_fetchAttributesForOIDArray](spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_storeAttributesForOIDArray](spotlight.c.md#sl_rpc_storeattributesforoidarray)

### add_filemeta

```c
bool add_filemeta(sl_array_t *reqinfo, sl_array_t *fm_array, const char *path, const struct stat *sp)
```

Defined at lines 518 to 613.

Add the requested metadata of one query result to a result array.

The values follow the order of reqinfo. The call knows the names, kMDItemPath, the sizes, the owner IDs, and the modification, access, and birth times. Names and paths go out in NFD, because macOS expects decomposed names. Unknown attributes, and kMDItemContentCreationDate without birth times, get nil. Without requested attributes, a path, or a stat, the call adds a single nil.

Parameters:
* `reqinfo`: requested attribute names
* `fm_array`: file metadata array of the reply
* `path`: path of the result, or NULL
* `sp`: stat of the result, or NULL

Returns: true

Calls: [atalk_stat_atime_timespec](../../include/atalk/compat.h.md#atalk_stat_atime_timespec), [atalk_stat_mtime_timespec](../../include/atalk/compat.h.md#atalk_stat_mtime_timespec), [atalk_timespec_to_timeval](../../include/atalk/compat.h.md#atalk_timespec_to_timeval), [charset_decompose](../../libatalk/unicode/charcnv.c.md#charset_decompose), [dalloc_strdup](../../libatalk/dalloc/dalloc.c.md#dalloc_strdup), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_cnid_fill_results](../spotlight/cnid/sl_cnid.c.md#sl_cnid_fill_results), [sl_rpc_fetchAttributesForOIDArray](spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_xapian_fill_results](../spotlight/xapian/sl_xapian.c.md#sl_xapian_fill_results), [tracker_cursor_cb](../spotlight/localsearch/sl_localsearch.c.md#tracker_cursor_cb)

### create_result_handle

```c
static bool create_result_handle(slq_t *slq)
```

Defined at lines 625 to 668.

Allocate the result buffers that the backend fills for a query.

The CNID list carries the query context. The file metadata array starts with the nil entry that every result list carries first.

Parameters:
* `slq`: query without result buffers

Returns: true, or false when the query has buffers already or an allocation fails

Called by: [add_results](spotlight.c.md#add_results), [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery)

### add_results

```c
static bool add_results(sl_array_t *array, slq_t *slq)
```

Defined at lines 685 to 731.

Move the gathered results of a query into a reply and start new result buffers.

The status is 35 while the query runs, so the client polls again, and 0 from SLQ_STATE_DONE on. The reply then takes the CNID list and the file metadata, and the sent results go with the reply.

Packaging failures leave the backend execution state unchanged so that cleanup can retain a query still owned by an asynchronous callback.

Parameters:
* `array`: reply array
* `slq`: query with results

Returns: true, or false when an allocation fails

Calls: [create_result_handle](spotlight.c.md#create_result_handle), [slq_state_name](spotlight_queries.c.md#slq_state_name)

Called by: [sl_rpc_fetchQueryResultsForContext](spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

### localsearch_backend_active

```c
static bool localsearch_backend_active(const AFPObj *obj)
```

Defined at lines 745 to 757.

Tell whether a volume that the session opened uses the localsearch backend.

Parameters:
* `obj`: session object, unused

Returns: true when a volume has the localsearch backend

Calls: [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_fetchPropertiesForContext

```c
static int sl_rpc_fetchPropertiesForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
```

Defined at lines 779 to 849.

Spotlight RPC fetchPropertiesForContext: describe the search store of a volume.

The reply dictionary names the query scopes, the volume path as the only path scope, and the volume UUID as store and volume UUID. It also states a persistent UUID, VolFS support, and the backup flag of a Time Machine volume.

Parameters:
* `obj`: session object, unused
* `query`: unpacked request, unused
* `reply`: reply that receives the dictionary
* `v`: volume of the request

Returns: 0, or -1 for a volume without UUID

Calls: [dalloc_strdup](../../libatalk/dalloc/dalloc.c.md#dalloc_strdup)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_openQuery

```c
static int sl_rpc_openQuery(AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, struct vol *v)
```

Defined at lines 872 to 1176.

Spotlight RPC openQueryWithParams:forContext: start a search on a volume.

The request carries the 2 context IDs, the query string, the requested attributes, the scope, and an optional CNID filter. The query string converts from UTF8-MAC to the volume charset. A nested scope array, as macOS Tahoe sends, unwraps one level. A scope outside the volume root, or one with a ".." component, falls back to the volume root.

The backend gets the query after its result buffers exist, so a synchronous backend can fill them at once. The query then joins the active list.

Parameters:
* `obj`: session object
* `query`: unpacked request
* `reply`: reply that receives status 0, or UINT64_MAX after a failure
* `v`: volume of the request

Returns: 0, also after a failure, which the reply status reports

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [create_result_handle](spotlight.c.md#create_result_handle), [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dalloc_value_for_key](../../libatalk/dalloc/dalloc.c.md#dalloc_value_for_key), [path_has_dot_component](../../libatalk/util/unix.c.md#path_has_dot_component), [sl_createCNIDArray](spotlight.c.md#sl_createcnidarray), [sl_sanitize_reqinfo](spotlight.c.md#sl_sanitize_reqinfo), [slq_add](spotlight_queries.c.md#slq_add), [slq_destroy](spotlight_queries.c.md#slq_destroy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

Calls through [`sl_backend_ops::sbo_init`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_cnid_init](../spotlight/cnid/sl_cnid.c.md#sl_cnid_init), [sl_localsearch_init](../spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_init), [sl_xapian_init](../spotlight/xapian/sl_xapian.c.md#sl_xapian_init)

Calls through [`sl_backend_ops::sbo_open_query`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_cnid_open_query](../spotlight/cnid/sl_cnid.c.md#sl_cnid_open_query), [sl_localsearch_open_query](../spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_open_query), [sl_xapian_open_query](../spotlight/xapian/sl_xapian.c.md#sl_xapian_open_query)

### sl_rpc_fetchQueryResultsForContext

```c
static int sl_rpc_fetchQueryResultsForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
```

Defined at lines 1192 to 1278.

Spotlight RPC fetchQueryResultsForContext: return the results that a query gathered.

The call finds the query by its context IDs, resets its idle timer, and lets the backend gather more results. A query in the error state, or any failure, closes the query, deferring destruction while a callback owns it.

Parameters:
* `obj`: session object, unused
* `query`: unpacked request
* `reply`: reply that receives the status and the results
* `v`: volume, unused

Returns: 0, or -1 after a failure

Calls: [add_results](spotlight.c.md#add_results), [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [slq_close](spotlight_queries.c.md#slq_close), [slq_for_ctx](spotlight_queries.c.md#slq_for_ctx), [slq_state_name](spotlight_queries.c.md#slq_state_name)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

Calls through [`sl_backend_ops::sbo_fetch_results`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_cnid_fetch_results](../spotlight/cnid/sl_cnid.c.md#sl_cnid_fetch_results), [sl_localsearch_fetch_results](../spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_fetch_results), [sl_xapian_fetch_results](../spotlight/xapian/sl_xapian.c.md#sl_xapian_fetch_results)

### sl_rpc_storeAttributesForOIDArray

```c
static int sl_rpc_storeAttributesForOIDArray(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
```

Defined at lines 1295 to 1353.

Spotlight RPC storeAttributes:forOIDArray:context: set the dates of an object.

Only the first CNID of the request counts. kMDItemFSContentChangeDate sets the access and modification times, and kMDItemLastUsedDate sets the access time alone. The volume root has CNID 2, and every other CNID under CNID_START fails.

Parameters:
* `obj`: session object, unused
* `query`: unpacked request
* `reply`: reply that receives status 0
* `vol`: volume of the object

Returns: 0, or -1 when the CNID does not resolve

Calls: [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dalloc_value_for_key](../../libatalk/dalloc/dalloc.c.md#dalloc_value_for_key), [dirlookup](directory.c.md#dirlookup), [movecwd](directory.c.md#movecwd), [sl_cnids_get_first](spotlight.c.md#sl_cnids_get_first)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_fetchAttributeNamesForOIDArray

```c
static int sl_rpc_fetchAttributeNamesForOIDArray(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
```

Defined at lines 1369 to 1413.

Spotlight RPC fetchAttributeNamesForOIDArray:context: list the attributes of an object.

The call answers for the first CNID of the request with a constant list. It names the name, the display name, the size, the owner IDs, and the content change date.

Parameters:
* `obj`: session object, unused
* `query`: unpacked request
* `reply`: reply that receives status 0, the CNID, and the names
* `vol`: volume, unused

Returns: 0, or -1 for a request without a CNID

Calls: [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dalloc_strdup](../../libatalk/dalloc/dalloc.c.md#dalloc_strdup), [sl_cnids_get_first](spotlight.c.md#sl_cnids_get_first)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_fetchAttributesForOIDArray

```c
static int sl_rpc_fetchAttributesForOIDArray(AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
```

Defined at lines 1429 to 1502.

Spotlight RPC fetchAttributes:forOIDArray:context: return the metadata of an object.

The call also serves fetchAllAttributes. It resolves the first CNID of the request to a path and stats it, and add_filemeta fills the requested attributes. A failure puts status UINT64_MAX in the reply and returns -1.

Parameters:
* `obj`: session object, unused
* `query`: unpacked request
* `reply`: reply that receives the status, the CNID, and the metadata
* `vol`: volume of the object

Returns: 0, or -1 after a failure

Calls: [add_filemeta](spotlight.c.md#add_filemeta), [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dirlookup](directory.c.md#dirlookup), [sl_cnids_get_first](spotlight.c.md#sl_cnids_get_first)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_closeQueryForContext

```c
static int sl_rpc_closeQueryForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
```

Defined at lines 1516 to 1561.

Spotlight RPC closeQueryForContext: end a query.

A finished query goes away at once. A running one moves to the cancelled list until its backend stops.

Parameters:
* `obj`: session object, unused
* `query`: unpacked request
* `reply`: reply that receives status 0 or UINT64_MAX
* `v`: volume, unused

Returns: 0, or -1 for an unknown context or an unexpected state

Calls: [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [slq_close](spotlight_queries.c.md#slq_close), [slq_for_ctx](spotlight_queries.c.md#slq_for_ctx)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### afp_spotlight_rpc

```c
int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1584 to 1732.

FPSpotlightRPC: answer the Spotlight commands of a client.

Each call first runs pending localsearch events and cleans up cancelled and idle queries. The request names a volume and a command in big-endian fields. The open command answers with the volume path, and the flags command with a fixed value. The RPC command unpacks a Spotlight request, runs the named RPC, and packs the reply.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK, AFPERR_NOOP without Spotlight, AFPERR_PARAM for a request under 2 bytes, or AFPERR_MISC for every later failure

Calls: [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dd_dump](spotlight.c.md#dd_dump), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [localsearch_backend_active](spotlight.c.md#localsearch_backend_active), [sl_pack](spotlight_marshalling.c.md#sl_pack), [sl_rpc_closeQueryForContext](spotlight.c.md#sl_rpc_closequeryforcontext), [sl_rpc_fetchAttributeNamesForOIDArray](spotlight.c.md#sl_rpc_fetchattributenamesforoidarray), [sl_rpc_fetchAttributesForOIDArray](spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_fetchPropertiesForContext](spotlight.c.md#sl_rpc_fetchpropertiesforcontext), [sl_rpc_fetchQueryResultsForContext](spotlight.c.md#sl_rpc_fetchqueryresultsforcontext), [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery), [sl_rpc_storeAttributesForOIDArray](spotlight.c.md#sl_rpc_storeattributesforoidarray), [sl_unpack_len](spotlight_marshalling.c.md#sl_unpack_len), [slq_cancelled_cleanup](spotlight_queries.c.md#slq_cancelled_cleanup), [slq_dump](spotlight_queries.c.md#slq_dump), [slq_idle_cleanup](spotlight_queries.c.md#slq_idle_cleanup), [spotlight_get_be32](spotlight.c.md#spotlight_get_be32), [spotlight_put_be32](spotlight.c.md#spotlight_put_be32), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

# Macros

* Undocumented: `MAX_SL_REQINFO_ATTRS`, `MAX_SL_RESULTS`
