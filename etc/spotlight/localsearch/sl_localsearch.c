/*
  Spotlight LocalSearch+TinySPARQL backend for Netatalk.

  Extracted from etc/afpd/spotlight.c; contains all GLib / LocalSeach-specific
  logic.  The rest of the Spotlight RPC machinery lives in spotlight.c.

  Copyright (c) 2012-2014 Ralph Boehme

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>
#include <talloc.h>

#ifdef HAVE_TRACKER3
#include <tracker-sparql.h>
#else
#include <tinysparql.h>
#endif

#include <atalk/cnid.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/iniparser_util.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/spotlight.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "etc/spotlight/localsearch/sparql_parser.h"
#include "etc/spotlight/spotlight_private.h"

#define MAX_SL_RESULTS 20

static int cnid_comp_fn(const void *p1, const void *p2)
{
    const uint64_t *cnid1 = p1, *cnid2 = p2;

    if (*cnid1 == *cnid2) {
        return 0;
    }

    return (*cnid1 < *cnid2) ? -1 : 1;
}

/*
 * Private per-connection Tracker state.  Stored in AFPObj::sl_ctx (cast from
 * void *).
 */
struct sl_ctx {
    TrackerSparqlConnection *tracker_con;
    GCancellable            *cancellable;
    GMainLoop               *mainloop;
};

/*
 * Private per-query state for the localsearch backend.
 * Stored in slq_t::slq_backend_private (cast from void *).
 */
struct sl_localsearch_query {
    TrackerSparqlCursor *cursor;
};

/******************************************************************************
 * Static helpers
 ******************************************************************************/

static char *tracker_to_unix_path(TALLOC_CTX *mem_ctx, const char *uri)
{
    GFile *f;
    char  *path;
    char  *talloc_path = NULL;
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

/******************************************************************************
 * Tracker async callbacks
 ******************************************************************************/

static void tracker_cursor_cb(GObject      *object,
                              GAsyncResult *res,
                              gpointer      user_data)
{
    GError                   *error = NULL;
    slq_t                    *slq   = user_data;
    struct sl_localsearch_query *lsq;
    struct sl_ctx            *ctx;
    gboolean                  more_results;
    const gchar              *uri;
    char                     *path;
    struct stat               sb;
    uint64_t                  uint64var;
    bool                      ok;
    cnid_t                    did, id;
    lsq = slq->slq_backend_private;
    ctx = slq->slq_obj->sl_ctx;

    if (slq->query_results == NULL) {
        LOG(log_error, logtype_sl,
            "cursor cb: ctx1: %" PRIx64 ", ctx2: %" PRIx64 ": no result handle",
            slq->slq_ctx1, slq->slq_ctx2);
        slq->slq_state = SLQ_STATE_ERROR;
        return;
    }

    LOG(log_debug, logtype_sl,
        "cursor cb[%d]: ctx1: %" PRIx64 ", ctx2: %" PRIx64,
        slq->query_results->num_results, slq->slq_ctx1, slq->slq_ctx2);
    more_results = tracker_sparql_cursor_next_finish(lsq->cursor, res, &error);

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
        LOG(log_debug, logtype_sl,
            "tracker_cursor_cb: done, %d result(s) accumulated in this batch",
            slq->query_results ? slq->query_results->num_results : -1);
        slq->slq_state = SLQ_STATE_DONE;
        return;
    }

    uri = tracker_sparql_cursor_get_string(lsq->cursor, 0, NULL);

    if (uri == NULL) {
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
            LOG(log_debug, logtype_sl,
                "skipping result, CNID not in client filter: %s", path);
            goto exit;
        }
    }

    LOG(log_debug, logtype_sl, "adding result CNID %" PRIu32 " (%s): %s",
        ntohl(id), S_ISDIR(sb.st_mode) ? "dir" : "file", path);
    dalloc_add_copy(slq->query_results->cnids->ca_cnids, &uint64var, uint64_t);
    ok = add_filemeta(slq->slq_reqinfo, slq->query_results->fm_array, path, &sb);

    if (!ok) {
        LOG(log_error, logtype_sl, "add_filemeta error");
        slq->slq_state = SLQ_STATE_ERROR;
        return;
    }

    slq->query_results->num_results++;
exit:

    if (slq->query_results->num_results < MAX_SL_RESULTS) {
        LOG(log_debug, logtype_sl,
            "cursor cb[%d]: ctx1: %" PRIx64 ", ctx2: %" PRIx64
            ": requesting more results",
            slq->query_results->num_results - 1, slq->slq_ctx1, slq->slq_ctx2);
        slq->slq_state = SLQ_STATE_RESULTS;
        tracker_sparql_cursor_next_async(lsq->cursor, ctx->cancellable,
                                         tracker_cursor_cb, slq);
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
    GError                   *error = NULL;
    slq_t                    *slq   = user_data;
    struct sl_localsearch_query *lsq = slq->slq_backend_private;
    struct sl_ctx            *ctx   = slq->slq_obj->sl_ctx;
    LOG(log_debug, logtype_sl,
        "query cb: ctx1: %" PRIx64 ", ctx2: %" PRIx64,
        slq->slq_ctx1, slq->slq_ctx2);
    lsq->cursor = tracker_sparql_connection_query_finish(
                      TRACKER_SPARQL_CONNECTION(object), res, &error);

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
    tracker_sparql_cursor_next_async(lsq->cursor, ctx->cancellable,
                                     tracker_cursor_cb, slq);
}

