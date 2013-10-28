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

#define USE_LIST

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <utime.h>

#include <atalk/list.h>
#include <atalk/errchk.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/talloc.h>
#include <atalk/dalloc.h>
#include <atalk/byteorder.h>
#include <atalk/netatalk_conf.h>
#include <atalk/volume.h>
#include <atalk/spotlight.h>

#include "directory.h"

static TALLOC_CTX *sl_ctx;
static void *sl_module;
static struct sl_module_export *sl_module_export;

/* Helper functions and stuff */
static const char *neststrings[] = {
    "",
    "\t",
    "\t\t",
    "\t\t\t",
    "\t\t\t\t",
    "\t\t\t\t\t",
    "\t\t\t\t\t\t",
};

static int dd_dump(DALLOC_CTX *dd, int nestinglevel)
{
    const char *type;

    LOG(log_debug, logtype_sl, "%s%s(#%d): {",
        neststrings[nestinglevel], talloc_get_name(dd), talloc_array_length(dd->dd_talloc_array));

    for (int n = 0; n < talloc_array_length(dd->dd_talloc_array); n++) {

        type = talloc_get_name(dd->dd_talloc_array[n]);

        if (STRCMP(type, ==, "DALLOC_CTX")
                   || STRCMP(type, ==, "sl_array_t")
                   || STRCMP(type, ==, "sl_filemeta_t")
                   || STRCMP(type, ==, "sl_dict_t")) {
            dd_dump(dd->dd_talloc_array[n], nestinglevel + 1);
        } else if (STRCMP(type, ==, "uint64_t")) {
            uint64_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(uint64_t));
            LOG(log_debug, logtype_sl, "%suint64_t: 0x%04x", neststrings[nestinglevel + 1], i);
        } else if (STRCMP(type, ==, "char *")) {
            LOG(log_debug, logtype_sl, "%sstring: %s", neststrings[nestinglevel + 1], (char *)dd->dd_talloc_array[n]);
        } else if (STRCMP(type, ==, "sl_bool_t")) {
            sl_bool_t bl;
            memcpy(&bl, dd->dd_talloc_array[n], sizeof(sl_bool_t));
            LOG(log_debug, logtype_sl, "%sbool: %s", neststrings[nestinglevel + 1], bl ? "true" : "false");
        } else if (STRCMP(type, ==, "sl_nil_t")) {
            LOG(log_debug, logtype_sl, "%snil", neststrings[nestinglevel + 1]);
        } else if (STRCMP(type, ==, "sl_time_t")) {
            sl_time_t t;
            struct tm *tm;
            char datestring[256];
            memcpy(&t, dd->dd_talloc_array[n], sizeof(sl_time_t));
            tm = localtime(&t.tv_sec);
            strftime(datestring, sizeof(datestring), "%Y-%m-%d %H:%M:%S", tm);
            LOG(log_debug, logtype_sl, "%ssl_time_t: %s.%06d", neststrings[nestinglevel + 1], datestring, t.tv_usec);
        } else if (STRCMP(type, ==, "sl_cnids_t")) {
            sl_cnids_t cnids;
            memcpy(&cnids, dd->dd_talloc_array[n], sizeof(sl_cnids_t));
            LOG(log_debug, logtype_sl, "%sCNIDs: unkn1: 0x%" PRIx16 ", unkn2: 0x%" PRIx32,
                   neststrings[nestinglevel + 1], cnids.ca_unkn1, cnids.ca_context);
            if (cnids.ca_cnids)
                dd_dump(cnids.ca_cnids, nestinglevel + 2);
        } else {
            LOG(log_debug, logtype_sl, "%stype: %s", neststrings[nestinglevel + 1], type);
        }
    }
    LOG(log_debug, logtype_sl, "%s}", neststrings[nestinglevel]);
}

#ifndef SPOT_TEST_MAIN
/**************************************************************************************************
 * Spotlight queries
 **************************************************************************************************/

static ATALK_LIST_HEAD(sl_queries);

/*!
 * Add a query to the list of active queries
 */
static int slq_add(slq_t *slq)
{
    list_add(&(slq->slq_list), &sl_queries);
    return 0;
}

