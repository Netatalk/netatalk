/*
  Copyright (c) 2012-2014 Ralph Boehme

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

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <utime.h>

#include <talloc.h>

#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH
#include <glib.h>
#endif

#include <atalk/byteorder.h>
#include <atalk/compat.h>
#include <atalk/dalloc.h>
#include <atalk/errchk.h>
#include <atalk/iniparser_util.h>
#include <atalk/list.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/spotlight.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "directory.h"
#include "etc/spotlight/spotlight_private.h"

#define MAX_SL_RESULTS 20
#define MAX_SL_QUERY_IDLE_TIME_ACTIVE 60
#define MAX_SL_QUERY_IDLE_TIME_TERMINAL 300

struct slq_state_names {
    slq_state_t state;
    const char *state_name;
};

static struct slq_state_names slq_state_names[] = {
    {SLQ_STATE_NEW, "SLQ_STATE_NEW"},
    {SLQ_STATE_RUNNING, "SLQ_STATE_RUNNING"},
    {SLQ_STATE_RESULTS, "SLQ_STATE_RESULTS"},
    {SLQ_STATE_FULL, "SLQ_STATE_FULL"},
    {SLQ_STATE_DONE, "SLQ_STATE_DONE"},
    {SLQ_STATE_CANCEL_PENDING, "SLQ_STATE_CANCEL_PENDING"},
    {SLQ_STATE_CANCELLED, "SLQ_STATE_CANCELLED"},
    {SLQ_STATE_ERROR, "SLQ_STATE_ERROR"}
};

static int cnid_comp_fn(const void *p1, const void *p2);
static bool create_result_handle(slq_t *slq);

/************************************************
 * Misc utility functions
 ************************************************/

static char *tab_level(TALLOC_CTX *mem_ctx, int level)
{
    int i;
    char *string = talloc_array(mem_ctx, char, level + 1);

    for (i = 0; i < level; i++) {
        string[i] = '\t';
    }

    string[i] = '\0';
    return string;
}

static char *dd_dump(DALLOC_CTX *dd, int nestinglevel)
{
    const char *type;
    uint64_t i;
    sl_bool_t bl;
    sl_time_t t;
    const struct tm *tm;
    char datestring[256];
    sl_cnids_t cnids;
    char *logstring, *nested_logstring;
    char *tab_string1, *tab_string2;
    tab_string1 = tab_level(dd, nestinglevel);
    tab_string2 = tab_level(dd, nestinglevel + 1);

    if (tab_string1 == NULL || tab_string2 == NULL) {
        return NULL;
    }

    logstring = talloc_asprintf(dd,
                                "%s%s(#%lu): {\n",
                                tab_string1,
                                talloc_get_name(dd),
                                talloc_array_length(dd->dd_talloc_array));

    for (int n = 0; n < talloc_array_length(dd->dd_talloc_array); n++) {
        type = talloc_get_name(dd->dd_talloc_array[n]);

        if (strequal(type, "DALLOC_CTX")
                || strequal(type, "sl_array_t")
                || strequal(type, "sl_filemeta_t")
                || strequal(type, "sl_dict_t")) {
            nested_logstring = dd_dump(dd->dd_talloc_array[n],
                                       nestinglevel + 1);

            if (!nested_logstring) {
                return NULL;
            }

            logstring = talloc_strdup_append(logstring,
                                             nested_logstring);

            if (!logstring) {
                return NULL;
            }
        } else if (strequal(type, "uint64_t")) {
            memcpy(&i, dd->dd_talloc_array[n], sizeof(uint64_t));
            logstring = talloc_asprintf_append(
                            logstring,
                            "%suint64_t: 0x%04" PRIx64 "\n",
                            tab_string2, i);

            if (!logstring) {
                return NULL;
            }
        } else if (strequal(type, "char *")) {
            logstring = talloc_asprintf_append(
                            logstring,
                            "%sstring: %s\n",
                            tab_string2,
                            (char *)dd->dd_talloc_array[n]);

            if (!logstring) {
                return NULL;
            }
        } else if (strequal(type, "smb_ucs2_t *")) {
            logstring = talloc_asprintf_append(
                            logstring,
                            "%sUTF16-string: %s\n",
                            tab_string2,
                            (char *)dd->dd_talloc_array[n]);

            if (!logstring) {
                return NULL;
            }
        } else if (strequal(type, "sl_bool_t")) {
            memcpy(&bl, dd->dd_talloc_array[n], sizeof(sl_bool_t));
            logstring = talloc_asprintf_append(
                            logstring,
                            "%sbool: %s\n",
                            tab_string2,
                            bl ? "true" : "false");

            if (!logstring) {
                return NULL;
            }
        } else if (strequal(type, "sl_nil_t")) {
            logstring = talloc_asprintf_append(
                            logstring,
                            "%snil\n",
                            tab_string2);

            if (!logstring) {
                return NULL;
            }
        } else if (strequal(type, "sl_time_t")) {
            memcpy(&t, dd->dd_talloc_array[n], sizeof(sl_time_t));
            tm = localtime(&t.tv_sec);
            strftime(datestring,
                     sizeof(datestring),
                     "%Y-%m-%d %H:%M:%S", tm);
            logstring = talloc_asprintf_append(
                            logstring,
                            "%ssl_time_t: %s.%06lu\n",
                            tab_string2,
                            datestring,
                            (unsigned long)t.tv_usec);

            if (!logstring) {
                return NULL;
            }
        } else if (strequal(type, "sl_cnids_t")) {
            memcpy(&cnids, dd->dd_talloc_array[n], sizeof(sl_cnids_t));
            logstring = talloc_asprintf_append(
                            logstring,
                            "%sCNIDs: unkn1: 0x%" PRIx16 ", unkn2: 0x%" PRIx32 "\n",
                            tab_string2,
                            cnids.ca_unkn1,
                            cnids.ca_context);

            if (!logstring) {
                return NULL;
            }

            if (cnids.ca_cnids) {
                nested_logstring = dd_dump(
                                       cnids.ca_cnids,
                                       nestinglevel + 2);

                if (!nested_logstring) {
                    return NULL;
                }

                logstring = talloc_strdup_append(logstring,
                                                 nested_logstring);

                if (!logstring) {
                    return NULL;
                }
            }
        } else {
            logstring = talloc_asprintf_append(
                            logstring,
                            "%stype: %s\n",
                            tab_string2,
                            type);

            if (!logstring) {
                return NULL;
            }
        }
    }

    logstring = talloc_asprintf_append(logstring,
                                       "%s}\n",
                                       tab_string1);

    if (!logstring) {
        return NULL;
    }

    return logstring;
}