/******************************************************************************
 * Backend vtable implementation
 ******************************************************************************/

static int sl_localsearch_init(AFPObj *obj)
{
    static bool initialized = false;
    struct sl_ctx *sl_ctx;
    GError        *error = NULL;
    const char    *attributes;

    if (initialized) {
        return 0;
    }

    LOG(log_info, logtype_sl, "Initializing localsearch backend");
    sl_ctx = talloc_zero(NULL, struct sl_ctx);
    obj->sl_ctx = sl_ctx;
    attributes = INIPARSER_GETSTR(obj->iniconfig, INISEC_GLOBAL,
                                  "spotlight attributes", NULL);

    if (attributes) {
        configure_spotlight_attributes(attributes);
    }

#if ((GLIB_MAJOR_VERSION <= 2) && (GLIB_MINOR_VERSION < 36))
    g_type_init();
#endif
    sl_ctx->mainloop    = g_main_loop_new(NULL, false);
    sl_ctx->cancellable = g_cancellable_new();
    setenv("DBUS_SESSION_BUS_ADDRESS",
           "unix:path=" _PATH_STATEDIR "spotlight.ipc", 1);
    setenv("XDG_DATA_HOME",          _PATH_STATEDIR, 0);
    setenv("XDG_CACHE_HOME",         _PATH_STATEDIR, 0);
    setenv("TRACKER_USE_LOG_FILES",  "1",             0);
    sl_ctx->tracker_con =
        tracker_sparql_connection_bus_new(INDEXER_DBUS_NAME, NULL, NULL, &error);

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

static void sl_localsearch_close(AFPObj *obj)
{
    /* nothing to do — Tracker connection is process-scoped */
}

static int sl_localsearch_open_query(slq_t *slq)
{
    EC_INIT;
    struct sl_localsearch_query *lsq;
    struct sl_ctx               *ctx  = slq->slq_obj->sl_ctx;
    gchar                       *sparql_query = NULL;
    char                        *uri_scope    = NULL;
    const char                  *saved_scope;
    GError                      *error = NULL;

    if (ctx->tracker_con == NULL) {
        LOG(log_error, logtype_sl, "no tracker connection");
        EC_FAIL;
    }

    /* Allocate per-query private state */
    lsq = talloc_zero(slq, struct sl_localsearch_query);
    EC_NULL(lsq);
    slq->slq_backend_private = lsq;
    /*
     * The SPARQL parser embeds slq_scope directly into the query as a
     * URI path component, so it must be percent-encoded.
     */
    uri_scope = g_uri_escape_string(slq->slq_scope,
                                    G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);
    EC_NULL(uri_scope);
    saved_scope      = slq->slq_scope;
    slq->slq_scope   = uri_scope;
    ret = map_spotlight_to_sparql_query(slq, &sparql_query);
    slq->slq_scope = (char *)saved_scope;
    g_free(uri_scope);
    uri_scope = NULL;

    if (ret != 0) {
        LOG(log_debug, logtype_sl, "SPARQL mapping returned non-zero");
        EC_FAIL;
    }

    LOG(log_debug, logtype_sl, "SPARQL query: \"%s\"", sparql_query);
    tracker_sparql_connection_query_async(ctx->tracker_con,
                                          sparql_query,
                                          ctx->cancellable,
                                          tracker_query_cb,
                                          slq);

    if (error) {
        LOG(log_error, logtype_sl, "Couldn't query Tracker: '%s'",
            error->message);
        g_clear_error(&error);
        EC_FAIL;
    }

    slq->slq_state = SLQ_STATE_RUNNING;
EC_CLEANUP:
    EC_EXIT;
}

static int sl_localsearch_fetch_results(slq_t *slq)
{
    struct sl_localsearch_query *lsq = slq->slq_backend_private;
    struct sl_ctx               *ctx = slq->slq_obj->sl_ctx;

    /*
     * When the result page was full the client is polling for more.
     * Re-arm the Tracker cursor to continue pulling the next batch.
     */
    if (slq->slq_state == SLQ_STATE_FULL) {
        slq->slq_state = SLQ_STATE_RESULTS;
        tracker_sparql_cursor_next_async(lsq->cursor, ctx->cancellable,
                                         tracker_cursor_cb, slq);
    }

    return 0;
}

static void sl_localsearch_close_query(slq_t *slq)
{
    struct sl_localsearch_query *lsq = slq->slq_backend_private;

    if (lsq && lsq->cursor) {
        g_object_unref(lsq->cursor);
        lsq->cursor = NULL;
    }

    /* lsq itself is talloc'd as a child of slq; freed with slq */
}

const sl_backend_ops sl_localsearch_ops = {
    .sbo_name          = "localsearch",
    .sbo_init          = sl_localsearch_init,
    .sbo_close         = sl_localsearch_close,
    .sbo_open_query    = sl_localsearch_open_query,
    .sbo_fetch_results = sl_localsearch_fetch_results,
    .sbo_close_query   = sl_localsearch_close_query,
};
