---
type: C Source File
title: "etc/afpd/spotlight_queries.c"
description: "Session-local Spotlight query ownership and deferred cancellation."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/spotlight_queries.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T06:55:24+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md)
* System headers: `assert.h`, `inttypes.h`, `talloc.h`, `time.h`

# Functions

### slq_state_name

```c
const char * slq_state_name(slq_state_t state)
```

Defined at lines 47 to 56. Declared in [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md).

Return the log name of a backend execution state.

Parameters:
* `state`: valid backend execution state

Returns: the corresponding static log string

Called by: [add_results](spotlight.c.md#add_results), [sl_rpc_fetchQueryResultsForContext](spotlight.c.md#sl_rpc_fetchqueryresultsforcontext), [slq_cancelled_cleanup](spotlight_queries.c.md#slq_cancelled_cleanup), [slq_close](spotlight_queries.c.md#slq_close), [slq_dump](spotlight_queries.c.md#slq_dump), [slq_idle_cleanup](spotlight_queries.c.md#slq_idle_cleanup)

### slq_remove

```c
static void slq_remove(slq_t *slq)
```

Defined at lines 66 to 101.

Unlink a query from its owner in constant time.

Detached queries, including failed opens, are safe to remove. Membership identifies the owning head independently of the backend execution state.

Parameters:
* `slq`: query to detach

Called by: [slq_cancel](spotlight_queries.c.md#slq_cancel), [slq_destroy](spotlight_queries.c.md#slq_destroy)

Uses file-scope variables: `sl_cancelled_queries`, `sl_queries`

### slq_add

```c
void slq_add(slq_t *slq)
```

Defined at lines 110 to 122. Declared in [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md).

Publish a detached query for client lookup, newest first.

Repeated context IDs resolve to the most recently published query.

Parameters:
* `slq`: detached query to publish

Called by: [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery)

Uses file-scope variables: `sl_queries`

### slq_for_ctx

```c
slq_t * slq_for_ctx(uint64_t ctx1, uint64_t ctx2)
```

Defined at lines 131 to 140. Declared in [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md).

Find the newest active query with a pair of client context IDs.

Parameters:
* `ctx1`: first context ID
* `ctx2`: second context ID

Returns: the query, or NULL when no active query matches

Called by: [sl_rpc_closeQueryForContext](spotlight.c.md#sl_rpc_closequeryforcontext), [sl_rpc_fetchQueryResultsForContext](spotlight.c.md#sl_rpc_fetchqueryresultsforcontext)

Uses file-scope variables: `sl_queries`

### slq_destroy

```c
void slq_destroy(slq_t *slq)
```

Defined at lines 151 to 164. Declared in [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md).

Close the backend, detach, and free a query, or ignore NULL.

The caller must ensure no asynchronous callback still owns the query. A failed open is detached; cancellation requires backend acknowledgement before this function may release the allocation.

Parameters:
* `slq`: query to release, or NULL

Calls: [slq_remove](spotlight_queries.c.md#slq_remove)

Called by: [sl_rpc_openQuery](spotlight.c.md#sl_rpc_openquery), [slq_cancelled_cleanup](spotlight_queries.c.md#slq_cancelled_cleanup), [slq_close](spotlight_queries.c.md#slq_close)

Calls through [`sl_backend_ops::sbo_close_query`](../../include/atalk/spotlight.h.md#struct-sl_backend_ops): [sl_cnid_close_query](../spotlight/cnid/sl_cnid.c.md#sl_cnid_close_query), [sl_localsearch_close_query](../spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_close_query), [sl_xapian_close_query](../spotlight/xapian/sl_xapian.c.md#sl_xapian_close_query)

### slq_cancel

```c
void slq_cancel(slq_t *slq)
```

Defined at lines 174 to 187. Declared in [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md).

Hide an active query while its asynchronous backend stops.

Retain the query and its backend resources until a callback sets SLQ_STATE_CANCELLED. Execution state and list membership are separate.

Parameters:
* `slq`: active query to cancel

Calls: [slq_remove](spotlight_queries.c.md#slq_remove)

Called by: [slq_close](spotlight_queries.c.md#slq_close)

Uses file-scope variables: `sl_cancelled_queries`

### slq_close

```c
int slq_close(slq_t *slq)
```

Defined at lines 199 to 225. Declared in [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md).

End an active query, retaining it while an asynchronous callback owns it.

RUNNING and RESULTS require cancellation acknowledgement. FULL, DONE and ERROR have no pending callback and can be destroyed immediately. Callers must preserve backend execution state when result packaging fails.

Parameters:
* `slq`: active query to close, or NULL

Returns: 0, or -1 for an unexpected execution state

Calls: [slq_cancel](spotlight_queries.c.md#slq_cancel), [slq_destroy](spotlight_queries.c.md#slq_destroy), [slq_state_name](spotlight_queries.c.md#slq_state_name)

Called by: [sl_rpc_closeQueryForContext](spotlight.c.md#sl_rpc_closequeryforcontext), [sl_rpc_fetchQueryResultsForContext](spotlight.c.md#sl_rpc_fetchqueryresultsforcontext), [slq_idle_cleanup](spotlight_queries.c.md#slq_idle_cleanup)

### slq_cancelled_cleanup

```c
void slq_cancelled_cleanup(void)
```

Defined at lines 230 to 244. Declared in [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md).

Release queries whose asynchronous callbacks acknowledged cancellation.

Calls: [slq_destroy](spotlight_queries.c.md#slq_destroy), [slq_state_name](spotlight_queries.c.md#slq_state_name)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

Uses file-scope variables: `sl_cancelled_queries`

### slq_idle_cleanup

```c
void slq_idle_cleanup(void)
```

Defined at lines 253 to 283. Declared in [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md).

Cancel idle running queries and destroy idle terminal queries.

Backend close hooks release only the current query's resources and do not pump callbacks or remove other queries, so the saved successor stays valid. NEW queries are left alone and do not restart traversal.

Calls: [slq_close](spotlight_queries.c.md#slq_close), [slq_state_name](spotlight_queries.c.md#slq_state_name)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

Uses file-scope variables: `sl_queries`

### slq_dump

```c
void slq_dump(void)
```

Defined at lines 288 to 297. Declared in [etc/spotlight/spotlight_private.h](../spotlight/spotlight_private.h.md).

Log every active query in client lookup order.

Calls: [slq_state_name](spotlight_queries.c.md#slq_state_name)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

Uses file-scope variables: `sl_queries`

# Macros

* Undocumented: `MAX_SL_QUERY_IDLE_TIME_ACTIVE`, `MAX_SL_QUERY_IDLE_TIME_TERMINAL`

# File-scope variables

`sl_cancelled_queries`, `sl_queries`