static int slq_remove(slq_t *slq)
{
    EC_INIT;
    struct list_head *p;
    slq_t *q = NULL;

    list_for_each(p, &sl_queries) {
        q = list_entry(p, slq_t, slq_list);
        if ((q->slq_ctx1 == slq->slq_ctx1) && (q->slq_ctx2 == slq->slq_ctx2)) {            
            list_del(p);
            break;
        }
        q = NULL;
    }

    if (q == NULL) {
        /* The SL query 'slq' was not found in the list, this is not supposed to happen! */
        LOG(log_warning, logtype_sl, "slq_remove: slq not in active query list");
    }

EC_CLEANUP:
    EC_EXIT;
}

static slq_t *slq_for_ctx(uint64_t ctx1, uint64_t ctx2)
{
    EC_INIT;
    slq_t *q = NULL;
    struct list_head *p;

    list_for_each(p, &sl_queries) {
        q = list_entry(p, slq_t, slq_list);

        LOG(log_debug, logtype_sl, "slq_for_ctx(ctx1: 0x%" PRIx64 ", ctx2: 0x%" PRIx64
            "): active: ctx1: 0x%" PRIx64 ", ctx2: 0x%" PRIx64,
            ctx1, ctx2, q->slq_ctx1, q->slq_ctx2);

        if ((q->slq_ctx1 == ctx1) && (q->slq_ctx2 == ctx2)) {            
            break;
        }
        q = NULL;
    }

EC_CLEANUP:
    if (ret != 0)
        q = NULL;
    return q;
}

/* Error handling for queries */
static void slq_error(slq_t *slq)
{
    if (!slq)
        return;
    sl_module_export->sl_mod_error(slq);
    slq_remove(slq);
    talloc_free(slq);
}

/**************************************************************************************************
 * Spotlight RPC functions
 **************************************************************************************************/

static int sl_rpc_fetchPropertiesForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
{
    EC_INIT;

    char *s;
    sl_dict_t *dict;
    sl_array_t *array;
    sl_uuid_t uuid;

    if (!v->v_uuid)
        EC_FAIL_LOG("sl_rpc_fetchPropertiesForContext: missing UUID for volume: %s", v->v_localname);

    dict = talloc_zero(reply, sl_dict_t);

    /* key/val 1 */
    s = dalloc_strdup(dict, "kMDSStoreMetaScopes");
    dalloc_add(dict, s, char *);

    array = talloc_zero(dict, sl_array_t);
    s = dalloc_strdup(array, "kMDQueryScopeComputer");
    dalloc_add(array, s, char *);
    dalloc_add(dict, array, sl_array_t);

    /* key/val 2 */
    s = dalloc_strdup(dict, "kMDSStorePathScopes");
    dalloc_add(dict, s, char *);

    array = talloc_zero(dict, sl_array_t);
    s = dalloc_strdup(array, v->v_path);
    dalloc_add(array, s, char *);
    dalloc_add(dict, array, sl_array_t);

    /* key/val 3 */
    s = dalloc_strdup(dict, "kMDSStoreUUID");
    dalloc_add(dict, s, char *);

    memcpy(uuid.sl_uuid, v->v_uuid, 16);
    dalloc_add_copy(dict, &uuid, sl_uuid_t);

    /* key/val 4 */
    s = dalloc_strdup(dict, "kMDSStoreHasPersistentUUID");
    dalloc_add(dict, s, char *);
    sl_bool_t b = true;
    dalloc_add_copy(dict, &b, sl_bool_t);

    dalloc_add(reply, dict, sl_dict_t);

EC_CLEANUP:
    EC_EXIT;
}

static int cnid_comp_fn(const void *p1, const void *p2)
{
    const uint64_t *cnid1 = p1, *cnid2 = p2;
    if (*cnid1 == *cnid2)
        return 0;
    if (*cnid1 < *cnid2)
        return -1;
    else
        return 1;            
}

