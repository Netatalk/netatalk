---
type: C Source File
title: "etc/spotlight/localsearch/sl_localsearch.c"
description: "9 functions, 2 types, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/localsearch/sl_localsearch.c"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/cnid.h](../../../include/atalk/cnid.h.md)
* [atalk/errchk.h](../../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../../include/atalk/globals.h.md)
* [atalk/iniparser_util.h](../../../include/atalk/iniparser_util.h.md)
* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../../include/atalk/netatalk_conf.h.md)
* [atalk/spotlight.h](../../../include/atalk/spotlight.h.md)
* [atalk/util.h](../../../include/atalk/util.h.md)
* [atalk/volume.h](../../../include/atalk/volume.h.md)
* [etc/afpd/volume.h](../../afpd/volume.h.md)
* [etc/spotlight/spotlight_private.h](../spotlight_private.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `etc/spotlight/localsearch/sparql_parser.h`, `glib.h`, `stdbool.h`, `stdint.h`, `string.h`, `sys/stat.h`, `talloc.h`, `tinysparql.h`, `unistd.h`

# Function tables

### sl_localsearch_ops

Initialized at line 446 as `struct sl_backend_ops`. Assigns:

* `sbo_close`: [sl_localsearch_close](sl_localsearch.c.md#sl_localsearch_close)
* `sbo_close_query`: [sl_localsearch_close_query](sl_localsearch.c.md#sl_localsearch_close_query)
* `sbo_fetch_results`: [sl_localsearch_fetch_results](sl_localsearch.c.md#sl_localsearch_fetch_results)
* `sbo_init`: [sl_localsearch_init](sl_localsearch.c.md#sl_localsearch_init)
* `sbo_open_query`: [sl_localsearch_open_query](sl_localsearch.c.md#sl_localsearch_open_query)

# Functions

### cnid_comp_fn

```c
static int cnid_comp_fn(const void *p1, const void *p2)
```

Defined at lines 52 to 61.

Called by: [tracker_cursor_cb](sl_localsearch.c.md#tracker_cursor_cb)

### tracker_to_unix_path

```c
static char * tracker_to_unix_path(TALLOC_CTX *mem_ctx, const char *uri)
```

Defined at lines 85 to 106.

Called by: [tracker_cursor_cb](sl_localsearch.c.md#tracker_cursor_cb)

### tracker_cursor_cb

```c
static void tracker_cursor_cb(GObject *object, GAsyncResult *res, gpointer user_data)
```

Defined at lines 112 to 254.

Calls: [add_filemeta](../../afpd/spotlight.c.md#add_filemeta), [cnid_comp_fn](sl_localsearch.c.md#cnid_comp_fn), [cnid_for_path](../../../libatalk/util/cnid.c.md#cnid_for_path), [cnid_volume_reset](../../afpd/volume.c.md#cnid_volume_reset), [tracker_to_unix_path](sl_localsearch.c.md#tracker_to_unix_path)

Called by: [sl_localsearch_fetch_results](sl_localsearch.c.md#sl_localsearch_fetch_results), [tracker_query_cb](sl_localsearch.c.md#tracker_query_cb)

### tracker_query_cb

```c
static void tracker_query_cb(GObject *object, GAsyncResult *res, gpointer user_data)
```

Defined at lines 256 to 285.

Calls: [tracker_cursor_cb](sl_localsearch.c.md#tracker_cursor_cb)

Called by: [sl_localsearch_open_query](sl_localsearch.c.md#sl_localsearch_open_query)

### sl_localsearch_init

```c
static int sl_localsearch_init(AFPObj *obj)
```

Defined at lines 291 to 337.

Calls: [configure_spotlight_attributes](sparql_map.c.md#configure_spotlight_attributes)

Called through [`sl_backend_ops::sbo_init`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

### sl_localsearch_close

```c
static void sl_localsearch_close(AFPObj *obj)
```

Defined at lines 339 to 342.

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

### sl_localsearch_open_query

```c
static int sl_localsearch_open_query(slq_t *slq)
```

Defined at lines 344 to 399.

Calls: [map_spotlight_to_sparql_query](sparql_parser.y.md#map_spotlight_to_sparql_query), [tracker_query_cb](sl_localsearch.c.md#tracker_query_cb)

Called through [`sl_backend_ops::sbo_open_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

### sl_localsearch_fetch_results

```c
static int sl_localsearch_fetch_results(slq_t *slq)
```

Defined at lines 401 to 432.

Calls: [tracker_cursor_cb](sl_localsearch.c.md#tracker_cursor_cb)

Called through [`sl_backend_ops::sbo_fetch_results`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_fetchQueryResultsForContext](../../afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

### sl_localsearch_close_query

```c
static void sl_localsearch_close_query(slq_t *slq)
```

Defined at lines 434 to 444.

Called through [`sl_backend_ops::sbo_close_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [slq_cancelled_cleanup](../../afpd/spotlight.c.md#slq_cancelled_cleanup), [slq_destroy](../../afpd/spotlight.c.md#slq_destroy)

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

# Types

### struct sl_ctx

Defined at line 67.
* `TrackerSparqlConnection * tracker_con`
* `GCancellable * cancellable`
* `GMainLoop * mainloop`

### struct sl_localsearch_query

Defined at line 77.
* `TrackerSparqlCursor * cursor`

# Macros

* Undocumented: `MAX_SL_RESULTS`
