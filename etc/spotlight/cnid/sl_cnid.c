/*
 * Spotlight CNID database search backend for Netatalk.
 *
 * Searches the Netatalk CNID database for the names that a Spotlight
 * query's name comparisons match.
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 * Copyright (c) 2026 Andy Lemin (andylemin)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*!
 * @file
 * @brief Spotlight CNID backend: file name searches in the CNID database
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
#include <atalk/sl_names.h>
#include <atalk/spotlight.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "etc/afpd/volume.h"
#include "etc/spotlight/spotlight_private.h"

/*!
 * @brief Order two 64-bit host-order CNIDs, for bsearch over the client filter
 *
 * @param[in] p1  first CNID
 * @param[in] p2  second CNID
 * @returns -1, 0, or 1
 */
static int cnid_comp_fn(const void *p1, const void *p2)
{
    const uint64_t *c1 = p1, *c2 = p2;

    if (*c1 == *c2) {
        return 0;
    }

    return (*c1 < *c2) ? -1 : 1;
}

/*
 * Candidate-buffer sizing. The buffer is allocated from 'spotlight results
 * limit'. An unlimited limit starts at SL_CNID_START_RESULTS and grows
 * SL_CNID_GROWTH times over while the search reports further matches. The capacity never
 * falls below CNID_FIND_MIN_RESULTS, the smallest buffer cnid_find_scoped()
 * takes.
 *
 * SL_CNID_MAX_CAP keeps the growth loop's arithmetic in range and bounds
 * what one query can allocate; it is the same ceiling the option itself
 * is clamped to.
 */
#define SL_CNID_START_RESULTS 10000
#define SL_CNID_MAX_CAP       SPOTLIGHT_RESULTS_LIMIT_MAX
#define SL_CNID_GROWTH        8

/*
 * Literal bytes a name comparison without != needs before the backend
 * searches for it. Shorter values match too many names of a typical CNID
 * database to be useful, so the comparison matches no name. This policy is
 * Spotlight-only: `nad find` and FPCatSearch / CatSearchExt accept 1- and
 * 2-character names through the CNID search calls.
 */
#define SL_CNID_MIN_TERMLEN 3

/*
 * Maximum name comparisons honoured per query; any past it matches no name.
 * Finder joins one predicate pair per typed word with ||, so this bounds the
 * number of words in a single search string.
 */
#define SL_CNID_MAX_TERMS   8

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

/* Directory path memo for sl_cnid_to_path(); see SL_CNID_DCACHE_MAX */
struct sl_did_cache_ent {
    cnid_t did;
    char  *path;
};

/*
 * Upper bound on memoized directory paths per query; results beyond
 * it still resolve correctly, just without the memo.
 */
#define SL_CNID_DCACHE_MAX 4096

/* Per-query private state for the CNID backend */
struct sl_cnid_query {
    /* Raw results from cnid_find(), allocated on this context */
    cnid_t *cnids;
    /* Capacity of cnids in entries */
    int    cap;
    /* Total CNIDs returned */
    int    count;
    /* Next index to process */
    int    pos;
    /* Result set was truncated by cnid_find() */
    bool   more_available;
    /* Directory-path memo, lazily allocated on this context */
    struct sl_did_cache_ent *dcache;
    int    dcache_n;
    /* The query's name comparisons */
    struct sl_name_query *names;
};

/****************************************************************************
 * Query state
 ****************************************************************************/

/*!
 * @brief Free a query's name comparisons as talloc frees its state
 *
 * @param[in] csq  query state
 * @returns 0, so talloc goes on to free the state
 */
static int sl_cnid_query_free(struct sl_cnid_query *csq)
{
    sl_name_query_free(csq->names);
    return 0;
}

/****************************************************************************
 * CNID → filesystem path reconstruction
 ****************************************************************************/

/*!
 * @brief Return the memoized path of a directory in the query cache
 *
 * @param[in] csq  query state
 * @param[in] did  directory CNID
 * @returns the path, or NULL without an entry
 */
static const char *sl_dcache_get(struct sl_cnid_query *csq, cnid_t did)
{
    for (int i = 0; i < csq->dcache_n; i++) {
        if (csq->dcache[i].did == did) {
            return csq->dcache[i].path;
        }
    }

    return NULL;
}

/*!
 * @brief Memoize a directory path for the rest of the query
 *
 * The cache allocates its SL_CNID_DCACHE_MAX entries on first use. A full
 * cache or a failed allocation skips the entry.
 *
 * @param[in,out] csq   query state
 * @param[in]     did   directory CNID
 * @param[in]     path  full path of the directory
 */
