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

#include "etc/afpd/volume.h"
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
 * qsort comparator for the 32-bit network-byte-order CNIDs in the
 * result buffer (cnid_comp_fn above compares the 64-bit host-order
 * values used for the client filter).
 */
static int cnid32_comp_fn(const void *p1, const void *p2)
{
    const cnid_t *c1 = p1, *c2 = p2;

    if (*c1 == *c2) {
        return 0;
    }

    return (*c1 < *c2) ? -1 : 1;
}

/*
 * Candidate-buffer sizing. The buffer holds the CNIDs of every search
 * term before deduplication, so it is allocated from 'spotlight results
 * limit'. An unlimited limit starts at SL_CNID_START_RESULTS and doubles
 * until cnid_find() stops reporting further matches.
 *
 * cnid_find() rejects a buffer smaller than CNID_FIND_MIN_RESULTS, so the
 * capacity must also leave every term a full minimum batch, or the terms
 * that find no room are silently dropped from the search.
 *
 * SL_CNID_MAX_CAP keeps the growth loop's arithmetic in range and bounds
 * what one query can allocate; it is the same ceiling the option itself
 * is clamped to.
 */
#define SL_CNID_START_RESULTS 10000
#define SL_CNID_MAX_CAP       SPOTLIGHT_RESULTS_LIMIT_MAX
#define SL_CNID_GROWTH        8

/*
 * The dbd CNID scheme is end-of-life: it is excluded from 'spotlight
 * results limit' and hard capped here, so neither a raised limit nor an
 * unlimited one widens a dbd search.
 */
#define SL_CNID_DBD_HARD_CAP  10000

/*
 * Minimum filename-substring length the Spotlight CNID backend will pass
 * through to cnid_find(). Sub-3-char terms produce too many matches
 * against a typical CNID database to be useful, so we silently return no
 * results. This policy is Spotlight-only: `nad find` and FPCatSearch /
 * CatSearchExt continue to accept 1- and 2-character prefixes through
 * the same cnid_find() API.
 */
#define SL_CNID_MIN_TERMLEN 3

/*
 * Maximum filename predicates honored per query. Finder joins one
 * predicate pair per typed word with ||, so this bounds the number
 * of words in a single search string.
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
};

/****************************************************************************
 * Query term extraction
 ****************************************************************************/

/*!
 * @brief Extract the quoted value starting at or after `p`
 *
 * Scans to the first unescaped closing quote, strips unescaped
 * leading/trailing '*' wildcards, drops one wrapping escaped-quote
 * pair (Finder's exact-phrase delimiters; the substring search
 * preserves adjacency by construction) and removes the remaining
 * backslash escapes.
 *
 * @param mem_ctx  talloc context for the returned term
 * @param p        query-string position the scan starts from
 * @param endp     set to just past the closing quote when a quoted
 *                 span was found (even if the term is rejected), NULL
 *                 when no quoted span follows — the caller must stop
 *                 scanning then
 * @returns talloc'd term, or NULL when no quoted value follows or
 *          the result is shorter than SL_CNID_MIN_TERMLEN
 */
static char *sl_cnid_quoted_value(TALLOC_CTX *mem_ctx, const char *p,
                                  const char **endp)
{
    const char *quote_start, *quote_end = NULL;
    char *term;
    size_t len;
    *endp = NULL;
    quote_start = strchr(p, '"');

    if (quote_start == NULL) {
        return NULL;
    }

    quote_start++;

    for (const char *scan = quote_start; *scan; scan++) {
        if (*scan == '\\' && scan[1] != '\0') {
            scan++;
            continue;
        }

        if (*scan == '"') {
            quote_end = scan;
            break;
        }
    }

    if (quote_end == NULL) {
        return NULL;
    }

    *endp = quote_end + 1;
    term = talloc_strndup(mem_ctx, quote_start,
                          (size_t)(quote_end - quote_start));

    if (term == NULL) {
        return NULL;
    }

    /* strip leading '*' wildcards */
    {
        const char *t = term;

        while (*t == '*') {
            t++;
        }

        if (t != term) {
            memmove(term, t, strnlen(t, MAXPATHLEN) + 1);
        }
    }
    /* strip trailing unescaped '*' wildcards */
    len = strnlen(term, MAXPATHLEN);

    while (len > 0 && term[len - 1] == '*'
            && !(len > 1 && term[len - 2] == '\\')) {
        term[--len] = '\0';
    }

    /* drop one wrapping escaped-quote pair: \"two words\" */
    if (len >= 4
            && term[0] == '\\' && term[1] == '"'
            && term[len - 1] == '"' && term[len - 2] == '\\') {
        term[len - 2] = '\0';
        memmove(term, term + 2, len - 3);
    }

    /* remove one level of backslash escaping: \" -> ", \* -> * */
    {
        const char *r = term;
        char *w = term;

        while (*r) {
            if (r[0] == '\\' && (r[1] == '"' || r[1] == '*')) {
                r++;
            }

            *w++ = *r++;
        }

        *w = '\0';
    }

    if (strnlen(term, MAXPATHLEN) < (size_t)SL_CNID_MIN_TERMLEN) {
        LOG(log_debug, logtype_sl,
            "cnid backend: ignoring term \"%s\" "
            "(length < SL_CNID_MIN_TERMLEN=%d)",
            term, SL_CNID_MIN_TERMLEN);
        return NULL;
    }

    return term;
}