static int sl_createCNIDArray(slq_t *slq, const DALLOC_CTX *p)
{
    EC_INIT;
    uint64_t *cnids = NULL;

    EC_NULL( cnids = talloc_array(slq, uint64_t, talloc_array_length(p)) );
    for (int i = 0; i < talloc_array_length(p); i++)
        memcpy(&cnids[i], p->dd_talloc_array[i], sizeof(uint64_t));
    qsort(cnids, talloc_array_length(p), sizeof(uint64_t), cnid_comp_fn);

    slq->slq_cnids = cnids;
    slq->slq_cnids_num = talloc_array_length(p);

EC_CLEANUP:
    if (ret != 0) {
        if (cnids)
            talloc_free(cnids);
    }
    EC_EXIT;
}

static int sl_rpc_openQuery(AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, struct vol *v)
{
    EC_INIT;
    char *sl_query;
    uint64_t *uint64;
    DALLOC_CTX *reqinfo;
    sl_array_t *array;
    sl_cnids_t *cnids;
    slq_t *slq = NULL;

    /* Allocate and initialize query object */
    slq = talloc_zero(sl_ctx, slq_t);
    slq->slq_state = SLQ_STATE_NEW;
    slq->slq_obj = obj;
    slq->slq_vol = v;

    /* convert spotlight query charset to host charset */
    EC_NULL_LOG( sl_query = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1, "kMDQueryString") );
    char slq_host[MAXPATHLEN + 1];
    uint16_t convflags = v->v_mtou_flags;
    size_t slq_maclen;
    if (convert_charset(CH_UTF8_MAC, v->v_volcharset, v->v_maccharset, sl_query, strlen(sl_query), slq_host, MAXPATHLEN, &convflags) == -1) {
        LOG(log_error, logtype_afpd, "sl_rpc_openQuery(\"%s\"): charset conversion failed", sl_query);
        EC_FAIL;
    }
    slq->slq_qstring = talloc_strdup(slq, slq_host);
    LOG(log_debug, logtype_sl, "sl_rpc_openQuery: %s", slq->slq_qstring);

    slq->slq_time = time(NULL);
    EC_NULL_LOG( uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 1) );
    slq->slq_ctx1 = *uint64;
    EC_NULL_LOG( uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 2) );
    slq->slq_ctx2 = *uint64;
    EC_NULL_LOG( reqinfo = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1, "kMDAttributeArray") );
    slq->slq_reqinfo = talloc_steal(slq, reqinfo);
    if ((cnids = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1, "kMDQueryItemArray"))) {
        EC_ZERO_LOG( sl_createCNIDArray(slq, cnids->ca_cnids) );
    }
        
    LOG(log_maxdebug, logtype_sl, "sl_rpc_openQuery: requested attributes:");
    dd_dump(slq->slq_reqinfo, 0);

    (void)slq_add(slq);
    
    /* Run the query */
    EC_ZERO( sl_module_export->sl_mod_start_search(slq) );

    array = talloc_zero(reply, sl_array_t);
    uint64_t sl_res = ret == 0 ? 0 : UINT64_MAX;
    dalloc_add_copy(array, &sl_res, uint64_t);
    dalloc_add(reply, array, sl_array_t);

EC_CLEANUP:
    if (ret != 0) {
        slq_error(slq);
    }
    EC_EXIT;
}

static int sl_rpc_fetchQueryResultsForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
{
    EC_INIT;
    slq_t *slq = NULL;
    uint64_t *uint64, ctx1, ctx2;

    /* Context */
    EC_NULL_LOG (uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 1) );
    ctx1 = *uint64;
    EC_NULL_LOG (uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 2) );
    ctx2 = *uint64;

    /* Get query for context */
    EC_NULL_LOG( slq = slq_for_ctx(ctx1, ctx2) );
    if (slq->slq_state != SLQ_STATE_RUNNING && slq->slq_state != SLQ_STATE_DONE) {
        EC_FAIL_LOG("Spotlight: attempt to fetch results for query that isn't active");
    }

    /* Create and pass reply handle */
    EC_NULL( slq->slq_reply = talloc_zero(reply, sl_array_t) );

    /* Fetch Tracker results*/
    EC_ZERO_LOG( sl_module_export->sl_mod_fetch_result(slq) );

    dalloc_add(reply, slq->slq_reply, sl_array_t);

EC_CLEANUP:
    if (ret != 0) {
        slq_error(slq);
    }
    EC_EXIT;
}

