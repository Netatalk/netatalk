---
type: C Header File
title: "include/atalk/spotlight.h"
description: "5 types, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/spotlight.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T06:55:24+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dalloc.h](dalloc.h.md)
* [atalk/globals.h](globals.h.md)
* [atalk/volume.h](volume.h.md)
* System headers: `stdbool.h`, `stddef.h`, `stdint.h`

# Included by

* [etc/afpd/afp_dsi.c](../../etc/afpd/afp_dsi.c.md)
* [etc/afpd/auth.c](../../etc/afpd/auth.c.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/file.c](../../etc/afpd/file.c.md)
* [etc/afpd/filedir.c](../../etc/afpd/filedir.c.md)
* [etc/afpd/ofork.c](../../etc/afpd/ofork.c.md)
* [etc/afpd/spotlight.c](../../etc/afpd/spotlight.c.md)
* [etc/afpd/spotlight_marshalling.c](../../etc/afpd/spotlight_marshalling.c.md)
* [etc/spotlight/cnid/sl_cnid.c](../../etc/spotlight/cnid/sl_cnid.c.md)
* [etc/spotlight/localsearch/sl_localsearch.c](../../etc/spotlight/localsearch/sl_localsearch.c.md)
* [etc/spotlight/localsearch/sparql_parser.y](../../etc/spotlight/localsearch/sparql_parser.y.md)
* [etc/spotlight/localsearch/spotlight_rawquery_lexer.l](../../etc/spotlight/localsearch/spotlight_rawquery_lexer.l.md)
* [etc/spotlight/sl_backends.h](../../etc/spotlight/sl_backends.h.md)
* [etc/spotlight/spotlight_private.h](../../etc/spotlight/spotlight_private.h.md)
* [etc/spotlight/xapian/sl_xapian.c](../../etc/spotlight/xapian/sl_xapian.c.md)

# Types

### struct _slq_t

Internal query data structure.

Defined at line 102.
* `struct _slq_t * slq_next`: Links owned by the query manager.
* `struct _slq_t * slq_prev`: Previous query in the same owner.
* `slq_membership_t slq_membership`: Zero initializes to detached.
* `slq_state_t slq_state`: State.
* `AFPObj * slq_obj`: global [AFPObj](globals.h.md#struct-afpobj) handle
* `const struct vol * slq_vol`: volume handle
* `char * slq_scope`: search scope
* `time_t slq_time`: last client activity timestamp
* `uint64_t slq_ctx1`: client context 1
* `uint64_t slq_ctx2`: client context 2
* `sl_array_t * slq_reqinfo`: array with requested metadata
* `const char * slq_qstring`: the Spotlight query string
* `uint64_t * slq_cnids`: Pointer to array with CNIDs.
* `size_t slq_cnids_num`: Size of slq_cnids array.
* `void * slq_backend_private`: backend-private query state
* `bool slq_allow_expr`: Whether to allow expressions.
* `uint64_t slq_result_limit`: max results, 0 = unlimited
* `struct sl_rslts * query_results`: query results

### struct sl_backend_ops

Defined at line 127.
* `const char * sbo_name`
* `int(* sbo_init`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery).
* `void(* sbo_close`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops).
* `int(* sbo_open_query`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery).
* `int(* sbo_fetch_results`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [sl_rpc_fetchQueryResultsForContext](../../etc/afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext).
* `void(* sbo_close_query`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [slq_destroy](../../etc/afpd/spotlight_queries.c.md#slq_destroy).
* `int(* sbo_index_event`: Assigned in [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [sl_index_event](../../etc/afpd/spotlight.c.md#sl_index_event).

### struct sl_cnids_t

Defined at line 54.
* `uint16_t ca_unkn1`
* `uint32_t ca_context`
* `DALLOC_CTX * ca_cnids`

### struct sl_rslts

Handle for query results.

Defined at line 95.
* `int num_results`
* `sl_cnids_t * cnids`
* `sl_array_t * fm_array`

### struct sl_uuid_t

Defined at line 51.
* `char sl_uuid`

# Typedefs and enums

* `typedef DALLOC_CTX sl_array_t`
* `typedef struct sl_backend_ops sl_backend_ops`
* `typedef bool sl_bool_t`
* `typedef DALLOC_CTX sl_dict_t`
* `typedef DALLOC_CTX sl_filemeta_t`
* `typedef int sl_nil_t`
* `typedef struct timeval sl_time_t`
* `typedef struct _slq_t slq_t`
* `enum sl_index_event_t`: `SL_INDEX_FILE_MODIFY`, `SL_INDEX_FILE_DELETE`, `SL_INDEX_DIR_DELETE`, `SL_INDEX_FILE_CREATE`, `SL_INDEX_DIR_CREATE`, `SL_INDEX_FILE_MOVE`, `SL_INDEX_DIR_MOVE`
* `enum slq_membership_t`: `SLQ_MEMBERSHIP_DETACHED`, `SLQ_MEMBERSHIP_ACTIVE`, `SLQ_MEMBERSHIP_CANCELLED`
* `enum slq_state_t`: `SLQ_STATE_NEW`, `SLQ_STATE_RUNNING`, `SLQ_STATE_RESULTS`, `SLQ_STATE_FULL`, `SLQ_STATE_DONE`, `SLQ_STATE_CANCEL_PENDING`, `SLQ_STATE_CANCELLED`, `SLQ_STATE_ERROR`

# Macros

* Undocumented: `SL_ENC_BIG_ENDIAN`, `SL_ENC_LITTLE_ENDIAN`, `SL_ENC_UTF_16`, `SPOTLIGHT_CMD_FLAGS`, `SPOTLIGHT_CMD_OPEN`, `SPOTLIGHT_CMD_OPEN2`, `SPOTLIGHT_CMD_RPC`, `SPOTLIGHT_H`