/*!
 * Append `term` to `terms` unless already present.
 *
 * @returns true when the term was appended
 */
static bool sl_cnid_add_term(char **terms, int *count, char *term)
{
    for (int j = 0; j < *count; j++) {
        if (strcmp(terms[j], term) == 0) {
            return false;
        }
    }

    terms[(*count)++] = term;
    return true;
}

/*!
 * @brief Extract all filename search terms from a Spotlight query
 *
 * Handles the common macOS patterns:
 *   kMDItemFSName = "foo*"cd       → "foo"
 *   kMDItemDisplayName = "*foo*"cd → "foo"
 *   _kMDItemFileName = "foo*"cd    → "foo"
 *   *=="foo*"cdw                   → "foo"
 *
 * Finder splits a multi-word search into one predicate per word
 * joined with ||, so every extracted term contributes to the
 * result set. Named filename attributes take precedence; the *==
 * "any attribute" form is scanned only when no named attribute
 * matched. Duplicate terms are collapsed.
 *
 * @returns the number of talloc'd terms stored in `terms`
 */
static int sl_cnid_extract_terms(TALLOC_CTX *mem_ctx, const char *qstring,
                                 char **terms, int max_terms)
{
    static const char *const name_keys[] = {
        "kMDItemFSName",
        "kMDItemDisplayName",
        "_kMDItemFileName",
        NULL
    };
    const char *p, *key, *eq, *next;
    char *term;
    int count = 0;

    if (qstring == NULL) {
        return 0;
    }

    for (size_t i = 0; name_keys[i] != NULL; i++) {
        key = name_keys[i];
        p = strstr(qstring, key);

        while (p != NULL && count < max_terms) {
            /* Require '=' directly after the key (spaces allowed): an
             * occurrence inside another predicate's quoted value must
             * not bind to a later predicate. */
            eq = p + strnlen(key, 32);

            while (*eq == ' ') {
                eq++;
            }

            if (*eq != '=') {
                p = strstr(p + 1, key);
                continue;
            }

            term = sl_cnid_quoted_value(mem_ctx, eq + 1, &next);

            if (next == NULL) {
                break;
            }

            p = next;

            if (term == NULL) {
                /* re-anchor: `p` is past a value, not at a key */
                p = strstr(p, key);
                continue;
            }

            if (sl_cnid_add_term(terms, &count, term)) {
                LOG(log_debug, logtype_sl,
                    "cnid backend: extracted search term \"%s\"", term);
            }

            p = strstr(p, key);
        }
    }

    if (count == 0) {
        /*
         * Handle macOS's "any attribute" wildcard operator:
         * *=="term*"cdw — treat it as a filename search, the only
         * predicate we support.
         */
        for (p = strstr(qstring, "*=="); p != NULL && count < max_terms;
                p = strstr(p, "*==")) {
            term = sl_cnid_quoted_value(mem_ctx, p + 3, &next);

            if (next == NULL) {
                break;
            }

            p = next;

            if (term == NULL) {
                continue;
            }

            if (sl_cnid_add_term(terms, &count, term)) {
                LOG(log_debug, logtype_sl,
                    "cnid backend: extracted term from *== pattern: \"%s\"",
                    term);
            }
        }
    }

    if (count == max_terms) {
        LOG(log_info, logtype_sl,
            "cnid backend: term limit (%d) reached", max_terms);
    }

    return count;
}

/****************************************************************************
 * CNID → filesystem path reconstruction
 ****************************************************************************/

