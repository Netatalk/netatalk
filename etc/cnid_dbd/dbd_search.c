/*
 * Copyright (C) Frank Lahm 2010
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <atalk/cnid_bdb_private.h>
#include <atalk/compat.h>           /* explicit_bzero */
#include <atalk/logger.h>

#include "dbd.h"
#include "dbif.h"
#include "pack.h"

int dbd_search(DBD *dbd, struct cnid_dbd_rqst *rqst, struct cnid_dbd_rply *rply)
{
    DBT key;
    int results;
    uint32_t offset;
    bool more = false;
    const char *search_name;
    size_t search_namelen;
    /* Single-threaded daemon (etc/cnid_dbd/main.c uses select()), so a
     * static reply buffer is safe and reused across requests without
     * locking. Per-request namelen bounds the bytes actually sent on
     * the wire; do not assume bytes past rply->namelen are zero. */
    static char resbuf[DBD_MAX_SRCH_RSLTS * sizeof(cnid_t)];
    rply->name = resbuf;
    rply->namelen = 0;
    /* Defence-in-depth: zero `resbuf` between requests so any future
     * write-side bug (e.g. forgetting to update rply->namelen) cannot
     * leak stale CNIDs from a prior request to the same TCP client. */
    explicit_bzero(resbuf, sizeof(resbuf));

    /* Peel off the 4-byte native-byte-order srch_offset prefix. */
    if (rqst->namelen < sizeof(uint32_t)) {
        LOG(log_warning, logtype_cnid,
            "dbd_search: malformed payload (namelen %zu < 4)",
            rqst->namelen);
        /* Client-induced validation error: MUST return 1 (not -1); see
         * etc/cnid_dbd/main.c — a -1 return aborts the entire daemon
         * which would be a remote-DoS vector. -1 is reserved for genuine
         * BDB engine errors that warrant daemon recycle. */
        rply->result = CNID_DBD_RES_ERR_DB;
        return 1;
    }

    memcpy(&offset, rqst->name, sizeof(offset));
    search_name    = rqst->name + sizeof(uint32_t);
    search_namelen = rqst->namelen - sizeof(uint32_t);

    if (offset > (uint32_t)DBD_SEARCH_MAX_OFFSET) {
        LOG(log_warning, logtype_cnid,
            "dbd_search: offset %u out of range (max %d)",
            (unsigned)offset, DBD_SEARCH_MAX_OFFSET);
        rply->result = CNID_DBD_RES_ERR_DB;
        return 1;
    }

    /* Lower bound only: a 4-byte payload (offset only, no search-name)
     * is malformed. The wire-level upper bound `rqst.namelen <= MAXPATHLEN`
     * is enforced at etc/cnid_dbd/comm.c, so any search_namelen reaching
     * here is already <= MAXPATHLEN - 4; a duplicate predicate would be
     * unreachable dead code. */
    if (search_namelen == 0) {
        LOG(log_warning, logtype_cnid,
            "dbd_search: zero-length search namelen");
        rply->result = CNID_DBD_RES_ERR_DB;
        return 1;
    }

    /* %.*s is critical: the wire payload is NOT NUL-terminated. */
    LOG(log_debug, logtype_cnid,
        "dbd_search(\"%.*s\", namelen=%zu, offset=%u)",
        (int)search_namelen, search_name, search_namelen,
        (unsigned)offset);
    memset(&key, 0, sizeof(key));
    /* BDB's DBT.data is `void *` because the cursor will overwrite it on
     * subsequent pget() calls. dbif_search saves the original pointer in
     * `namebkp` before that happens, so we never read through key.data
     * after the first cursor advance. The (uintptr_t) bridge silences
     * -Wcast-qual; the const-strip is intentional and the rqst.name
     * buffer is a writeable heap allocation owned by comm_rcv. */
    key.data = (void *)(uintptr_t)search_name;
    key.size = (uint32_t)search_namelen;
    /* `more` is the "is there a (offset + DBD_MAX_SRCH_RSLTS + 1)-th
     * matching entry?" flag set by dbif_search's post-fill cursor peek.
     * Critical: do NOT key the reply code on `results == DBD_MAX_SRCH_RSLTS`.
     * The boundary case where the matching range contains exactly
     * `offset + DBD_MAX_SRCH_RSLTS` entries fills the buffer to the brim
     * and has no further entry, so SRCH_DONE is the correct answer for
     * that case — the legacy heuristic mis-emitted SRCH_CNT, forcing a
     * spurious extra paginated request. */
    results = dbif_search(dbd, &key, resbuf, offset, &more);

    if (results < 0) {
        LOG(log_error, logtype_cnid,
            "dbd_search(\"%.*s\"): db error",
            (int)search_namelen, search_name);
        rply->result = CNID_DBD_RES_ERR_DB;
        /* Genuine DB error -> daemon-fatal */
        return -1;
    }

    rply->namelen = (size_t)results * sizeof(cnid_t);
    rply->result  = more
                    ? CNID_DBD_RES_SRCH_CNT
                    : CNID_DBD_RES_SRCH_DONE;
    return 1;
}
