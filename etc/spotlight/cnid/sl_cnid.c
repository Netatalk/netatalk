/*
 * Spotlight CNID database search backend for Netatalk.
 *
 * Uses cnid_find() to perform LIKE-based filename searches directly in
 * the Netatalk CNID database.
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <talloc.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/spotlight.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "etc/spotlight/spotlight_private.h"

static int cnid_comp_fn(const void *p1, const void *p2)
{
    const uint64_t *c1 = p1, *c2 = p2;

    if (*c1 == *c2) {
        return 0;
    }

    return (*c1 < *c2) ? -1 : 1;
}

/*
 * Maximum number of CNID results requested per cnid_find() call.
 *
 * The CNID DBD backend can return up to 100 IDs in one search reply
 * (DBD_MAX_SRCH_RSLTS). Keep this buffer at least that large, otherwise
 * dbd_rpc() treats the larger reply as protocol garbage and resets.
 */
#define SL_CNID_MAX_RESULTS 100

/*
 * Maximum results returned per RPC reply page.
 *
 * Must match the cap enforced by the RPC layer (MAX_SL_RESULTS in
 * sl_localsearch.c).  When there are more results than this, we set
 * SLQ_STATE_FULL so the client polls for the next page via
 * fetchQueryResults / sbo_fetch_results.
 */
#define SL_CNID_PAGE_SIZE   20

/* Maximum path depth when walking the DID chain */
#define SL_CNID_MAX_DEPTH   64

/* Per-query private state for the CNID backend */
struct sl_cnid_query {
    cnid_t cnids[SL_CNID_MAX_RESULTS]; /* raw results from cnid_find()      */
    int    count;                      /* total CNIDs returned              */
    int    pos;                        /* next index to process             */
};

/****************************************************************************
 * Query term extraction
 ****************************************************************************/

/*!
 * @brief Extract a filename search term from a Spotlight query string
 *
 * Handles the common macOS patterns:
 *   kMDItemFSName = "foo*"cd       → "foo"
 *   kMDItemDisplayName = "*foo*"cd → "foo"
 *   _kMDItemFileName = "foo*"cd    → "foo"
 *
 * Returns a talloc-allocated copy of the extracted term, or NULL if the
 * query does not contain a supported filename predicate.
 */
static char *sl_cnid_extract_term(TALLOC_CTX *mem_ctx, const char *qstring)
{
    static const char *const name_keys[] = {
        "kMDItemFSName",
        "kMDItemDisplayName",
        "_kMDItemFileName",
        NULL
    };
    const char *p, *key, *eq, *quote_start, *quote_end;
    char *term;

    if (qstring == NULL) {
        return NULL;
    }

    for (size_t i = 0; name_keys[i] != NULL; i++) {
        key = name_keys[i];
        p = strstr(qstring, key);

        if (p == NULL) {
            continue;
        }

        /* Find '=' after the key */
        eq = strchr(p + strnlen(key, 32), '=');

        if (eq == NULL) {
            continue;
        }

        /* Find the opening quote */
        quote_start = strchr(eq + 1, '"');

        if (quote_start == NULL) {
            continue;
        }

        quote_start++; /* skip '"' itself */
        /* Find closing quote */
        quote_end = strchr(quote_start, '"');

        if (quote_end == NULL) {
            continue;
        }

        /* Copy the raw value between quotes */
        term = talloc_strndup(mem_ctx, quote_start,
                              (size_t)(quote_end - quote_start));

        if (term == NULL) {
            return NULL;
        }

        /* Strip leading '*' wildcards */
        {
            const char *t = term;

            while (*t == '*') {
                t++;
            }

            if (t != term) {
                memmove(term, t, strnlen(t, MAXPATHLEN) + 1);
            }
        }
        /* Strip trailing '*' wildcards */
        {
            size_t len = strnlen(term, MAXPATHLEN);

            while (len > 0 && term[len - 1] == '*') {
                term[--len] = '\0';
            }
        }

        /* Skip empty or whitespace-only terms */
        if (term[0] == '\0') {
            return NULL;
        }

        LOG(log_debug, logtype_sl, "cnid backend: extracted search term \"%s\"",
            term);
        return term;
    }

    /*
     * Handle macOS's "any attribute" wildcard operator: *=="term*"cdw
     * Treat it as a filename search — it is the only predicate we support.
     */
    p = strstr(qstring, "*==");

    if (p != NULL) {
        quote_start = strchr(p + 3, '"');

        if (quote_start != NULL) {
            quote_start++;
            quote_end = strchr(quote_start, '"');

            if (quote_end != NULL) {
                term = talloc_strndup(mem_ctx, quote_start,
                                      (size_t)(quote_end - quote_start));

                if (term != NULL) {
                    const char *t = term;

                    while (*t == '*') {
                        t++;
                    }

                    if (t != term) {
                        memmove(term, t, strnlen(t, MAXPATHLEN) + 1);
                    }

                    size_t len = strnlen(term, MAXPATHLEN);

                    while (len > 0 && term[len - 1] == '*') {
                        term[--len] = '\0';
                    }

                    if (term[0] != '\0') {
                        LOG(log_debug, logtype_sl,
                            "cnid backend: extracted term from *== pattern: \"%s\"",
                            term);
                        return term;
                    }
                }
            }
        }
    }

    LOG(log_debug, logtype_sl,
        "cnid backend: no supported filename predicate in query \"%s\"", qstring);
    return NULL;
}