static void sl_dcache_put(struct sl_cnid_query *csq, cnid_t did,
                          const char *path)
{
    if (csq->dcache_n >= SL_CNID_DCACHE_MAX) {
        return;
    }

    if (csq->dcache == NULL) {
        csq->dcache = talloc_array(csq, struct sl_did_cache_ent,
                                   SL_CNID_DCACHE_MAX);

        if (csq->dcache == NULL) {
            return;
        }
    }

    csq->dcache[csq->dcache_n].path = talloc_strdup(csq, path);

    if (csq->dcache[csq->dcache_n].path == NULL) {
        return;
    }

    csq->dcache[csq->dcache_n].did = did;
    csq->dcache_n++;
}

/*!
 * @brief Reconstruct the full filesystem path for a CNID
 *
 * Walks the DID chain upward via repeated cnid_resolve() calls,
 * prepending path components, until reaching DIRDID_ROOT or a
 * directory whose path is already memoized in the query's cache.
 * Ancestor directory paths discovered along the way are memoized,
 * so results sharing a directory resolve it once per query.
 *
 * @param mem_ctx  talloc context for the returned string
 * @param vol      volume whose CNID database to query
 * @param csq      per-query state carrying the directory memo
 * @param cnid     network-byte-order CNID (as returned by cnid_find)
 * @return         talloc-allocated full path, or NULL on error
 */
static char *sl_cnid_to_path(TALLOC_CTX *mem_ctx,
                             const struct vol *vol,
                             struct sl_cnid_query *csq,
                             cnid_t cnid)
{
    char       buf[12 + MAXPATHLEN + 1];
    const char *components[SL_CNID_MAX_DEPTH];
    cnid_t      walk_dids[SL_CNID_MAX_DEPTH];
    const char *base  = NULL;
    int         depth = 0;
    cnid_t      id    = cnid;
    const char *name;
    char       *path;

    while (id != DIRDID_ROOT && id != 0 && depth < SL_CNID_MAX_DEPTH) {
        base = sl_dcache_get(csq, id);

        if (base != NULL) {
            break;
        }

        walk_dids[depth] = id;
        name = cnid_resolve(vol->v_cdb, &id, buf, sizeof(buf));

        if (name == NULL) {
            LOG(log_debug, logtype_sl,
                "cnid_to_path: cnid_resolve failed for id %u", ntohl(id));
            return NULL;
        }

        components[depth++] = talloc_strdup(mem_ctx, name);
    }

    if (base == NULL) {
        base = vol->v_path;
    }

    path = talloc_strdup(mem_ctx, base);

    for (int i = depth - 1; i >= 0; i--) {
        path = talloc_asprintf(mem_ctx, "%s/%s", path, components[i]);

        if (path == NULL) {
            return NULL;
        }

        if (i > 0) {
            /* ancestor directory: memoize for the other results */
            sl_dcache_put(csq, walk_dids[i], path);
        }
    }

    return path;
}

/****************************************************************************
 * Backend vtable implementation
 ****************************************************************************/

/*!
 * @brief Start the CNID backend, which keeps no global state
 *
 * @param[in] obj  AFP object, unused
 * @returns 0
 */
static int sl_cnid_init(AFPObj *obj _U_)
{
    /* No global state needed for the CNID backend */
    return 0;
}

/*!
 * @brief Stop the CNID backend, which has nothing to release
 *
 * @param[in] obj  AFP object, unused
 */
static void sl_cnid_close(AFPObj *obj _U_)
{
    /* Nothing to do */
}

/*!
 * @brief Add up to SL_CNID_PAGE_SIZE results from the CNID buffer to the result
 * page
 *
 * The call resolves each remaining CNID to a path. It skips a CNID outside
 * the client filter, a path that does not resolve or lies out of scope, and
 * a name the query's comparisons do not match.
 * It also skips a file that fails stat or a read access check. A full page
 * with CNIDs left sets SLQ_STATE_FULL, so the client polls for the next page.
 * With no CNIDs left the call sets SLQ_STATE_DONE, and a failed add_filemeta
 * sets SLQ_STATE_ERROR.
 *
 * @param[in,out] slq  query with the result page and the backend state
 * @returns 0, or -1 on error
 */
