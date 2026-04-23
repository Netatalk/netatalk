/*
  Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef SPOTLIGHT_H
#define SPOTLIGHT_H

#include <stdint.h>
#include <stdbool.h>

#include <atalk/globals.h>
#include <atalk/volume.h>
#include <atalk/dalloc.h>

/******************************************************************************
 * Spotlight RPC and marshalling stuff
 ******************************************************************************/

/* FPSpotlightRPC subcommand codes */
#define SPOTLIGHT_CMD_OPEN    1
#define SPOTLIGHT_CMD_FLAGS   2
#define SPOTLIGHT_CMD_RPC     3
#define SPOTLIGHT_CMD_OPEN2   4

/* Can be ored and used as flags */
#define SL_ENC_LITTLE_ENDIAN 1
#define SL_ENC_BIG_ENDIAN    2
#define SL_ENC_UTF_16        4

typedef DALLOC_CTX     sl_array_t;    /*!< an array of elements           */
typedef DALLOC_CTX     sl_dict_t;     /*!< an array of key/value elements */
typedef DALLOC_CTX     sl_filemeta_t; /*!< contains one sl_array_t        */
typedef int            sl_nil_t;      /*!< a nil element                  */
typedef bool           sl_bool_t;     /*!< a boolean, we avoid bool_t     */
typedef struct timeval sl_time_t;
typedef struct {
    char sl_uuid[16];
}  sl_uuid_t;                         /*!< a UUID                         */
typedef struct {
    uint16_t   ca_unkn1;
    uint32_t   ca_context;
    DALLOC_CTX *ca_cnids;
}  sl_cnids_t;                        /*!< an array of CNIDs              */

/******************************************************************************
 * Some helper stuff dealing with queries
 ******************************************************************************/

/*! query state */
typedef enum {
    SLQ_STATE_NEW,            /*!< Query received from client           */
    SLQ_STATE_RUNNING,        /*!< Query dispatched to backend          */
    SLQ_STATE_RESULTS,        /*!< Async backend query read             */
    SLQ_STATE_FULL,           /*!< result queue is full                 */
    SLQ_STATE_DONE,           /*!< Got all results from backend         */
    SLQ_STATE_CANCEL_PENDING, /*!< a cancel op for the query is pending */
    SLQ_STATE_CANCELLED,      /*!< the query has been cancelled         */
    SLQ_STATE_ERROR	          /*!< an error happended somewhere         */
} slq_state_t;

/*! Handle for query results */
struct sl_rslts {
    int         num_results;
    sl_cnids_t *cnids;
    sl_array_t *fm_array;
};

/*! Internal query data structure */
typedef struct _slq_t {
    struct list_head  slq_list;         /*!< queries are stored in a list    */
    slq_state_t       slq_state;        /*!< State                           */
    AFPObj           *slq_obj;          /*!< global AFPObj handle            */
    const struct vol *slq_vol;          /*!< volume handle                   */
    char             *slq_scope;        /*!< search scope                    */
    time_t            slq_time;         /*!< last client activity timestamp  */
    uint64_t          slq_ctx1;         /*!< client context 1                */
    uint64_t          slq_ctx2;         /*!< client context 2                */
    sl_array_t       *slq_reqinfo;      /*!< array with requested metadata   */
    const char       *slq_qstring;      /*!< the Spotlight query string      */
    uint64_t         *slq_cnids;        /*!< Pointer to array with CNIDs     */
    size_t            slq_cnids_num;    /*!< Size of slq_cnids array         */
    void             *slq_backend_private; /*!< backend-private query state  */
    bool              slq_allow_expr;   /*!< Whether to allow expressions    */
    uint64_t          slq_result_limit; /*!< Whether to LIMIT SPARQL results */
    struct sl_rslts  *query_results;    /*!< query results                   */
} slq_t;

/******************************************************************************
 * Pluggable search backend vtable
 ******************************************************************************/

typedef struct sl_backend_ops {
    const char  *sbo_name;

    /* Called once globally when the first volume using this backend is opened. */
    int  (*sbo_init)(AFPObj *obj);

    /* Called on afpd shutdown. */
    void (*sbo_close)(AFPObj *obj);

    /* Translate slq->slq_qstring into results.
     * May be async (localsearch) or synchronous (cnid).
     * On return slq_state must be SLQ_STATE_RUNNING or SLQ_STATE_DONE. */
    int  (*sbo_open_query)(slq_t *slq);

    /* Fetch the next batch of results into slq->query_results.
     * For async backends: pumps the event loop.
     * For sync backends: no-op after sbo_open_query() already set SLQ_STATE_DONE. */
    int  (*sbo_fetch_results)(slq_t *slq);

    /* Cancel and clean up query state. */
    void (*sbo_close_query)(slq_t *slq);
} sl_backend_ops;

/******************************************************************************
 * Function declarations
 ******************************************************************************/

extern int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen _U_,
                             char *rbuf, size_t *rbuflen);
extern int sl_pack(DALLOC_CTX *query, char *buf);
extern int sl_unpack(DALLOC_CTX *query, const char *buf);
extern void configure_spotlight_attributes(const char *attributes);

#endif /* SPOTLIGHT_H */
