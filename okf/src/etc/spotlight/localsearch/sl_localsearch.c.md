---
type: C Source File
title: "etc/spotlight/localsearch/sl_localsearch.c"
description: "Spotlight LocalSearch backend: SPARQL queries to the indexer."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/localsearch/sl_localsearch.c"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Initialized at line 16 as `struct sl_backend_ops`. Assigns:

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

Defined at lines 64 to 73.

Order two 64-bit host-order CNIDs, for bsearch over the client filter.

Parameters:
* `p1`: first CNID
* `p2`: second CNID

Returns: -1, 0, or 1

Called by: [tracker_cursor_cb](sl_localsearch.c.md#tracker_cursor_cb)

### tracker_to_unix_path

```c
static char * tracker_to_unix_path(TALLOC_CTX *mem_ctx, const char *uri)
```

Defined at lines 104 to 125.

Convert a file URI from the indexer into a local path.

Parameters:
* `mem_ctx`: talloc context for the path
* `uri`: file URI

Returns: the path, or NULL for a URI without a local path

Called by: [tracker_cursor_cb](sl_localsearch.c.md#tracker_cursor_cb)

### tracker_cursor_cb

```c
static void tracker_cursor_cb(GObject *object, GAsyncResult *res, gpointer user_data)
```

Defined at lines 145 to 287.

Take one row of the indexer cursor into the result page, then ask for the next row.

A cancel request ends the query as cancelled, and a cursor error or a URI without a local path as an error. The call skips a file that fails stat or a read access check, has no CNID, or lies outside the client filter. A CNID reset during the lookup marks the volume reset and fails the query. After MAX_SL_RESULTS results the call marks the page full.

Parameters:
* `object`: cursor, unused
* `res`: result of the cursor step
* `user_data`: query

Calls: [add_filemeta](../../afpd/spotlight.c.md#add_filemeta), [cnid_comp_fn](sl_localsearch.c.md#cnid_comp_fn), [cnid_for_path](../../../libatalk/util/cnid.c.md#cnid_for_path), [cnid_volume_reset](../../afpd/volume.c.md#cnid_volume_reset), [tracker_to_unix_path](sl_localsearch.c.md#tracker_to_unix_path)

Called by: [sl_localsearch_fetch_results](sl_localsearch.c.md#sl_localsearch_fetch_results), [tracker_query_cb](sl_localsearch.c.md#tracker_query_cb)

### tracker_query_cb

```c
static void tracker_query_cb(GObject *object, GAsyncResult *res, gpointer user_data)
```

Defined at lines 299 to 328.

Finish the SPARQL query of the indexer and start reading its cursor.

A cancel request ends the query as cancelled, and a query error as an error.

Parameters:
* `object`: indexer connection
* `res`: result of the query
* `user_data`: query

Calls: [tracker_cursor_cb](sl_localsearch.c.md#tracker_cursor_cb)

Called by: [sl_localsearch_open_query](sl_localsearch.c.md#sl_localsearch_open_query)

### sl_localsearch_init

```c
static int sl_localsearch_init(AFPObj *obj)
```

Defined at lines 346 to 392.

Connect the process to the localsearch indexer over the Spotlight D-Bus session.

Until a connect succeeds, each call sets up a context in obj and applies the spotlight attributes option. It also points the D-Bus and XDG environment at the netatalk state directory. After a success, later calls return at once.

Parameters:
* `obj`: AFP object, receives the backend context

Returns: 0, or -1 when the indexer connection fails

Calls: [configure_spotlight_attributes](sparql_map.c.md#configure_spotlight_attributes)

Called through [`sl_backend_ops::sbo_init`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

### sl_localsearch_close

```c
static void sl_localsearch_close(AFPObj *obj)
```

Defined at lines 400 to 403.

Stop the localsearch backend, whose indexer connection lives as long as the process.

Parameters:
* `obj`: AFP object, unused

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

### sl_localsearch_open_query

```c
static int sl_localsearch_open_query(slq_t *slq)
```

Defined at lines 415 to 470.

Translate a Spotlight query to SPARQL and start it on the indexer.

The mapping sees the scope percent-encoded as a URI path, and slq keeps the raw scope. The query runs asynchronously, and tracker_query_cb picks up its cursor.

Parameters:
* `slq`: query

Returns: 0, or -1 without an indexer connection or when the mapping fails

Calls: [map_spotlight_to_sparql_query](sparql_parser.y.md#map_spotlight_to_sparql_query), [tracker_query_cb](sl_localsearch.c.md#tracker_query_cb)

Called through [`sl_backend_ops::sbo_open_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_openQuery](../../afpd/spotlight.c.md#sl_rpc_openquery)

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

### sl_localsearch_fetch_results

```c
static int sl_localsearch_fetch_results(slq_t *slq)
```

Defined at lines 483 to 514.

Resume the indexer cursor after a full page, and run the GLib callbacks that are ready.

A client can poll right after the query starts, before the first cursor callback ran. The call then runs the ready callbacks without blocking. The first page, or the end of an empty result set, can then go out at once.

Parameters:
* `slq`: query

Returns: 0

Calls: [tracker_cursor_cb](sl_localsearch.c.md#tracker_cursor_cb)

Called through [`sl_backend_ops::sbo_fetch_results`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [sl_rpc_fetchQueryResultsForContext](../../afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

### sl_localsearch_close_query

```c
static void sl_localsearch_close_query(slq_t *slq)
```

Defined at lines 523 to 533.

Release the indexer cursor of a query.

The query state is a talloc child of slq and goes with it.

Parameters:
* `slq`: query

Called through [`sl_backend_ops::sbo_close_query`](../../../include/atalk/spotlight.h.md#struct-sl_backend_ops) by: [slq_cancelled_cleanup](../../afpd/spotlight.c.md#slq_cancelled_cleanup), [slq_destroy](../../afpd/spotlight.c.md#slq_destroy)

Dispatched via: [sl_localsearch_ops](sl_localsearch.c.md#sl_localsearch_ops)

# Types

### struct sl_ctx

Defined at line 79.
* `TrackerSparqlConnection * tracker_con`
* `GCancellable * cancellable`
* `GMainLoop * mainloop`

### struct sl_localsearch_query

Defined at line 89.
* `TrackerSparqlCursor * cursor`

# Macros

* Undocumented: `MAX_SL_RESULTS`