static int sl_cnid_fill_results(slq_t *slq)
{
    EC_INIT;
    struct sl_cnid_query *csq = slq->slq_backend_private;
    int        page_count = 0;
    cnid_t     id;
    uint64_t   uint64var;
    char      *path = NULL;
    const char *name;
    struct stat sb;
    bool        ok;

    if (slq->query_results == NULL) {
        LOG(log_error, logtype_sl, "cnid backend: no result handle");
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    if (csq == NULL) {
        if (slq->slq_state == SLQ_STATE_DONE) {
            EC_EXIT;
        }

        LOG(log_error, logtype_sl, "cnid backend: no query state");
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

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

        path = sl_cnid_to_path(slq->query_results, slq->slq_vol, csq, id);

        if (path == NULL) {
            LOG(log_debug, logtype_sl,
                "cnid backend: could not resolve path for CNID %" PRIu64,
                uint64var);
            continue;
        }

        /* Guard for renames racing the query, which the database-side scope
         * cannot see */
        if (!sl_path_in_scope(path, slq->slq_scope)) {
            LOG(log_debug, logtype_sl,
                "cnid backend: out of scope, skipping: %s", path);
            continue;
        }

        name = strrchr(path, '/') + 1;

        if (!sl_name_query_match(csq->names, name, strlen(name))) {
            LOG(log_debug, logtype_sl,
                "cnid backend: the query does not match, skipping: %s", path);
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

/*!
 * @brief Size the candidate buffer for the wanted results
 *
 * A limit under CNID_FIND_MIN_RESULTS still gets that much room, and the
 * caller trims the surplus results afterwards. The capacity stops at
 * SL_CNID_MAX_CAP.
 *
 * @param[in] want  number of results wanted
 * @returns the capacity in entries
 */
static int sl_cnid_cap_for(uint64_t want)
{
    if (want < CNID_FIND_MIN_RESULTS) {
        want = CNID_FIND_MIN_RESULTS;
    }

    if (want > SL_CNID_MAX_CAP) {
        want = SL_CNID_MAX_CAP;
    }

    return (int)want;
}

/*!
 * @brief Tell whether a search scope names the volume root, trailing slashes
 * aside
 *
 * @param[in] scope     scope path of the query
 * @param[in] vol_path  path of the volume
 * @returns true when both paths match without their trailing slashes
 */
static bool sl_cnid_scope_is_vol_root(const char *scope,
                                      const char *vol_path)
{
    size_t sl = strlen(scope);
    size_t vl = strlen(vol_path);

    while (sl > 1 && scope[sl - 1] == '/') {
        sl--;
    }

    while (vl > 1 && vol_path[vl - 1] == '/') {
        vl--;
    }

    return sl == vl && strncmp(scope, vol_path, sl) == 0;
}

/*!
 * @brief Run a filename search in the CNID database for a Spotlight query
 *
 * A query without a name comparison, or with a scope that does not resolve,
 * returns no results. A scope below the volume root restricts the search to
 * its subtree inside the database. A query without a result limit starts with
 * SL_CNID_START_RESULTS entries. While the database reports further matches,
 * the search reruns on a buffer of SL_CNID_GROWTH times the size. A bounded
 * query keeps up to its limit of results. The first page fills at once.
 *
 * A scope lookup that trips a CNID reset marks the volume reset and fails the
 * query.
 *
 * @param[in,out] slq  query
 * @returns 0, or -1 on error
 */
static int sl_cnid_open_query(slq_t *slq)
{
    EC_INIT;
    struct sl_name_query *names;
    cnid_t scope_did;
    uint64_t limit;
    bool unlimited;
    struct sl_cnid_query *csq = NULL;
    LOG(log_debug, logtype_sl,
        "cnid backend: open_query called, qstring: \"%s\"",
        slq->slq_qstring);
    /* the query and the names come in the volume's charset */
    names = slq->slq_qstring == NULL ? NULL
            : sl_name_query_parse_in(slq->slq_qstring, SL_CNID_MIN_TERMLEN,
                                     SL_CNID_MAX_TERMS,
                                     slq->slq_vol->v_volcharset);

    if (names == NULL) {
        /* no name comparison the backend serves (kMDItemTextContent, date
         * ranges), only refused ones, or a query that does not parse or
         * convert */
        LOG(log_debug, logtype_sl,
            "cnid backend: no name comparison to search, returning 0 "
            "results");
        slq->slq_state = SLQ_STATE_DONE;
        EC_EXIT;
    }

    csq = talloc_zero(slq, struct sl_cnid_query);

    if (csq == NULL) {
        sl_name_query_free(names);
        LOG(log_error, logtype_sl, "cnid backend: out of memory");
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    csq->names = names;
    talloc_set_destructor(csq, sl_cnid_query_free);
    slq->slq_backend_private = csq;
    limit = slq->slq_result_limit;
    unlimited = (limit == 0);
    csq->cap = sl_cnid_cap_for(unlimited ? SL_CNID_START_RESULTS : limit);
    csq->cnids = talloc_array(csq, cnid_t, (size_t)csq->cap);

    if (csq->cnids == NULL) {
        LOG(log_error, logtype_sl, "cnid backend: out of memory");
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    /*
     * Resolve the scope directory to its CNID once;
     * cnid_find_scoped() then restricts matching to that subtree
     * inside the database, so scoped results are complete up to
     * the same candidate cap unscoped searches have.
     */
    scope_did = CNID_INVALID;

    if (slq->slq_scope != NULL
            && !sl_cnid_scope_is_vol_root(slq->slq_scope,
                                          slq->slq_vol->v_path)) {
        cnid_t pdid;
        scope_did = cnid_for_path(slq->slq_vol->v_cdb,
                                  slq->slq_vol->v_path,
                                  slq->slq_scope, &pdid);

        if (scope_did == CNID_INVALID) {
            if (CNID_ERRNO() == CNID_ERR_RESET) {
                /* Resolving a scope inserts its path components, so a search
                 * can trip the wipe */
                LOG(log_error, logtype_sl,
                    "cnid backend: CNID table for volume '%s' was reset while "
                    "resolving the search scope", slq->slq_vol->v_path);
                cnid_volume_reset(slq->slq_vol);
                slq->slq_state = SLQ_STATE_ERROR;
                EC_FAIL;
            }

            LOG(log_info, logtype_sl,
                "cnid backend: cannot resolve scope \"%s\", "
                "returning 0 results", slq->slq_scope);
            slq->slq_state = SLQ_STATE_DONE;
            EC_EXIT;
        }

        if (scope_did == DIRDID_ROOT) {
            scope_did = CNID_INVALID;
        }
    }

    /*
     * With no limit the result count is unknown up front, so the buffer
     * starts small and the search re-runs against a larger buffer while the
     * backend still reports further matches. Bounded searches allocate once.
     */
    for (;;) {
        csq->count = cnid_find_scoped(slq->slq_vol->v_cdb, csq->names,
                                      scope_did, csq->cnids,
                                      (size_t)csq->cap * sizeof(cnid_t),
                                      &csq->more_available);

        if (csq->count < 0) {
            LOG(log_error, logtype_sl, "cnid backend: the search failed");
            csq->count = 0;
            EC_FAIL;
        }

        LOG(log_debug, logtype_sl,
            "cnid backend: the search returned %d result(s)", csq->count);

        if (!unlimited || !csq->more_available || csq->cap >= SL_CNID_MAX_CAP) {
            break;
        }

        {
            /* Grow in large steps: every round re-runs the search */
            int grown = sl_cnid_cap_for((uint64_t)csq->cap * SL_CNID_GROWTH);
            cnid_t *bigger = talloc_realloc(csq, csq->cnids, cnid_t,
                                            (size_t)grown);

            if (bigger == NULL) {
                LOG(log_error, logtype_sl, "cnid backend: out of memory");
                slq->slq_state = SLQ_STATE_ERROR;
                EC_FAIL;
            }

            LOG(log_debug, logtype_sl,
                "cnid backend: growing candidate buffer %d -> %d",
                csq->cap, grown);
            csq->cnids = bigger;
            csq->cap = grown;
        }
    }

    /*
     * The capacity is at least CNID_FIND_MIN_RESULTS, so a search bounded
     * under it can collect more results than asked for.
     */
    if (!unlimited && (uint64_t)csq->count > limit) {
        csq->count = (int)limit;
        LOG(log_info, logtype_sl,
            "cnid backend: capping results at %" PRIu64, limit);
    }

    EC_ZERO(sl_cnid_fill_results(slq));
EC_CLEANUP:
    EC_EXIT;
}

/*!
 * @brief Fill the next result page of a CNID query
 *
 * A page that holds results already goes out first, since open_query fills
 * the first one. A finished query adds nothing.
 *
 * @param[in,out] slq  query
 * @returns 0, or -1 without a result set or when the fill fails
 */
static int sl_cnid_fetch_results(slq_t *slq)
{
    if (slq->query_results == NULL) {
        LOG(log_error, logtype_sl, "cnid backend: no result handle");
        slq->slq_state = SLQ_STATE_ERROR;
        return -1;
    }

    /*
     * open_query() pre-fills the current result page. Let the RPC layer send
     * it before filling the next page into the fresh handle it creates.
     */
    if (slq->query_results->num_results > 0 || slq->slq_state == SLQ_STATE_DONE) {
        return 0;
    }

    return sl_cnid_fill_results(slq);
}

/*!
 * @brief Free the backend state of a CNID query
 *
 * @param[in,out] slq  query
 */
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