static int sl_rpc_storeAttributesForOIDArray(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
{
    EC_INIT;
    uint64_t uint64;
    sl_array_t *array;
    sl_cnids_t *cnids;
    sl_time_t *sl_time;
    cnid_t id;
    char *path;
    struct dir *dir;
    
    EC_NULL_LOG( cnids = dalloc_get(query, "DALLOC_CTX", 0, "sl_cnids_t", 2) );
    memcpy(&uint64, cnids->ca_cnids->dd_talloc_array[0], sizeof(uint64_t));
    id = (cnid_t)uint64;
    LOG(log_debug, logtype_sl, "sl_rpc_storeAttributesForOIDArray: CNID: %" PRIu32, id);

    if (htonl(id) == DIRDID_ROOT) {
        path = vol->v_path;
    } else if (id < CNID_START) {
        EC_FAIL;
    } else {
        cnid_t did;
        char buffer[12 + MAXPATHLEN + 1];

        did = htonl(id);
        EC_NULL_LOG( path = cnid_resolve(vol->v_cdb, &did, buffer, sizeof(buffer)) );
        EC_NULL_LOG( dir = dirlookup(vol, did) );
        EC_NEG1_LOG( movecwd(vol, dir) );
    }

    if ((sl_time = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1, "DALLOC_CTX", 1, "kMDItemLastUsedDate"))) {
        struct utimbuf utimes;
        utimes.actime = utimes.modtime = sl_time->tv_sec;
        utime(path, &utimes);
    }

    array = talloc_zero(reply, sl_array_t);
    uint64_t sl_res = 0;
    dalloc_add_copy(array, &sl_res, uint64_t);
    dalloc_add(reply, array, sl_array_t);

EC_CLEANUP:
    EC_EXIT;
}

static int sl_rpc_fetchAttributeNamesForOIDArray(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
{
    EC_INIT;
    uint64_t uint64;
    sl_cnids_t *cnids;
    sl_time_t *sl_time;
    cnid_t id;
    char *path;
    struct dir *dir;
    
    EC_NULL_LOG( cnids = dalloc_get(query, "DALLOC_CTX", 0, "sl_cnids_t", 1) );
    memcpy(&uint64, cnids->ca_cnids->dd_talloc_array[0], sizeof(uint64_t));
    id = (cnid_t)uint64;
    LOG(log_debug, logtype_sl, "sl_rpc_fetchAttributeNamesForOIDArray: CNID: %" PRIu32, id);

    if (htonl(id) == DIRDID_ROOT) {
        path = vol->v_path;
    } else if (id < CNID_START) {
        EC_FAIL;
    } else {
        cnid_t did;
        char buffer[12 + MAXPATHLEN + 1];

        did = htonl(id);
        EC_NULL_LOG( path = cnid_resolve(vol->v_cdb, &did, buffer, sizeof(buffer)) );
        EC_NULL_LOG( dir = dirlookup(vol, did) );
        EC_NEG1_LOG( movecwd(vol, dir) );
    }

    /* Result array */
    sl_array_t *array = talloc_zero(reply, sl_array_t);
    dalloc_add(reply, array, sl_array_t);

    /* Return result value 0 */
    uint64_t sl_res = 0;
    dalloc_add_copy(array, &sl_res, uint64_t);

    /* Return CNID array */
    sl_cnids_t *replycnids = talloc_zero(reply, sl_cnids_t);
    replycnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);
    replycnids->ca_unkn1 = 0xfec;
    replycnids->ca_context = cnids->ca_context;
    uint64 = (uint64_t)id;
    dalloc_add_copy(replycnids->ca_cnids, &uint64, uint64_t);
    dalloc_add(array, replycnids, sl_cnids_t);

    /* Return filemeta array */

    /*
     * FIXME: this should return the real attributes from all known metadata sources
     * (Tracker and filesystem)
     */
    sl_array_t *mdattrs = talloc_zero(reply, sl_array_t);
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSName"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemDisplayName"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSSize"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSOwnerUserID"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSOwnerGroupID"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSContentChangeDate"), "char *");

    sl_filemeta_t *fmeta = talloc_zero(reply, sl_filemeta_t);
    dalloc_add(fmeta, mdattrs, sl_array_t);
    dalloc_add(array, fmeta, sl_filemeta_t);

