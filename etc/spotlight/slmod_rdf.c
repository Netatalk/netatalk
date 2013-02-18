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
#include <tracker.h>

#include <atalk/util.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/spotlight.h>

#include "slmod_rdf_parser.h"

#define MAX_SL_RESULTS 20

TrackerClient *client;

static int sl_mod_init(void *p)
{
    EC_INIT;
    GError *error = NULL;
    const char *msg = p;

    LOG(log_info, logtype_sl, "Initializing Tracker 0.6 RDF Spotlight module");

    g_type_init();
    setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/tmp/spotlight.ipc", 1);

    become_root();
    client = tracker_connect(FALSE);
    unbecome_root();

    if (!client) {
        LOG(log_error, logtype_sl, "Failed connecting to Tracker");
        EC_FAIL;
    }

EC_CLEANUP:
    EC_EXIT;
}


static int sl_mod_start_search(void *p)
{
    EC_INIT;
    slq_t *slq = p; 
    GError *error = NULL;

    LOG(log_debug, logtype_sl, "sl_mod_start_search: Spotlight query: \"%s\"", slq->slq_qstring);

    EC_ZERO_LOG( map_spotlight_to_rdf_query(slq,
//                                            (ServiceType *)(&slq->slq_service),
                                            &slq->slq_trackerquery) );

    LOG(log_debug, logtype_sl, "sl_mod_start_search: Tracker service: %s, query: \"%s\"",
        tracker_type_to_service_name(slq->slq_service),
        slq->slq_trackerquery ? slq->slq_trackerquery : "false");

    if (slq->slq_trackerquery)
        slq->slq_state = SLQ_STATE_RUNNING;
    else
        slq->slq_state = SLQ_STATE_DONE;

EC_CLEANUP:
    EC_EXIT;
}

static int add_filemeta(sl_array_t *reqinfo, sl_array_t *fm_array, cnid_t id, const char *path)
{
    EC_INIT;
    sl_array_t *meta;
    sl_nil_t nil = 0;
    int i, metacount;

    if ((metacount = talloc_array_length(reqinfo->dd_talloc_array)) == 0) {
        dalloc_add_copy(fm_array, &nil, sl_nil_t);
        goto EC_CLEANUP;
    }

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
    sl_cnids_t *cnids;
    sl_filemeta_t *fm;
    sl_array_t *fm_array;
    sl_nil_t nil;
    uint64_t uint64;
    gboolean qres, firstmatch = true;
    gchar **results = NULL, **result;

    /* Prepare CNIDs */
    cnids = talloc_zero(slq->slq_reply, sl_cnids_t);
    cnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);
    cnids->ca_unkn1 = 0xadd;
    cnids->ca_context = slq->slq_ctx2;

    /* Prepare FileMeta */
    fm = talloc_zero(slq->slq_reply, sl_filemeta_t);
    fm_array = talloc_zero(fm, sl_array_t);
    dalloc_add(fm, fm_array, sl_array_t);

    if (slq->slq_state == SLQ_STATE_RUNNING) {
        /* Run the query */
        become_root();
        results = tracker_search_text(client,
                                      time(NULL),
                                      slq->slq_service,
                                      slq->slq_trackerquery,
                                      slq->slq_offset,
                                      MAX_SL_RESULTS,
                                      &error);
        unbecome_root();

        if (error) {
            slq->slq_state = SLQ_STATE_DONE;
            LOG(log_error, logtype_sl, "Couldn't query Tracker: '%s'", error->message);
            g_clear_error(&error);
            EC_FAIL;
        }

        if (!results) {
            slq->slq_state = SLQ_STATE_DONE;
            LOG(log_debug, logtype_sl, "sl_mod_fetch_result: no results found");
            EC_EXIT_STATUS(0);
        }

        for (result = results; *result; result++) {
            LOG(log_debug, logtype_sl, "sl_mod_fetch_result: result %d: %s", slq->slq_offset, *result);

            if (firstmatch) {
                /* For some reason the list of results always starts with a nil entry */
                dalloc_add_copy(fm_array, &nil, sl_nil_t);
                firstmatch = false;
            }

            if ((id = cnid_for_path(slq->slq_vol->v_cdb, slq->slq_vol->v_path, *result, &did)) == CNID_INVALID) {
                LOG(log_error, logtype_sl, "sl_mod_fetch_result: cnid_for_path error: %s", *result);
                goto loop_continue;
            }
            LOG(log_debug, logtype_sl, "Result %d: CNID: %" PRIu32 ", path: \"%s\"", i, ntohl(id), *result);

            uint64 = ntohl(id);
            if (slq->slq_cnids) {
                if (!bsearch(&uint64, slq->slq_cnids, slq->slq_cnids_num, sizeof(uint64_t), cnid_cmp_fn))
                    goto loop_continue;
            }

            dalloc_add_copy(cnids->ca_cnids, &uint64, uint64_t);
            add_filemeta(slq->slq_reqinfo, fm_array, id, *result);

        loop_continue:
            i++;
            slq->slq_offset++;
        }

        if (i < MAX_SL_RESULTS)
            slq->slq_state = SLQ_STATE_DONE;
    }

    uint64 = (i > 0) ? 35 : 0; /* OS X AFP server returns 35 here if results are found */
    dalloc_add_copy(slq->slq_reply, &uint64, uint64_t);
    dalloc_add(slq->slq_reply, cnids, sl_cnids_t);
    dalloc_add(slq->slq_reply, fm, sl_filemeta_t);

EC_CLEANUP:
    if (results) {
        g_free(results);
        results = NULL;
    }
    EC_EXIT;
}

/* Free ressources allocated in this module */
static int sl_mod_close_query(void *p)
{
    EC_INIT;
    slq_t *slq = p;

EC_CLEANUP:
    EC_EXIT;
}

static int sl_mod_error(void *p)
{
    EC_INIT;
    slq_t *slq = p;

    if (!slq)
        goto EC_CLEANUP;

EC_CLEANUP:
    EC_EXIT;
}

static int sl_mod_index_file(const void *p)
{
    return 0;
}

struct sl_module_export sl_mod = {
    SL_MODULE_VERSION,
    sl_mod_init,
    sl_mod_start_search,
    sl_mod_fetch_result,
    sl_mod_close_query,
    sl_mod_error,
    sl_mod_index_file
};
