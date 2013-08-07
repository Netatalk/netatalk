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

#include <atalk/dalloc.h>
#include <atalk/globals.h>
#include <atalk/volume.h>

/**************************************************************************************************
 * Spotlight module stuff
 **************************************************************************************************/

#define SL_MODULE_VERSION 1

struct sl_module_export {
    int sl_mod_version;
    int (*sl_mod_init)        (void *);
    int (*sl_mod_start_search)(void *);
    int (*sl_mod_fetch_result)(void *);
    int (*sl_mod_end_search)  (void *);
    int (*sl_mod_fetch_attrs) (void *);
    int (*sl_mod_error)       (void *);
    int (*sl_mod_index_file)  (const void *);
};

extern int sl_mod_load(const char *path);
extern void sl_index_file(const char *path);

/**************************************************************************************************
 * Spotlight RPC and marshalling stuff
 **************************************************************************************************/

/* FPSpotlightRPC subcommand codes */
#define SPOTLIGHT_CMD_OPEN    1
#define SPOTLIGHT_CMD_FLAGS   2
#define SPOTLIGHT_CMD_RPC     3
#define SPOTLIGHT_CMD_OPEN2   4

/* Can be ored and used as flags */
#define SL_ENC_LITTLE_ENDIAN 1
#define SL_ENC_BIG_ENDIAN    2
#define SL_ENC_UTF_16        4

typedef DALLOC_CTX     sl_array_t;    /* an array of elements                                           */
typedef DALLOC_CTX     sl_dict_t;     /* an array of key/value elements                                 */
typedef DALLOC_CTX     sl_filemeta_t; /* contains one sl_array_t                                        */
typedef int            sl_nil_t;      /* a nil element                                                  */
typedef bool           sl_bool_t;     /* a boolean, we avoid bool_t as it's a define for something else */
typedef struct timeval sl_time_t;     /* a boolean, we avoid bool_t as it's a define for something else */
typedef struct {
    char sl_uuid[16];
}  sl_uuid_t;                         /* a UUID                                                         */
typedef struct {
    uint16_t   ca_unkn1;
    uint32_t   ca_context;
    DALLOC_CTX *ca_cnids;
}  sl_cnids_t;                        /* an array of CNIDs                                              */

/**************************************************************************************************
 * Some helper stuff dealing with queries
 **************************************************************************************************/

/* Internal query state */
typedef enum {
    SLQ_STATE_NEW      = 1,           /* Query received from client                                     */
    SLQ_STATE_RUNNING  = 2,           /* Query dispatched to Tracker                                    */
    SLQ_STATE_DONE     = 3,           /* Tracker finished                                               */
    SLQ_STATE_END      = 4,           /* Query results returned to client                               */
    SLQ_STATE_ATTRS    = 5            /* Fetch metadata for an object                                   */
} slq_state_t;

/* Internal query data structure */
typedef struct _slq_t {
    struct list_head  slq_list;           /* queries are stored in a list                                   */
    slq_state_t       slq_state;          /* State                                                          */
    AFPObj           *slq_obj;            /* global AFPObj handle                                           */
    const struct vol *slq_vol;            /* volume handle                                                  */
    DALLOC_CTX       *slq_reply;          /* reply handle                                                   */
    time_t            slq_time;           /* timestamp where we received this query                         */
    uint64_t          slq_ctx1;           /* client context 1                                               */
    uint64_t          slq_ctx2;           /* client context 2                                               */
    sl_array_t       *slq_reqinfo;        /* array with requested metadata                                  */
    const char       *slq_qstring;        /* the Spotlight query string                                     */
    uint64_t         *slq_cnids;          /* Pointer to array with CNIDs to which a query applies           */
    size_t            slq_cnids_num;      /* Size of slq_cnids array                                        */
    const char       *slq_path;           /* Path to file or dir, used in fetchAttributes                   */
#ifdef HAVE_TRACKER_SPARQL
    void             *slq_tracker_cursor; /* Tracker SPARQL query result cursor                             */
#endif
#ifdef HAVE_TRACKER_RDF
    char             *slq_trackerquery;   /* RDF query string  */
    char             *slq_fts;            /* FTS search string */
    int               slq_service;        /* Tracker service   */
    int               slq_offset;         /* search offset     */
#endif
} slq_t;

/**************************************************************************************************
 * Function declarations
 **************************************************************************************************/

extern int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen);
extern int sl_pack(DALLOC_CTX *query, char *buf);
extern int sl_unpack(DALLOC_CTX *query, const char *buf);

#endif /* SPOTLIGHT_H */