EC_CLEANUP:
    EC_EXIT;
}

static int sl_rpc_fetchAttributesForOIDArray(AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
{
    EC_INIT;
    slq_t *slq = NULL;
    uint64_t uint64;
    sl_cnids_t *cnids;
    sl_time_t *sl_time;
    cnid_t id;
    struct dir *dir;
    sl_array_t *reqinfo;



    /* Allocate and initialize query object */
    slq = talloc_zero(reply, slq_t);
    slq->slq_state = SLQ_STATE_ATTRS;
    slq->slq_obj = obj;
    slq->slq_vol = vol;
    EC_NULL( slq->slq_reply = talloc_zero(reply, sl_array_t) );
    EC_NULL( reqinfo = dalloc_get(query, "DALLOC_CTX", 0, "sl_array_t", 1) );
    slq->slq_reqinfo = talloc_steal(slq, reqinfo);
    
    EC_NULL_LOG( cnids = dalloc_get(query, "DALLOC_CTX", 0, "sl_cnids_t", 2) );
    memcpy(&uint64, cnids->ca_cnids->dd_talloc_array[0], sizeof(uint64_t));
    id = (cnid_t)uint64;

    if (htonl(id) == DIRDID_ROOT) {
        slq->slq_path = talloc_strdup(slq, vol->v_path);
    } else if (id < CNID_START) {
        EC_FAIL;
    } else {
        cnid_t did;
        char buffer[12 + MAXPATHLEN + 1];
        char *name;
        did = htonl(id);
        EC_NULL( name = cnid_resolve(vol->v_cdb, &did, buffer, sizeof(buffer)) );
        EC_NULL( dir = dirlookup(vol, did) );
        EC_NULL( slq->slq_path = talloc_asprintf(slq, "%s/%s", bdata(dir->d_fullpath), name) );
    }

    /* Return result value 0 */
    uint64_t sl_res = 0;
    dalloc_add_copy(slq->slq_reply, &sl_res, uint64_t);

    /* Return CNID array */
    sl_cnids_t *replycnids = talloc_zero(reply, sl_cnids_t);
    replycnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);
    replycnids->ca_unkn1 = 0xfec;
    replycnids->ca_context = cnids->ca_context;
    uint64 = (uint64_t)id;
    dalloc_add_copy(replycnids->ca_cnids, &uint64, uint64_t);
    dalloc_add(slq->slq_reply, replycnids, sl_cnids_t);

    /* Fetch attributes from module */
    EC_ZERO_LOG( sl_module_export->sl_mod_fetch_attrs(slq) );

    dalloc_add(reply, slq->slq_reply, sl_array_t);

EC_CLEANUP:
    EC_EXIT;
}

static int sl_rpc_closeQueryForContext(const AFPObj *obj, const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *v)
{
    EC_INIT;
    slq_t *slq = NULL;
    uint64_t *uint64, ctx1, ctx2;
    sl_array_t *array;
    
    /* Context */
    EC_NULL_LOG (uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 1) );
    ctx1 = *uint64;
    EC_NULL_LOG (uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 2) );
    ctx2 = *uint64;

    /* Get query for context and free it */
    EC_NULL_LOG( slq = slq_for_ctx(ctx1, ctx2) );
    if (slq->slq_state != SLQ_STATE_DONE)
        LOG(log_warning, logtype_sl, "Closing active query");
    sl_module_export->sl_mod_end_search(slq);
    slq_remove(slq);
    talloc_free(slq);
    slq = NULL;

    array = talloc_zero(reply, sl_array_t);
    uint64_t sl_res = 0;
    dalloc_add_copy(array, &sl_res, uint64_t);
    dalloc_add(reply, array, sl_array_t);

EC_CLEANUP:
    if (ret != 0) {
        slq_error(slq);
    }
    EC_EXIT;
}

/**************************************************************************************************
 * Spotlight module functions
 **************************************************************************************************/

