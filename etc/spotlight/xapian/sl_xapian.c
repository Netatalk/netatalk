/*
 * Spotlight Xapian backend for Netatalk.
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

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include <talloc.h>

#include <atalk/cnid.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/spotlight.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "etc/spotlight/spotlight_private.h"
#include "etc/spotlight/xapian/sl_xapian.h"

#define SL_XAPIAN_PAGE_SIZE 20
/* Safety cap used when the global result limit is configured as unlimited. */
#define SL_XAPIAN_DEFAULT_LIMIT 10000
#define SL_XAPIAN_DB_ID_MAX 128

struct sl_xapian_seeded {
    char *volpath;
    struct sl_xapian_seeded *next;
};

struct sl_xapian_query {
    char **paths;
    size_t count;
    size_t pos;
};

static struct sl_xapian_seeded *seeded_volumes;

static int sl_xapian_mkdir_state(const char *path, mode_t mode, char *errbuf,
                                 size_t errlen)
{
    struct stat st;

    if (mkdir(path, mode) != 0 && errno != EEXIST) {
        snprintf(errbuf, errlen, "%s: %s", path, strerror(errno));
        return -1;
    }

    if (stat(path, &st) != 0) {
        snprintf(errbuf, errlen, "%s: %s", path, strerror(errno));
        return -1;
    }

    if (!S_ISDIR(st.st_mode)) {
        snprintf(errbuf, errlen, "%s exists but is not a directory", path);
        return -1;
    }

    if ((st.st_mode & mode) != mode && chmod(path, mode) != 0) {
        snprintf(errbuf, errlen, "%s: chmod failed: %s", path, strerror(errno));
        return -1;
    }

    return 0;
}

static int sl_xapian_ensure_state_root(char *errbuf, size_t errlen)
{
    int ret = 0;
    become_root();

    if (sl_xapian_mkdir_state(_PATH_STATEDIR "spotlight", 0755,
                              errbuf, errlen) != 0
            || sl_xapian_mkdir_state(_PATH_STATEDIR "spotlight/xapian", 01777,
                                     errbuf, errlen) != 0) {
        ret = -1;
    }

    unbecome_root();
    return ret;
}

static int cnid_comp_fn(const void *p1, const void *p2)
{
    const uint64_t *c1 = p1, *c2 = p2;

    if (*c1 == *c2) {
        return 0;
    }

    return (*c1 < *c2) ? -1 : 1;
}

static bool sl_xapian_path_in_scope(const char *path, const char *scope)
{
    if (path == NULL || scope == NULL) {
        return false;
    }

    if (scope[0] == '\0' || (scope[0] == '/' && scope[1] == '\0')) {
        return true;
    }

    while (*scope != '\0' && *scope == *path) {
        scope++;
        path++;
    }

    if (*scope != '\0') {
        return false;
    }

    return *path == '\0' || *path == '/';
}

static bool sl_xapian_seeded(const struct vol *vol)
{
    for (struct sl_xapian_seeded *p = seeded_volumes; p != NULL; p = p->next) {
        if (strcmp(p->volpath, vol->v_path) == 0) {
            return true;
        }
    }

    return false;
}

static int sl_xapian_mark_seeded(const struct vol *vol)
{
    struct sl_xapian_seeded *p;
    p = calloc(1, sizeof(*p));

    if (p == NULL) {
        return -1;
    }

    p->volpath = strdup(vol->v_path);

    if (p->volpath == NULL) {
        free(p);
        return -1;
    }

    p->next = seeded_volumes;
    seeded_volumes = p;
    return 0;
}

static void sl_xapian_unmark_seeded(const struct vol *vol)
{
    struct sl_xapian_seeded **pp = &seeded_volumes;

    if (vol == NULL) {
        return;
    }

    while (*pp != NULL) {
        struct sl_xapian_seeded *p = *pp;

        if (strcmp(p->volpath, vol->v_path) == 0) {
            *pp = p->next;
            free(p->volpath);
            free(p);
            return;
        }

        pp = &p->next;
    }
}

static bool sl_xapian_db_id_char_safe(unsigned char c)
{
    return (c >= 'A' && c <= 'Z')
           || (c >= 'a' && c <= 'z')
           || (c >= '0' && c <= '9')
           || c == '-' || c == '_' || c == '.';
}