static const char *sl_dcache_get(struct sl_cnid_query *csq, cnid_t did)
{
    for (int i = 0; i < csq->dcache_n; i++) {
        if (csq->dcache[i].did == did) {
            return csq->dcache[i].path;
        }
    }

    return NULL;
}

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

        /*
         * Guard for cases the database-side scope cannot cover: an old
         * cnid_dbd daemon ignoring the scope field, the mysql CTE
         * fallback on old servers, and renames racing the query.
         */
        if (!sl_path_in_scope(path, slq->slq_scope)) {
            LOG(log_debug, logtype_sl,
                "cnid backend: out of scope, skipping: %s", path);
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
 * Candidate capacity for `want` results across `nterms` search terms.
 *
 * Every term needs a full CNID_FIND_MIN_RESULTS batch of room or
 * cnid_find() refuses the call, so a small limit still allocates enough
 * for all the terms; the surplus is trimmed from the results afterwards.
 */
static int sl_cnid_cap_for(uint64_t want, int nterms)
{
    uint64_t floor = (uint64_t)nterms * CNID_FIND_MIN_RESULTS;

    if (want < floor) {
        want = floor;
    }

    if (want > SL_CNID_MAX_CAP) {
        want = SL_CNID_MAX_CAP;
    }

    return (int)want;
}

/*!
 * @brief Search every term into the candidate buffer and deduplicate
 *
 * Finder joins one predicate per typed word with ||, so the terms are
 * alternatives: one cnid_find() per term into the shared buffer, then a
 * sort and unique pass because a name can match more than one term.
 * Each call's slice is kept a multiple of CNID_FIND_MIN_RESULTS so the
 * DBD pagination loop fills complete batches.
 *
 * Restartable: count and the truncation flag are reset on entry so the
 * caller can re-run against a grown buffer.
 *
 * @return 0 on success, -1 when a backend search failed
 */
static int sl_cnid_collect(slq_t *slq, struct sl_cnid_query *csq,
                           char **terms, int nterms, cnid_t scope_did)
{
    csq->count = 0;
    csq->more_available = false;

    for (int i = 0; i < nterms; i++) {
        /*
         * Hold back a batch for each term still to come: a broad early
         * term would otherwise consume the buffer and the rest of the
         * words would silently drop out of the search.
         */
        int reserve = (nterms - 1 - i) * CNID_FIND_MIN_RESULTS;
        int remaining = csq->cap - csq->count - reserve;
        int found;
        bool more = false;
        /* whole batches only: cnid_find() discards a partial tail */
        remaining -= remaining % CNID_FIND_MIN_RESULTS;

        /* cnid_find() requires at least a minimum batch of room */
        if (remaining < CNID_FIND_MIN_RESULTS) {
            csq->more_available = true;
            LOG(log_debug, logtype_sl,
                "cnid backend: candidate buffer full before term \"%s\"",
                terms[i]);
            break;
        }

        LOG(log_debug, logtype_sl,
            "cnid backend: calling cnid_find for term \"%s\"", terms[i]);
        found = cnid_find_scoped(slq->slq_vol->v_cdb,
                                 terms[i], strnlen(terms[i], MAXPATHLEN),
                                 scope_did,
                                 csq->cnids + csq->count,
                                 (size_t)remaining * sizeof(cnid_t),
                                 &more);

        if (found < 0) {
            LOG(log_error, logtype_sl,
                "cnid backend: cnid_find failed for term \"%s\"", terms[i]);
            return -1;
        }

        LOG(log_debug, logtype_sl,
            "cnid backend: cnid_find(\"%s\") returned %d result(s)",
            terms[i], found);
        csq->count += found;

        if (more) {
            csq->more_available = true;
        }
    }

    if (nterms > 1 && csq->count > 1) {
        int w = 1;
        qsort(csq->cnids, (size_t)csq->count, sizeof(cnid_t),
              cnid32_comp_fn);

        for (int r = 1; r < csq->count; r++) {
            if (csq->cnids[r] != csq->cnids[w - 1]) {
                csq->cnids[w++] = csq->cnids[r];
            }
        }

        csq->count = w;
    }

    return 0;
}

/*!
 * True when `scope` names the volume root (ignoring trailing
 * slashes).
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

static int sl_cnid_open_query(slq_t *slq)
{
    EC_INIT;
    char *terms[SL_CNID_MAX_TERMS];
    int nterms;
    cnid_t scope_did;
    uint64_t limit;
    bool unlimited;
    struct sl_cnid_query *csq = NULL;
    LOG(log_debug, logtype_sl,
        "cnid backend: open_query called, qstring: \"%s\"",
        slq->slq_qstring);
    nterms = sl_cnid_extract_terms(slq, slq->slq_qstring, terms,
                                   SL_CNID_MAX_TERMS);

    if (nterms == 0) {
        /*
         * Query type not supported (e.g. kMDItemTextContent, date
         * ranges). Return no results gracefully.
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
    limit = slq->slq_result_limit;

    /*
     * dbd is end-of-life and takes no part in 'spotlight results limit':
     * it keeps its own hard cap whatever the option says.
     */
    if (slq->slq_vol->v_cnidscheme != NULL
            && STRCMP(slq->slq_vol->v_cnidscheme, ==, "dbd")
            && (limit == 0 || limit > SL_CNID_DBD_HARD_CAP)) {
        LOG(log_debug, logtype_sl,
            "cnid backend: dbd scheme is capped at %d results",
            SL_CNID_DBD_HARD_CAP);
        limit = SL_CNID_DBD_HARD_CAP;
    }

    unlimited = (limit == 0);
    csq->cap = sl_cnid_cap_for(unlimited ? SL_CNID_START_RESULTS : limit,
                               nterms);
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
     * starts small and the whole collection re-runs against a doubled
     * buffer while the backend still reports further matches. Bounded
     * searches allocate once.
     */
    for (;;) {
        EC_ZERO(sl_cnid_collect(slq, csq, terms, nterms, scope_did));

        if (!unlimited || !csq->more_available || csq->cap >= SL_CNID_MAX_CAP) {
            break;
        }

        {
            /*
             * Grow in large steps: every round re-runs the search, and a
             * substring match cannot use an index, so each round is a
             * full scan per term.
             */
            int grown = sl_cnid_cap_for((uint64_t)csq->cap * SL_CNID_GROWTH,
                                        nterms);
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
     * The capacity is rounded up to a whole batch, so a bounded search
     * can collect a few more candidates than asked for.
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
