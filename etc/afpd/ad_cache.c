/*
 * Copyright (c) 2026 Andy Lemin (andylemin)
 *
 * AD (AppleDouble) metadata cache layer for dircache.
 * Provides ad_store_to_cache(), ad_rebuild_from_cache(), and
 * ad_metadata_cached() — the unified AD metadata access point
 * for all etc/afpd/ callers.
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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <atalk/adouble.h>
#include <atalk/directory.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "dircache.h"
#include "directory.h"

#include "ad_cache.h"
#include "unix.h"

/* AD cache statistics — logged by log_dircache_stat() in dircache.c */
unsigned long long ad_cache_hits;
unsigned long long ad_cache_misses;
unsigned long long ad_cache_no_ad;

/*!
 * @brief Store AD metadata results into struct dir cache fields
 *
 * Called after every ad_metadata() disk read when a cached entry is available.
 * Pre-computes the served FILPBIT_MDATE value as max(ad_mdate, dcache_mtime).
 *
 * ad_entry() returns NULL if the entry doesn't exist in this AD version.
 * This is fine — zero-initialized cache fields are valid defaults:
 *   zero FinderInfo = no custom attributes
 *   zero FileDatesI = overridden by stat mtime fallback
 *   zero AFPFileI = no attributes
 */
void ad_store_to_cache(struct adouble *adp, struct dir *cached)
{
    const char *ade;

    if ((ade = ad_entry(adp, ADEID_FINDERI)) != NULL) {
        memcpy(cached->dcache_finderinfo, ade, ADEDLEN_FINDERI);
    }

    if ((ade = ad_entry(adp, ADEID_FILEDATESI)) != NULL) {
        memcpy(cached->dcache_filedatesi, ade, ADEDLEN_FILEDATESI);
    }

    if ((ade = ad_entry(adp, ADEID_AFPFILEI)) != NULL) {
        memcpy(cached->dcache_afpfilei, ade, ADEDLEN_AFPFILEI);
    }

    cached->dcache_rlen = adp->ad_rlen;
    /* Pre-compute served mdate = max(ad_mdate, stat_mtime).
     * Consumed by files (FILPBIT_MDATE checks AD first, then stat).
     * Not consumed by directories (DIRPBIT_MDATE always uses stat mtime).
     * Harmless for both — dirs simply never read this slot for MDATE. */
    uint32_t ad_mdate;
    memcpy(&ad_mdate, cached->dcache_filedatesi + AD_DATE_MODIFY,
           sizeof(ad_mdate));
    time_t ad_mtime = AD_DATE_TO_UNIX(ad_mdate);

    if (cached->dcache_mtime > ad_mtime) {
        uint32_t st_mdate = AD_DATE_FROM_UNIX(cached->dcache_mtime);
        memcpy(cached->dcache_filedatesi + AD_DATE_MODIFY,
               &st_mdate, sizeof(st_mdate));
    }
}

/*!
 * @brief Populate struct adouble from struct dir cache fields
 *
 * Sets up the entry directory in adp so that ad_getattr(), ad_getdate(),
 * ad_entry() all work normally — downstream code is completely unchanged.
 *
 * @pre ad_init() must have been called on adp to set up
 * ad_eid[] offsets and valid_data_len via ad_init_offsets().
 * As of E-006, ad_init() now calls ad_init_offsets() internally.
 */
void ad_rebuild_from_cache(struct adouble *adp, const struct dir *cached)
{
    AFP_ASSERT(adp->ad_magic == AD_MAGIC);
    AFP_ASSERT(adp->valid_data_len > 0);
    LOG(log_debug, logtype_afpd,
        "ad_rebuild_from_cache: serving AD from cache (rlen:%lld)",
        (long long)cached->dcache_rlen);
    char *base = adp->ad_data;

    /* ADEID_FINDERI: 32 bytes */
    if (adp->ad_eid[ADEID_FINDERI].ade_off > 0
            && adp->ad_eid[ADEID_FINDERI].ade_off + ADEDLEN_FINDERI <= AD_DATASZ_MAX) {
        memcpy(base + adp->ad_eid[ADEID_FINDERI].ade_off,
               cached->dcache_finderinfo, ADEDLEN_FINDERI);
    }

    /* ADEID_FILEDATESI: 16 bytes (SERVED values with pre-computed mdate) */
    if (adp->ad_eid[ADEID_FILEDATESI].ade_off > 0
            && adp->ad_eid[ADEID_FILEDATESI].ade_off + ADEDLEN_FILEDATESI <=
            AD_DATASZ_MAX) {
        memcpy(base + adp->ad_eid[ADEID_FILEDATESI].ade_off,
               cached->dcache_filedatesi, ADEDLEN_FILEDATESI);
    }

    /* ADEID_AFPFILEI: 4 bytes */
    if (adp->ad_eid[ADEID_AFPFILEI].ade_off > 0
            && adp->ad_eid[ADEID_AFPFILEI].ade_off + ADEDLEN_AFPFILEI <= AD_DATASZ_MAX) {
        memcpy(base + adp->ad_eid[ADEID_AFPFILEI].ade_off,
               cached->dcache_afpfilei, ADEDLEN_AFPFILEI);
    }

    adp->ad_rlen = cached->dcache_rlen;
}