static uint64_t sl_xapian_fnv1a_update(uint64_t hash, const char *s)
{
    while (s != NULL && *s != '\0') {
        hash ^= (unsigned char) * s++;
        hash *= 1099511628211ULL;
    }

    return hash;
}

static bool sl_xapian_db_id_safe(const char *id)
{
    size_t len;

    if (id == NULL || id[0] == '\0') {
        return false;
    }

    if (strcmp(id, ".") == 0 || strcmp(id, "..") == 0) {
        return false;
    }

    len = strnlen(id, SL_XAPIAN_DB_ID_MAX + 1);

    if (len > SL_XAPIAN_DB_ID_MAX) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        if (!sl_xapian_db_id_char_safe((unsigned char)id[i])) {
            return false;
        }
    }

    return true;
}

static uint64_t sl_xapian_db_id_hash(const struct vol *vol, const char *id)
{
    uint64_t hash = 1469598103934665603ULL;
    hash = sl_xapian_fnv1a_update(hash, id);
    hash = sl_xapian_fnv1a_update(hash, "\xff");
    return sl_xapian_fnv1a_update(hash, vol ? vol->v_path : NULL);
}

static char *sl_xapian_db_path(TALLOC_CTX *mem_ctx, const struct vol *vol)
{
    const char *uuid = vol->v_uuid && vol->v_uuid[0] != '\0'
                       ? vol->v_uuid : vol->v_localname;
    char *db_id;

    if (uuid == NULL || uuid[0] == '\0') {
        uuid = "unknown";
    }

    if (sl_xapian_db_id_safe(uuid)) {
        db_id = talloc_strdup(mem_ctx, uuid);
    } else {
        db_id = talloc_asprintf(mem_ctx, "unsafe-%016" PRIx64,
                                sl_xapian_db_id_hash(vol, uuid));
    }

    if (db_id == NULL) {
        return NULL;
    }

    return talloc_asprintf(mem_ctx, _PATH_STATEDIR "spotlight/xapian/%s", db_id);
}

static const char *sl_xapian_volume_uuid(const struct vol *vol)
{
    if (vol == NULL) {
        return "";
    }

    if (vol->v_uuid != NULL && vol->v_uuid[0] != '\0') {
        return vol->v_uuid;
    }

    if (vol->v_localname != NULL && vol->v_localname[0] != '\0') {
        return vol->v_localname;
    }

    return "";
}

static int sl_xapian_ensure_seeded(slq_t *slq, const char *db_path)
{
    char errbuf[512] = {0};
    const char *volume_uuid = sl_xapian_volume_uuid(slq->slq_vol);
    int ready;

    if (sl_xapian_seeded(slq->slq_vol)) {
        return 0;
    }

    if (sl_xapian_ensure_state_root(errbuf, sizeof(errbuf)) != 0) {
        LOG(log_error, logtype_sl,
            "xapian backend: failed to prepare state directory: %s",
            errbuf);
        return -1;
    }

    ready = sl_xapian_index_ready(db_path, slq->slq_vol->v_path, volume_uuid,
                                  errbuf, sizeof(errbuf));

    if (ready > 0) {
        if (sl_xapian_mark_seeded(slq->slq_vol) != 0) {
            LOG(log_error, logtype_sl, "xapian backend: out of memory");
            return -1;
        }

        return 0;
    }

    if (ready < 0) {
        LOG(log_warning, logtype_sl,
            "xapian backend: index metadata check failed for %s: %s",
            slq->slq_vol->v_path, errbuf);
    }

    LOG(log_info, logtype_sl,
        "xapian backend: reconciling index for volume %s",
        slq->slq_vol->v_path);

    if (sl_xapian_reconcile(db_path, slq->slq_vol->v_path, volume_uuid,
                            errbuf, sizeof(errbuf)) != 0) {
        LOG(log_error, logtype_sl,
            "xapian backend: reconcile failed for %s: %s",
            slq->slq_vol->v_path, errbuf);
        return -1;
    }

    if (sl_xapian_mark_seeded(slq->slq_vol) != 0) {
        LOG(log_error, logtype_sl, "xapian backend: out of memory");
        return -1;
    }

    return 0;
}