static int cnid_comp_fn(const void *p1, const void *p2)
{
    const uint64_t *cnid1 = p1, *cnid2 = p2;

    if (*cnid1 == *cnid2) {
        return 0;
    }

    if (*cnid1 < *cnid2) {
        return -1;
    }

    return 1;
}

static int sl_createCNIDArray(slq_t *slq, const DALLOC_CTX *p)
{
    EC_INIT;
    uint64_t *cnids = NULL;
    EC_NULL(cnids = talloc_array(slq, uint64_t, talloc_array_length(p)));

    for (int i = 0; i < talloc_array_length(p); i++) {
        memcpy(&cnids[i], p->dd_talloc_array[i], sizeof(uint64_t));
    }

    qsort(cnids, talloc_array_length(p), sizeof(uint64_t), cnid_comp_fn);
    slq->slq_cnids = cnids;
    slq->slq_cnids_num = talloc_array_length(p);
EC_CLEANUP:

    if (ret != 0 && cnids) {
        talloc_free(cnids);
    }

    EC_EXIT;
}

/*!
 * @brief Add requested metadata for a query result element
 *
 * This could be rewritten to something more sophisticated like
 * querying metadata from Tracker.
 *
 * If path or sp is NULL, simply add nil values for all attributes.
 */
bool add_filemeta(sl_array_t *reqinfo,
                  sl_array_t *fm_array,
                  const char *path,
                  const struct stat *sp)
{
    sl_array_t *meta;
    sl_nil_t nil;
    size_t metacount;
    uint64_t uint64var;
    sl_time_t sl_time;
    const char *p;
    char *name;
    metacount = talloc_array_length(reqinfo->dd_talloc_array);

    if (metacount == 0 || path == NULL || sp == NULL) {
        dalloc_add_copy(fm_array, &nil, sl_nil_t);
        return true;
    }

    meta = talloc_zero(fm_array, sl_array_t);
    /*
     * macOS expects filenames and paths in NFD (decomposed) form.
     * Convert the NFC server-side path to NFD before returning string
     * attributes. Fall back to the original path if conversion fails.
     */
    size_t path_len = strnlen(path, MAXPATHLEN);
    size_t nfd_buf_size = path_len * 3 + 1;
    char *nfd_path = talloc_array(meta, char, nfd_buf_size);

    if (nfd_path == NULL
            || charset_decompose(CH_UTF8, (char *)path, path_len,
                                 nfd_path, nfd_buf_size) == (size_t) -1) {
        nfd_path = (char *)path;
    }

    for (size_t i = 0; i < metacount; i++) {
        if (strequal(reqinfo->dd_talloc_array[i], "kMDItemDisplayName")
                || strequal(reqinfo->dd_talloc_array[i], "kMDItemFSName")
                || strequal(reqinfo->dd_talloc_array[i], "_kMDItemFileName")) {
            if ((p = strrchr(nfd_path, '/'))) {
                name = dalloc_strdup(meta, p + 1);
                dalloc_add(meta, name, "char *");
            }
        } else if (strequal(reqinfo->dd_talloc_array[i],
                            "kMDItemPath")) {
            name = dalloc_strdup(meta, nfd_path);
            dalloc_add(meta, name, "char *");
        } else if (strequal(reqinfo->dd_talloc_array[i], "kMDItemFSSize")
                   || strequal(reqinfo->dd_talloc_array[i], "kMDItemLogicalSize")) {
            uint64var = sp->st_size;
            dalloc_add_copy(meta, &uint64var, uint64_t);
        } else if (strequal(reqinfo->dd_talloc_array[i],
                            "kMDItemFSOwnerUserID")) {
            uint64var = sp->st_uid;
            dalloc_add_copy(meta, &uint64var, uint64_t);
        } else if (strequal(reqinfo->dd_talloc_array[i],
                            "kMDItemFSOwnerGroupID")) {
            uint64var = sp->st_gid;
            dalloc_add_copy(meta, &uint64var, uint64_t);
        } else if (strequal(reqinfo->dd_talloc_array[i],
                            "kMDItemFSContentChangeDate")
                   || strequal(reqinfo->dd_talloc_array[i],
                               "kMDItemContentModificationDate")) {
            struct timespec ts = atalk_stat_mtime_timespec(sp);
            atalk_timespec_to_timeval(&sl_time, &ts);
            dalloc_add_copy(meta, &sl_time, sl_time_t);
        } else if (strequal(reqinfo->dd_talloc_array[i],
                            "kMDItemLastUsedDate")) {
            struct timespec ts = atalk_stat_atime_timespec(sp);
            atalk_timespec_to_timeval(&sl_time, &ts);
            dalloc_add_copy(meta, &sl_time, sl_time_t);
        } else if (strequal(reqinfo->dd_talloc_array[i],
                            "kMDItemContentCreationDate")) {
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC
            struct timespec ts = sp->st_birthtimespec;
            atalk_timespec_to_timeval(&sl_time, &ts);
            dalloc_add_copy(meta, &sl_time, sl_time_t);
#else
            dalloc_add_copy(meta, &nil, sl_nil_t);
#endif
        } else {
            dalloc_add_copy(meta, &nil, sl_nil_t);
        }
    }

    dalloc_add(fm_array, meta, sl_array_t);
    return true;
}

/*!
 * Allocate result handle used in the async search backend result
 * handler for storing results
 */
