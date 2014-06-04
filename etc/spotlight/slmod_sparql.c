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
#include <locale.h>

#include <gio/gio.h>
#include <tracker-sparql.h>
#include <libtracker-miner/tracker-miner.h>

#include <atalk/util.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/spotlight.h>

#include "slmod_sparql_parser.h"

#define MAX_SL_RESULTS 20

static TrackerSparqlConnection *connection;
#if 0
static TrackerMinerManager *manager;
#endif

static char *tracker_to_unix_path(const char *uri)
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
    AFPObj *obj = (AFPObj *)p;
    const char *attributes;

    LOG(log_info, logtype_sl, "Initializing Spotlight module");

    g_type_init();
    setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=" _PATH_STATEDIR "/spotlight.ipc", 1);
    setenv("TRACKER_SPARQL_BACKEND", "bus", 1);

#ifdef DEBUG
    setenv("TRACKER_VERBOSITY", "3", 1);
    dup2(type_configs[logtype_sl].fd, 1);
    dup2(type_configs[logtype_sl].fd, 2);
#endif

    become_root();
    connection = tracker_sparql_connection_get(NULL, &error);
#if 0 /* this may hang, so disable it as we don't use the miner anyway  */
    manager = tracker_miner_manager_new_full(FALSE, &error);
#endif
    unbecome_root();

    if (!connection) {
        LOG(log_error, logtype_sl, "Couldn't obtain a direct connection to the Tracker store: %s",
            error ? error->message : "unknown error");
        g_clear_error(&error);
        EC_FAIL;
    }

#if 0
    if (!manager) {
        LOG(log_error, logtype_sl, "Couldn't connect to Tracker miner");
        g_clear_error(&error);
        EC_FAIL;
    }
#endif

    attributes = atalk_iniparser_getstring(obj->iniconfig, INISEC_GLOBAL, "spotlight attributes", NULL);
    if (attributes) {
        configure_spotlight_attributes(attributes);
    }

EC_CLEANUP:
    EC_EXIT;
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
    gchar *sparql_query;
    GError *error = NULL;

    LOG(log_debug, logtype_sl, "sl_mod_start_search: Spotlight query string: \"%s\"", slq->slq_qstring);

    EC_ZERO_LOGSTR( map_spotlight_to_sparql_query(slq, &sparql_query),
                    "Mapping Spotlight query failed: \"%s\"", slq->slq_qstring );
    LOG(log_debug, logtype_sl, "sl_mod_start_search: SPARQL query: \"%s\"", sparql_query);

#if 0
    /* Start the async query */
    tracker_sparql_connection_query_async(connection, sparql_query, NULL, tracker_cb, slq);
#endif

    become_root();
    slq->slq_tracker_cursor = tracker_sparql_connection_query(connection, sparql_query, NULL, &error);
    unbecome_root();

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

static int add_filemeta(sl_array_t *reqinfo, sl_array_t *fm_array, cnid_t id, const char *path, const struct stat *sp)
{
    EC_INIT;
    sl_array_t *meta;
    sl_nil_t nil = 0;
    int i, metacount;
    uint64_t uint64;
    sl_time_t sl_time;

    if ((metacount = talloc_array_length(reqinfo->dd_talloc_array)) == 0) {
        dalloc_add_copy(fm_array, &nil, sl_nil_t);
        goto EC_CLEANUP;
    }

    LOG(log_debug, logtype_sl, "add_filemeta: metadata count: %d", metacount);

    meta = talloc_zero(fm_array, sl_array_t);

    for (i = 0; i < metacount; i++) {
        if (STRCMP(reqinfo->dd_talloc_array[i], ==, "kMDItemDisplayName") ||
            STRCMP(reqinfo->dd_talloc_array[i], ==, "kMDItemFSName")) {
            char *p, *name;
            if ((p = strrchr(path, '/'))) {
                name = dalloc_strdup(meta, p + 1);
                dalloc_add(meta, name, "char *");
            }
        } else if (STRCMP(reqinfo->dd_talloc_array[i], ==, "kMDItemFSSize")) {
            uint64 = sp->st_size;
            dalloc_add_copy(meta, &uint64, uint64_t);
        } else if (STRCMP(reqinfo->dd_talloc_array[i], ==, "kMDItemFSOwnerUserID")) {
            uint64 = sp->st_uid;
            dalloc_add_copy(meta, &uint64, uint64_t);
        } else if (STRCMP(reqinfo->dd_talloc_array[i], ==, "kMDItemFSOwnerGroupID")) {
            uint64 = sp->st_gid;
            dalloc_add_copy(meta, &uint64, uint64_t);
        } else if (STRCMP(reqinfo->dd_talloc_array[i], ==, "kMDItemFSContentChangeDate")) {
            sl_time.tv_sec = sp->st_mtime;
            dalloc_add_copy(meta, &sl_time, sl_time_t);
        } else {
            dalloc_add_copy(meta, &nil, sl_nil_t);
        }
    }

    dalloc_add(fm_array, meta, sl_array_t);

EC_CLEANUP:
    EC_EXIT;
}