/****************************************************************************
 * CNID → filesystem path reconstruction
 ****************************************************************************/

/*!
 * @brief Reconstruct the full filesystem path for a CNID
 *
 * Walks the DID chain upward via repeated cnid_resolve() calls, prepending
 * path components, until reaching DIRDID_ROOT (the volume root).
 *
 * @param mem_ctx  talloc context for the returned string
 * @param vol      volume whose CNID database to query
 * @param cnid     network-byte-order CNID (as returned by cnid_find)
 * @return         talloc-allocated full path, or NULL on error
 */
static char *sl_cnid_to_path(TALLOC_CTX *mem_ctx,
                             const struct vol *vol,
                             cnid_t cnid)
{
    char      buf[12 + MAXPATHLEN + 1];
    const char *components[SL_CNID_MAX_DEPTH];
    int        depth = 0;
    cnid_t     id    = cnid;
    const char *name;
    char      *path  = NULL;

    while (id != DIRDID_ROOT && id != 0 && depth < SL_CNID_MAX_DEPTH) {
        name = cnid_resolve(vol->v_cdb, &id, buf, sizeof(buf));

        if (name == NULL) {
            LOG(log_debug, logtype_sl,
                "cnid_to_path: cnid_resolve failed for id %u", ntohl(id));
            return NULL;
        }

        components[depth++] = talloc_strdup(mem_ctx, name);
    }

    if (depth == 0) {
        /* CNID was DIRDID_ROOT itself — return volume path */
        return talloc_strdup(mem_ctx, vol->v_path);
    }

    /* Build path: vol_path / components[depth-1] / ... / components[0] */
    path = talloc_strdup(mem_ctx, vol->v_path);

    for (int i = depth - 1; i >= 0; i--) {
        path = talloc_asprintf(mem_ctx, "%s/%s", path, components[i]);

        if (path == NULL) {
            return NULL;
        }
    }

    return path;
}

/****************************************************************************
 * Backend vtable implementation
 ****************************************************************************/

static int sl_cnid_init(AFPObj *obj _U_)
{
    /* No global state needed for the CNID backend */
    return 0;
}

static void sl_cnid_close(AFPObj *obj _U_)
{
    /* Nothing to do */
}

/*!
 * @brief Emit up to SL_CNID_PAGE_SIZE results from the private CNID buffer.
 *
 * Iterates over the remaining entries in csq->cnids[csq->pos..csq->count-1],
 * resolving each CNID to a filesystem path and adding it to query_results.
 * Stops after SL_CNID_PAGE_SIZE accepted results or when the buffer is
 * exhausted, whichever comes first.
 *
 * Sets slq_state to:
 *   SLQ_STATE_FULL  — page is full, more results remain; client must poll
 *   SLQ_STATE_DONE  — all results have been delivered
 *   SLQ_STATE_ERROR — add_filemeta() failed
 *
 * @return 0 on success, -1 on error
 */