static bool create_result_handle(slq_t *slq)
{
    sl_nil_t nil = 0;
    struct sl_rslts *query_results;

    if (slq->query_results) {
        LOG(log_error, logtype_sl, "unexpected existing result handle");
        return false;
    }

    query_results = talloc_zero(slq, struct sl_rslts);
    /* CNIDs */
    query_results->cnids = talloc_zero(query_results, sl_cnids_t);

    if (query_results->cnids == NULL) {
        return false;
    }

    query_results->cnids->ca_cnids = talloc_zero(query_results->cnids,
                                     DALLOC_CTX);

    if (query_results->cnids->ca_cnids == NULL) {
        return false;
    }

    query_results->cnids->ca_unkn1 = 0xadd;
    query_results->cnids->ca_context = (uint32_t)slq->slq_ctx2;
    /* FileMeta */
    query_results->fm_array = talloc_zero(query_results, sl_array_t);

    if (query_results->fm_array == NULL) {
        return false;
    }

    /* For some reason the list of results always starts with a nil entry */
    dalloc_add_copy(query_results->fm_array, &nil, sl_nil_t);
    slq->query_results = query_results;
    return true;
}

static bool add_results(sl_array_t *array, slq_t *slq)
{
    sl_filemeta_t *fm;
    uint64_t status;
    /* FileMeta */
    fm = talloc_zero(array, sl_filemeta_t);

    if (!fm) {
        return false;
    }

    /*
     * Status 35 (0x23) signals "results pending, poll again".
     * All states prior to DONE (RUNNING, RESULTS, FULL) return 0x23;
     * DONE and beyond return 0.
     */
    if (slq->slq_state >= SLQ_STATE_DONE) {
        status = 0;
    } else {
        status = 35;
    }

    LOG(log_debug, logtype_sl,
        "add_results: dispatching %d result(s), status %" PRIu64
        ", state=%s (ctx1: %" PRIx64 ", ctx2: %" PRIx64 ")",
        slq->query_results->num_results, status,
        slq_state_names[slq->slq_state].state_name,
        slq->slq_ctx1, slq->slq_ctx2);
    dalloc_add_copy(array, &status, uint64_t);
    dalloc_add(array, slq->query_results->cnids, sl_cnids_t);

    if (slq->query_results->num_results > 0) {
        dalloc_add(fm, slq->query_results->fm_array, sl_array_t);
    }

    dalloc_add(array, fm, sl_filemeta_t);
    /* This ensure the results get clean up after been sent to the client */
    talloc_steal(array, slq->query_results);
    slq->query_results = NULL;

    if (!create_result_handle(slq)) {
        LOG(log_error, logtype_sl, "couldn't add result handle");
        slq->slq_state = SLQ_STATE_ERROR;
        return false;
    }

    return true;
}

/******************************************************************************
 * Spotlight queries
 ******************************************************************************/

static ATALK_LIST_HEAD(sl_queries);
static ATALK_LIST_HEAD(sl_cancelled_queries);

/*!
 * Add a query to the list of active queries
 */
static void slq_add(slq_t *slq)
{
    list_add(&(slq->slq_list), &sl_queries);
}

/*!
 * Add a query to the list of cancelled queries
 */
static void slq_cancelled_add(slq_t *slq)
{
    list_add(&(slq->slq_list), &sl_cancelled_queries);
}

/*!
 * Remove a query from the active list
 *
 * Uses pointer identity rather than ctx-value comparison so that a query
 * that was never enqueued (e.g. openQuery failure before slq_add()) cannot
 * accidentally remove an unrelated active query whose ctx IDs happen to
 * match the zero-initialized or partially-parsed values.
 */
static void slq_remove(slq_t *slq)
{
    struct list_head *p;
    const slq_t *q = NULL;
    list_for_each(p, &sl_queries) {
        q = list_entry(p, slq_t, slq_list);

        if (q == slq) {
            list_del(p);
            break;
        }
    }
    return;
}

static slq_t *slq_for_ctx(uint64_t ctx1, uint64_t ctx2)
{
    slq_t *q = NULL;
    struct list_head *p;
    list_for_each(p, &sl_queries) {
        q = list_entry(p, slq_t, slq_list);

        if ((q->slq_ctx1 == ctx1) && (q->slq_ctx2 == ctx2)) {
            break;
        }

        q = NULL;
    }
    return q;
}

/*!
 * Invoke the backend close_query hook then free the slq
 */
static void slq_destroy(slq_t *slq)
{
    if (slq == NULL) {
        return;
    }

    if (slq->slq_vol && slq->slq_vol->v_sl_backend
            && slq->slq_vol->v_sl_backend->sbo_close_query) {
        slq->slq_vol->v_sl_backend->sbo_close_query(slq);
    }

    slq_remove(slq);
    talloc_free(slq);
}

/*!
 * Cancel a query (move to cancelled list; backend cleanup deferred)
 */
static void slq_cancel(slq_t *slq)
{
    slq->slq_state = SLQ_STATE_CANCEL_PENDING;
    slq_remove(slq);
    slq_cancelled_add(slq);
}

/*!
 * Free all fully-cancelled queries
 */
static void slq_cancelled_cleanup(void)
{
    slq_t *q = NULL;
    struct list_head *p = sl_cancelled_queries.next;

    while (p != &sl_cancelled_queries) {
        struct list_head *next = p->next;
        q = list_entry(p, slq_t, slq_list);

        if (q->slq_state == SLQ_STATE_CANCELLED) {
            LOG(log_debug, logtype_sl,
                "ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": cancelled",
                q->slq_ctx1, q->slq_ctx2);

            if (q->slq_vol && q->slq_vol->v_sl_backend
                    && q->slq_vol->v_sl_backend->sbo_close_query) {
                q->slq_vol->v_sl_backend->sbo_close_query(q);
            }

            list_del(p);
            talloc_free(q);
        } else {
            LOG(log_debug, logtype_sl,
                "ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": %s",
                q->slq_ctx1, q->slq_ctx2,
                slq_state_names[q->slq_state].state_name);
        }

        p = next;
    }

    return;
}

