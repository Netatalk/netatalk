---
type: C Header File
title: "include/atalk/spotlight.h"
description: "5 types, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/spotlight.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at line 95.
* `struct list_head slq_list`
* `slq_state_t slq_state`
* `AFPObj * slq_obj`
* `const struct vol * slq_vol`
* `char * slq_scope`
* `time_t slq_time`
* `uint64_t slq_ctx1`
* `uint64_t slq_ctx2`
* `sl_array_t * slq_reqinfo`
* `const char * slq_qstring`
* `uint64_t * slq_cnids`
* `size_t slq_cnids_num`
* `void * slq_backend_private`
* `bool slq_allow_expr`
* `uint64_t slq_result_limit`
* `struct sl_rslts * query_results`

### struct sl_backend_ops

Defined at line 118.
* `const char * sbo_name`
* `int(* sbo_init`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery).
* `void(* sbo_close`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops).
* `int(* sbo_open_query`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery).
* `int(* sbo_fetch_results`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [sl_rpc_fetchQueryResultsForContext](../../etc/afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext).
* `void(* sbo_close_query`: Assigned in [sl_cnid_ops](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_ops), [sl_localsearch_ops](../../etc/spotlight/localsearch/sl_localsearch.c.md#sl_localsearch_ops), [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [slq_cancelled_cleanup](../../etc/afpd/spotlight.c.md#slq_cancelled_cleanup), [slq_destroy](../../etc/afpd/spotlight.c.md#slq_destroy).
* `int(* sbo_index_event`: Assigned in [sl_xapian_ops](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ops); called through by [sl_index_event](../../etc/afpd/spotlight.c.md#sl_index_event).

### struct sl_cnids_t

Defined at line 54.
* `uint16_t ca_unkn1`
* `uint32_t ca_context`
* `DALLOC_CTX * ca_cnids`

### struct sl_rslts

Defined at line 88.
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
* `enum slq_state_t`: `SLQ_STATE_NEW`, `SLQ_STATE_RUNNING`, `SLQ_STATE_RESULTS`, `SLQ_STATE_FULL`, `SLQ_STATE_DONE`, `SLQ_STATE_CANCEL_PENDING`, `SLQ_STATE_CANCELLED`, `SLQ_STATE_ERROR`

# Macros

* Undocumented: `SL_ENC_BIG_ENDIAN`, `SL_ENC_LITTLE_ENDIAN`, `SL_ENC_UTF_16`, `SPOTLIGHT_CMD_FLAGS`, `SPOTLIGHT_CMD_OPEN`, `SPOTLIGHT_CMD_OPEN2`, `SPOTLIGHT_CMD_RPC`, `SPOTLIGHT_H`
