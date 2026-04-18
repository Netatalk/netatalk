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

#include <glib.h>
#include <talloc.h>

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
#include "etc/spotlight/sparql_parser.h"

#define MAX_SL_RESULTS 20
#define MAX_SL_QUERY_IDLE_TIME 60

struct timeval convert_timespec_to_timeval(const struct timespec ts)
{
    struct timeval tv;
    tv.tv_sec = ts.tv_sec;
    tv.tv_usec = ts.tv_nsec / 1000;
    return tv;
}

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


static char *tracker_to_unix_path(TALLOC_CTX *mem_ctx, const char *uri);
static int cnid_comp_fn(const void *p1, const void *p2);
static bool create_result_handle(slq_t *slq);
static bool add_filemeta(sl_array_t *reqinfo,
                         sl_array_t *fm_array,
                         const char *path,
                         const struct stat *sp);

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
    int n;
    uint64_t i;
    sl_bool_t bl;
    sl_time_t t;
    struct tm *tm;
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

    for (n = 0; n < talloc_array_length(dd->dd_talloc_array); n++) {
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

    if (ret != 0) {
        if (cnids) {
            talloc_free(cnids);
        }
    }

    EC_EXIT;
}

static char *tracker_to_unix_path(TALLOC_CTX *mem_ctx, const char *uri)
{
    GFile *f;
    char *path;
    char *talloc_path = NULL;
    f = g_file_new_for_uri(uri);

    if (!f) {
        return NULL;
    }

    path = g_file_get_path(f);
    g_object_unref(f);

    if (!path) {
        return NULL;
    }

    talloc_path = talloc_strdup(mem_ctx, path);
    g_free(path);
    return talloc_path;
}

/*!
 * @brief Add requested metadata for a query result element
 *
 * This could be rewritten to something more sophisticated like
 * querying metadata from Tracker.
 *
 * If path or sp is NULL, simply add nil values for all attributes.
 */
