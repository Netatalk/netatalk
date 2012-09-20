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

#include <tracker-sparql.h>

#include <atalk/util.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>

#include "spotlight.h"

static TrackerSparqlConnection *connection;

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
    const char *sparql_query_format = "SELECT nie:url(?f) WHERE { ?f nie:url ?name FILTER regex(?name, \"%s\")}";
    const char *slquery = slq->slq_qstring;
    char *word, *p;

    LOG(log_debug, logtype_sl, "query_word_from_sl_query: \"%s\"", slquery);

    EC_NULL_LOG( word = strstr(slquery, "*==") );
    word += 4; /* skip *== and the left enclosing quote */
    EC_NULL( word = talloc_strdup(slq, word) );
    /* Search asterisk */
    EC_NULL_LOG( p = strchr(word, '*') );
    *p = 0;

    sparql_query = talloc_asprintf(slq, sparql_query_format, word);

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

EC_CLEANUP:
    EC_EXIT;
}

static int sl_mod_fetch_result(void *p)
{
    EC_INIT;
    slq_t *slq = p;
    GError *error = NULL;

    if (slq->slq_tracker_cursor) {
        int i = 0;
        while (tracker_sparql_cursor_next(slq->slq_tracker_cursor, NULL, &error)) {
            LOG(log_debug, logtype_sl, "Result [%d]: %s",
                i++, tracker_sparql_cursor_get_string(slq->slq_tracker_cursor, 0, NULL));
        }
    } else {
        LOG(log_debug, logtype_sl, "sl_mod_fetch_result: no results found");
    }

EC_CLEANUP:
    if (slq->slq_tracker_cursor)
        g_object_unref(slq->slq_tracker_cursor);
    EC_EXIT;
}

struct sl_module_export sl_mod = {
    SL_MODULE_VERSION,
    sl_mod_init,
    sl_mod_start_search,
    sl_mod_fetch_result,
    NULL
};
