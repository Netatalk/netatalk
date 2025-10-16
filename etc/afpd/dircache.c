/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <bstrlib.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/queue.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "dircache.h"
#include "directory.h"
#include "hash.h"


/*
 * Directory Cache
 * ===============
 *
 * Cache files and directories in a LRU cache.
 *
 * The directory cache caches directories and files(!). The main reason for having the cache
 * is avoiding recursive walks up the path, querying the CNID database each time, when
 * we have to calculate the location of e.g. directory with CNID 30, which is located in a dir with
 * CNID 25, next CNID 20 and then CNID 2 (the volume root as per AFP spec).
 * If all these dirs where in the cache, each database look up can be avoided. Additionally there's
 * the element "fullpath" in struct dir, which is used to avoid the recursion in any case. Wheneveer
 * a struct dir is initialized, the fullpath to the directory is stored there.
 *
 * In order to speed up the CNID query for files too, which e.g. happens when a directory is enumerated,
 * files are stored too in the dircache. In order to differentiate between files and dirs, we set
 * the flag DIRF_ISFILE in struct dir.d_flags for files.
 *
 * The most frequent codepatch that leads to caching is directory enumeration (cf enumerate.c):
 * - if a element is a directory:
 *   (1) the cache is searched by dircache_search_by_name()
 *   (2) if it wasn't found a new struct dir is created and cached both from within dir_add()
 * - for files the caching happens a little bit down the call chain:
 *   (3) first getfilparams() is called, which calls
 *   (4) getmetadata() where the cache is searched with dircache_search_by_name()
 *   (5) if the element is not found
 *   (6) get_id() queries the CNID from the database
 *   (7) then a struct dir is initialized via dir_new() (note the fullpath arg is NULL)
 *   (8) finally added to the cache with dircache_add()
 * (2) of course does contain the steps 6,7 and 8.
 *
 * The dircache is a LRU cache, whenever it fills up we call dircache_evict internally which removes
 * DIRCACHE_FREE_QUANTUM elements from the cache.
 *
 * There is only one cache for all volumes, so of course we use the volume id in hashing calculations.
 *
 * In order to avoid cache poisoning, we store the cached entries st_ctime from stat in
 * struct dir.ctime_dircache. Later when we search the cache we compare the stored
 * value with the result of a fresh stat. If the times differ, we remove the cached
 * entry and return "no entry found in cache".
 * A elements ctime changes when
 *   1) the element is renamed
 *      (we loose the cached entry here, but it will expire when the cache fills)
 *   2) its a directory and an object has been created therein
 *   3) the element is deleted and recreated under the same name
 * Using ctime leads to cache eviction in case 2) where it wouldn't be necessary, because
 * the dir itself (name, CNID, ...) hasn't changed, but there's no other way.
 *
 * Indexes
 * =======
 *
 * The maximum dircache size is:
 * max(DEFAULT_MAX_DIRCACHE_SIZE, min(size, MAX_POSSIBLE_DIRCACHE_SIZE)).
 * It is a hashtable which we use to store "struct dir"s in. If the cache get full, oldest
 * entries are evicted in chunks of DIRCACHE_FREE.
 *
 * We have/need two indexes:
 * - a DID/name index on the main dircache, another hashtable
 * - a queue index on the dircache, for evicting the oldest entries
 *
 * Debugging
 * =========
 *
 * Sending SIGINT to a afpd child causes it to dump the dircache to a file "/tmp/dircache.PID".
 */

/********************************************************
 * Local funcs and variables
 ********************************************************/

/*****************************
 *       the dircache        */

static hash_t       *dircache;        /* The actual cache */
static unsigned int dircache_maxsize; /* cache maximum size */

/*
 * Configuration variables for cache validation behavior
 * These control how frequently we validate cached entries against filesystem
 */
static unsigned int dircache_validation_freq = DEFAULT_DIRCACHE_VALIDATION_FREQ;
static unsigned int dircache_metadata_window = DEFAULT_DIRCACHE_METADATA_WINDOW;
static unsigned int dircache_metadata_threshold =
    DEFAULT_DIRCACHE_METADATA_THRESHOLD;

/* Counter for probabilistic validation - thread-safe with compiler builtins */
static volatile uint64_t validation_counter = 0;

static struct dircache_stat {
    unsigned long long lookups;
    unsigned long long hits;
    unsigned long long misses;
    unsigned long long added;
    unsigned long long removed;
    unsigned long long expunged;
    unsigned long long evicted;
    unsigned long long invalid_on_use;  /* entries that failed when used */
} dircache_stat;