int sl_mod_load(const char *path)
{
    EC_INIT;

    sl_ctx = talloc_new(NULL);

    if ((sl_module = mod_open(path)) == NULL) {
        LOG(log_error, logtype_sl, "Failed to load module \'%s\': %s", path, mod_error());
        EC_FAIL;
    }

    if ((sl_module_export = mod_symbol(sl_module, "sl_mod")) == NULL) {
        LOG(log_error, logtype_sl, "sl_mod_load(%s): mod_symbol error for symbol %s", path, "sl_mod");
        EC_FAIL;
    }

    sl_module_export->sl_mod_init("test");
   
EC_CLEANUP:
    EC_EXIT;
}

/**
 * Index a file
 **/
void sl_index_file(const char *path)
{
    if (sl_module_export && sl_module_export->sl_mod_index_file)
        sl_module_export->sl_mod_index_file(path);
}

/**************************************************************************************************
 * AFP functions
 **************************************************************************************************/

int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    EC_INIT;
    TALLOC_CTX *tmp_ctx = talloc_new(NULL);
    uint16_t vid;
    int cmd;
    int endianess = SL_ENC_LITTLE_ENDIAN;
    struct vol      *vol;
    DALLOC_CTX *query;
    DALLOC_CTX *reply;
    char *rpccmd;
    int len;

    *rbuflen = 0;

    if (sl_module == NULL)
        return AFPERR_NOOP;

    ibuf += 2;
    ibuflen -= 2;

    vid = SVAL(ibuf, 0);
    LOG(log_debug, logtype_sl, "afp_spotlight_rpc(vid: %" PRIu16 ")", vid);

    if ((vol = getvolbyvid(vid)) == NULL) {
        LOG(log_error, logtype_sl, "afp_spotlight_rpc: bad volume id: %" PRIu16 ")", vid);
        ret = AFPERR_ACCESS;
        goto EC_CLEANUP;
    }

    /*    IVAL(ibuf, 2): unknown, always 0x00008004, some flags ? */

    cmd = RIVAL(ibuf, 6);
    LOG(log_debug, logtype_sl, "afp_spotlight_rpc(cmd: %d)", cmd);

    /*    IVAL(ibuf, 10: unknown, always 0x00000000 */

    switch (cmd) {

    case SPOTLIGHT_CMD_OPEN:
    case SPOTLIGHT_CMD_OPEN2:
        RSIVAL(rbuf, 0, ntohs(vid));
        RSIVAL(rbuf, 4, 0);
        len = strlen(vol->v_path) + 1;
        strncpy(rbuf + 8, vol->v_path, len);
        *rbuflen += 8 + len;
        break;

    case SPOTLIGHT_CMD_FLAGS:
        RSIVAL(rbuf, 0, 0x0100006b); /* Whatever this value means... flags? Helios uses 0x1eefface */
        *rbuflen += 4;
        break;

    case SPOTLIGHT_CMD_RPC:
        EC_NULL( query = talloc_zero(tmp_ctx, DALLOC_CTX) );
        EC_NULL( reply = talloc_zero(tmp_ctx, DALLOC_CTX) );
        EC_NEG1_LOG( sl_unpack(query, ibuf + 22) );

        LOG(log_debug, logtype_sl, "afp_spotlight_rpc: Request dump:");
        dd_dump(query, 0);

        EC_NULL_LOG( rpccmd = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "char *", 0) );

        if (STRCMP(rpccmd, ==, "fetchPropertiesForContext:")) {
            EC_ZERO_LOG( sl_rpc_fetchPropertiesForContext(obj, query, reply, vol) );
        } else if (STRCMP(rpccmd, ==, "openQueryWithParams:forContext:")) {
            EC_ZERO_LOG( sl_rpc_openQuery(obj, query, reply, vol) );
        } else if (STRCMP(rpccmd, ==, "fetchQueryResultsForContext:")) {
            EC_ZERO_LOG( sl_rpc_fetchQueryResultsForContext(obj, query, reply, vol) );
        } else if (STRCMP(rpccmd, ==, "storeAttributes:forOIDArray:context:")) {
            EC_ZERO_LOG( sl_rpc_storeAttributesForOIDArray(obj, query, reply, vol) );
        } else if (STRCMP(rpccmd, ==, "fetchAttributeNamesForOIDArray:context:")) {
            EC_ZERO_LOG( sl_rpc_fetchAttributeNamesForOIDArray(obj, query, reply, vol) );
        } else if (STRCMP(rpccmd, ==, "fetchAttributes:forOIDArray:context:")) {
            EC_ZERO_LOG( sl_rpc_fetchAttributesForOIDArray(obj, query, reply, vol) );
        } else if (STRCMP(rpccmd, ==, "closeQueryForContext:")) {
            EC_ZERO_LOG( sl_rpc_closeQueryForContext(obj, query, reply, vol) );
        } else {
            LOG(log_error, logtype_sl, "afp_spotlight_rpc: unknown Spotlight RPC: %s", rpccmd);
        }

        LOG(log_debug, logtype_sl, "afp_spotlight_rpc: Reply dump:");
        dd_dump(reply, 0);

        memset(rbuf, 0, 4);
        *rbuflen += 4;

        EC_NEG1_LOG( len = sl_pack(reply, rbuf + 4) );
        *rbuflen += len;
        break;
    }

