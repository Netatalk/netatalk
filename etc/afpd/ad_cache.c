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

/*!
 * @file
 * @brief AppleDouble metadata and resource fork caching in the directory cache
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include <atalk/adouble.h>
#include <atalk/compat.h>
#include <atalk/directory.h>
#include <atalk/ea.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "fork.h"

#include "ad_cache.h"
#include "unix.h"

/* AD cache statistics — logged by log_dircache_stat() in dircache.c */
unsigned long long ad_cache_hits;
unsigned long long ad_cache_misses;
unsigned long long ad_cache_no_ad;

/*!
 * @brief Store the AppleDouble metadata of an object in its dircache entry
 *
 * Every ad_metadata disk read with a dircache entry at hand ends here. The
 * entry keeps the Finder info, the dates, the AFP file info, and the resource
 * fork length. The stored modification date is the later of the AppleDouble
 * date and the stat mtime, as FILPBIT_MDATE serves it. An entry that the
 * AppleDouble version lacks stays zero, a valid default.
 *
 * @param[in]     adp     AppleDouble with its metadata loaded
 * @param[in,out] cached  dircache entry of the object
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
 * @brief Fill an AppleDouble from the metadata cached in a dircache entry
 *
 * The cached Finder info, dates, and AFP file info land at the entry offsets
 * of adp. The ad_getattr, ad_getdate, and ad_entry calls then work as after a
 * disk read. The ad_init call sets up those offsets, so the caller runs it
 * first.
 *
 * @param[in,out] adp     AppleDouble initialized with ad_init
 * @param[in]     cached  dircache entry with cached metadata
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
 * @brief Read the AppleDouble metadata of an object through the dircache
 *
 * The call closes the metadata before it returns, so callers that write open
 * it separately. The cache serves while the entry's ctime and inode match the
 * caller's recent stat; strict, as rename and delete need, takes a stat of
 * its own when the caller has none. A mismatch brings the entry to the stat
 * with dir_takes_stat() and reads from disk; an entry that does not then
 * hold the stat takes nothing from the read. A cached absence of metadata
 * answers ENOENT without a disk read.
 *
 * @param[in]     name       file or directory
 * @param[in]     flags      ADFLAGS_DIR and ADFLAGS_CHECK_OF, as for
 *                           ad_metadata
 * @param[in,out] adp        initialized AppleDouble receiving the metadata
 * @param[in]     vol        volume of the object
 * @param[in,out] dir        dircache entry of the object, or NULL
 * @param[in]     strict     true to stat the object when recent_st is NULL
 * @param[in]     recent_st  fresh stat of the object, or NULL
 * @returns 0, or -1 with errno set, ENOENT when the object has no metadata
 */
