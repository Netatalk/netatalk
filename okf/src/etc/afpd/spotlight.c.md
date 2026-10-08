---
type: C Source File
title: "etc/afpd/spotlight.c"
description: "33 functions, 1 type, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/spotlight.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/byteorder.h](../../include/atalk/byteorder.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dalloc.h](../../include/atalk/dalloc.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/iniparser_util.h](../../include/atalk/iniparser_util.h.md)
* [atalk/list.h](../../include/atalk/list.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/spotlight.h](../../include/atalk/spotlight.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [directory.h](directory.h.md)
* [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md)
* System headers: `errno.h`, `inttypes.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `strings.h`, `talloc.h`, `time.h`, `utime.h`

# Functions

### spotlight_get_be32

```c
static uint32_t spotlight_get_be32(const char *buf, size_t offset)
```

Defined at lines 76 to 81.

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### spotlight_put_be32

```c
static void spotlight_put_be32(char *buf, size_t offset, uint32_t val)
```

Defined at lines 83 to 87.

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### tab_level

```c
static char * tab_level(TALLOC_CTX *mem_ctx, int level)
```

Defined at lines 93 to 104.

Called by: [dd_dump](spotlight.c.md#dd_dump)

### sl_index_event

```c
int sl_index_event(const AFPObj *obj, const struct vol *vol, sl_index_event_t event, const char *path, const char *oldpath)
```

Defined at lines 106 to 119.

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [moveandrename](filedir.c.md#moveandrename), [of_closefork](ofork.c.md#of_closefork)

Calls through [`sl_backend_ops::sbo_index_event`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_xapian_index_event](../spotlight/xapian/sl_xapian.c.md#sl_xapian_index_event)

### dd_dump

```c
static char * dd_dump(DALLOC_CTX *dd, int nestinglevel)
```

Defined at lines 121 to 282.

Calls: [tab_level](spotlight.c.md#tab_level)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### cnid_comp_fn

```c
static int cnid_comp_fn(const void *p1, const void *p2)
```

Defined at lines 284 to 297.

Called by: [sl_createCNIDArray](spotlight.c.md#sl_createcnidarray)

### sl_createCNIDArray

```c
int sl_createCNIDArray(slq_t *slq, const DALLOC_CTX *p)
```

Defined at lines 299 to 345.

Calls: [cnid_comp_fn](spotlight.c.md#cnid_comp_fn)

Called by: [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery)

### sl_reqinfo_contains

```c
static bool sl_reqinfo_contains(sl_array_t *reqinfo, const char *attr)
```

Defined at lines 347 to 356.

Called by: [sl_sanitize_reqinfo](spotlight.c.md#sl_sanitize_reqinfo)

### sl_sanitize_reqinfo

```c
static sl_array_t * sl_sanitize_reqinfo(TALLOC_CTX *mem_ctx, const sl_array_t *reqinfo)
```

Defined at lines 358 to 398.

Calls: [dalloc_strdup](../../libatalk/dalloc/dalloc.c.md#dalloc_strdup), [sl_reqinfo_contains](spotlight.c.md#sl_reqinfo_contains)

Called by: [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery)

### sl_path_in_scope

```c
bool sl_path_in_scope(const char *path, const char *scope)
```

Defined at lines 405 to 425.

True when `path` equals `scope` or lies underneath it. Component boundary aware: '/srv/afp2' does not match '/srv/afp22/x'. An empty or "/" scope matches everything.

Called by: [sl_cnid_fill_results](../spotlight/cnid/sl_cnid.c.md#sl_cnid_fill_results), [sl_xapian_fill_results](../spotlight/xapian/sl_xapian.c.md#sl_xapian_fill_results)

### sl_path_has_dotdot

```c
static bool sl_path_has_dotdot(const char *path)
```

Defined at lines 430 to 441.

True when any component of `path` is "..".

Called by: [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery)

### sl_cnids_get_first

```c
static bool sl_cnids_get_first(const sl_cnids_t *cnids, uint64_t *cnid)
```

Defined at lines 451 to 460.

Read the first CNID of an unpacked CNIDs element.

The element is client-supplied and may legitimately carry no entries, in which case ca_cnids holds no array at all.

Returns: false when the element carries no CNID

Called by: [sl_rpc_fetchAttributeNamesForOIDArray](spotlight.c.md#sl_rpc_fetchattributenamesforoidarray), [sl_rpc_fetchAttributesForOIDArray](spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_storeAttributesForOIDArray](spotlight.c.md#sl_rpc_storeattributesforoidarray)

### add_filemeta

```c
bool add_filemeta(sl_array_t *reqinfo, sl_array_t *fm_array, const char *path, const struct stat *sp)
```

Defined at lines 470 to 557.

Add requested metadata for a query result element.

This could be rewritten to something more sophisticated like querying metadata from Tracker.

If path or sp is NULL, simply add nil values for all attributes.

Calls: [atalk_stat_atime_timespec](../../include/atalk/compat.h.md#atalk_stat_atime_timespec), [atalk_stat_mtime_timespec](../../include/atalk/compat.h.md#atalk_stat_mtime_timespec), [atalk_timespec_to_timeval](../../include/atalk/compat.h.md#atalk_timespec_to_timeval), [charset_decompose](../../libatalk/unicode/charcnv.c.md#charset_decompose), [dalloc_strdup](../../libatalk/dalloc/dalloc.c.md#dalloc_strdup), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_cnid_fill_results](../spotlight/cnid/sl_cnid.c.md#sl_cnid_fill_results), [sl_rpc_fetchAttributesForOIDArray](spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_xapian_fill_results](../spotlight/xapian/sl_xapian.c.md#sl_xapian_fill_results), [tracker_cursor_cb](../spotlight/localsearch/sl_localsearch.c.md#tracker_cursor_cb)

Mentioned in the documentation of: [sl_cnid_fill_results](../spotlight/cnid/sl_cnid.c.md#sl_cnid_fill_results)

### create_result_handle

```c
static bool create_result_handle(slq_t *slq)
```

Defined at lines 563 to 601.

Allocate result handle used in the async search backend result handler for storing results

Called by: [add_results](spotlight.c.md#add_results), [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery)

### add_results

```c
static bool add_results(sl_array_t *array, slq_t *slq)
```

Defined at lines 603 to 650.

Calls: [create_result_handle](spotlight.c.md#create_result_handle)

Called by: [sl_rpc_fetchQueryResultsForContext](spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

### ATALK_LIST_HEAD

```c
static ATALK_LIST_HEAD(sl_queries)
```

Declared at etc/afpd/spotlight.c line 656; no definition in the scanned sources.

### slq_add

```c
static void slq_add(slq_t *slq)
```

Defined at lines 662 to 665.

Add a query to the list of active queries

Called by: [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery)

Mentioned in the documentation of: [slq_remove](spotlight.c.md#slq_remove)

### slq_cancelled_add

```c
static void slq_cancelled_add(slq_t *slq)
```

Defined at lines 670 to 673.

Add a query to the list of cancelled queries

Called by: [slq_cancel](spotlight.c.md#slq_cancel)

### slq_remove

```c
static void slq_remove(slq_t *slq)
```

Defined at lines 683 to 696.

Remove a query from the active list

Uses pointer identity rather than ctx-value comparison so that a query that was never enqueued (e.g. openQuery failure before [slq_add()](spotlight.c.md#slq_add)) cannot accidentally remove an unrelated active query whose ctx IDs happen to match the zero-initialized or partially-parsed values.

Called by: [slq_cancel](spotlight.c.md#slq_cancel), [slq_destroy](spotlight.c.md#slq_destroy)

### slq_for_ctx

```c
static slq_t * slq_for_ctx(uint64_t ctx1, uint64_t ctx2)
```

Defined at lines 698 to 712.

Called by: [sl_rpc_closeQueryForContext](spotlight.c.md#sl_rpc_closequeryforcontext), [sl_rpc_fetchQueryResultsForContext](spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

### slq_destroy

```c
static void slq_destroy(slq_t *slq)
```

Defined at lines 717 to 730.

Invoke the backend close_query hook then free the slq

Calls: [slq_remove](spotlight.c.md#slq_remove)

Called by: [sl_rpc_closeQueryForContext](spotlight.c.md#sl_rpc_closequeryforcontext), [sl_rpc_fetchQueryResultsForContext](spotlight.c.md#sl_rpc_fetchqueryresultsforcontext), [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery), [slq_idle_cleanup](spotlight.c.md#slq_idle_cleanup)

Calls through [`sl_backend_ops::sbo_close_query`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_cnid_close_query](../spotlight/cnid/sl_cnid.c.md#sl_cnid_close_query), [sl_localsearch_close_query](../spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_close_query), [sl_xapian_close_query](../spotlight/xapian/sl_xapian.c.md#sl_xapian_close_query)

### slq_cancel

```c
static void slq_cancel(slq_t *slq)
```

Defined at lines 735 to 740.

Cancel a query (move to cancelled list; backend cleanup deferred)

Calls: [slq_cancelled_add](spotlight.c.md#slq_cancelled_add), [slq_remove](spotlight.c.md#slq_remove)

Called by: [sl_rpc_closeQueryForContext](spotlight.c.md#sl_rpc_closequeryforcontext), [slq_idle_cleanup](spotlight.c.md#slq_idle_cleanup)

### slq_cancelled_cleanup

```c
static void slq_cancelled_cleanup(void)
```

Defined at lines 745 to 777.

Free all fully-cancelled queries

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

Calls through [`sl_backend_ops::sbo_close_query`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_cnid_close_query](../spotlight/cnid/sl_cnid.c.md#sl_cnid_close_query), [sl_localsearch_close_query](../spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_close_query), [sl_xapian_close_query](../spotlight/xapian/sl_xapian.c.md#sl_xapian_close_query)

### slq_idle_cleanup

```c
static void slq_idle_cleanup(void)
```

Defined at lines 782 to 836.

Cancel or destroy queries idle longer than MAX_SL_QUERY_IDLE_TIME

Calls: [slq_cancel](spotlight.c.md#slq_cancel), [slq_destroy](spotlight.c.md#slq_destroy)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### slq_dump

```c
static void slq_dump(void)
```

Defined at lines 838 to 851.

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_fetchPropertiesForContext

```c
static int sl_rpc_fetchPropertiesForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
```

Defined at lines 876 to 946.

Return true if any currently-open volume uses the localsearch backend

Calls: [dalloc_strdup](../../libatalk/dalloc/dalloc.c.md#dalloc_strdup)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_openQuery

```c
static int sl_rpc_openQuery(AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, struct vol *v)
```

Defined at lines 948 to 1252.

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [create_result_handle](spotlight.c.md#create_result_handle), [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dalloc_value_for_key](../../libatalk/dalloc/dalloc.c.md#dalloc_value_for_key), [sl_createCNIDArray](spotlight.c.md#sl_createcnidarray), [sl_path_has_dotdot](spotlight.c.md#sl_path_has_dotdot), [sl_sanitize_reqinfo](spotlight.c.md#sl_sanitize_reqinfo), [slq_add](spotlight.c.md#slq_add), [slq_destroy](spotlight.c.md#slq_destroy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

Calls through [`sl_backend_ops::sbo_init`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_cnid_init](../spotlight/cnid/sl_cnid.c.md#sl_cnid_init), [sl_localsearch_init](../spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_init), [sl_xapian_init](../spotlight/xapian/sl_xapian.c.md#sl_xapian_init)

Calls through [`sl_backend_ops::sbo_open_query`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_cnid_open_query](../spotlight/cnid/sl_cnid.c.md#sl_cnid_open_query), [sl_localsearch_open_query](../spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_open_query), [sl_xapian_open_query](../spotlight/xapian/sl_xapian.c.md#sl_xapian_open_query)

### sl_rpc_fetchQueryResultsForContext

```c
static int sl_rpc_fetchQueryResultsForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
```

Defined at lines 1254 to 1340.

Calls: [add_results](spotlight.c.md#add_results), [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [slq_destroy](spotlight.c.md#slq_destroy), [slq_for_ctx](spotlight.c.md#slq_for_ctx)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

Calls through [`sl_backend_ops::sbo_fetch_results`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_cnid_fetch_results](../spotlight/cnid/sl_cnid.c.md#sl_cnid_fetch_results), [sl_localsearch_fetch_results](../spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_fetch_results), [sl_xapian_fetch_results](../spotlight/xapian/sl_xapian.c.md#sl_xapian_fetch_results)

### sl_rpc_storeAttributesForOIDArray

```c
static int sl_rpc_storeAttributesForOIDArray(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
```

Defined at lines 1342 to 1400.

Calls: [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dalloc_value_for_key](../../libatalk/dalloc/dalloc.c.md#dalloc_value_for_key), [dirlookup](directory.c.md#dirlookup), [movecwd](directory.c.md#movecwd), [sl_cnids_get_first](spotlight.c.md#sl_cnids_get_first)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_fetchAttributeNamesForOIDArray

```c
static int sl_rpc_fetchAttributeNamesForOIDArray(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
```

Defined at lines 1402 to 1446.

Calls: [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dalloc_strdup](../../libatalk/dalloc/dalloc.c.md#dalloc_strdup), [sl_cnids_get_first](spotlight.c.md#sl_cnids_get_first)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_fetchAttributesForOIDArray

```c
static int sl_rpc_fetchAttributesForOIDArray(AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
```

Defined at lines 1448 to 1521.

Calls: [add_filemeta](spotlight.c.md#add_filemeta), [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dirlookup](directory.c.md#dirlookup), [sl_cnids_get_first](spotlight.c.md#sl_cnids_get_first)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_rpc_closeQueryForContext

```c
static int sl_rpc_closeQueryForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
```

Defined at lines 1523 to 1588.

Calls: [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [slq_cancel](spotlight.c.md#slq_cancel), [slq_destroy](spotlight.c.md#slq_destroy), [slq_for_ctx](spotlight.c.md#slq_for_ctx)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### afp_spotlight_rpc

```c
int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1594 to 1742.

Calls: [dalloc_get](../../libatalk/dalloc/dalloc.c.md#dalloc_get), [dd_dump](spotlight.c.md#dd_dump), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [sl_pack](spotlight_marshalling.c.md#sl_pack), [sl_rpc_closeQueryForContext](spotlight.c.md#sl_rpc_closequeryforcontext), [sl_rpc_fetchAttributeNamesForOIDArray](spotlight.c.md#sl_rpc_fetchattributenamesforoidarray), [sl_rpc_fetchAttributesForOIDArray](spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_fetchPropertiesForContext](spotlight.c.md#sl_rpc_fetchpropertiesforcontext), [sl_rpc_fetchQueryResultsForContext](spotlight.c.md#sl_rpc_fetchqueryresultsforcontext), [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery), [sl_rpc_storeAttributesForOIDArray](spotlight.c.md#sl_rpc_storeattributesforoidarray), [sl_unpack_len](spotlight_marshalling.c.md#sl_unpack_len), [slq_cancelled_cleanup](spotlight.c.md#slq_cancelled_cleanup), [slq_dump](spotlight.c.md#slq_dump), [slq_idle_cleanup](spotlight.c.md#slq_idle_cleanup), [spotlight_get_be32](spotlight.c.md#spotlight_get_be32), [spotlight_put_be32](spotlight.c.md#spotlight_put_be32), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

# Types

### struct slq_state_names

Defined at line 57.
* `slq_state_t state`
* `const char * state_name`

# Macros

* Undocumented: `MAX_SL_QUERY_IDLE_TIME_ACTIVE`, `MAX_SL_QUERY_IDLE_TIME_TERMINAL`, `MAX_SL_REQINFO_ATTRS`, `MAX_SL_RESULTS`, `USE_LIST`

# File-scope variables

`slq_state_names`