static bool add_filemeta(sl_array_t *reqinfo,
                         sl_array_t *fm_array,
                         const char *path,
                         const struct stat *sp)
{
    sl_array_t *meta;
    sl_nil_t nil;
    int i, metacount;
    uint64_t uint64var;
    sl_time_t sl_time;
    char *p, *name;
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
    size_t nfd_buf_size = strlen(path) * 3 + 1;
    char *nfd_path = talloc_array(meta, char, nfd_buf_size);

    if (nfd_path == NULL
            || charset_decompose(CH_UTF8, (char *)path, strlen(path),
                                 nfd_path, nfd_buf_size) == (size_t) -1) {
        nfd_path = (char *)path;
    }

    for (i = 0; i < metacount; i++) {
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
            sl_time = convert_timespec_to_timeval(sp->st_mtim);
            dalloc_add_copy(meta, &sl_time, sl_time_t);
        } else if (strequal(reqinfo->dd_talloc_array[i],
                            "kMDItemLastUsedDate")) {
            sl_time = convert_timespec_to_timeval(sp->st_atim);
            dalloc_add_copy(meta, &sl_time, sl_time_t);
        } else if (strequal(reqinfo->dd_talloc_array[i],
                            "kMDItemContentCreationDate")) {
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC
            sl_time = convert_timespec_to_timeval(sp->st_birthtimespec);
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
 * @brief Allocate result handle used in the async Tracker cursor result
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
    query_results->cnids->ca_context = slq->slq_ctx2;
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
        "dispatching %d result(s) to client, status %" PRIu64 " (ctx1: %" PRIx64
        ", ctx2: %" PRIx64 ")",
        slq->query_results->num_results, status, slq->slq_ctx1, slq->slq_ctx2);
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
 * @brief Add a query to the list of active queries
 */
static void slq_add(slq_t *slq)
{
    list_add(&(slq->slq_list), &sl_queries);
}

/*!
 * @brief Add a query to the list of active queries
 */
static void slq_cancelled_add(slq_t *slq)
{
    list_add(&(slq->slq_list), &sl_cancelled_queries);
}

/*!
 * @brief Remove a query from the active list
 */
static void slq_remove(slq_t *slq)
{
    struct list_head *p;
    slq_t *q = NULL;
    list_for_each(p, &sl_queries) {
        q = list_entry(p, slq_t, slq_list);

        if ((q->slq_ctx1 == slq->slq_ctx1) && (q->slq_ctx2 == slq->slq_ctx2)) {
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
 * @brief Remove a query from the active queue and free it
 */
static void slq_destroy(slq_t *slq)
{
    if (slq == NULL) {
        return;
    }

    slq_remove(slq);
    talloc_free(slq);
}

/*!
 * @brief Cancel a query
 */
static void slq_cancel(slq_t *slq)
{
    slq->slq_state = SLQ_STATE_CANCEL_PENDING;
    slq_remove(slq);
    slq_cancelled_add(slq);
}

/*!
 * @brief talloc destructor cb
 */
static int slq_free_cb(slq_t *slq)
{
    if (slq->tracker_cursor) {
        g_object_unref(slq->tracker_cursor);
    }

    return 0;
}

/*!
 * @brief Free all cancelled queries
 */
static void slq_cancelled_cleanup(void)
{
    struct list_head *p;
    slq_t *q = NULL;
    list_for_each(p, &sl_cancelled_queries) {
        q = list_entry(p, slq_t, slq_list);

        if (q->slq_state == SLQ_STATE_CANCELLED) {
            LOG(log_debug, logtype_sl,
                "ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": cancelled",
                q->slq_ctx1, q->slq_ctx2);
            list_del(p);
            talloc_free(q);
        } else {
            LOG(log_debug, logtype_sl,
                "ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": %s",
                q->slq_ctx1, q->slq_ctx2, slq_state_names[q->slq_state].state_name);
        }
    }
    return;
}

/*!
 * @brief Cancel or destroy queries idle longer than MAX_SL_QUERY_IDLE_TIME
 *
 * Called on every Spotlight RPC to reap queries abandoned by clients that
 * disconnected or crashed without sending closeQueryForContext.
 */
static void slq_idle_cleanup(void)
{
    struct list_head *p;
    slq_t *q = NULL;
    time_t now = time(NULL);
    bool found;

    /*
     * list_for_each is not safe for deletion, so restart after each
     * mutation. The active query list is short and bounded, so the
     * overhead is negligible.
     */
    do {
        found = false;
        list_for_each(p, &sl_queries) {
            q = list_entry(p, slq_t, slq_list);

            if (q->slq_time > now || now - q->slq_time < MAX_SL_QUERY_IDLE_TIME) {
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
    slq_t *q = NULL;
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

/************************************************
 * Tracker async callbacks
 ************************************************/

static void tracker_cursor_cb(GObject      *object,
                              GAsyncResult *res,
                              gpointer      user_data)
{
    GError *error = NULL;
    slq_t *slq = user_data;
    gboolean more_results;
    const gchar *uri;
    char *path;
    struct stat sb;
    uint64_t uint64var;
    bool ok;
    cnid_t did, id;
    LOG(log_debug, logtype_sl,
        "cursor cb[%d]: ctx1: %" PRIx64 ", ctx2: %" PRIx64,
        slq->query_results->num_results, slq->slq_ctx1, slq->slq_ctx2);
    more_results = tracker_sparql_cursor_next_finish(slq->tracker_cursor,
                   res,
                   &error);

    if (slq->slq_state == SLQ_STATE_CANCEL_PENDING) {
        LOG(log_debug, logtype_sl,
            "cursor cb: ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": cancelled",
            slq->slq_ctx1, slq->slq_ctx2);
        slq->slq_state = SLQ_STATE_CANCELLED;
        return;
    }

    if (error) {
        LOG(log_error, logtype_sl, "Tracker cursor: %s", error->message);
        g_error_free(error);
        slq->slq_state = SLQ_STATE_ERROR;
        return;
    }

    if (!more_results) {
        LOG(log_debug, logtype_sl, "tracker_cursor_cb: done");
        slq->slq_state = SLQ_STATE_DONE;
        return;
    }

    uri = tracker_sparql_cursor_get_string(slq->tracker_cursor, 0, NULL);

    if (uri == NULL) {
        /*
         * Not sure how this could happen if
         * tracker_sparql_cursor_next_finish() returns true, but I've
         * seen it.
         */
        LOG(log_debug, logtype_sl, "no URI for result");
        return;
    }

    LOG(log_debug, logtype_sl, "URI: %s", uri);
    path = tracker_to_unix_path(slq->query_results, uri);

    if (path == NULL) {
        LOG(log_error, logtype_sl, "error converting Tracker URI: %s", uri);
        slq->slq_state = SLQ_STATE_ERROR;
        return;
    }

    if (stat(path, &sb) != 0) {
        LOG(log_debug, logtype_sl, "skipping result, stat failed: %s", path);
        goto exit;
    }

    if (access(path, R_OK) != 0) {
        LOG(log_debug, logtype_sl, "skipping result, access denied: %s", path);
        goto exit;
    }

    id = cnid_for_path(slq->slq_vol->v_cdb, slq->slq_vol->v_path, path, &did);

    if (id == CNID_INVALID) {
        LOG(log_debug, logtype_sl,
            "skipping result, no CNID (file moved or deleted?): %s", path);
        goto exit;
    }

    uint64var = ntohl(id);

    if (slq->slq_cnids) {
        ok = bsearch(&uint64var, slq->slq_cnids, slq->slq_cnids_num,
                     sizeof(uint64_t), cnid_comp_fn);

        if (!ok) {
            LOG(log_debug, logtype_sl, "skipping result, CNID not in client filter: %s",
                path);
            goto exit;
        }
    }

    LOG(log_debug, logtype_sl, "adding result CNID %" PRIu32 ": %s",
        ntohl(id), path);
    dalloc_add_copy(slq->query_results->cnids->ca_cnids,
                    &uint64var, uint64_t);
    ok = add_filemeta(slq->slq_reqinfo, slq->query_results->fm_array,
                      path, &sb);

    if (!ok) {
        LOG(log_error, logtype_sl, "add_filemeta error");
        slq->slq_state = SLQ_STATE_ERROR;
        return;
    }

    slq->query_results->num_results++;
exit:

    if (slq->query_results->num_results < MAX_SL_RESULTS) {
        LOG(log_debug, logtype_sl,
            "cursor cb[%d]: ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": requesting more results",
            slq->query_results->num_results - 1, slq->slq_ctx1, slq->slq_ctx2);
        slq->slq_state = SLQ_STATE_RESULTS;
        tracker_sparql_cursor_next_async(slq->tracker_cursor,
                                         slq->slq_obj->sl_ctx->cancellable,
                                         tracker_cursor_cb,
                                         slq);
    } else {
        LOG(log_debug, logtype_sl,
            "cursor cb[%d]: ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": full",
            slq->query_results->num_results - 1, slq->slq_ctx1, slq->slq_ctx2);
        slq->slq_state = SLQ_STATE_FULL;
    }
}

static void tracker_query_cb(GObject      *object,
                             GAsyncResult *res,
                             gpointer      user_data)
{
    GError *error = NULL;
    slq_t *slq = user_data;
    LOG(log_debug, logtype_sl,
        "query cb: ctx1: %" PRIx64 ", ctx2: %" PRIx64,
        slq->slq_ctx1, slq->slq_ctx2);
    slq->tracker_cursor = tracker_sparql_connection_query_finish(
                              TRACKER_SPARQL_CONNECTION(object),
                              res,
                              &error);

    if (slq->slq_state == SLQ_STATE_CANCEL_PENDING) {
        slq->slq_state = SLQ_STATE_CANCELLED;
        return;
    }

    if (error) {
        slq->slq_state = SLQ_STATE_ERROR;
        LOG(log_error, logtype_sl, "Tracker query error: %s", error->message);
        g_error_free(error);
        return;
    }

    slq->slq_state = SLQ_STATE_RESULTS;
    tracker_sparql_cursor_next_async(slq->tracker_cursor,
                                     slq->slq_obj->sl_ctx->cancellable,
                                     tracker_cursor_cb,
                                     slq);
}

/*******************************************************************************
 * Spotlight RPC functions
 ******************************************************************************/

static int sl_rpc_fetchPropertiesForContext(const AFPObj *obj,
        const DALLOC_CTX *query,
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
    uint64_t *uint64;
    DALLOC_CTX *reqinfo;
    sl_array_t *array;
    sl_cnids_t *cnids;
    slq_t *slq = NULL;
    char slq_host[MAXPATHLEN + 1];
    uint16_t convflags = v->v_mtou_flags;
    uint64_t result;
    gchar *sparql_query;
    GError *error = NULL;
    bool ok;
    sl_array_t *scope_array;
    char *scope = NULL;
    array = talloc_zero(reply, sl_array_t);

    if (obj->sl_ctx->tracker_con == NULL) {
        LOG(log_error, logtype_sl, "no tracker connection");
        EC_FAIL;
    }

    /* Allocate and initialize query object */
    slq = talloc_zero(obj->sl_ctx, slq_t);
    slq->slq_state = SLQ_STATE_NEW;
    slq->slq_obj = obj;
    slq->slq_vol = v;
    slq->slq_allow_expr = obj->options.flags & OPTION_SPOTLIGHT_EXPR ? true : false;
    slq->slq_result_limit = obj->options.sparql_limit;
    talloc_set_destructor(slq, slq_free_cb);
    LOG(log_debug, logtype_sl, "Spotlight: expr: %s, limit: %" PRIu64,
        slq->slq_allow_expr ? "yes" : "no", slq->slq_result_limit);
    /* convert spotlight query charset to host charset */
    sl_query = dalloc_value_for_key(query, "DALLOC_CTX", 0,
                                    "DALLOC_CTX", 1,
                                    "kMDQueryString",
                                    "char *");

    if (sl_query == NULL) {
        EC_FAIL;
    }

    LOG(log_debug, logtype_sl, "Spotlight query pre-conversion: \"%s\"", sl_query);
    ret = convert_charset(CH_UTF8_MAC, v->v_volcharset, v->v_maccharset,
                          sl_query, strlen(sl_query), slq_host, MAXPATHLEN,
                          &convflags);

    if (ret == -1) {
        LOG(log_error, logtype_sl, "charset conversion failed");
        EC_FAIL;
    }

    slq->slq_qstring = talloc_strdup(slq, slq_host);
    LOG(log_debug, logtype_sl, "Spotlight query: \"%s\"", slq->slq_qstring);
    slq->slq_time = time(NULL);
    uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 1);

    if (uint64 == NULL) {
        EC_FAIL;
    }

    slq->slq_ctx1 = *uint64;
    uint64 = dalloc_get(query, "DALLOC_CTX", 0, "DALLOC_CTX", 0, "uint64_t", 2);

    if (uint64 == NULL) {
        EC_FAIL;
    }

    slq->slq_ctx2 = *uint64;
    reqinfo = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1,
                                   "kMDAttributeArray", "sl_array_t");

    if (reqinfo == NULL) {
        EC_FAIL;
    }

    slq->slq_reqinfo = talloc_steal(slq, reqinfo);
    scope_array = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1,
                                       "kMDScopeArray", "sl_array_t");

    if (scope_array == NULL) {
        scope = g_uri_escape_string(v->v_path,
                                    G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);
    } else {
        scope = g_uri_escape_string(scope_array->dd_talloc_array[0],
                                    G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);
    }

    if (scope == NULL) {
        LOG(log_error, logtype_sl, "failed to setup search scope");
        EC_FAIL;
    }

    slq->slq_scope = talloc_strdup(slq, scope);
    g_free(scope);

    if (slq->slq_scope == NULL) {
        LOG(log_error, logtype_sl, "talloc_strdup failed");
        EC_FAIL;
    }

    LOG(log_debug, logtype_sl, "Search scope: \"%s\"", slq->slq_scope);
    cnids = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1,
                                 "kMDQueryItemArray", "sl_array_t");

    if (cnids) {
        EC_ZERO_LOG(sl_createCNIDArray(slq, cnids->ca_cnids));
    }

    ret = map_spotlight_to_sparql_query(slq, &sparql_query);

    if (ret != 0) {
        LOG(log_debug, logtype_sl, "mapping returned non-zero");
        EC_FAIL;
    }

    LOG(log_debug, logtype_sl, "SPARQL query: \"%s\"", sparql_query);
    tracker_sparql_connection_query_async(obj->sl_ctx->tracker_con,
                                          sparql_query,
                                          slq->slq_obj->sl_ctx->cancellable,
                                          tracker_query_cb,
                                          slq);

    if (error) {
        LOG(log_error, logtype_sl, "Couldn't query the Tracker Store: '%s'",
            error->message);
        g_clear_error(&error);
        EC_FAIL;
    }

    slq->slq_state = SLQ_STATE_RUNNING;
    ok = create_result_handle(slq);

    if (!ok) {
        LOG(log_error, logtype_sl, "create_result_handle error");
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    slq_add(slq);
EC_CLEANUP:

    if (ret != 0 && slq != NULL) {
        slq_destroy(slq);
        result = UINT64_MAX;
        ret = 0;
    } else {
        result = 0;
    }

    dalloc_add_copy(array, &result, uint64_t);
    dalloc_add(reply, array, sl_array_t);
    EC_EXIT;
}

static int sl_rpc_fetchQueryResultsForContext(const AFPObj *obj,
        const DALLOC_CTX *query,
        DALLOC_CTX *reply,
        const struct vol *v)
{
    EC_INIT;
    slq_t *slq = NULL;
    uint64_t *uint64, ctx1, ctx2, status;
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
            "fetchQueryResults: no query for ctx1: %" PRIx64 ", ctx2: %" PRIx64, ctx1,
            ctx2);
        EC_FAIL;
    }

    /* Reset idle timer: client is still actively polling this query */
    slq->slq_time = time(NULL);

    switch (slq->slq_state) {
    case SLQ_STATE_RUNNING:
    case SLQ_STATE_RESULTS:
    case SLQ_STATE_FULL:
    case SLQ_STATE_DONE:
        ok = add_results(array, slq);

        if (!ok) {
            LOG(log_error, logtype_sl, "error adding results");
            EC_FAIL;
        }

        if (slq->slq_state == SLQ_STATE_FULL) {
            slq->slq_state = SLQ_STATE_RESULTS;
            tracker_sparql_cursor_next_async(
                slq->tracker_cursor,
                slq->slq_obj->sl_ctx->cancellable,
                tracker_cursor_cb,
                slq);
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
    slq_destroy(slq);
    status = UINT64_MAX;
    dalloc_add_copy(array, &status, uint64_t);
    dalloc_add(reply, array, sl_array_t);
    EC_EXIT;
}

static int sl_rpc_storeAttributesForOIDArray(const AFPObj *obj,
        const DALLOC_CTX *query,
        DALLOC_CTX *reply,
        const struct vol *vol)
{
    EC_INIT;
    uint64_t uint64;
    sl_array_t *array;
    sl_cnids_t *cnids;
    sl_time_t *sl_time;
    cnid_t id;
    char *path;
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

    /*
     * We're possibly supposed to update attributes in two places: the
     * database and the filesystem.  Due to the lack of documentation
     * and not yet implemented database updates, we cherry pick attributes
     * that seems to be candidates for updating filesystem metadata.
     */

    if ((sl_time = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX", 1,
                                        "DALLOC_CTX", 1,
                                        "kMDItemFSContentChangeDate", "sl_time_t"))) {
        struct utimbuf utimes;
        utimes.actime = utimes.modtime = sl_time->tv_sec;
        utime(path, &utimes);
    } else if ((sl_time = dalloc_value_for_key(query, "DALLOC_CTX", 0, "DALLOC_CTX",
                          1, "DALLOC_CTX", 1, "kMDItemLastUsedDate", "sl_time_t"))) {
        /*
         * Update atime only. Using utimensat with UTIME_OMIT on the mtime
         * slot avoids touching mtime.
         */
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

static int sl_rpc_fetchAttributeNamesForOIDArray(const AFPObj *obj,
        const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
{
    EC_INIT;
    uint64_t uint64;
    sl_cnids_t *cnids;
    cnid_t id;
    char *path;
    struct dir *dir;
    EC_NULL_LOG(cnids = dalloc_get(query, "DALLOC_CTX", 0, "sl_cnids_t", 1));
    memcpy(&uint64, cnids->ca_cnids->dd_talloc_array[0], sizeof(uint64_t));
    id = (cnid_t)uint64;
    LOG(log_debug, logtype_sl,
        "sl_rpc_fetchAttributeNamesForOIDArray: CNID: %" PRIu32, id);

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
    dalloc_add(mdattrs, dalloc_strdup(mdattrs, "kMDItemFSContentChangeDate"),
               "char *");
    sl_filemeta_t *fmeta = talloc_zero(reply, sl_filemeta_t);
    dalloc_add(fmeta, mdattrs, sl_array_t);
    dalloc_add(array, fmeta, sl_filemeta_t);
EC_CLEANUP:
    EC_EXIT;
}

static int sl_rpc_fetchAttributesForOIDArray(AFPObj *obj,
        const DALLOC_CTX *query, DALLOC_CTX *reply, const struct vol *vol)
{
    EC_INIT;
    uint64_t uint64;
    sl_cnids_t *cnids, *replycnids;
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

static int sl_rpc_closeQueryForContext(const AFPObj *obj,
                                       const DALLOC_CTX *query,
                                       DALLOC_CTX *reply,
                                       const struct vol *v)
{
    EC_INIT;
    slq_t *slq = NULL;
    uint64_t *uint64, ctx1, ctx2;
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
 * Spotlight functions
 ******************************************************************************/

int spotlight_init(AFPObj *obj)
{
    static bool initialized = false;
    const char *attributes;
    struct sl_ctx *sl_ctx;
    GError *error = NULL;

    if (initialized) {
        return 0;
    }

    LOG(log_info, logtype_sl, "Initializing Spotlight");
    sl_ctx = talloc_zero(NULL, struct sl_ctx);
    obj->sl_ctx = sl_ctx;
    attributes = INIPARSER_GETSTR(obj->iniconfig, INISEC_GLOBAL,
                                  "spotlight attributes", NULL);

    if (attributes) {
        configure_spotlight_attributes(attributes);
    }

    /*
     * Tracker uses glibs event dispatching, so we need a mainloop
     */
#if ((GLIB_MAJOR_VERSION <= 2) && (GLIB_MINOR_VERSION < 36))
    g_type_init();
#endif
    sl_ctx->mainloop = g_main_loop_new(NULL, false);
    sl_ctx->cancellable = g_cancellable_new();
    setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=" _PATH_STATEDIR "spotlight.ipc",
           1);
    setenv("XDG_DATA_HOME", _PATH_STATEDIR, 0);
    setenv("XDG_CACHE_HOME", _PATH_STATEDIR, 0);
    setenv("TRACKER_USE_LOG_FILES", "1", 0);
    sl_ctx->tracker_con =
        tracker_sparql_connection_bus_new(INDEXER_DBUS_NAME,
                                          NULL, NULL, &error);

    if (error) {
        LOG(log_error, logtype_sl, "Could not connect to indexer: %s",
            error->message);
        sl_ctx->tracker_con = NULL;
        g_error_free(error);
        return -1;
    }

    LOG(log_info, logtype_sl, "connected to indexer");
    initialized = true;
    return 0;
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
    bool event;
    *rbuflen = 0;

    if (!(obj->options.flags & OPTION_SPOTLIGHT)) {
        return AFPERR_NOOP;
    }

    spotlight_init(obj);
    slq_dump();
    /*
     * Process finished glib events
     */
    event = true;

    while (event) {
        event = g_main_context_iteration(NULL, false);
    }

    slq_cancelled_cleanup();
    slq_idle_cleanup();
    ibuf += 2;
    ibuflen -= 2;
    vid = SVAL(ibuf, 0);
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
        len = strlen(vol->v_path) + 1;
        strlcpy(rbuf + 8, vol->v_path, len);
        *rbuflen += 8 + len;
        break;

    case SPOTLIGHT_CMD_FLAGS:
        /* Whatever this value means... flags? Helios uses 0x1eefface */
        RSIVAL(rbuf, 0, 0x0100006b);
        *rbuflen += 4;
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
    }

EC_CLEANUP:
    talloc_free(tmp_ctx);

    if (ret != AFP_OK) {
        *rbuflen = 0;
        return AFPERR_MISC;
    }

    EC_EXIT;
}