/*!
 * Cancel or destroy queries idle longer than MAX_SL_QUERY_IDLE_TIME
 */
static void slq_idle_cleanup(void)
{
    struct list_head *p;
    slq_t *q = NULL;
    time_t now = time(NULL);
    time_t idle_limit;
    bool found;

    do {
        found = false;
        list_for_each(p, &sl_queries) {
            q = list_entry(p, slq_t, slq_list);

            switch (q->slq_state) {
            case SLQ_STATE_DONE:
            case SLQ_STATE_FULL:
            case SLQ_STATE_ERROR:
                idle_limit = MAX_SL_QUERY_IDLE_TIME_TERMINAL;
                break;

            default:
                idle_limit = MAX_SL_QUERY_IDLE_TIME_ACTIVE;
                break;
            }

            if (q->slq_time > now || now - q->slq_time < idle_limit) {
                continue;
            }

            LOG(log_debug, logtype_sl,
                "ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": idle timeout, state: %s",
                q->slq_ctx1, q->slq_ctx2,
                slq_state_names[q->slq_state].state_name);

            switch (q->slq_state) {
            case SLQ_STATE_DONE:
            case SLQ_STATE_FULL:
            case SLQ_STATE_ERROR:
                slq_destroy(q);
                break;

            case SLQ_STATE_RUNNING:
            case SLQ_STATE_RESULTS:
                slq_cancel(q);
                break;

            default:
                break;
            }

            found = true;
            break;
        }
    } while (found);
}

static void slq_dump(void)
{
    struct list_head *p;
    const slq_t *q = NULL;
    int i = 0;
    list_for_each(p, &sl_queries) {
        q = list_entry(p, slq_t, slq_list);
        LOG(log_debug, logtype_sl,
            "query[%d]: ctx1: %" PRIx64 ", ctx2: %" PRIx64 ", state: %s",
            i++, q->slq_ctx1, q->slq_ctx2,
            slq_state_names[q->slq_state].state_name);
    }
    return;
}

/*!
 * Return true if any currently-open volume uses the localsearch backend
 */
#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH
static bool localsearch_backend_active(const AFPObj *obj)
{
    const struct vol *v;

    for (v = getvolumes(); v != NULL; v = v->v_next) {
        if (v->v_sl_backend != NULL
                && strcmp(v->v_sl_backend->sbo_name, "localsearch") == 0) {
            return true;
        }
    }

    return false;
}
#endif /* SPOTLIGHT_BACKEND_LOCALSEARCH */

/*******************************************************************************
 * Spotlight RPC functions
 ******************************************************************************/

static int sl_rpc_fetchPropertiesForContext(const AFPObj *obj _U_,
        const DALLOC_CTX *query _U_,
        DALLOC_CTX *reply,
        const struct vol *v)
{
    EC_INIT;
    char *s;
    sl_dict_t *dict;
    sl_array_t *array;
    sl_uuid_t uuid;

    if (!v->v_uuid) {
        EC_FAIL_LOG("missing UUID for volume: %s", v->v_localname);
    }

    dict = talloc_zero(reply, sl_dict_t);
    sl_bool_t b;
    /* kMDSStoreMetaScopes */
    s = dalloc_strdup(dict, "kMDSStoreMetaScopes");
    dalloc_add(dict, s, char *);
    array = talloc_zero(dict, sl_array_t);
    s = dalloc_strdup(array, "kMDQueryScopeComputer");
    dalloc_add(array, s, char *);
    s = dalloc_strdup(array, "kMDQueryScopeAllIndexed");
    dalloc_add(array, s, char *);
    s = dalloc_strdup(array, "kMDQueryScopeComputerIndexed");
    dalloc_add(array, s, char *);
    dalloc_add(dict, array, sl_array_t);
    /* kMDSStorePathScopes */
    s = dalloc_strdup(dict, "kMDSStorePathScopes");
    dalloc_add(dict, s, char *);
    array = talloc_zero(dict, sl_array_t);
    s = dalloc_strdup(array, v->v_path);
    dalloc_add(array, s, char *);
    dalloc_add(dict, array, sl_array_t);
    /* kMDSStoreUUID */
    s = dalloc_strdup(dict, "kMDSStoreUUID");
    dalloc_add(dict, s, char *);
    memcpy(uuid.sl_uuid, v->v_uuid, 16);
    dalloc_add_copy(dict, &uuid, sl_uuid_t);
    /* kMDSVolumeUUID */
    s = dalloc_strdup(dict, "kMDSVolumeUUID");
    dalloc_add(dict, s, char *);
    dalloc_add_copy(dict, &uuid, sl_uuid_t);
    /* kMDSStoreHasPersistentUUID */
    s = dalloc_strdup(dict, "kMDSStoreHasPersistentUUID");
    dalloc_add(dict, s, char *);
    b = true;
    dalloc_add_copy(dict, &b, sl_bool_t);
    /* kMDSStoreIsBackup */
    s = dalloc_strdup(dict, "kMDSStoreIsBackup");
    dalloc_add(dict, s, char *);
    b = (v->v_flags & AFPVOL_TM) ? true : false;
    dalloc_add_copy(dict, &b, sl_bool_t);
    /* kMDSStoreSupportsVolFS */
    s = dalloc_strdup(dict, "kMDSStoreSupportsVolFS");
    dalloc_add(dict, s, char *);
    b = true;
    dalloc_add_copy(dict, &b, sl_bool_t);
    dalloc_add(reply, dict, sl_dict_t);
    LOG(log_debug, logtype_sl,
        "fetchPropertiesForContext: vol=\"%s\", uuid=%s, is_backup=%s",
        v->v_localname,
        v->v_uuid ? v->v_uuid : "(null)",
        (v->v_flags & AFPVOL_TM) ? "true" : "false");
    LOG(log_debug, logtype_sl,
        "fetchPropertiesForContext: kMDSStorePathScopes=\"%s\"",
        v->v_path);
EC_CLEANUP:
    EC_EXIT;
}