EC_CLEANUP:
    talloc_free(tmp_ctx);
    if (ret != AFP_OK) {
        *rbuflen = 0;
        return AFPERR_MISC;
    }
    EC_EXIT;
}
#endif

/**************************************************************************************************
 * Testing
 **************************************************************************************************/

#ifdef SPOT_TEST_MAIN

int main(int argc, char **argv)
{
    EC_INIT;
    TALLOC_CTX *mem_ctx = talloc_new(NULL);
    DALLOC_CTX *dd = talloc_zero(mem_ctx, DALLOC_CTX);
    uint64_t i;

    set_processname("spot");
    setuplog("default:info,spotlight:debug", "/dev/tty");

    LOG(log_info, logtype_sl, "Start");

    i = 1;
    dalloc_add_copy(dd, &i, uint64_t);
    char *str = dalloc_strdup(dd, "hello world");
    dalloc_add(dd, str, char *);
    sl_bool_t b = true;
    dalloc_add_copy(dd, &b, sl_bool_t);

    /* add a nested array */
    DALLOC_CTX *nested = talloc_zero(dd, DALLOC_CTX);
    i = 3;
    dalloc_add_copy(nested, &i, uint64_t);
    dalloc_add(dd, nested, DALLOC_CTX);

    /* test an allocated CNID array */
    uint64_t id = 16;
    sl_cnids_t *cnids = talloc_zero(dd, sl_cnids_t);
    cnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);
    cnids->ca_unkn1 = 1;
    dalloc_add_copy(cnids->ca_cnids, &id, uint64_t);
    dalloc_add(dd, cnids, sl_cnids_t);

    /* Now the Spotlight types */
    sl_array_t *sl_array = talloc_zero(dd, sl_array_t);
    i = 0x1234;
    dalloc_add_copy(sl_array, &i, uint64_t);

    sl_dict_t *sl_dict = talloc_zero(dd, sl_dict_t);
    i = 0xffff;
    dalloc_add_copy(sl_dict, &i, uint64_t);
    dalloc_add(sl_array, sl_dict, sl_dict_t);

    dalloc_add(dd, sl_array, sl_array_t);

    dd_dump(dd, 0);

    /* now parse a real spotlight packet */
    if (argc > 1) {
        char ibuf[8192];
        char rbuf[8192];
        int fd;
        size_t len;
        DALLOC_CTX *query;

        EC_NULL( query = talloc_zero(mem_ctx, DALLOC_CTX) );

        EC_NEG1_LOG( fd = open(argv[1], O_RDONLY) );
        EC_NEG1_LOG( len = read(fd, ibuf, 8192) );
        close(fd);
        EC_NEG1_LOG( sl_unpack(query, ibuf + 24) );

        /* Now dump the whole thing */
        dd_dump(query, 0);
    }

#if 0
    /* packing  */
    int qlen;
    char buf[MAX_SLQ_DAT];
    EC_NEG1_LOG( qlen = sl_pack(query, buf) );

    EC_NEG1_LOG( fd = open("test.bin", O_RDWR) );
    lseek(fd, 24, SEEK_SET);
    write(fd, buf, qlen);
    close(fd);
#endif

EC_CLEANUP:
    if (mem_ctx) {
        talloc_free(mem_ctx);
        mem_ctx = NULL;
    }
    EC_EXIT;
}
#endif