static int sl_cnid_fill_results(slq_t *slq)
{
    EC_INIT;
    struct sl_cnid_query *csq = slq->slq_backend_private;
    int        page_count = 0;
    cnid_t     id;
    uint64_t   uint64var;
    char      *path = NULL;
    struct stat sb;
    bool        ok;

    while (csq->pos < csq->count && page_count < SL_CNID_PAGE_SIZE) {
        /* network byte order, as returned by cnid_find */
        id = csq->cnids[csq->pos++];
        /* Convert to host byte order for filter comparison and result storage */
        uint64var = ntohl(id);

        if (slq->slq_cnids) {
            ok = bsearch(&uint64var, slq->slq_cnids, slq->slq_cnids_num,
                         sizeof(uint64_t), cnid_comp_fn);

            if (!ok) {
                LOG(log_debug, logtype_sl,
                    "cnid backend: skipping CNID %" PRIu64
                    ", not in client filter", uint64var);
                continue;
            }
        }

        path = sl_cnid_to_path(slq->query_results, slq->slq_vol, id);

        if (path == NULL) {
            LOG(log_debug, logtype_sl,
                "cnid backend: could not resolve path for CNID %" PRIu64,
                uint64var);
            continue;
        }

        if (stat(path, &sb) != 0) {
            LOG(log_debug, logtype_sl,
                "cnid backend: stat failed, skipping: %s", path);
            continue;
        }

        if (access(path, R_OK) != 0) {
            LOG(log_debug, logtype_sl,
                "cnid backend: access denied, skipping: %s", path);
            continue;
        }

        LOG(log_debug, logtype_sl,
            "cnid backend: adding result CNID %" PRIu64 " (%s): %s",
            uint64var, S_ISDIR(sb.st_mode) ? "dir" : "file", path);
        dalloc_add_copy(slq->query_results->cnids->ca_cnids,
                        &uint64var, uint64_t);
        ok = add_filemeta(slq->slq_reqinfo, slq->query_results->fm_array,
                          path, &sb);

        if (!ok) {
            LOG(log_error, logtype_sl, "cnid backend: add_filemeta error");
            slq->slq_state = SLQ_STATE_ERROR;
            EC_FAIL;
        }

        slq->query_results->num_results++;
        page_count++;
    }

    if (csq->pos < csq->count) {
        LOG(log_debug, logtype_sl,
            "cnid backend: page full (%d results), %d remaining",
            page_count, csq->count - csq->pos);
        slq->slq_state = SLQ_STATE_FULL;
    } else {
        LOG(log_debug, logtype_sl,
            "cnid backend: all results delivered (%d this page, %d total)",
            page_count, slq->query_results->num_results);
        slq->slq_state = SLQ_STATE_DONE;
    }

EC_CLEANUP:
    EC_EXIT;
}

static int sl_cnid_open_query(slq_t *slq)
{
    EC_INIT;
    char                 *term = NULL;
    struct sl_cnid_query *csq  = NULL;
    LOG(log_debug, logtype_sl,
        "cnid backend: open_query called, qstring: \"%s\"", slq->slq_qstring);
    term = sl_cnid_extract_term(slq, slq->slq_qstring);

    if (term == NULL) {
        /*
         * Query type not supported (e.g. kMDItemTextContent, date ranges).
         * Return no results gracefully.
         */
        LOG(log_debug, logtype_sl,
            "cnid backend: no extractable term, returning 0 results");
        slq->slq_state = SLQ_STATE_DONE;
        EC_EXIT;
    }

    csq = talloc_zero(slq, struct sl_cnid_query);

    if (csq == NULL) {
        LOG(log_error, logtype_sl, "cnid backend: out of memory");
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    slq->slq_backend_private = csq;
    LOG(log_debug, logtype_sl,
        "cnid backend: calling cnid_find for term \"%s\"", term);
    csq->count = cnid_find(slq->slq_vol->v_cdb,
                           term, strnlen(term, MAXPATHLEN),
                           csq->cnids, sizeof(csq->cnids));

    if (csq->count < 0) {
        LOG(log_error, logtype_sl,
            "cnid backend: cnid_find failed for term \"%s\"", term);
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    LOG(log_debug, logtype_sl,
        "cnid backend: cnid_find(\"%s\") returned %d result(s)", term, csq->count);
    EC_ZERO(sl_cnid_fill_results(slq));
EC_CLEANUP:
    EC_EXIT;
}

static int sl_cnid_fetch_results(slq_t *slq)
{
    return sl_cnid_fill_results(slq);
}

static void sl_cnid_close_query(slq_t *slq)
{
    if (slq->slq_backend_private) {
        talloc_free(slq->slq_backend_private);
        slq->slq_backend_private = NULL;
    }
}

const sl_backend_ops sl_cnid_ops = {
    .sbo_name          = "cnid",
    .sbo_init          = sl_cnid_init,
    .sbo_close         = sl_cnid_close,
    .sbo_open_query    = sl_cnid_open_query,
    .sbo_fetch_results = sl_cnid_fetch_results,
    .sbo_close_query   = sl_cnid_close_query,
};