static int cnid_cmp_fn(const void *p1, const void *p2)
{
    const uint64_t *cnid1 = p1, *cnid2 = p2;
    if (*cnid1 == *cnid2)
        return 0;
    if (*cnid1 < *cnid2)
        return -1;
    else
        return 1;            
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
    sl_nil_t nil;
    uint64_t uint64;
    gboolean qres, firstmatch = true;

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
    /* For some reason the list of results always starts with a nil entry */
    dalloc_add_copy(fm_array, &nil, sl_nil_t);
    dalloc_add(fm, fm_array, sl_array_t);

    LOG(log_debug, logtype_sl, "sl_mod_fetch_result: now interating Tracker results cursor");

    while ((slq->slq_state == SLQ_STATE_RUNNING) && (i <= MAX_SL_RESULTS)) {
        become_root();
        qres = tracker_sparql_cursor_next(slq->slq_tracker_cursor, NULL, &error);
        unbecome_root();

        if (!qres)
            break;

#if 0
        if (firstmatch) {
            /* For some reason the list of results always starts with a nil entry */
            dalloc_add_copy(fm_array, &nil, sl_nil_t);
            firstmatch = false;
        }
#endif

        become_root();
        uri = tracker_sparql_cursor_get_string(slq->slq_tracker_cursor, 0, NULL);
        unbecome_root();

        EC_NULL_LOG( path = tracker_to_unix_path(uri) );

        if ((id = cnid_for_path(slq->slq_vol->v_cdb, slq->slq_vol->v_path, path, &did)) == CNID_INVALID) {
            LOG(log_debug, logtype_sl, "sl_mod_fetch_result: cnid_for_path error: %s", path);
            goto loop_cleanup;
        }
        LOG(log_debug, logtype_sl, "Result %d: CNID: %" PRIu32 ", path: \"%s\"", i, ntohl(id), path);

        uint64 = ntohl(id);
        if (slq->slq_cnids) {
            if (!bsearch(&uint64, slq->slq_cnids, slq->slq_cnids_num, sizeof(uint64_t), cnid_cmp_fn))
                goto loop_cleanup;
        }

        struct stat sb;
        if (stat(path, &sb) != 0)
            goto loop_cleanup;

        dalloc_add_copy(cnids->ca_cnids, &uint64, uint64_t);
        add_filemeta(slq->slq_reqinfo, fm_array, id, path, &sb);

    loop_cleanup:
        g_free(path);
        i++;
   }

    if (error) {
        LOG(log_error, logtype_sl, "Couldn't query the Tracker Store: '%s'",
            error ? error->message : "unknown error");
        g_clear_error (&error);
        EC_FAIL;
    }

    if (i < MAX_SL_RESULTS)
        slq->slq_state = SLQ_STATE_DONE;

    uint64 = (i > 0) ? 35 : 0; /* OS X AFP server returns 35 here if results are found */
    dalloc_add_copy(slq->slq_reply, &uint64, uint64_t);
    dalloc_add(slq->slq_reply, cnids, sl_cnids_t);
    dalloc_add(slq->slq_reply, fm, sl_filemeta_t);

EC_CLEANUP:
    if (ret != 0) {
        if (slq->slq_tracker_cursor) {
            g_object_unref(slq->slq_tracker_cursor);
            slq->slq_tracker_cursor = NULL;
        }
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

static int sl_mod_fetch_attrs(void *p)
{
    EC_INIT;
    slq_t *slq = p;
    sl_filemeta_t *fm;
    sl_array_t *fm_array;
    sl_nil_t nil;

    LOG(log_debug, logtype_sl, "sl_mod_fetch_attrs(\"%s\")", slq->slq_path);

    struct stat sb;
    EC_ZERO( stat(slq->slq_path, &sb) );

    /* Prepare FileMeta */
    fm = talloc_zero(slq->slq_reply, sl_filemeta_t);
    fm_array = talloc_zero(fm, sl_array_t);
    dalloc_add(fm, fm_array, fm_array_t);
    /* For some reason the list of results always starts with a nil entry */
    dalloc_add_copy(fm_array, &nil, sl_nil_t);

    add_filemeta(slq->slq_reqinfo, fm_array, CNID_INVALID, slq->slq_path, &sb);

    /* Now add result */
    dalloc_add(slq->slq_reply, fm, sl_filemeta_t);

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

static int sl_mod_index_file(const void *p)
{
    /*
     * This seems to cause index problems on volumes that are watched and indexed
     * by Tracker, so we disable this extra manual indexing for now.
     * It's primary pupose was ensuring files created via AFP are indexed on large
     * volumes where the filesystem event notification engine (eg FAM or FEN) may
     * impose limits on the maximum number of watched directories.
     */
    return 0;

#if 0
#ifdef HAVE_TRACKER_MINER
    EC_INIT;
    const char *f = p;

    if (!f)
        goto EC_CLEANUP;

    GError *error = NULL;
    GFile *file = NULL;

    file = g_file_new_for_commandline_arg(f);

    become_root();
    tracker_miner_manager_index_file(manager, file, &error);
    unbecome_root();

    if (error)
        LOG(log_error, logtype_sl, "sl_mod_index_file(\"%s\"): indexing failed", f);
    else
        LOG(log_debug, logtype_sl, "sl_mod_index_file(\"%s\"): indexing file was successful", f);

EC_CLEANUP:
    if (file)
        g_object_unref(file);
    EC_EXIT;
#else
    return 0;
#endif /* HAVE_TRACKER_MINER */
#endif /* 0 */
}

struct sl_module_export sl_mod = {
    SL_MODULE_VERSION,
    sl_mod_init,
    sl_mod_start_search,
    sl_mod_fetch_result,
    sl_mod_close_query,
    sl_mod_fetch_attrs,
    sl_mod_error,
    sl_mod_index_file
};