static int sl_rpc_openQuery(AFPObj *obj,
                            const DALLOC_CTX *query,
                            DALLOC_CTX *reply,
                            struct vol *v)
{
    EC_INIT;
    char *sl_query;
    const uint64_t *uint64;
    DALLOC_CTX *reqinfo;
    sl_array_t *array;
    const sl_cnids_t *cnids = NULL;
    const sl_array_t *cnid_array = NULL;
    slq_t *slq = NULL;
    char slq_host[MAXPATHLEN + 1];
    uint16_t convflags = v->v_mtou_flags;
    ssize_t convret;
    uint64_t result;
    bool ok;
    bool open_ok = false;
    const sl_array_t *scope_array;
    const char *raw_scope = NULL;
    array = talloc_zero(reply, sl_array_t);

    if (v->v_sl_backend == NULL) {
        LOG(log_error, logtype_sl, "no Spotlight search backend configured for volume");
        EC_FAIL;
    }

    /* Allocate and initialize query object */
    /*
     * AFPObj is a static/global object, not a talloc context.
     * Allocate slq as a standalone talloc root and free it via slq_destroy().
     */
    slq = talloc_zero(NULL, slq_t);

    if (slq == NULL) {
        LOG(log_error, logtype_sl, "openQuery: failed to allocate query context");
        EC_FAIL;
    }

    slq->slq_state = SLQ_STATE_NEW;
    slq->slq_obj = obj;
    slq->slq_vol = v;
    slq->slq_allow_expr = obj->options.flags & OPTION_SPOTLIGHT_EXPR ? true : false;
    slq->slq_result_limit = obj->options.sparql_limit;
    LOG(log_debug, logtype_sl, "Spotlight: expr: %s, limit: %" PRIu64,
        slq->slq_allow_expr ? "yes" : "no", slq->slq_result_limit);
    /* convert spotlight query charset to host charset */
    sl_query = dalloc_value_for_key(query, "DALLOC_CTX", 0,
                                    "DALLOC_CTX", 1,
                                    "kMDQueryString",
                                    "char *");

    if (sl_query == NULL) {
        LOG(log_error, logtype_sl, "openQuery: kMDQueryString not found in request");
        EC_FAIL;
    }

    LOG(log_debug, logtype_sl, "Spotlight query pre-conversion: \"%s\"", sl_query);
    convret = convert_charset(CH_UTF8_MAC, v->v_volcharset, v->v_maccharset,
                              sl_query, strnlen(sl_query, MAXPATHLEN), slq_host, MAXPATHLEN,
                              &convflags);

    if (convret == -1) {
        LOG(log_error, logtype_sl, "charset conversion failed");
        EC_FAIL;
    }

    slq->slq_qstring = talloc_strdup(slq, slq_host);
    LOG(log_debug, logtype_sl, "Spotlight query: \"%s\"", slq->slq_qstring);
    slq->slq_time = time(NULL);
    uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 1);

    if (uint64 == NULL) {
        LOG(log_error, logtype_sl, "openQuery: ctx1 not found in request");
        EC_FAIL;
    }

    slq->slq_ctx1 = *uint64;
    uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 2);

    if (uint64 == NULL) {
        LOG(log_error, logtype_sl, "openQuery: ctx2 not found in request");
        EC_FAIL;
    }

    slq->slq_ctx2 = *uint64;
    reqinfo = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1,
                                   "kMDAttributeArray", "sl_array_t");

    if (reqinfo == NULL) {
        LOG(log_error, logtype_sl, "openQuery: kMDAttributeArray not found in request");
        EC_FAIL;
    }

    slq->slq_reqinfo = talloc_steal(slq, reqinfo);
    scope_array = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1,
                                       "kMDScopeArray", "sl_array_t");

    if (scope_array == NULL
            || talloc_array_length(scope_array->dd_talloc_array) == 0) {
        raw_scope = v->v_path;
    } else {
        const void *first = scope_array->dd_talloc_array[0];

        /*
         * macOS may wrap the scope path in a nested array (observed in Tahoe
         * for volume-specific searches): kMDScopeArray → sl_array_t → char *
         * vs. the normal flat form:   kMDScopeArray → char *
         * Unwrap one extra level when the first element is itself an array.
         */
        if (first != NULL
                && strcmp(talloc_get_name(first), "sl_array_t") == 0) {
            const sl_array_t *inner = first;
            first = (talloc_array_length(inner->dd_talloc_array) > 0)
                    ? inner->dd_talloc_array[0] : NULL;
        }

        if (first == NULL) {
            LOG(log_error, logtype_sl, "empty kMDScopeArray");
            EC_FAIL;
        }

        if (strcmp(talloc_get_name(first), "char *") != 0) {
            LOG(log_error, logtype_sl, "unexpected kMDScopeArray element type: %s",
                talloc_get_name(first));
            EC_FAIL;
        }

        raw_scope = first;
    }

    slq->slq_scope = talloc_strdup(slq, raw_scope);

    if (slq->slq_scope == NULL) {
        LOG(log_error, logtype_sl, "talloc_strdup failed");
        EC_FAIL;
    }

    LOG(log_debug, logtype_sl, "Search scope: \"%s\"", slq->slq_scope);
    cnids = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1,
                                 "kMDQueryItemArray", "sl_cnids_t");

    if (cnids && cnids->ca_cnids) {
        EC_ZERO_LOG(sl_createCNIDArray(slq, cnids->ca_cnids));
    } else {
        cnid_array = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1,
                                          "kMDQueryItemArray", "sl_array_t");

        if (cnid_array) {
            EC_ZERO_LOG(sl_createCNIDArray(slq, cnid_array));
        }
    }

    EC_ZERO_LOG(v->v_sl_backend->sbo_init(obj));
    /*
     * create_result_handle must be called before sbo_open_query so that
     * synchronous backends (e.g. cnid) can write results into
     * slq->query_results immediately during sbo_open_query.
     */
    ok = create_result_handle(slq);

    if (!ok) {
        LOG(log_error, logtype_sl, "create_result_handle error");
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    EC_ZERO_LOG(v->v_sl_backend->sbo_open_query(slq));
    slq_add(slq);
    open_ok = true;
EC_CLEANUP: {
        uint64_t log_ctx1 = slq ? slq->slq_ctx1 : 0;
        uint64_t log_ctx2 = slq ? slq->slq_ctx2 : 0;

        if (!open_ok) {
            if (slq != NULL) {
                slq_destroy(slq);
            }

            result = UINT64_MAX;
            ret = 0;
        } else {
            if (ret != 0) {
                LOG(log_warning, logtype_sl,
                    "openQuery: ignoring non-zero internal ret=%d after success"
                    " (ctx1: %" PRIx64 ", ctx2: %" PRIx64 ")",
                    ret, log_ctx1, log_ctx2);
            }

            result = 0;
            ret = 0;
        }

        LOG(log_debug, logtype_sl,
            "openQuery: returning result=%" PRIu64
            " (ctx1: %" PRIx64 ", ctx2: %" PRIx64 ")",
            result, log_ctx1, log_ctx2);
    }
    dalloc_add_copy(array, &result, uint64_t);
    dalloc_add(reply, array, sl_array_t);
    EC_EXIT;
}