/*!
 * @brief Unified AD metadata access with integrated cache management
 *
 * Read-only metadata accessor with internal ad_close(). Callers needing a
 * writable fd (e.g., for ad_setid/ad_flush) must call ad_open() separately.
 *
 * strict=false: cache-first (enumerate, FPGetFileDirParms).
 * strict=true: validate ctime/inode before serving (moveandrename, deletecurdir).
 * recent_st: optional stat to avoid duplicate ostat() in strict path.
 */
int ad_metadata_cached(const char *name, int flags, struct adouble *adp,
                       const struct vol *vol, struct dir *dir,
                       bool strict, struct stat *recent_st)
{
    /* Strict path — validate ctime/inode BEFORE serving from cache.
     * If changed, use dir_modify(DCMOD_STAT) for coherent cache update:
     *   - inode change: CNID refresh from DB + stat update + AD invalidation
     *   - ctime change: stat update + AD invalidation
     * Then fall through to disk_read for fresh AD data via ad_metadata(). */
    if (strict && dir) {
        struct stat st_buf;
        struct stat *stp = recent_st;

        if (!stp) {
            if (ostat(name, &st_buf, 0) != 0) {
                goto disk_read;
            }

            stp = &st_buf;
        }

        if (dir->dcache_ino != stp->st_ino || dir->dcache_ctime != stp->st_ctime) {
            /* External change detected — dir_modify handles coherence:
             * updates stat fields, refreshes CNID on inode change,
             * and invalidates dcache_rlen so we fall through to disk_read */
            dir_modify(vol, dir, &(struct dir_modify_args) {
                .flags = DCMOD_STAT,
                .st = stp,
            });
        } else if (dir->dcache_rlen >= 0) {
            /* ctime/inode match — stat+AD are both valid, serve from cache */
            adp->ad_adflags = flags;
            ad_rebuild_from_cache(adp, dir);
            ad_cache_hits++;
            return 0;
        } else if (dir->dcache_rlen == (off_t) -2) {
            /* Confirmed no AD exists and ctime unchanged — still no AD */
            ad_cache_no_ad++;
            errno = ENOENT;
            return -1;
        }

        /* dcache_rlen == -1: not yet loaded (or just invalidated), fall through */
    }

    if (!strict && dir && dir->dcache_rlen >= 0) {
        /* If caller provided a recent stat, validate ctime/inode. */
        if (recent_st
                && (dir->dcache_ctime != recent_st->st_ctime
                    || dir->dcache_ino != recent_st->st_ino)) {
            dir_modify(vol, dir, &(struct dir_modify_args) {
                .flags = DCMOD_STAT,
                .st = recent_st,
            });
            goto disk_read;
        }

        adp->ad_adflags = flags;
        ad_rebuild_from_cache(adp, dir);
        ad_cache_hits++;
        return 0;
    }

    if (dir && dir->dcache_rlen == (off_t) -2) {
        /* If caller provided a recent stat and ctime/inode changed,
         * the AD file may have been created since — re-check on disk. */
        if (recent_st
                && (dir->dcache_ctime != recent_st->st_ctime
                    || dir->dcache_ino != recent_st->st_ino)) {
            dir_modify(vol, dir, &(struct dir_modify_args) {
                .flags = DCMOD_STAT,
                .st = recent_st,
            });
            goto disk_read;
        }

        /* Confirmed no AD exists and ctime unchanged — still no AD */
        ad_cache_no_ad++;
        errno = ENOENT;
        return -1;
    }

disk_read:
    /* Cache miss or strict validation detected stale data — read from disk */
    ad_cache_misses++;
    int ret = ad_metadata(name, flags, adp);

    if (ret == 0) {
        if (dir && dir->d_did != CNID_INVALID) {
            /* Guard: dir_modify(DCMOD_STAT) may have evicted the entry
             * (d_did set to CNID_INVALID) during strict validation above.
             * Don't store to an evicted entry — it's unreachable. */
            ad_store_to_cache(adp, dir);
        }

        /* Always close internally — callers needing a writable fd
         * must call ad_open() separately after ad_metadata_cached() returns. */
        ad_close(adp, ADFLAGS_HF | flags);
    } else if (ret < 0 && errno == ENOENT && dir
               && dir->d_did != CNID_INVALID) {
        /* Free rfork buffer FIRST — rfork_cache_free() uses dcache_rlen for budget.
         * Setting rlen = -2 before freeing would corrupt rfork_cache_used tracking. */
        if (dir->dcache_rfork_buf) {
            rfork_cache_free(dir);
            rfork_stat_invalidated++;
        }

        dir->dcache_rlen = (off_t) -2;
    }

    return ret;
}