static int sl_xapian_fill_results(slq_t *slq)
{
    EC_INIT;
    struct sl_xapian_query *xsq = slq->slq_backend_private;
    int page_count = 0;

    if (slq->query_results == NULL) {
        LOG(log_error, logtype_sl, "xapian backend: no result handle");
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    if (xsq == NULL) {
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    while (xsq->pos < xsq->count && page_count < SL_XAPIAN_PAGE_SIZE) {
        const char *path = xsq->paths[xsq->pos++];
        struct stat sb;
        cnid_t did, id;
        uint64_t uint64var;
        bool ok;

        if (!sl_xapian_path_in_scope(path, slq->slq_scope)) {
            continue;
        }

        if (stat(path, &sb) != 0) {
            LOG(log_debug, logtype_sl,
                "xapian backend: skipping stale result: %s", path);
            continue;
        }

        if (access(path, R_OK) != 0) {
            LOG(log_debug, logtype_sl,
                "xapian backend: access denied: %s", path);
            continue;
        }

        id = cnid_for_path(slq->slq_vol->v_cdb, slq->slq_vol->v_path, path, &did);

        if (id == CNID_INVALID) {
            LOG(log_debug, logtype_sl,
                "xapian backend: no CNID for result: %s", path);
            continue;
        }

        uint64var = ntohl(id);

        if (slq->slq_cnids) {
            ok = bsearch(&uint64var, slq->slq_cnids, slq->slq_cnids_num,
                         sizeof(uint64_t), cnid_comp_fn);

            if (!ok) {
                continue;
            }
        }

        dalloc_add_copy(slq->query_results->cnids->ca_cnids,
                        &uint64var, uint64_t);
        ok = add_filemeta(slq->slq_reqinfo, slq->query_results->fm_array,
                          path, &sb);

        if (!ok) {
            LOG(log_error, logtype_sl, "xapian backend: add_filemeta error");
            slq->slq_state = SLQ_STATE_ERROR;
            EC_FAIL;
        }

        slq->query_results->num_results++;
        page_count++;
    }

    slq->slq_state = xsq->pos < xsq->count ? SLQ_STATE_FULL : SLQ_STATE_DONE;
EC_CLEANUP:
    EC_EXIT;
}

static int sl_xapian_init(AFPObj *obj _U_)
{
    static bool initialized;

    if (initialized) {
        return 0;
    }

    LOG(log_info, logtype_sl, "Initializing xapian backend");
    initialized = true;
    return 0;
}

static void sl_xapian_close(AFPObj *obj _U_)
{
    struct sl_xapian_seeded *p = seeded_volumes;

    while (p != NULL) {
        struct sl_xapian_seeded *next = p->next;
        free(p->volpath);
        free(p);
        p = next;
    }

    seeded_volumes = NULL;
}

static int sl_xapian_open_query(slq_t *slq)
{
    EC_INIT;
    struct sl_xapian_query *xsq;
    const char *db_path = NULL;
    char errbuf[512] = {0};
    size_t limit;
    int more = 0;
    xsq = talloc_zero(slq, struct sl_xapian_query);

    if (xsq == NULL) {
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    slq->slq_backend_private = xsq;
    db_path = sl_xapian_db_path(slq, slq->slq_vol);

    if (db_path == NULL) {
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    EC_ZERO(sl_xapian_ensure_seeded(slq, db_path));
    limit = slq->slq_result_limit ? slq->slq_result_limit : SL_XAPIAN_DEFAULT_LIMIT;

    if (sl_xapian_query(db_path, slq->slq_scope, slq->slq_qstring,
                        0, limit, &xsq->paths, &xsq->count, &more,
                        errbuf, sizeof(errbuf)) != 0) {
        LOG(log_error, logtype_sl,
            "xapian backend: query failed: %s", errbuf);
        slq->slq_state = SLQ_STATE_ERROR;
        EC_FAIL;
    }

    LOG(log_debug, logtype_sl,
        "xapian backend: query returned %zu candidate result(s)%s",
        xsq->count, more ? " (truncated)" : "");

    if (more) {
        LOG(log_info, logtype_sl,
            "xapian backend: query truncated at %zu candidate result(s); "
            "set \"sparql results limit\" to a larger nonzero value to raise the cap",
            limit);
    }

    EC_ZERO(sl_xapian_fill_results(slq));
EC_CLEANUP:
    EC_EXIT;
}

static int sl_xapian_fetch_results(slq_t *slq)
{
    if (slq->query_results == NULL) {
        LOG(log_error, logtype_sl, "xapian backend: no result handle");
        slq->slq_state = SLQ_STATE_ERROR;
        return -1;
    }

    if (slq->query_results->num_results > 0 || slq->slq_state == SLQ_STATE_DONE) {
        return 0;
    }

    return sl_xapian_fill_results(slq);
}

static void sl_xapian_close_query(slq_t *slq)
{
    struct sl_xapian_query *xsq = slq->slq_backend_private;

    if (xsq != NULL) {
        sl_xapian_free_paths(xsq->paths, xsq->count);
        xsq->paths = NULL;
        xsq->count = 0;
        talloc_free(xsq);
        slq->slq_backend_private = NULL;
    }
}

static int sl_xapian_index_event(const AFPObj *obj _U_,
                                 const struct vol *vol,
                                 sl_index_event_t event,
                                 const char *path,
                                 const char *oldpath)
{
    TALLOC_CTX *tmp = talloc_new(NULL);
    const char *db_path;
    char errbuf[512] = {0};
    int ret = 0;

    if (tmp == NULL) {
        return -1;
    }

    db_path = sl_xapian_db_path(tmp, vol);

    if (db_path == NULL) {
        talloc_free(tmp);
        return -1;
    }

    if (sl_xapian_ensure_state_root(errbuf, sizeof(errbuf)) != 0) {
        LOG(log_warning, logtype_sl,
            "xapian backend: failed to prepare state directory: %s",
            errbuf);
        talloc_free(tmp);
        return -1;
    }

    switch (event) {
    case SL_INDEX_FILE_CREATE:
    case SL_INDEX_DIR_CREATE:
    case SL_INDEX_FILE_MODIFY:
        ret = sl_xapian_upsert_path(db_path, path, errbuf, sizeof(errbuf));
        break;

    case SL_INDEX_FILE_DELETE:
        ret = sl_xapian_delete_path(db_path, path, errbuf, sizeof(errbuf));
        break;

    case SL_INDEX_DIR_DELETE:
        ret = sl_xapian_delete_prefix(db_path, path, errbuf, sizeof(errbuf));
        break;

    case SL_INDEX_FILE_MOVE:
        ret = sl_xapian_delete_path(db_path, oldpath, errbuf, sizeof(errbuf));

        if (ret == 0) {
            ret = sl_xapian_upsert_path(db_path, path, errbuf, sizeof(errbuf));
        }

        break;

    case SL_INDEX_DIR_MOVE:
        ret = sl_xapian_reindex_subtree(db_path, vol->v_path, path, oldpath,
                                        errbuf, sizeof(errbuf));
        break;
    }

    if (ret != 0) {
        char dirty_errbuf[512] = {0};
        sl_xapian_unmark_seeded(vol);

        if (sl_xapian_mark_dirty(db_path, dirty_errbuf,
                                 sizeof(dirty_errbuf)) != 0) {
            LOG(log_warning, logtype_sl,
                "xapian backend: failed to mark index dirty for %s: %s",
                vol->v_path, dirty_errbuf);
        }

        LOG(log_warning, logtype_sl,
            "xapian backend: incremental index update failed for %s: %s",
            path ? path : "(null)", errbuf);
    }

    talloc_free(tmp);
    return ret;
}

const sl_backend_ops sl_xapian_ops = {
    .sbo_name          = "xapian",
    .sbo_init          = sl_xapian_init,
    .sbo_close         = sl_xapian_close,
    .sbo_open_query    = sl_xapian_open_query,
    .sbo_fetch_results = sl_xapian_fetch_results,
    .sbo_close_query   = sl_xapian_close_query,
    .sbo_index_event   = sl_xapian_index_event,
};