static int sl_rpc_fetchQueryResultsForContext(const AFPObj *obj _U_,
        const DALLOC_CTX *query,
        DALLOC_CTX *reply,
        const struct vol *v _U_)
{
    EC_INIT;
    slq_t *slq = NULL;
    const uint64_t *uint64;
    uint64_t ctx1, ctx2, status;
    sl_array_t *array;
    bool ok;
    array = talloc_zero(reply, sl_array_t);

    if (array == NULL) {
        return false;
    }

    /* Context */
    uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 1);

    if (uint64 == NULL) {
        EC_FAIL;
    }

    ctx1 = *uint64;
    uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 2);

    if (uint64 == NULL) {
        EC_FAIL;
    }

    ctx2 = *uint64;
    LOG(log_debug, logtype_sl,
        "fetchQueryResults: ctx1: %" PRIx64 ", ctx2: %" PRIx64, ctx1, ctx2);
    /* Get query for context */
    slq = slq_for_ctx(ctx1, ctx2);

    if (slq == NULL) {
        LOG(log_error, logtype_sl,
            "fetchQueryResults: no query for ctx1: %" PRIx64 ", ctx2: %" PRIx64,
            ctx1, ctx2);
        EC_FAIL;
    }

    /* Reset idle timer: client is still actively polling this query */
    slq->slq_time = time(NULL);

    switch (slq->slq_state) {
    case SLQ_STATE_RUNNING:
    case SLQ_STATE_RESULTS:
    case SLQ_STATE_FULL:
    case SLQ_STATE_DONE:
        LOG(log_debug, logtype_sl,
            "fetchQueryResults: dispatching %d result(s), state=%s",
            slq->query_results ? slq->query_results->num_results : -1,
            slq_state_names[slq->slq_state].state_name);
        EC_ZERO_LOG(slq->slq_vol->v_sl_backend->sbo_fetch_results(slq));
        ok = add_results(array, slq);

        if (!ok) {
            LOG(log_error, logtype_sl, "error adding results");
            EC_FAIL;
        }

        break;

    case SLQ_STATE_ERROR:
        LOG(log_error, logtype_sl, "query in error state");
        EC_FAIL;

    default:
        LOG(log_error, logtype_sl, "unexpected query state %d", slq->slq_state);
        EC_FAIL;
    }

    dalloc_add(reply, array, sl_array_t);
    EC_EXIT;
EC_CLEANUP:
    LOG(log_error, logtype_sl,
        "fetchQueryResults: failed (ctx1: %" PRIx64 ", ctx2: %" PRIx64 ")",
        ctx1, ctx2);
    slq_destroy(slq);
    status = UINT64_MAX;
    dalloc_add_copy(array, &status, uint64_t);
    dalloc_add(reply, array, sl_array_t);
    EC_EXIT;
}

static int sl_rpc_storeAttributesForOIDArray(const AFPObj *obj _U_,
        const DALLOC_CTX *query,
        DALLOC_CTX *reply,
        const struct vol *vol)
{
    EC_INIT;
    uint64_t uint64;
    sl_array_t *array;
    const sl_cnids_t *cnids;
    const sl_time_t *sl_time;
    cnid_t id;
    const char *path;
    struct dir *dir;
    EC_NULL_LOG(cnids = dalloc_get(query, "DALLOC_CTX", 0, "sl_cnids_t", 2));
    memcpy(&uint64, cnids->ca_cnids->dd_talloc_array[0], sizeof(uint64_t));
    id = (cnid_t)uint64;
    LOG(log_debug, logtype_sl, "CNID: %" PRIu32, id);

    if (htonl(id) == DIRDID_ROOT) {
        path = vol->v_path;
    } else if (id < CNID_START) {
        EC_FAIL;
    } else {
        cnid_t did;
        char buffer[12 + MAXPATHLEN + 1];
        did = htonl(id);
        EC_NULL_LOG(path = cnid_resolve(vol->v_cdb, &did, buffer, sizeof(buffer)));
        EC_NULL_LOG(dir = dirlookup(vol, did));
        EC_NEG1_LOG(movecwd(vol, dir));
    }

    if ((sl_time = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1,
                                        "DALLOC_CTX", 1,
                                        "kMDItemFSContentChangeDate", "sl_time_t"))) {
        struct timespec ts[2];
        ts[0].tv_sec  = sl_time->tv_sec;
        ts[0].tv_nsec = sl_time->tv_usec * 1000;
        ts[1] = ts[0];
        utimensat(AT_FDCWD, path, ts, 0);
    } else if ((sl_time = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX",
                          1, "DALLOC_CTX", 1, "kMDItemLastUsedDate", "sl_time_t"))) {
        struct timespec ts[2] = {{0}};
        ts[0].tv_sec  = sl_time->tv_sec;
        ts[0].tv_nsec = sl_time->tv_usec * 1000;
        ts[1].tv_nsec = UTIME_OMIT;
        utimensat(AT_FDCWD, path, ts, 0);
    }

    array = talloc_zero(reply, sl_array_t);
    uint64_t sl_res = 0;
    dalloc_add_copy(array, &sl_res, uint64_t);
    dalloc_add(reply, array, sl_array_t);