/******************************************************************************
 * Tier 2: Resource Fork data cache implementation
 *
 * Budget management globals and stat counters are defined in dircache.c
 * (non-static) and accessed here via extern declarations in dircache.h.
 ******************************************************************************/

/*!
 * @brief Free a single entry's rfork buffer, remove from rfork LRU, update counter
 *
 * Uses dcache_rlen for the buffer size.
 * INVARIANT: dcache_rlen >= 0 when dcache_rfork_buf != NULL.
 * Uses production-safe fallback: if dcache_rlen < 0 (invariant violation),
 * logs error, frees buffer, but does NOT touch budget counter (unknown size).
 * Handles rfork_lru_node == NULL gracefully (orphaned buffer from
 * enqueue failure — budget is still decremented correctly).
 * Decrements rfork_lru_count when removing from LRU.
 * Does NOT reset dcache_rlen — the AD metadata remains valid.
 */
void rfork_cache_free(struct dir *entry)
{
    if (entry->dcache_rfork_buf == NULL) {
        LOG(log_error, logtype_afpd,
            "rfork_cache_free: called with NULL buf (did:%u)",
            ntohl(entry->d_did));
        return;
    }

    if (entry->dcache_rlen < 0) {
        /* INVARIANT VIOLATION: buf != NULL but rlen < 0.
         * Production-safe: log error, free buf, but don't touch budget
         * (unknown size would corrupt the counter). */
        LOG(log_error, logtype_afpd,
            "rfork_cache_free: INVARIANT VIOLATION buf!=NULL but rlen=%lld (did:%u)",
            (long long)entry->dcache_rlen, ntohl(entry->d_did));
        free(entry->dcache_rfork_buf);
        entry->dcache_rfork_buf = NULL;

        if (entry->rfork_lru_node) {
            entry->rfork_lru_node->prev->next = entry->rfork_lru_node->next;
            entry->rfork_lru_node->next->prev = entry->rfork_lru_node->prev;
            free(entry->rfork_lru_node);
            entry->rfork_lru_node = NULL;
            rfork_lru_count--;
        }

        return;
    }

    /* Normal path: decrement budget by dcache_rlen, free buf, remove from LRU */
    if ((size_t)entry->dcache_rlen > rfork_cache_used) {
        LOG(log_error, logtype_afpd,
            "rfork_cache_free: budget underflow detected "
            "(rlen=%lld, used=%zu, did:%u) — resetting to 0",
            (long long)entry->dcache_rlen, rfork_cache_used,
            ntohl(entry->d_did));
        rfork_cache_used = 0;
    } else {
        rfork_cache_used -= (size_t)entry->dcache_rlen;
    }

    free(entry->dcache_rfork_buf);
    entry->dcache_rfork_buf = NULL;

    if (entry->rfork_lru_node) {
        /* Manual unlink from rfork LRU doubly-linked list.
         * WARNING: Do NOT use dequeue() here — it frees the qnode before we can
         * access node->data to get the struct dir. We must manually unlink. */
        entry->rfork_lru_node->prev->next = entry->rfork_lru_node->next;
        entry->rfork_lru_node->next->prev = entry->rfork_lru_node->prev;
        free(entry->rfork_lru_node);
        entry->rfork_lru_node = NULL;
        rfork_lru_count--;
    }
}

/*!
 * @brief Evict rfork buffers from rfork LRU head (oldest/LRU) until under budget
 *
 * Queue convention: sentinel->next = head = LRU (oldest),
 *                   sentinel->prev = tail = MRU (newest).
 * Called by rfork_cache_store_from_fd() when budget would be exceeded.
 * O(k) where k = entries evicted.
 */
void rfork_cache_evict_to_budget(size_t needed)
{
    /* Walk rfork LRU from head (oldest/LRU) → tail (newest/MRU), freeing
     * buffers until under budget. O(k) where k = entries evicted.
     * WARNING: Do NOT use dequeue() here — it frees the qnode before we
     * can access node->data to get the struct dir. Manual unlinking required. */
    while (rfork_lru && rfork_lru->next != rfork_lru
            && rfork_cache_used + needed > rfork_cache_budget) {
        qnode_t *head = rfork_lru->next;
        struct dir *entry = (struct dir *)head->data;
        AFP_ASSERT(entry->dcache_rfork_buf != NULL);
        rfork_cache_free(entry);
        rfork_stat_evicted++;
    }
}

