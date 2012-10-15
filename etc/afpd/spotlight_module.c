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

#include <string.h>

#include <gio/gio.h>
#include <tracker-sparql.h>

#include <atalk/util.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>

#include "spotlight.h"

static TrackerSparqlConnection *connection;

char *tracker_to_unix_path(const char *uri)
{
    EC_INIT;
    GFile *f = NULL;
    char *path = NULL;

    EC_NULL_LOG( f = g_file_new_for_uri(uri) );
    EC_NULL_LOG( path = g_file_get_path(f) );

EC_CLEANUP:
    if (f)
        g_object_unref(f);
    if (ret != 0)
        return NULL;
    return path;
}

static int sl_mod_init(void *p)
{
    EC_INIT;
    GError *error = NULL;
    const char *msg = p;

    LOG(log_note, logtype_sl, "sl_mod_init: %s", msg);
    setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/tmp/spotlight.ipc", 1);

    g_type_init();
    connection = tracker_sparql_connection_get(NULL, &error);
    if (!connection) {
        LOG(log_error, logtype_sl, "Couldn't obtain a direct connection to the Tracker store: %s",
            error ? error->message : "unknown error");
        g_clear_error(&error);
        EC_FAIL;
    }

EC_CLEANUP:
    EC_EXIT;
}

/*!
 * Return talloced query from query string
 * *=="query*"
 */
static const gchar *map_spotlight_to_sparql_query(slq_t *slq)
{
    EC_INIT;
    const gchar *sparql_query;
    const char *sparql_query_format = "SELECT nie:url(?uri) WHERE {?uri fts:match '%s' . ?uri nie:url ?url FILTER(fn:starts-with(?url, 'file://%s')) }";
    const char *slquery = slq->slq_qstring;
    char *word, *p;

    LOG(log_debug, logtype_sl, "query_word_from_sl_query: \"%s\"", slquery);

    EC_NULL_LOG( word = strstr(slquery, "*==") );
    word += 4; /* skip *== and the left enclosing quote */
    EC_NULL( word = dalloc_strdup(slq, word) );
    /* Search asterisk */
    EC_NULL_LOG( p = strchr(word, '*') );
    p[1] = 0;
    sparql_query = talloc_asprintf(slq, sparql_query_format, word, slq->slq_vol->v_path);

    LOG(log_debug, logtype_sl, "query_word_from_sl_query: \"%s\"", sparql_query);

EC_CLEANUP:
    if (ret != 0)
        sparql_query = NULL;
    return sparql_query;
}

static void tracker_cb(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
    slq_t *slq = user_data;
    TrackerSparqlCursor *cursor;
    GError *error = NULL;

    LOG(log_debug, logtype_sl, "tracker_cb");

    cursor = tracker_sparql_connection_query_finish(connection, res, &error);

    if (error) {
        LOG(log_error, logtype_sl, "sl_mod_fetch_result: Couldn't query the Tracker Store: '%s'",
            error ? error->message : "unknown error");
        g_clear_error(&error);
        return;
    }

    slq->slq_tracker_cursor = cursor;
}

static int sl_mod_start_search(void *p)
{
    EC_INIT;
    slq_t *slq = p; 
    const gchar *sparql_query;
    GError *error = NULL;

    LOG(log_debug, logtype_sl, "sl_mod_start_search: Spotlight query string: \"%s\"", slq->slq_qstring);

    EC_NULL_LOG( sparql_query = map_spotlight_to_sparql_query(slq) );
    LOG(log_debug, logtype_sl, "sl_mod_start_search: SPARQL query: \"%s\"", sparql_query);

#if 0
    /* Start the async query */
    tracker_sparql_connection_query_async(connection, sparql_query, NULL, tracker_cb, slq);
#endif

    slq->slq_tracker_cursor = tracker_sparql_connection_query(connection, sparql_query, NULL, &error);
    if (error) {
        LOG(log_error, logtype_sl, "Couldn't query the Tracker Store: '%s'",
            error ? error->message : "unknown error");
        g_clear_error(&error);
        EC_FAIL;
    }
    slq->slq_state = SLQ_STATE_RUNNING;

EC_CLEANUP:
    EC_EXIT;
}