int ad_metadata_cached(const char *name, int flags, struct adouble *adp,
                       const struct vol *vol, struct dir *dir,
                       bool strict, struct stat *recent_st)
{
    struct stat st_buf;
    struct stat *stp = recent_st;

    if (strict && dir && !stp) {
        if (ostat(name, &st_buf, 0) != 0) {
            goto disk_read;
        }

        stp = &st_buf;
    }

    /* a stat with another ctime or inode: the metadata, or an AD file
     * created since, is read from disk */
    if (dir && stp && dir_stat_differs(dir, stp)) {
        dir = dir_takes_stat(vol, dir, stp) ? dir : NULL;
        goto disk_read;
    }

    if (dir && dir->dcache_rlen >= 0) {
        adp->ad_adflags = flags;
        ad_rebuild_from_cache(adp, dir);
        ad_cache_hits++;
        return 0;
    }

    if (dir && ad_rlen_meta_absent(dir->dcache_rlen)) {
        /* Confirmed no AD metadata and ctime unchanged — still none */
        ad_cache_no_ad++;
        errno = ENOENT;
        return -1;
    }

disk_read:
    /* Cache miss, or a stat the entry did not show — read from disk */
    ad_cache_misses++;
    int ret = ad_metadata(name, flags, adp);

    if (ret == 0) {
        if (dir) {
            ad_store_to_cache(adp, dir);
        }

        /* Always close internally — callers needing a writable fd
         * must call ad_open() separately after ad_metadata_cached() returns. */
        ad_close(adp, ADFLAGS_HF | flags);
    } else if (ret < 0 && errno == ENOENT && dir) {
        int saved_errno = errno;

        /* Free rfork buffer FIRST — rfork_cache_free() uses dcache_rlen for
         * budget. Minting a sentinel before freeing would corrupt
         * rfork_cache_used tracking. */
        if (dir->dcache_rfork_buf) {
            rfork_cache_free(dir);
            rfork_stat_invalidated++;
        }

        /* Metadata ENOENT does not prove the rfork is absent: with ea=sys
         * it is a separate EA (Samba vfs_fruit, macOS-native writers).
         * ad_reso_size() returns 0 for any failure, so errno must confirm
         * absence before minting NO_AD; an ambiguous probe leaves UNKNOWN.
         * Directories have no resource fork. */
        if (flags & ADFLAGS_DIR) {
            dir->dcache_rlen = AD_RLEN_NO_AD;
        } else {
            errno = 0;
            off_t rlen = ad_reso_size(name, 0, NULL);

            if (rlen > 0) {
                dir->dcache_rlen = AD_RLEN_RFORK_ONLY;
            } else if (rlen == 0 && (errno == 0 || errno == ENOENT
                                     || errno == ENOATTR)) {
                dir->dcache_rlen = AD_RLEN_NO_AD;
            }
        }

        errno = saved_errno;
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
 * @brief Free the cached resource fork data of a dircache entry
 *
 * The buffer leaves the resource fork LRU list, and its size, dcache_rlen,
 * comes off the budget counter. A negative dcache_rlen with a buffer breaks
 * the invariant, so the call logs an error and frees the buffer without
 * touching the counter. A buffer without an LRU node still frees correctly.
 * The AppleDouble metadata of the entry stays valid.
 *
 * @param[in,out] entry  dircache entry with a cached resource fork
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
 * @brief Free cached resource forks from the oldest end of the LRU list until
 * needed bytes fit
 *
 * The head of the list holds the oldest buffer and the tail the newest. The
 * rfork_cache_store_from_fd call runs it before it caches a new fork, and the
 * cost grows with the number of evicted buffers only.
 *
 * @param[in] needed  bytes about to join the cache
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
 * @brief Cache the entire resource fork of a file by reading it from the open
 * fork
 *
 * The ad_read call covers every storage format, so the call reads dcache_rlen
 * bytes through it, after a stat of the descriptor that stamps the copy
 * (rfork_stamp()). It refuses a closed resource fork, an empty or unknown
 * length, a fork above rfork_max_entry_size, a fork another session has open
 * for writing, and a symlink's, whose opens no lock shows. It also refuses
 * while the cache holds over 2 times its budget. Older buffers leave first
 * when the budget lacks room. A short read means the fork changed since its
 * metadata entered the cache, so the call invalidates the cached metadata as
 * well.
 *
 * @param[in,out] entry  dircache entry of the file
 * @param[in,out] adp    AppleDouble with the resource fork open
 * @param[in]     eid    ADEID_RFORK
 * @returns 0, or -1 when the fork is not cacheable or the read fell short
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

    if (!ad_rsrc_open(adp)) {
        return -1;
    }

    if (entry->dcache_rlen <= 0) {
        return -1;
    }

    size_t rlen = (size_t)entry->dcache_rlen;

    if (rlen > rfork_max_entry_size) {
        return -1;
    }

    /* another session's writes would reach the fork unseen; a symlink's
     * emulated data fork carries no session's open to probe */
    if (ad_data_fileno(adp) < 0
            || ad_testlock_range(adp, ADEID_DFORK, AD_FILELOCK_RSRC_OPEN_WR,
                                 1) != 0) {
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

    struct rfork_buf *buf = malloc(sizeof(*buf) + rlen);

    struct stat st;

    /* a write that lands after the stat, past the ctime's resolution, leaves
     * a stamp the reads refuse */
    if (!buf || fstat(ad_reso_fileno(adp), &st) != 0) {
        free(buf);
        return -1;
    }

    buf->stamp = rfork_stamp(&st);
    /* AFP clients may read resource forks in chunks (e.g., 32KB at a time). We
     * upgrade first read to full-fork ad_read() to ensure the cache contains the
     * whole fork. Subsequent FPReads are served from cache.
     * ad_read() handles all storage formats (EA, macOS xattr, AD v2 sidecar). */
    ssize_t bytes_read = ad_read(adp, eid, 0, buf->data, rlen);

    if (bytes_read != entry->dcache_rlen) {
        /* Short read: fork size changed since Tier 1 was populated.
         * Invalidate ALL cached metadata (not just rlen) — AD fields may also
         * be stale. Self-healing: next ad_metadata_cached() re-reads from disk. */
        free(buf);
        LOG(log_debug, logtype_afpd,
            "rfork_cache_store_from_fd: short read (%zd vs %lld) — "
            "external change detected, invalidating AD cache (did:%u)",
            bytes_read, (long long)entry->dcache_rlen, ntohl(entry->d_did));
        entry->dcache_rlen = AD_RLEN_UNKNOWN;
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

/******************************************************************************
 * Tier 3: EA names and values per entry
 ******************************************************************************/

/* names cached per entry; Finder's quarantine, tags, where-froms and
 * last-used stamps on one file total a few hundred bytes */
#define EA_CACHE_NAMES 1024

/* value records cached per entry; a value that does not fit is read
 * every time */
#define EA_CACHE_VALUES 2048

struct ea_cache_stats ea_cache_stat;

/*!
 * @brief The entry whose EA cache applies to this request, its stat
 *        brought up to the request's fresh stat, or NULL
 *
 * cname() has stat'ed a file; a directory addressed by its DID is stat'ed
 * here, past the bypass tests, so a request the cache does not serve costs
 * no stat. A get names its attribute, a list passes NULL:
 * the names the list filter hides are afpd's own, written behind the
 * entry by fork writes and FinderInfo stores, so a request under one
 * takes no entry. Shared volumes never cache, so their requests skip the
 * lookup. A symlink's own EAs and its target's differ, so a symlink is
 * never cached; with follow symlinks = yes the stat cannot tell one, so
 * kXAttrNoFollow requests bypass the cache there. An entry whose inode is
 * not the object's is left to validation. A ctime change the stat shows
 * below the second, where the filesystem keeps nanoseconds, drops the
 * cache that dir_stat_refresh() cannot see. When of_statdir()'s EACCES
 * fallback has moved the cwd to the parent, from this stat or from the list
 * handler's own, u_name takes the directory's own name, whatever that stat
 * found, so the backend and the cache address it from there on every
 * volume; the volume root has no parent directory and keeps ".".
 *
 * @param[in]     vol    volume of the request
 * @param[in,out] path   the request's path as cname() left it; a
 *                       directory's stat is taken here, a file's d_cached
 *                       memo updated
 * @param[in]     oflag  O_NOFOLLOW for a kXAttrNoFollow request, else 0
 * @param[in]     name   attribute name of a get, NULL for a names list
 * @returns the entry, or NULL for a request the cache does not serve
 */
struct dir *ea_cache_entry(struct vol *vol, struct path *path, int oflag,
                           const char *name)
{
    struct dir *entry;
    uint32_t nsec;
    bool serve = !((vol->v_flags & AFPVOL_MULTIPROTO)
                   || ((oflag & O_NOFOLLOW)
                       && (vol->v_flags & AFPVOL_FOLLOWSYM))
                   || (name && (!(NOT_NETATALK_EA(name))
                                || sys_ea_protected(name))));

    if (serve && !path->st_valid) {
        of_statdir(vol, path);
    }

    if (path_isadir(path) && *path->m_name == '\0' && curdir != path->d_dir
            && path->d_dir->d_did != DIRDID_ROOT) {
        path->u_name = cfrombstr(path->d_dir->d_u_name);
    }

    if (!serve || path->st_errno != 0) {
        return NULL;
    }

    entry = path_entry(vol, curdir, path);

    if (entry == NULL || S_ISLNK(path->st.st_mode)
            || entry->dcache_ino != path->st.st_ino) {
        return NULL;
    }

    dir_stat_refresh(vol, entry, &path->st);
    nsec = (uint32_t)atalk_stat_ctime_timespec(&path->st).tv_nsec;

    if (entry->dcache_ea_ctime_nsec != nsec) {
        ea_cache_free(entry);
        entry->dcache_ea_ctime_nsec = nsec;
    }

    return entry;
}

/*!
 * @brief The cached names, or NULL when the entry has none cached
 *
 * @param[in]  entry  dircache entry of the object
 * @param[out] len    bytes in the names on a hit
 * @returns the names as the backend listed them, or NULL
 */
const char *ea_cache_names(struct dir *entry, size_t *len)
{
    if (entry->dcache_eanames == NULL) {
        ea_cache_stat.misses++;
        return NULL;
    }

    ea_cache_stat.hits++;
    *len = entry->dcache_ealen;
    return entry->dcache_eanames;
}

/*!
 * @brief Cache the object's EA names, after ea_cache_names() found none
 *
 * One byte over len, so an object without EAs holds a pointer too: the
 * pointer is what says the names are valid.
 *
 * @param[in,out] entry  dircache entry of the object
 * @param[in]     names  the names as the backend listed them
 * @param[in]     len    bytes in names; over EA_CACHE_NAMES nothing is kept
 */
void ea_cache_store_names(struct dir *entry, const char *names, size_t len)
{
    char *copy;

    if (len > EA_CACHE_NAMES) {
        ea_cache_stat.oversize++;
        return;
    }

    copy = malloc(len + 1);

    if (copy == NULL) {
        return;
    }

    memcpy(copy, names, len);
    entry->dcache_eanames = copy;
    entry->dcache_ealen = (uint16_t)len;
    ea_cache_stat.stored++;
}

/*!
 * @brief Drop the entry's EA cache
 *
 * @param[in,out] entry  dircache entry whose EA names and values are released
 */
void ea_cache_free(struct dir *entry)
{
    if (entry->dcache_eanames || entry->dcache_eavals) {
        ea_cache_stat.dropped++;
    }

    free(entry->dcache_eanames);
    free(entry->dcache_eavals);
    entry->dcache_eanames = NULL;
    entry->dcache_eavals = NULL;
    entry->dcache_ealen = 0;
    entry->dcache_eavlen = 0;
}

/*!
 * @brief The cached value of name, or NULL when it is not cached
 *
 * @param[in]  entry  dircache entry of the object
 * @param[in]  name   attribute name in the unix charset
 * @param[out] len    bytes in the value on a hit
 * @returns the value as the backend delivered it, or NULL
 */
const char *ea_cache_value(struct dir *entry, const char *name, size_t *len)
{
    const char *rec;
    const char *end;
    size_t namelen = strlen(name);

    if (entry->dcache_eavlen == 0) {
        ea_cache_stat.misses++;
        return NULL;
    }

    rec = entry->dcache_eavals;
    end = rec + entry->dcache_eavlen;

    while (rec < end) {
        uint8_t rnamelen = (uint8_t)rec[0];
        uint16_t vallen;
        memcpy(&vallen, rec + 1, sizeof(vallen));

        if (rnamelen == namelen && memcmp(rec + 3, name, namelen) == 0) {
            ea_cache_stat.hits++;
            *len = vallen;
            return rec + 3 + namelen;
        }

        rec += 3 + rnamelen + vallen;
    }

    ea_cache_stat.misses++;
    return NULL;
}

/*!
 * @brief Cache one value the object was read for
 *
 * @param[in,out] entry  dircache entry of the object
 * @param[in]     name   attribute name, at most UINT8_MAX bytes
 * @param[in]     value  the value as the backend delivered it
 * @param[in]     len    bytes in value; a record that does not fit within
 *                       EA_CACHE_VALUES is not kept
 */
void ea_cache_store_value(struct dir *entry, const char *name,
                          const char *value, size_t len)
{
    size_t namelen = strlen(name);
    size_t reclen = 3 + namelen + len;
    uint16_t vallen;
    char *vals;
    char *rec;

    if (namelen > UINT8_MAX || len > EA_CACHE_VALUES
            || entry->dcache_eavlen + reclen > EA_CACHE_VALUES) {
        ea_cache_stat.oversize++;
        return;
    }

    vals = realloc(entry->dcache_eavals, entry->dcache_eavlen + reclen);

    if (vals == NULL) {
        return;
    }

    rec = vals + entry->dcache_eavlen;
    vallen = (uint16_t)len;
    rec[0] = (char)namelen;
    memcpy(rec + 1, &vallen, sizeof(vallen));
    memcpy(rec + 3, name, namelen);
    memcpy(rec + 3 + namelen, value, len);
    entry->dcache_eavals = vals;
    entry->dcache_eavlen += reclen;
    ea_cache_stat.stored++;
}

/*!
 * @brief Does the entry's cached list prove an attribute missing?
 *
 * The list holds the names as the client saw them, which an ASCII name
 * matches after its conversion, case apart, since some attribute stores
 * fold it; any other name, and an uncached list, leave the answer to the
 * backend.
 *
 * @param[in] entry  dircache entry of the object
 * @param[in] name   attribute name in the unix charset
 * @returns true when the list is cached and lacks the name
 */
bool ea_cache_absent(const struct dir *entry, const char *name)
{
    const char *p = entry->dcache_eanames;
    const char *end;
    size_t len = strlen(name);

    if (p == NULL) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        if ((unsigned char)name[i] >= 0x80) {
            return false;
        }
    }

    end = p + entry->dcache_ealen;

    while (p < end) {
        size_t n = strnlen(p, (size_t)(end - p));

        if (n == len && strncasecmp(p, name, len) == 0) {
            return false;
        }

        p += n + 1;
    }

    ea_cache_stat.absent++;
    return true;
}