EC_CLEANUP:
    EC_EXIT;
}

static int sl_rpc_fetchAttributeNamesForOIDArray(const AFPObj *obj _U_,
        const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol _U_)
{
    EC_INIT;
    uint64_t uint64;
    const sl_cnids_t *cnids;
    cnid_t id;
    EC_NULL_LOG(cnids = dalloc_get(query, "DALLOC_CTX", 0, "sl_cnids_t", 1));
    memcpy(&uint64, cnids->ca_cnids->dd_talloc_array[0], sizeof(uint64_t));
    id = (cnid_t)uint64;
    LOG(log_debug, logtype_sl,
        "sl_rpc_fetchAttributeNamesForOIDArray: CNID: %" PRIu32, id);
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
    sl_array_t *mdattrs = talloc_zero(reply, sl_array_t);
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSName"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemDisplayName"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSSize"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSOwnerUserID"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSOwnerGroupID"), "char *");
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSContentChangeDate"),
               "char *");
    sl_filemeta_t *fmeta = talloc_zero(reply, sl_filemeta_t);
    dalloc_add(fmeta, mdattrs, sl_array_t);
    dalloc_add(array, fmeta, sl_filemeta_t);
EC_CLEANUP:
    EC_EXIT;
}

static int sl_rpc_fetchAttributesForOIDArray(AFPObj *obj _U_,
        const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
{
    EC_INIT;
    uint64_t uint64;
    const sl_cnids_t *cnids;
    sl_cnids_t *replycnids;
    cnid_t id, did;
    struct dir *dir;
    sl_array_t *array, *reqinfo, *fm_array;
    char buffer[12 + MAXPATHLEN + 1];
    char *name, *path;
    sl_filemeta_t *fm;
    sl_nil_t nil;
    uint64_t sl_res;
    struct stat sb;
    array = talloc_zero(reply, sl_array_t);
    replycnids = talloc_zero(reply, sl_cnids_t);
    replycnids->ca_cnids = talloc_zero(replycnids, DALLOC_CTX);
    fm = talloc_zero(array, sl_filemeta_t);
    fm_array = talloc_zero(fm, sl_array_t);

    if (array == NULL || replycnids == NULL || replycnids->ca_cnids == NULL
            || fm == NULL || fm_array == NULL) {
        EC_FAIL;
    }

    reqinfo = dalloc_get(query, "DALLOC_CTX", 0, "sl_array_t", 1);

    if (reqinfo == NULL) {
        EC_FAIL;
    }

    cnids = dalloc_get(query, "DALLOC_CTX", 0, "sl_cnids_t", 2);

    if (cnids == NULL) {
        EC_FAIL;
    }

    memcpy(&uint64, cnids->ca_cnids->dd_talloc_array[0], sizeof(uint64_t));
    id = (cnid_t)uint64;

    if (htonl(id) == DIRDID_ROOT) {
        path = talloc_strdup(reply, vol->v_path);
    } else if (id < CNID_START) {
        EC_FAIL;
    } else {
        did = htonl(id);
        EC_NULL(name = cnid_resolve(vol->v_cdb, &did, buffer, sizeof(buffer)));
        EC_NULL(dir = dirlookup(vol, did));
        EC_NULL(path = talloc_asprintf(reply, "%s/%s", bdata(dir->d_fullpath), name));
    }

    EC_ZERO(stat(path, &sb));
    sl_res = 0;
    dalloc_add_copy(array, &sl_res, uint64_t);
    replycnids->ca_unkn1 = 0xfec;
    replycnids->ca_context = cnids->ca_context;
    uint64 = (uint64_t)id;
    dalloc_add_copy(replycnids->ca_cnids, &uint64, uint64_t);
    dalloc_add(array, replycnids, sl_cnids_t);
    dalloc_add(fm, fm_array, fm_array_t);
    dalloc_add_copy(fm_array, &nil, sl_nil_t);
    add_filemeta(reqinfo, fm_array, path, &sb);
    /* Now add result */
    dalloc_add(array, fm, sl_filemeta_t);
    dalloc_add(reply, array, sl_array_t);
    EC_EXIT;
EC_CLEANUP:
    sl_res = UINT64_MAX;
    dalloc_add_copy(array, &sl_res, uint64_t);
    dalloc_add(array, fm, sl_filemeta_t);
    dalloc_add(reply, array, sl_array_t);
    EC_EXIT;
}

static int sl_rpc_closeQueryForContext(const AFPObj *obj _U_,
                                       const DALLOC_CTX *query,
                                       DALLOC_CTX *reply,
                                       const struct vol *v _U_)
{
    EC_INIT;
    slq_t *slq = NULL;
    const uint64_t *uint64;
    uint64_t ctx1, ctx2;
    sl_array_t *array;
    uint64_t sl_result;
    array = talloc_zero(reply, sl_array_t);
    /* Context */
    uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 1);

    if (uint64 == NULL) {
        EC_FAIL;
    }

    ctx1 = *uint64;
    uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 2);

    if (uint64 == NULL) {
        EC_FAIL;
    }

    ctx2 = *uint64;
    /* Get query for context and free it */
    slq = slq_for_ctx(ctx1, ctx2);

    if (slq == NULL) {
        EC_FAIL;
    }

    switch (slq->slq_state) {
    case SLQ_STATE_FULL:
    case SLQ_STATE_DONE:
    case SLQ_STATE_ERROR:
        LOG(log_debug, logtype_sl, "close: destroying query: state %s",
            slq_state_names[slq->slq_state].state_name);
        slq_destroy(slq);
        break;

    case SLQ_STATE_RUNNING:
    case SLQ_STATE_RESULTS:
        LOG(log_debug, logtype_sl, "close: cancel query: state %s",
            slq_state_names[slq->slq_state].state_name);
        slq_cancel(slq);
        break;

    default:
        LOG(log_error, logtype_sl, "Unexpected state %d", slq->slq_state);
        EC_FAIL;
    }

    sl_result = 0;
