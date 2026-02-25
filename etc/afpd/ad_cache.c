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
#include <string.h>
#include <sys/stat.h>

#include <atalk/adouble.h>
#include <atalk/directory.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/volume.h>

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
        dir->dcache_rlen = (off_t) -2;
    }

    return ret;
}