/* FNV 1a */
static hash_val_t hash_vid_did(const void *key)
{
    const struct dir *k = (const struct dir *)key;
    hash_val_t hash = 2166136261;
    hash ^= k->d_vid >> 8;
    hash *= 16777619;
    hash ^= k->d_vid;
    hash *= 16777619;
    hash ^= k->d_did >> 24;
    hash *= 16777619;
    hash ^= (k->d_did >> 16) & 0xff;
    hash *= 16777619;
    hash ^= (k->d_did >> 8) & 0xff;
    hash *= 16777619;
    hash ^= (k->d_did >> 0) & 0xff;
    hash *= 16777619;
    return hash;
}

static int hash_comp_vid_did(const void *key1, const void *key2)
{
    const struct dir *k1 = key1;
    const struct dir *k2 = key2;
    return !(k1->d_did == k2->d_did && k1->d_vid == k2->d_vid);
}

/**************************************************
 * DID/name index on dircache (another hashtable) */

static hash_t *index_didname;

#undef get16bits
#if defined(__i386__)
#define get16bits(d) (*((const uint16_t *) (d)))
#else
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)    \
                      +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

static hash_val_t hash_didname(const void *p)
{
    const struct dir *key = (const struct dir *)p;
    const unsigned char *data = key->d_u_name->data;
    int len = key->d_u_name->slen;
    hash_val_t hash = key->d_pdid + key->d_vid;
    hash_val_t tmp;
    int rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (; len > 0; len--) {
        hash  += get16bits(data);
        tmp    = (get16bits(data + 2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2 * sizeof(uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
    case 3:
        hash += get16bits(data);
        hash ^= hash << 16;
        hash ^= data[sizeof(uint16_t)] << 18;
        hash += hash >> 11;
        break;

    case 2:
        hash += get16bits(data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;

    case 1:
        hash += *data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;
    return hash;
}

static int hash_comp_didname(const void *k1, const void *k2)
{
    const struct dir *key1 = (const struct dir *)k1;
    const struct dir *key2 = (const struct dir *)k2;
    return !(key1->d_vid == key2->d_vid
             && key1->d_pdid == key2->d_pdid
             && (bstrcmp(key1->d_u_name, key2->d_u_name) == 0));
}

/***************************
 * queue index on dircache */

static q_t *index_queue;    /* the index itself */
static unsigned long queue_count;

/*!
 * @brief Determine if cache entry should be validated against filesystem
 *
 * Uses probabilistic validation to reduce filesystem calls while still
 * detecting external changes. Internal netatalk operations use explicit
 * cache invalidation via dir_remove() calls, so frequent validation is
 * only needed to detect external filesystem changes.
 *
 * @returns 1 if validation should be performed, 0 otherwise
 */
static int should_validate_cache_entry(void)
{
    /* Thread-safe increment using compiler builtins */
    uint64_t count = __sync_fetch_and_add(&validation_counter, 1);

    /* Validate every Nth access to detect external changes */
    if (dircache_validation_freq == 0) {
        return 1;  /* Always validate if freq is 0 (invalid config) */
    }

    /* Use the fetched value + 1 (post-increment semantics) */
    return ((count + 1) % dircache_validation_freq == 0);
}

/*!
 * @brief Smart validation for directory cache entries
 *
 * Distinguishes between metadata-only changes (permissions, xattrs) and
 * content changes (files added/removed) to avoid unnecessary cache invalidation.
 * This is critical for AFP performance as directory ctime changes frequently
 * for reasons that don't affect cached directory information.
 *
 * @param cdir   (r) cached directory entry
 * @param st     (r) fresh stat information from filesystem
 *
 * @returns 1 if entry should be invalidated, 0 if still valid
 */
static int cache_entry_externally_modified(struct dir *cdir,
        const struct stat *st)
{
    AFP_ASSERT(cdir);
    AFP_ASSERT(st);

    /* Inode number changed means file was deleted and recreated */
    if (cdir->dcache_ino != st->st_ino) {
        LOG(log_debug, logtype_afpd,
            "dircache: inode changed for \"%s\" (%llu -> %llu)",
            cfrombstr(cdir->d_u_name),
            (unsigned long long)cdir->dcache_ino,
            (unsigned long long)st->st_ino);
        return 1;
    }

    /* No ctime change means no external modification */
    if (cdir->dcache_ctime == st->st_ctime) {
        return 0;
    }

    /* Files: any ctime change is significant */
    if (cdir->d_flags & DIRF_ISFILE) {
        LOG(log_debug, logtype_afpd,
            "dircache: file ctime changed for \"%s\"",
            cfrombstr(cdir->d_u_name));
        return 1;
    }

    /*
     * Directories: ctime changes for multiple reasons:
     * 1. Content changes (files added/removed) - should invalidate
     * 2. Metadata changes (permissions, xattrs) - should NOT invalidate
     * 3. Subdirectory changes - should NOT invalidate parent
     *
     * Heuristic: Recent, small ctime changes are likely metadata-only.
     * Older or larger changes suggest content modification.
     */
    time_t now = time(NULL);
    time_t ctime_age = now - st->st_ctime;
    time_t ctime_delta = st->st_ctime - cdir->dcache_ctime;

    if (ctime_age <= dircache_metadata_window &&
            ctime_delta <= dircache_metadata_threshold) {
        /*
         * Recent, small change - likely metadata update.
         * Update cached ctime to prevent repeated checks and continue.
         */
        LOG(log_debug, logtype_afpd,
            "dircache: metadata-only change detected for \"%s\", updating cached ctime",
            cfrombstr(cdir->d_u_name));
        cdir->dcache_ctime = st->st_ctime;
        return 0;
    }

    /* Significant change - assume content modification */
    LOG(log_debug, logtype_afpd,
        "dircache: significant change detected for \"%s\" (age=%lds, delta=%lds)",
        cfrombstr(cdir->d_u_name), (long)ctime_age, (long)ctime_delta);
    return 1;
}

/*!
 * @brief Remove a fixed number of (oldest) entries from the cache and indexes
 *
 * The default is to remove the 256 oldest entries from the cache.
 * 1. Get the oldest entry
 * 2. If it's in use i.e. open forks reference it or it's curdir requeue it,
 *    don't remove it
 * 3. Remove the dir from the main cache and the didname index
 * 4. Free the struct dir structure and all its members
 */
static void dircache_evict(void)
{
    int i = DIRCACHE_FREE_QUANTUM;
    struct dir *dir;
    LOG(log_debug, logtype_afpd, "dircache: {starting cache eviction}");

    while (i--) {
        if ((dir = (struct dir *)dequeue(index_queue)) == NULL) { /* 1 */
            dircache_dump();
            AFP_PANIC("dircache_evict");
        }

        queue_count--;

        if (curdir == dir) {                          /* 2 */
            if ((dir->qidx_node = enqueue(index_queue, dir)) == NULL) {
                dircache_dump();
                AFP_PANIC("dircache_evict");
            }

            queue_count++;
            continue;
        }

        dircache_remove(NULL, dir, DIRCACHE | DIDNAME_INDEX); /* 3 */
        dir_free(dir);                                        /* 4 */
    }

    AFP_ASSERT(queue_count == dircache->hash_nodecount);
    dircache_stat.evicted += DIRCACHE_FREE_QUANTUM;
    LOG(log_debug, logtype_afpd, "dircache: {finished cache eviction}");
}


/********************************************************
 * Interface
 ********************************************************/

/*!
 * @brief Search the dircache via a CNID for a directory
 *
 * Found cache entries are expunged if both the parent directory st_ctime and the objects
 * st_ctime are modified.
 * This func builds on the fact, that all our code only ever needs to and does search
 * the dircache by CNID expecting directories to be returned, but not files.
 * Thus
 * (1) if we find a file for a given CNID we
 *     (1a) remove it from the cache
 *     (1b) return NULL indicating nothing found
 * (2) we can then use d_fullpath to stat the directory
 *
 * @param vol      (r) pointer to struct vol
 * @param cnid     (r) CNID of the directory to search
 *
 * @returns            Pointer to struct dir if found, else NULL
 */
struct dir *dircache_search_by_did(const struct vol *vol, cnid_t cnid)
{
    struct dir *cdir = NULL;
    struct dir key;
    struct stat st;
    hnode_t *hn;
    AFP_ASSERT(vol);
    AFP_ASSERT(ntohl(cnid) >= CNID_START);
    dircache_stat.lookups++;
    key.d_vid = vol->v_vid;
    key.d_did = cnid;

    if ((hn = hash_lookup(dircache, &key))) {
        cdir = hnode_get(hn);
    }

    if (cdir) {
        /* Files found when expecting directories are always invalid */
        if (cdir->d_flags & DIRF_ISFILE) {
            LOG(log_debug, logtype_afpd,
                "dircache(cnid:%u): {not a directory:\"%s\"}",
                ntohl(cnid), cfrombstr(cdir->d_u_name));
            (void)dir_remove(vol, cdir);
            dircache_stat.expunged++;
            return NULL;
        }

        /*
         * OPTIMIZATION: Skip validation most of the time.
         * Internal netatalk operations invalidate cache explicitly.
         * Periodic validation catches external filesystem changes.
         */
        if (should_validate_cache_entry()) {
            /* Mark entry as validated to track unvalidated entries */
            cdir->d_flags &= ~DIRF_UNVALIDATED;

            /* Check if file still exists */
            if (ostat(cfrombstr(cdir->d_fullpath), &st, vol_syml_opt(vol)) != 0) {
                LOG(log_debug, logtype_afpd,
                    "dircache(cnid:%u): {missing:\"%s\"}",
                    ntohl(cnid), cfrombstr(cdir->d_fullpath));
                (void)dir_remove(vol, cdir);
                dircache_stat.expunged++;
                return NULL;
            }

            /* Smart validation: distinguish meaningful changes */
            if (cache_entry_externally_modified(cdir, &st)) {
                LOG(log_debug, logtype_afpd,
                    "dircache(cnid:%u): {externally modified:\"%s\"}",
                    ntohl(cnid), cfrombstr(cdir->d_u_name));
                (void)dir_remove(vol, cdir);
                dircache_stat.expunged++;
                return NULL;
            }

            /* Entry validated and still valid */
            LOG(log_debug, logtype_afpd,
                "dircache(cnid:%u): {validated: path:\"%s\"}",
                ntohl(cnid), cfrombstr(cdir->d_fullpath));
        } else {
            /* Skipped validation for performance - mark as unvalidated */
            cdir->d_flags |= DIRF_UNVALIDATED;
            LOG(log_debug, logtype_afpd,
                "dircache(cnid:%u): {cached (unvalidated): path:\"%s\"}",
                ntohl(cnid), cfrombstr(cdir->d_fullpath));
        }

        dircache_stat.hits++;
    } else {
        LOG(log_debug, logtype_afpd,
            "dircache(cnid:%u): {not in cache}", ntohl(cnid));
        dircache_stat.misses++;
    }

    return cdir;
}

/*!
 * @brief Search the cache via did/name hashtable
 *
 * Found cache entries are expunged if both the parent directory st_ctime and the objects
 * st_ctime are modified.
 *
 * @param vol      (r) volume
 * @param dir      (r) directory
 * @param name     (r) name (server side encoding)
 * @param len      (r) strlen of name
 *
 * @returns pointer to struct dir if found in cache, else NULL
 */
struct dir *dircache_search_by_name(const struct vol *vol,
                                    const struct dir *dir,
                                    char *name,
                                    int len)
{
    struct dir *cdir = NULL;
    struct dir key;
    struct stat st;
    hnode_t *hn;
    struct tagbstring uname = {-1, len, (unsigned char *)name};
    AFP_ASSERT(vol);
    AFP_ASSERT(dir);
    AFP_ASSERT(name);
    AFP_ASSERT(len == strlen(name));
    AFP_ASSERT(len < 256);
    dircache_stat.lookups++;
    LOG(log_debug, logtype_afpd,
        "dircache_search_by_name(did:%u, \"%s\")",
        ntohl(dir->d_did), name);

    if (dir->d_did != DIRDID_ROOT_PARENT) {
        key.d_vid = vol->v_vid;
        key.d_pdid = dir->d_did;
        key.d_u_name = &uname;

        if ((hn = hash_lookup(index_didname, &key))) {
            cdir = hnode_get(hn);
        }
    }

    if (cdir) {
        /* Periodic validation to detect external changes */
        if (should_validate_cache_entry()) {
            /* Mark entry as validated */
            cdir->d_flags &= ~DIRF_UNVALIDATED;

            /* Check if file still exists */
            if (ostat(cfrombstr(cdir->d_fullpath), &st, vol_syml_opt(vol)) != 0) {
                LOG(log_debug, logtype_afpd,
                    "dircache(did:%u,\"%s\"): {missing:\"%s\"}",
                    ntohl(dir->d_did), name, cfrombstr(cdir->d_fullpath));
                (void)dir_remove(vol, cdir);
                dircache_stat.expunged++;
                return NULL;
            }

            /* Smart validation for external modifications */
            if (cache_entry_externally_modified(cdir, &st)) {
                LOG(log_debug, logtype_afpd,
                    "dircache(did:%u,\"%s\"): {externally modified}",
                    ntohl(dir->d_did), name);
                (void)dir_remove(vol, cdir);
                dircache_stat.expunged++;
                return NULL;
            }
        } else {
            /* Mark as unvalidated since we skipped validation */
            cdir->d_flags |= DIRF_UNVALIDATED;
        }

        LOG(log_debug, logtype_afpd,
            "dircache(did:%u,\"%s\"): {found in cache}",
            ntohl(dir->d_did), name);
        dircache_stat.hits++;
    } else {
        LOG(log_debug, logtype_afpd,
            "dircache(did:%u,\"%s\"): {not in cache}",
            ntohl(dir->d_did), name);
        dircache_stat.misses++;
    }

    return cdir;
}

/*!
 * @brief create struct dir from struct path
 *
 * Add a struct dir to the cache and its indexes.
 *
 * @param vol   (r) pointer to volume
 * @param dir   (r) pointer to parent directory
 *
 * @returns 0 on success, -1 on error which should result in an abort
 */
int dircache_add(const struct vol *vol,
                 struct dir *dir)
{
    struct dir key;
    hnode_t *hn;
    AFP_ASSERT(dir);
    AFP_ASSERT(ntohl(dir->d_pdid) >= 2);

    if (ntohl(dir->d_did) < CNID_START) {
        LOG(log_error, logtype_afpd,
            "dircache_add(): did:%u is less than the allowed %d. Try rebuilding the CNID database for: \"%s\"",
            ntohl(dir->d_did), CNID_START, cfrombstr(dir->d_u_name));
    }

    AFP_ASSERT(ntohl(dir->d_did) >= CNID_START);
    AFP_ASSERT(dir->d_u_name);
    AFP_ASSERT(dir->d_vid);
    AFP_ASSERT(dircache->hash_nodecount <= dircache_maxsize);

    /* Check if cache is full */
    if (dircache->hash_nodecount == dircache_maxsize) {
        dircache_evict();
    }

    /*
     * Make sure we don't add duplicates
     */
    /* Search primary cache by CNID */
    key.d_vid = dir->d_vid;
    key.d_did = dir->d_did;

    if ((hn = hash_lookup(dircache, &key))) {
        /* Found an entry with the same CNID, delete it */
        dir_remove(vol, hnode_get(hn));
        dircache_stat.expunged++;
    }

    key.d_vid = vol->v_vid;
    key.d_pdid = dir->d_pdid;
    key.d_u_name = dir->d_u_name;

    if ((hn = hash_lookup(index_didname, &key))) {
        /* Found an entry with the same DID/name, delete it */
        dir_remove(vol, hnode_get(hn));
        dircache_stat.expunged++;
    }

    /* Add it to the main dircache */
    if (hash_alloc_insert(dircache, dir, dir) == 0) {
        dircache_dump();
        exit(EXITERR_SYS);
    }

    /* Add it to the did/name index */
    if (hash_alloc_insert(index_didname, dir, dir) == 0) {
        dircache_dump();
        exit(EXITERR_SYS);
    }

    /* Add it to the fifo queue index */
    if ((dir->qidx_node = enqueue(index_queue, dir)) == NULL) {
        dircache_dump();
        exit(EXITERR_SYS);
    } else {
        queue_count++;
    }

    dircache_stat.added++;
    LOG(log_debug, logtype_afpd, "dircache(did:%u,'%s'): {added}",
        ntohl(dir->d_did), cfrombstr(dir->d_u_name));
    AFP_ASSERT(queue_count == index_didname->hash_nodecount
               && queue_count == dircache->hash_nodecount);
    return 0;
}

/*!
  * @brief Remove an entry from the dircache
  *
  * Callers outside of dircache.c should call this with
  * flags = QUEUE_INDEX | DIDNAME_INDEX | DIRCACHE.
  */
void dircache_remove(const struct vol *vol _U_, struct dir *dir, int flags)
{
    hnode_t *hn;
    AFP_ASSERT(dir);
    AFP_ASSERT((flags & ~(QUEUE_INDEX | DIDNAME_INDEX | DIRCACHE)) == 0);

    if (flags & QUEUE_INDEX) {
        /* remove it from the queue index */
        dequeue(dir->qidx_node->prev); /* this effectively deletes the dequeued node */
        queue_count--;
    }

    if (flags & DIDNAME_INDEX) {
        if ((hn = hash_lookup(index_didname, dir)) == NULL) {
            LOG(log_error, logtype_afpd, "dircache_remove(%u,\"%s\"): not in didname index",
                ntohl(dir->d_did), cfrombstr(dir->d_u_name));
            dircache_dump();
            AFP_PANIC("dircache_remove");
        }

        hash_delete_free(index_didname, hn);
    }

    if (flags & DIRCACHE) {
        if ((hn = hash_lookup(dircache, dir)) == NULL) {
            LOG(log_error, logtype_afpd, "dircache_remove(%u,\"%s\"): not in dircache",
                ntohl(dir->d_did), cfrombstr(dir->d_u_name));
            dircache_dump();
            AFP_PANIC("dircache_remove");
        }

        hash_delete_free(dircache, hn);
    }

    LOG(log_debug, logtype_afpd, "dircache(did:%u,\"%s\"): {removed}",
        ntohl(dir->d_did), cfrombstr(dir->d_u_name));
    dircache_stat.removed++;
    AFP_ASSERT(queue_count == index_didname->hash_nodecount
               && queue_count == dircache->hash_nodecount);
}

/*!
 * @brief Remove all child entries of a directory from the dircache
 *
 * When a directory is renamed or moved, the full paths stored in the
 * dircache become invalid for all child entries of the renamed dir.
 * This function prunes orphaned child dircache entries of given dir.
 * CNID entries use parent DIDs and name, and requre recursion to get
 * the full path, therefore parent changes do not invalidate the CNIDs.
 *
 * @param vol  (r) volume
 * @param dir  (r) parent directory whose children should be removed
 */
void dircache_remove_children(const struct vol *vol, struct dir *dir)
{
    struct dir *entry;
    hnode_t *hn;
    hscan_t hs;
    /* Store dir pointers */
    struct dir **to_remove = NULL;
    int remove_count = 0;
    int remove_capacity = 0;

    if (!dir || !dir->d_fullpath) {
        return;
    }

    LOG(log_debug, logtype_afpd,
        "dircache_remove_children: removing children of \"%s\" (did:%u)",
        cfrombstr(dir->d_fullpath), ntohl(dir->d_did));
    /* Two stages to avoid modifying the hash during iteration */
    /* First pass: collect entries to remove */
    hash_scan_begin(&hs, dircache);

    while ((hn = hash_scan_next(&hs))) {
        entry = hnode_get(hn);

        /* Skip the parent directory itself */
        if (entry == dir) {
            continue;
        }

        /* Check if entry is a decendant by comparing parent path */
        if (entry->d_fullpath && dir->d_fullpath) {
            const char *entry_path = cfrombstr(entry->d_fullpath);
            const char *parent_path = cfrombstr(dir->d_fullpath);
            size_t parent_len = blength(dir->d_fullpath);

            /* Check if entry's path starts with parent's path followed by '/' */
            if (parent_path && entry_path && parent_len > 0 &&
                    strncmp(entry_path, parent_path, parent_len) == 0 &&
                    entry_path[parent_len] == '/') {
                /* Expand to_remove array */
                if (remove_count >= remove_capacity) {
                    remove_capacity = remove_capacity ? remove_capacity * 2 : 16;
                    to_remove = realloc(to_remove, remove_capacity * sizeof(struct dir*));

                    if (!to_remove) {
                        LOG(log_error, logtype_afpd,
                            "dircache_remove_children: out of memory");
                        return;
                    }
                }

                /* Store the dir pointer for removal */
                to_remove[remove_count] = entry;
                remove_count++;
                LOG(log_debug, logtype_afpd,
                    "dircache_remove_children: marking child \"%s\" (did:%u) for removal",
                    cfrombstr(entry->d_u_name), ntohl(entry->d_did));
            }
        }
    }

    /* Second pass: remove collected entries from dircache */
    for (int i = 0; i < remove_count; i++) {
        entry = to_remove[i];
        LOG(log_debug, logtype_afpd,
            "dircache_remove_children: removing stale \"%s\" (did:%u) from dircache",
            cfrombstr(entry->d_u_name), ntohl(entry->d_did));
        /* Remove entry from dircache */
        dir_remove(vol, entry);
    }

    if (to_remove) {
        free(to_remove);
    }

    if (remove_count) {
        LOG(log_info, logtype_afpd,
            "dircache_remove_children: removed %d dircache entries of \"%s\"",
            remove_count, cfrombstr(dir->d_fullpath));
    } else {
        LOG(log_debug, logtype_afpd,
            "dircache_remove_children: removed %d dircache entries of \"%s\"",
            remove_count, cfrombstr(dir->d_fullpath));
    }
}

/*!
 * @brief Initialize the dircache and indexes
 *
 * This is called in child afpd initialization. The maximum cache size will be
 * max(DEFAULT_MAX_DIRCACHE_SIZE, min(size, MAX_POSSIBLE_DIRCACHE_SIZE)).
 * It initializes a hashtable which we use to store a directory cache in.
 * It also initializes two indexes:
 * - a DID/name index on the main dircache
 * - a queue index on the dircache
 *
 * @param reqsize   (r) requested maximum size from afp.conf
 *
 * @returns 0 on success, -1 on error
 */
int dircache_init(int reqsize)
{
    dircache_maxsize = DEFAULT_MAX_DIRCACHE_SIZE;

    /* Initialize the main dircache */
    if (reqsize > DEFAULT_MAX_DIRCACHE_SIZE
            && reqsize < MAX_POSSIBLE_DIRCACHE_SIZE) {
        while ((dircache_maxsize < MAX_POSSIBLE_DIRCACHE_SIZE)
                && (dircache_maxsize < reqsize)) {
            dircache_maxsize *= 2;
        }
    }

    if ((dircache = hash_create(dircache_maxsize, hash_comp_vid_did,
                                hash_vid_did)) == NULL) {
        return -1;
    }

    LOG(log_debug, logtype_afpd, "dircache_init: done. max dircache size: %u",
        dircache_maxsize);

    /* Initialize did/name index hashtable */
    if ((index_didname = hash_create(dircache_maxsize, hash_comp_didname,
                                     hash_didname)) == NULL) {
        return -1;
    }

    /* Initialize index queue */
    if ((index_queue = queue_init()) == NULL) {
        return -1;
    } else {
        queue_count = 0;
    }

    /* Initialize index queue */
    if ((invalid_dircache_entries = queue_init()) == NULL) {
        return -1;
    }

    /* As long as directory.c hasn't got its own initializer call, we do it for it */
    rootParent.d_did = DIRDID_ROOT_PARENT;
    rootParent.d_fullpath = bfromcstr("ROOT_PARENT");
    rootParent.d_m_name = bfromcstr("ROOT_PARENT");
    rootParent.d_u_name = rootParent.d_m_name;
    rootParent.d_rights_cache = 0xffffffff;
    return 0;
}

/*!
 * @brief Log dircache statistics
 *
 * Includes hit ratio percentage for monitoring cache effectiveness,
 * validation-specific metrics to monitor performance impact of
 * the optimization changes, and username for tracking per-user stats.
 * Shows both expunged (caught by validation) and invalid_on_use (missed by validation).
 */
void log_dircache_stat(void)
{
    double hit_ratio = 0.0;
    double validation_ratio = 0.0;

    if (dircache_stat.lookups > 0) {
        hit_ratio = ((double)dircache_stat.hits / (double)dircache_stat.lookups) *
                    100.0;

        if (dircache_validation_freq > 0) {
            /* Thread-safe read of validation counter */
            uint64_t counter_value = __sync_fetch_and_add(&validation_counter, 0);
            validation_ratio = ((double)counter_value / (double)dircache_validation_freq) /
                               (double)dircache_stat.lookups * 100.0;
        }
    }

    /* Get username from AFPobj if available. AFPobj->username is an array,
     * not a pointer so only need to check if AFPobj is non-null. */
    extern AFPObj *AFPobj;
    const char *username = (AFPobj
                            && AFPobj->username[0]) ? AFPobj->username : "unknown";
    LOG(log_info, logtype_afpd,
        "dircache statistics: (user: %s) "
        "entries: %lu, lookups: %llu, hits: %llu (%.1f%%), misses: %llu, "
        "validations: ~%llu (%.1f%%), "
        "added: %llu, removed: %llu, expunged: %llu, invalid_on_use: %llu, evicted: %llu, "
        "validation_freq: %u",
        username,
        queue_count,
        dircache_stat.lookups,
        dircache_stat.hits, hit_ratio,
        dircache_stat.misses,
        dircache_validation_freq > 0 ? __sync_fetch_and_add(&validation_counter,
                0) / dircache_validation_freq : 0, validation_ratio,
        dircache_stat.added,
        dircache_stat.removed,
        dircache_stat.expunged,
        dircache_stat.invalid_on_use,
        dircache_stat.evicted,
        dircache_validation_freq);
}

/*!
 * @brief Dump dircache to /tmp/dircache.PID
 */
void dircache_dump(void)
{
    char tmpnam[MAXPATHLEN + 1];
    FILE *dump;
    qnode_t *n = index_queue->next;
    hnode_t *hn;
    hscan_t hs;
    const struct dir *dir;
    int i;
    LOG(log_warning, logtype_afpd, "Dumping directory cache...");
    snprintf(tmpnam, sizeof(tmpnam) - 1, "%s/dircache.%u", tmpdir(), getpid());

    if ((dump = fopen(tmpnam, "w+")) == NULL) {
        LOG(log_error, logtype_afpd, "dircache_dump: %s", strerror(errno));
        return;
    }

    setbuf(dump, NULL);
    fprintf(dump, "Number of cache entries in LRU queue: %lu\n", queue_count);
    fprintf(dump, "Configured maximum cache size: %u\n\n", dircache_maxsize);
    fprintf(dump, "Primary CNID index:\n");
    fprintf(dump, "       VID     DID    CNID STAT PATH\n");
    fprintf(dump,
            "====================================================================\n");
    hash_scan_begin(&hs, dircache);
    i = 1;

    while ((hn = hash_scan_next(&hs))) {
        dir = hnode_get(hn);
        fprintf(dump, "%05u: %3u  %6u  %6u %s    %s\n",
                i++,
                ntohs(dir->d_vid),
                ntohl(dir->d_pdid),
                ntohl(dir->d_did),
                dir->d_flags & DIRF_ISFILE ? "f" : "d",
                cfrombstr(dir->d_fullpath));
    }

    fprintf(dump, "\nSecondary DID/name index:\n");
    fprintf(dump, "       VID     DID    CNID STAT PATH\n");
    fprintf(dump,
            "====================================================================\n");
    hash_scan_begin(&hs, index_didname);
    i = 1;

    while ((hn = hash_scan_next(&hs))) {
        dir = hnode_get(hn);
        fprintf(dump, "%05u: %3u  %6u  %6u %s    %s\n",
                i++,
                ntohs(dir->d_vid),
                ntohl(dir->d_pdid),
                ntohl(dir->d_did),
                dir->d_flags & DIRF_ISFILE ? "f" : "d",
                cfrombstr(dir->d_fullpath));
    }

    fprintf(dump, "\nLRU Queue:\n");
    fprintf(dump, "       VID     DID    CNID STAT PATH\n");
    fprintf(dump,
            "====================================================================\n");

    for (i = 1; i <= queue_count; i++) {
        if (n == index_queue) {
            break;
        }

        dir = (struct dir *)n->data;
        fprintf(dump, "%05u: %3u  %6u  %6u %s    %s\n",
                i,
                ntohs(dir->d_vid),
                ntohl(dir->d_pdid),
                ntohl(dir->d_did),
                dir->d_flags & DIRF_ISFILE ? "f" : "d",
                cfrombstr(dir->d_fullpath));
        n = n->next;
    }

    fprintf(dump, "\n");
    fflush(dump);
    fclose(dump);
    return;
}

/*!
 * @brief Set directory cache validation parameters
 *
 * Allows runtime configuration of cache validation behavior for performance tuning.
 * Lower validation frequency improves performance but may delay detection of
 * external filesystem changes.
 *
 * @param freq       (r) validation frequency (1 = validate every access, 5 = every 5th access)
 * @param meta_win   (r) metadata change time window in seconds
 * @param meta_thresh (r) metadata change threshold in seconds
 *
 * @returns 0 on success, -1 on invalid parameters
 */
int dircache_set_validation_params(unsigned int freq,
                                   unsigned int meta_win,
                                   unsigned int meta_thresh)
{
    if (freq == 0 || freq > 100) {
        LOG(log_error, logtype_afpd,
            "dircache_set_validation_params: invalid frequency %u (must be 1-100)", freq);
        return -1;
    }

    if (meta_win > 3600 || meta_thresh > 1800) {
        LOG(log_error, logtype_afpd,
            "dircache_set_validation_params: invalid metadata parameters (%u, %u)",
            meta_win, meta_thresh);
        return -1;
    }

    dircache_validation_freq = freq;
    dircache_metadata_window = meta_win;
    dircache_metadata_threshold = meta_thresh;
    /* Thread-safe reset of validation counter using compiler builtins */
    __sync_lock_test_and_set(&validation_counter, 0);
    LOG(log_info, logtype_afpd,
        "dircache: validation parameters updated (freq=%u, window=%us, threshold=%us), counter reset",
        freq, meta_win, meta_thresh);
    return 0;
}

/*!
 * @brief Reset validation counter for consistent testing
 *
 * Resets the global validation counter to ensure predictable
 * validation patterns between test runs or configuration changes.
 */
void dircache_reset_validation_counter(void)
{
    /* Thread-safe reset using compiler builtins */
    __sync_lock_test_and_set(&validation_counter, 0);
    LOG(log_debug, logtype_afpd, "dircache: validation counter reset");
}

/*!
 * @brief Report that a cache entry was invalid when actually used
 *
 * This function should be called when a cached directory entry that was
 * returned without validation (for performance) turns out to be invalid
 * when actually accessed (e.g., file doesn't exist, has been modified, etc).
 * This helps track the effectiveness of the validation frequency setting.
 *
 * @param dir (r) The directory entry that was found to be invalid
 */
void dircache_report_invalid_entry(struct dir *dir)
{
    if (dir && (dir->d_flags & DIRF_UNVALIDATED)) {
        /* Only count entries that were returned without validation */
        dircache_stat.invalid_on_use++;
        LOG(log_debug, logtype_afpd,
            "dircache: unvalidated entry was invalid on use: \"%s\"",
            cfrombstr(dir->d_u_name));
    }
}
