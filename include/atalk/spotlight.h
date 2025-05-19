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


#include <gio/gio.h>

#ifndef HAVE_TRACKER3
#include <libtracker-miner/tracker-miner.h>
#endif

#include <tracker-sparql.h>

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

/* an array of elements */
typedef DALLOC_CTX     sl_array_t;
/* an array of key/value elements */
typedef DALLOC_CTX     sl_dict_t;
/* contains one sl_array_t */
typedef DALLOC_CTX     sl_filemeta_t;
/* a nil element */
typedef int            sl_nil_t;
/* a boolean, we avoid bool_t */
typedef bool           sl_bool_t;
typedef struct timeval sl_time_t;
typedef struct {
    /* a UUID */
    char sl_uuid[16];
}  sl_uuid_t;
typedef struct {
    /* an array of CNIDs */
    uint16_t   ca_unkn1;
    uint32_t   ca_context;
    DALLOC_CTX *ca_cnids;
}  sl_cnids_t;

/******************************************************************************
 * Some helper stuff dealing with queries
 ******************************************************************************/

/* query state */
typedef enum {
    /* Query received from client */
    SLQ_STATE_NEW,
    /* Query dispatched to Tracker */
    SLQ_STATE_RUNNING,
    /* Async Tracker query read */
    SLQ_STATE_RESULTS,
    /* result queue is full */
    SLQ_STATE_FULL,
    /* Got all results from Tracker */
    SLQ_STATE_DONE,
    /* a cancel op for the query is pending */
    SLQ_STATE_CANCEL_PENDING,
    /* the query has been cancelled */
    SLQ_STATE_CANCELLED,
    /* an error happended somewhere */
    SLQ_STATE_ERROR
} slq_state_t;

/* Handle for query results */
struct sl_rslts {
    int         num_results;
    sl_cnids_t *cnids;
    sl_array_t *fm_array;
};

/* Internal query data structure */
typedef struct _slq_t {
    /* queries are stored in a list */
    struct list_head  slq_list;
    /* State */
    slq_state_t       slq_state;
    /* global AFPObj handle */
    AFPObj           *slq_obj;
    /* volume handle */
    const struct vol *slq_vol;
    /* search scope */
    char             *slq_scope;
    /* timestamp received query */
    time_t            slq_time;
    /* client context 1 */
    uint64_t          slq_ctx1;
    /* client context 2 */
    uint64_t          slq_ctx2;
    /* array with requested metadata */
    sl_array_t       *slq_reqinfo;
    /* the Spotlight query string */
    const char       *slq_qstring;
    /* Pointer to array with CNIDs */
    uint64_t         *slq_cnids;
    /* Size of slq_cnids array */
    size_t            slq_cnids_num;
    /* Tracker SPARQL cursor */
    void             *tracker_cursor;
    /* Whether to allow expressions */
    bool              slq_allow_expr;
    /* Whether to LIMIT SPARQL results */
    uint64_t          slq_result_limit;
    /* query results */
    struct sl_rslts  *query_results;
} slq_t;

struct sl_ctx {
    TrackerSparqlConnection *tracker_con;
    GCancellable *cancellable;
    GMainLoop *mainloop;
    /* list of active queries */
    slq_t *query_list;
};

/******************************************************************************
 * Function declarations
 ******************************************************************************/

extern int spotlight_init(AFPObj *obj);
extern int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen _U_,
                             char *rbuf, size_t *rbuflen);
extern int sl_pack(DALLOC_CTX *query, char *buf);
extern int sl_unpack(DALLOC_CTX *query, const char *buf);
extern void configure_spotlight_attributes(const char *attributes);

#endif /* SPOTLIGHT_H */