/*!
 * @brief Store resource fork data by reading directly from the ad fd
 *
 * Allocates dcache_rlen bytes and does ad_read(adp, eid, 0, buf, dcache_rlen).
 * ad_read() handles all storage formats (EA, macOS xattr, AD v2 sidecar).
 * Guards: returns -1 if !AD_RSRC_OPEN(adp) (fd not open), dcache_rlen <= 0,
 * or rlen > rfork_max_entry_size.
 * Validates that ad_read returns exactly dcache_rlen bytes — if not, the fork
 * size changed since Tier 1 metadata was cached: invalidates ALL cached AD
 * metadata (Tier 1) and returns -1 (self-healing).
 *
 * @returns 0 on success, -1 if not cacheable, short read, or ad_read failed.
 */
int rfork_cache_store_from_fd(struct dir *entry, struct adouble *adp, int eid)
{
    AFP_ASSERT(entry->d_flags & DIRF_ISFILE);

    if (!(entry->d_flags & DIRF_ISFILE)) {
        LOG(log_error, logtype_afpd,
            "rfork_cache_store_from_fd: called for non-file entry did:%u",
            ntohl(entry->d_did));
        return -1;
    }

    /* Defense-in-depth: free any existing buffer (should never be needed) */
    if (entry->dcache_rfork_buf) {
        LOG(log_error, logtype_afpd,
            "rfork_cache_store_from_fd: buf already set (did:%u), freeing first",
            ntohl(entry->d_did));
        rfork_cache_free(entry);
    }

    if (!AD_RSRC_OPEN(adp)) {
        return -1;
    }

    if (entry->dcache_rlen <= 0) {
        return -1;
    }

    size_t rlen = (size_t)entry->dcache_rlen;

    if (rlen > rfork_max_entry_size) {
        return -1;
    }

    /* Pathological check. Skip if oversubscribed by >2x — eviction won't free enough.
     * Use subtraction to avoid size_t overflow on 32-bit platforms. */
    if (rfork_cache_used > rfork_cache_budget
            && rfork_cache_used - rfork_cache_budget > rfork_cache_budget) {
        return -1;
    }

    /* Evict LRU entries until budget has room */
    if (rfork_cache_used + rlen > rfork_cache_budget) {
        rfork_cache_evict_to_budget(rlen);
    }

    /* Post-eviction check */
    if (rfork_cache_used + rlen > rfork_cache_budget) {
        return -1;
    }

    char *buf = malloc(rlen);

    if (!buf) {
        return -1;
    }

    /* AFP clients may read resource forks in chunks (e.g., 32KB at a time). We
     * upgrade first read to full-fork ad_read() to ensure the cache contains the
     * whole fork. Subsequent FPReads are served from cache.
     * ad_read() handles all storage formats (EA, macOS xattr, AD v2 sidecar). */
    ssize_t bytes_read = ad_read(adp, eid, 0, buf, (size_t)entry->dcache_rlen);

    if (bytes_read != entry->dcache_rlen) {
        /* Short read: fork size changed since Tier 1 was populated.
         * Invalidate ALL cached metadata (not just rlen) — AD fields may also
         * be stale. Self-healing: next ad_metadata_cached() re-reads from disk. */
        free(buf);
        LOG(log_debug, logtype_afpd,
            "rfork_cache_store_from_fd: short read (%zd vs %lld) — "
            "external change detected, invalidating AD cache (did:%u)",
            bytes_read, (long long)entry->dcache_rlen, ntohl(entry->d_did));
        entry->dcache_rlen = (off_t) -1;
        memset(entry->dcache_finderinfo, 0, 32);
        memset(entry->dcache_filedatesi, 0, 16);
        memset(entry->dcache_afpfilei, 0, 4);
        return -1;
    }

    /* Store buf in entry, update budget, add to rfork LRU */
    entry->dcache_rfork_buf = buf;
    rfork_cache_used += (size_t)entry->dcache_rlen;

    if (rfork_cache_used > rfork_stat_used_max) {
        rfork_stat_used_max = rfork_cache_used;
    }

    entry->rfork_lru_node = enqueue(rfork_lru, entry);

    if (entry->rfork_lru_node == NULL) {
        /* enqueue() malloc failed — clean up to prevent orphaned buffer */
        free(entry->dcache_rfork_buf);
        entry->dcache_rfork_buf = NULL;
        rfork_cache_used -= (size_t)entry->dcache_rlen;
        return -1;
    }

    rfork_lru_count++;
    return 0;
}