static int add_filemeta(sl_array_t *reqinfo, sl_array_t *fm_array, cnid_t id, const char *path)
{
    EC_INIT;
    sl_array_t *meta;
    sl_nil_t nil = 0;
    int i, metacount;

    if ((metacount = talloc_array_length(reqinfo->dd_talloc_array)) == 0)
        EC_FAIL;

    LOG(log_debug, logtype_sl, "add_filemeta: metadata count: %d", metacount);

    meta = talloc_zero(fm_array, sl_array_t);

    for (i = 0; i < metacount; i++) {
        if (STRCMP(reqinfo->dd_talloc_array[i], ==, "kMDItemDisplayName")) {
            char *p, *name;
            if ((p = strrchr(path, '/'))) {
                name = dalloc_strdup(meta, p + 1);
                dalloc_add(meta, name, "char *");
            }
        } else {
            dalloc_add_copy(meta, &nil, sl_nil_t);
        }
    }

    dalloc_add(fm_array, meta, sl_array_t);

EC_CLEANUP:
    EC_EXIT;
}

static int sl_mod_fetch_result(void *p)
{
    EC_INIT;
    slq_t *slq = p;
    GError *error = NULL;
    int i = 0;
    cnid_t did, id;
    const gchar *uri;
    char *path;
    sl_cnids_t *cnids;
    sl_filemeta_t *fm;
    sl_array_t *fm_array;
    uint64_t uint64;

    if (!slq->slq_tracker_cursor) {
        LOG(log_debug, logtype_sl, "sl_mod_fetch_result: no results found");
        goto EC_CLEANUP;
    }

    /* Prepare CNIDs */
    cnids = talloc_zero(slq->slq_reply, sl_cnids_t);
    cnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);
    cnids->ca_unkn1 = 0xadd;
    cnids->ca_context = slq->slq_ctx2;

    /* Prepare FileMeta */
    fm = talloc_zero(slq->slq_reply, sl_filemeta_t);
    fm_array = talloc_zero(fm, sl_array_t);
    dalloc_add(fm, fm_array, sl_array_t);

    /* For some reason the list of results always starts with a nil entry */
    sl_nil_t nil;
    dalloc_add_copy(fm_array, &nil, sl_nil_t);

    LOG(log_debug, logtype_sl, "sl_mod_fetch_result: now interating Tracker results cursor");

    while (tracker_sparql_cursor_next(slq->slq_tracker_cursor, NULL, &error)) {
        uri = tracker_sparql_cursor_get_string(slq->slq_tracker_cursor, 0, NULL);
        LOG(log_debug, logtype_sl, "Result %d: uri: \"%s\"", i, uri);

        EC_NULL_LOG( path = tracker_to_unix_path(uri) );
        LOG(log_debug, logtype_sl, "sl_mod_fetch_result: path(volpath: %s): \"%s\"", slq->slq_vol->v_path, path);

        if ((id = cnid_for_path(slq->slq_vol->v_cdb, slq->slq_vol->v_path, path, &did)) == CNID_INVALID) {
            LOG(log_error, logtype_sl, "sl_mod_fetch_result: cnid_for_path error");
            goto loop_cleanup;
        }
        LOG(log_debug, logtype_sl, "Result %d: CNID: %" PRIu32 ", path: \"%s\"", i++, ntohl(id), path);

        uint64 = ntohl(id);
        dalloc_add_copy(cnids->ca_cnids, &uint64, uint64_t);
        add_filemeta(slq->slq_reqinfo, fm_array, id, path);

    loop_cleanup:
        g_free(path);
    }

    if (error) {
        LOG(log_error, logtype_sl, "Couldn't query the Tracker Store: '%s'",
            error ? error->message : "unknown error");
        g_clear_error (&error);
        EC_FAIL;
    }

    dalloc_add(slq->slq_reply, cnids, sl_cnids_t);
    dalloc_add(slq->slq_reply, fm, sl_filemeta_t);

EC_CLEANUP:
    if (slq->slq_tracker_cursor) {
        g_object_unref(slq->slq_tracker_cursor);
        slq->slq_tracker_cursor = NULL;
    }
    EC_EXIT;
}

/* Free ressources allocated in this module */
static int sl_mod_close_query(void *p)
{
    EC_INIT;
    slq_t *slq = p;

    if (slq->slq_tracker_cursor) {
        g_object_unref(slq->slq_tracker_cursor);
        slq->slq_tracker_cursor = NULL;
    }

EC_CLEANUP:
    EC_EXIT;
}

static int sl_mod_error(void *p)
{
    EC_INIT;
    slq_t *slq = p;

    if (!slq)
        goto EC_CLEANUP;

    if (slq->slq_tracker_cursor) {
        g_object_unref(slq->slq_tracker_cursor);
        slq->slq_tracker_cursor = NULL;
    }

EC_CLEANUP:
    EC_EXIT;
}

struct sl_module_export sl_mod = {
    SL_MODULE_VERSION,
    sl_mod_init,
    sl_mod_start_search,
    sl_mod_fetch_result,
    sl_mod_close_query,
    sl_mod_error
};