EC_CLEANUP:

    if (ret != 0) {
        sl_result = UINT64_MAX;
    }

    dalloc_add_copy(array, &sl_result, uint64_t);
    dalloc_add(reply, array, sl_array_t);
    EC_EXIT;
}

/******************************************************************************
 * AFP functions
 ******************************************************************************/

int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen,
                      char *rbuf, size_t *rbuflen)
{
    EC_INIT;
    TALLOC_CTX *tmp_ctx = talloc_new(NULL);
    uint16_t vid;
    int cmd;
    struct vol      *vol;
    DALLOC_CTX *query;
    DALLOC_CTX *reply;
    char *rpccmd;
    int len;
    *rbuflen = 0;

    if (!(obj->options.flags & OPTION_SPOTLIGHT)) {
        return AFPERR_NOOP;
    }

    slq_dump();
#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH

    /*
     * Process pending GLib/Tracker async events.
     * Only needed when the localsearch backend is in use.
     */
    if (localsearch_backend_active(obj)) {
        bool event = true;

        while (event) {
            event = g_main_context_iteration(NULL, false);
        }
    }

#endif /* SPOTLIGHT_BACKEND_LOCALSEARCH */
    slq_cancelled_cleanup();
    slq_idle_cleanup();
    ibuf += 2;
    ibuflen -= 2;
    vid = (uint16_t)SVAL(ibuf, 0);
    LOG(log_debug, logtype_sl, "afp_spotlight_rpc(vid: %" PRIu16 ")", vid);

    if ((vol = getvolbyvid(vid)) == NULL) {
        LOG(log_error, logtype_sl, "afp_spotlight_rpc: bad volume id: %" PRIu16 ")",
            vid);
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
        len = (int)strlcpy(rbuf + 8, vol->v_path, MAXPATHLEN) + 1;
        *rbuflen += 8 + len;
        LOG(log_debug, logtype_sl,
            "SPOTLIGHT_CMD_OPEN%s: vid=%" PRIu16 ", path=\"%s\"",
            cmd == SPOTLIGHT_CMD_OPEN2 ? "2" : "",
            ntohs(vid), vol->v_path);
        break;

    case SPOTLIGHT_CMD_FLAGS:
        /* Whatever this value means... flags? Helios uses 0x1eefface */
        RSIVAL(rbuf, 0, 0x0100006b);
        *rbuflen += 4;
        LOG(log_debug, logtype_sl,
            "SPOTLIGHT_CMD_FLAGS: returning 0x0100006b");
        break;

    case SPOTLIGHT_CMD_RPC:
        EC_NULL(query = talloc_zero(tmp_ctx, DALLOC_CTX));
        EC_NULL(reply = talloc_zero(tmp_ctx, DALLOC_CTX));
        EC_NEG1_LOG(sl_unpack(query, ibuf + 22));
        LOG(log_debug, logtype_sl, "Spotlight RPC request:\n%s",
            dd_dump(query, 0));
        EC_NULL_LOG(rpccmd = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0,
                                        "char *", 0));

        if (STRCMP(rpccmd, ==, "fetchPropertiesForContext:")) {
            EC_ZERO_LOG(sl_rpc_fetchPropertiesForContext(obj, query, reply, vol));
        } else if (STRCMP(rpccmd, ==, "openQueryWithParams:forContext:")) {
            EC_ZERO_LOG(sl_rpc_openQuery(obj, query, reply, vol));
        } else if (STRCMP(rpccmd, ==, "fetchQueryResultsForContext:")) {
            EC_ZERO_LOG(sl_rpc_fetchQueryResultsForContext(obj, query, reply, vol));
        } else if (STRCMP(rpccmd, ==, "storeAttributes:forOIDArray:context:")) {
            EC_ZERO_LOG(sl_rpc_storeAttributesForOIDArray(obj, query, reply, vol));
        } else if (STRCMP(rpccmd, ==, "fetchAttributeNamesForOIDArray:context:")) {
            EC_ZERO_LOG(sl_rpc_fetchAttributeNamesForOIDArray(obj, query, reply, vol));
        } else if (STRCMP(rpccmd, ==, "fetchAttributes:forOIDArray:context:")
                   || STRCMP(rpccmd, ==, "fetchAllAttributes:forOIDArray:context:")) {
            EC_ZERO_LOG(sl_rpc_fetchAttributesForOIDArray(obj, query, reply, vol));
        } else if (STRCMP(rpccmd, ==, "closeQueryForContext:")) {
            EC_ZERO_LOG(sl_rpc_closeQueryForContext(obj, query, reply, vol));
        } else {
            LOG(log_error, logtype_sl, "afp_spotlight_rpc: unknown Spotlight RPC: %s",
                rpccmd);
        }

        LOG(log_debug, logtype_sl, "Spotlight RPC reply dump:\n%s",
            dd_dump(reply, 0));
        memset(rbuf, 0, 4);
        *rbuflen += 4;
        EC_NEG1_LOG(len = sl_pack(reply, rbuf + 4));
        *rbuflen += len;
        break;

    default:
        LOG(log_error, logtype_sl, "afp_spotlight_rpc: unknown cmd: %d", cmd);
        ret = AFPERR_PARAM;
        goto EC_CLEANUP;
    }

EC_CLEANUP:
    talloc_free(tmp_ctx);

    if (ret != AFP_OK) {
        *rbuflen = 0;
        return AFPERR_MISC;
    }

    EC_EXIT;
}
