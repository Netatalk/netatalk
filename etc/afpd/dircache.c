/*
 * Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>
 * Copyright (c) 2025-2026 Andy Lemin (andylemin)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
#include <atalk/netatalk_conf.h>
#include <atalk/queue.h>
#include <atalk/server_ipc.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "ad_cache.h"
#include "dircache.h"
#include "directory.h"
#include "hash.h"


/*!
 * @file
 * @brief Directory Cache
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
 *    1. the cache is searched by dircache_search_by_name()
 *    2. if it wasn't found a new struct dir is created and cached both from within dir_add()
 * - for files the caching happens a little bit down the call chain:
 *    3. first getfilparams() is called, which calls
 *    4. getmetadata() where the cache is searched with dircache_search_by_name()
 *    5. if the element is not found
 *    6. get_id() queries the CNID from the database
 *    7. then a struct dir is initialized via dir_new() (note the fullpath arg is NULL)
 *    8. finally added to the cache with dircache_add()
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
 *   1. the element is renamed
 *      (we lose the cached entry here, but it will expire when the cache fills)
 *   2. its a directory and an object has been created therein
 *   3. the element is deleted and recreated under the same name
 * Using ctime leads to cache eviction in case 2) where it wouldn't be necessary, because
 * the dir itself (name, CNID, ...) hasn't changed, but there's no other way.
 *
 * Indexes
 * =======
 *
 * The maximum dircache size is:
 * max(DEFAULT_DIRCACHE_SIZE, min(size, MAX_DIRCACHE_SIZE)).
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

static hash_t       *dircache;
static unsigned int dircache_maxsize;

/* Peak cached entries reached this session (shared by LRU and ARC) */
static unsigned long queue_count_max = 0;

/*
 * Configuration variables for cache validation behavior
 * Controls how frequently we validate cached entries against filesystem
 */
static unsigned int dircache_validation_freq = DEFAULT_DIRCACHE_VALIDATION_FREQ;

/* Counter for probabilistic validation - thread-safe with compiler builtins */
static volatile uint64_t validation_counter = 0;

static struct dircache_stat {
    unsigned long long lookups;
    /*!< Hits in T1/T2 (cached data, instant return) */
    unsigned long long hits;
    /*!< Hits in B1/B2 (ghost entries, instant return in our impl) */
    unsigned long long ghost_hits;
    /*!< True misses (not in any ARC list) */
    unsigned long long misses;
    unsigned long long added;
    unsigned long long removed;
    unsigned long long expunged;
    unsigned long long evicted;
    /*!< entries that failed when used */
    unsigned long long invalid_on_use;
} dircache_stat;

/* Cache hint processing statistics — logged by log_dircache_stat() */
static struct {
    unsigned long long hints_received;   /* Total hints read from pipe */
    unsigned long long
    hints_acted_on;   /* Hints that matched a local cache entry */
    unsigned long long
    hints_no_match;   /* Hints for entries not in local cache (zero cost) */
} cache_hint_stat;

/********************************************************
 * ARC (Adaptive Replacement Cache) Implementation
 * Based on: Megiddo & Modha, IBM 2004
 * "Outperforming LRU with an Adaptive Replacement Cache Algorithm"
 * https://theory.stanford.edu/~megiddo/pdf/IEEE_COMPUTER_0404.pdf
 ********************************************************/

/* ARC list location for cache entries */
typedef enum {
    ARC_NONE = 0,   /* Not in any ARC list (matches calloc() zero init) */
    ARC_T1,         /* Recent entries */
    ARC_T2,         /* Frequent entries */
    ARC_B1,         /* Ghost entries evicted from T1 */
    ARC_B2          /* Ghost entries evicted from T2 */
} arc_list_t;

/*
 * NOTE: ARC Ghost entries in this implementation use full struct dir with
 * DIRF_ARC_GHOST flag set. This keeps them in the hash tables for O(1)
 * lookup while marking them as not-fully-cached. The ghost retains ALL
 * data (d_did, d_vid, d_pdid, d_u_name, d_fullpath) which allows instant
 * return and promotion back to T2 while still allowing the ARC to learn.
 *
 * This differs from the IBM paper's lightweight ghost (metadata only)
 * but is appropriate for Netatalk where entries are relatively small
 * and the hash tables already exist.
 */

/* ARC cache state */
static struct {
    /* Configuration */
    int enabled;           /* 0 = LRU mode, 1 = ARC mode */

    /* The four ARC lists (using existing queue.h) */
    q_t *t1;               /* Recent entries (cached) */
    q_t *t2;               /* Frequent entries (cached) */
    q_t *b1;               /* Ghost entries from T1 (full struct dir with DIRF_ARC_GHOST) */
    q_t *b2;               /* Ghost entries from T2 (full struct dir with DIRF_ARC_GHOST) */

    /* List sizes (maintained explicitly for O(1) access) */
    size_t t1_size;
    size_t t2_size;
    size_t b1_size;
    size_t b2_size;

    /* Adaptation parameter (0 ≤ p ≤ c) */
    size_t p;              /* Target size for T1 */
    size_t c;              /* Total cache size */

    /* Statistics */
    struct {
        uint64_t hits_t1;         /* Hits in T1 */
        uint64_t hits_t2;         /* Hits in T2 */
        uint64_t ghost_hits_b1;   /* Ghost hits in B1 */
        uint64_t ghost_hits_b2;   /* Ghost hits in B2 */
        uint64_t adaptations;     /* Number of p adjustments */
        uint64_t evictions_t1;    /* Evictions from T1 */
        uint64_t evictions_t2;    /* Evictions from T2 */
        uint64_t promotions_t1_to_t2;  /* Promotions from T1 to T2 */
        size_t p_min, p_max;      /* Min/max p values seen */
        uint64_t p_increases;     /* Times p increased */
        uint64_t p_decreases;     /* Times p decreased */
    } stats;
} arc_cache = {
    .enabled = 0,  /* Default to LRU mode */
    .t1 = NULL, .t2 = NULL, .b1 = NULL, .b2 = NULL,
    .t1_size = 0, .t2_size = 0, .b1_size = 0, .b2_size = 0,
    .p = 0, .c = 0,
    .stats = {0}
};

/*
 * NOTE: Ghost entries in B1/B2 are stored as full struct dir with DIRF_ARC_GHOST
 * flag set. This allows O(1) hash table lookup and instant promotion back to T2.
 * No separate ghost_entry_t type or pool is needed.
 */

/* FNV 1a - inline for performance */
static inline hash_val_t hash_vid_did(const void *key)
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

static inline int hash_comp_vid_did(const void *key1, const void *key2)
{
    const struct dir *k1 = key1;
    const struct dir *k2 = key2;
    return !(k1->d_did == k2->d_did && k1->d_vid == k2->d_vid);
}

/********************************************************
 * ARC Core Algorithm Functions
 ********************************************************/

/*!
 * @brief Initialize ARC cache structures
 *
 * @param[in] cache_size  Total cache size (c)
 * @returns 0 on success, -1 on error
 */
static int arc_init(size_t cache_size)
{
    /* Initialize the four queues */
    arc_cache.t1 = queue_init();
    arc_cache.t2 = queue_init();
    arc_cache.b1 = queue_init();
    arc_cache.b2 = queue_init();

    if (!arc_cache.t1 || !arc_cache.t2 || !arc_cache.b1 || !arc_cache.b2) {
        LOG(log_error, logtype_afpd, "arc_init: failed to initialize queues");
        return -1;
    }

    /* Initialize sizes */
    arc_cache.c = cache_size;
    arc_cache.p = 0;  /* Start with no preference (will adapt) */
    arc_cache.t1_size = 0;
    arc_cache.t2_size = 0;
    arc_cache.b1_size = 0;
    arc_cache.b2_size = 0;
    /* No separate ghost pool needed - ghosts are full struct dir with DIRF_ARC_GHOST flag */
    /* Initialize statistics */
    memset(&arc_cache.stats, 0, sizeof(arc_cache.stats));
    arc_cache.stats.p_min = cache_size;  /* Will track minimum */
    arc_cache.stats.p_max = 0;            /* Will track maximum */
    arc_cache.enabled = 1;
    return 0;
}

/*!
 * @brief Destroy ARC cache structures
 */
static void arc_destroy(void)
{
    if (!arc_cache.enabled) {
        return;
    }

    /* Destroy queues
     * T1/T2: don't free data, hash table cleanup handles it
     * B1/B2: ghosts are full struct dir, also freed via hash table cleanup
     */
    if (arc_cache.t1) {
        queue_destroy(arc_cache.t1, NULL);
    }

    if (arc_cache.t2) {
        queue_destroy(arc_cache.t2, NULL);
    }

    if (arc_cache.b1) {
        queue_destroy(arc_cache.b1, NULL);
    }

    if (arc_cache.b2) {
        queue_destroy(arc_cache.b2, NULL);
    }

    /* Reset state */
    arc_cache.enabled = 0;
    arc_cache.t1 = arc_cache.t2 = arc_cache.b1 = arc_cache.b2 = NULL;
    arc_cache.t1_size = arc_cache.t2_size = 0;
    arc_cache.b1_size = arc_cache.b2_size = 0;
    arc_cache.p = arc_cache.c = 0;
    LOG(log_debug, logtype_afpd, "ARC cache destroyed");
}

/*!
 * @brief Verify ARC invariants (debug mode)
 *
 * Checks that ARC list sizes satisfy all constraints from the paper.
 * Called after each operation in debug builds.
 */
static void arc_verify_invariants(void)
{
#ifdef DEBUG
    AFP_ASSERT(arc_cache.enabled);
    /* Invariant 1: Total cached entries = c */
    AFP_ASSERT(arc_cache.t1_size + arc_cache.t2_size <= arc_cache.c);
    /* Invariant 2: Total ghost entries ≤ c */
    AFP_ASSERT(arc_cache.b1_size + arc_cache.b2_size <= arc_cache.c);
    /* Invariant 3: p within bounds */
    AFP_ASSERT(arc_cache.p <= arc_cache.c);
    /* Invariant 4: Total directory size ≤ 2c */
    size_t l1 = arc_cache.t1_size + arc_cache.b1_size;
    size_t l2 = arc_cache.t2_size + arc_cache.b2_size;
    AFP_ASSERT(l1 + l2 <= 2 * arc_cache.c);

    /* Once directory is full, maintain exact sizes */
    if (l1 + l2 == 2 * arc_cache.c) {
        AFP_ASSERT(arc_cache.t1_size + arc_cache.t2_size == arc_cache.c);
        AFP_ASSERT(arc_cache.b1_size + arc_cache.b2_size == arc_cache.c);
    }

#endif
}

/*!
 * @brief Ensure ghost directory has room for one more entry
 *
 * Maintains the ARC invariant: B1 + B2 ≤ c
 *
 * Called before operations that will add a ghost entry to B1 or B2.
 * If ghost directory is at or over capacity, deletes the LRU ghost
 * from the specified target list to make room.
 *
 * This handles the edge case where entries are removed from the cache
 * via dircache_remove() (bypassing ghost lists), causing the ghost
 * directory to stay at capacity while the cache shrinks.
 *
 * @param[in] target_list  Which ghost list will receive new entry (ARC_B1 or ARC_B2)
 */
static void arc_ensure_ghost_capacity(arc_list_t target_list)
{
    AFP_ASSERT(target_list == ARC_B1 || target_list == ARC_B2);

    /* Quick exit: already have room */
    if ((arc_cache.b1_size + arc_cache.b2_size) < arc_cache.c) {
        return;
    }

    /* Ghost directory at or over capacity - must delete LRU from target list
     * to maintain invariant B1+B2 <= c */
    q_t *target_queue = (target_list == ARC_B1) ? arc_cache.b1 : arc_cache.b2;
    size_t *target_size = (target_list == ARC_B1) ? &arc_cache.b1_size :
                          &arc_cache.b2_size;

    /* Dequeue LRU ghost from target list */
    if (*target_size > 0) {
        struct dir *ghost = (struct dir *)dequeue(target_queue);

        if (ghost) {
            (*target_size)--;
            ghost->qidx_node = NULL;  /* Clear queue pointer */
            LOG(log_debug, logtype_afpd,
                "arc_ensure_ghost_capacity: deleted %s ghost (did:%u) to maintain invariant (B1=%zu, B2=%zu)",
                target_list == ARC_B1 ? "B1" : "B2",
                ntohl(ghost->d_did),
                arc_cache.b1_size,
                arc_cache.b2_size);
            /* Remove from hash tables and free */
            dircache_remove(NULL, ghost, DIRCACHE | DIDNAME_INDEX);
            dir_free(ghost);
        }
    }
}

/*!
 * @brief Find an evictable victim in a cache queue, skipping curdir
 *
 * Only searches T1 and T2 (the real cache queues).
 * B1/B2 are ghost lists with full data.
 *
 * Tries the LRU entry first; if it's curdir, tries the second-LRU.
 *
 * @param[in] queue      The ARC queue to search (T1 or T2)
 * @param[in] queue_size Number of entries in the queue
 * @return evictable dir entry, or NULL if queue is empty/only contains curdir
 */
static struct dir *arc_find_victim(q_t *queue, size_t queue_size)
{
    struct dir *victim;

    if (queue_size == 0 || queue->next == queue) {
        return NULL;
    }

    victim = (struct dir *)queue->next->data;

    if (!victim || !victim->qidx_node) {
        return NULL;
    }

    if (victim == curdir) {
        if (queue_size < 2 || queue->next->next == queue) {
            return NULL;
        }

        victim = (struct dir *)queue->next->next->data;

        if (!victim || !victim->qidx_node || victim == curdir) {
            return NULL;
        }
    }

    return victim;
}

/*!
 * @brief Evict a victim from a cache queue to the corresponding ghost list
 *
 * Moves victim's queue node from src to dst, sets ghost flag, and updates
 * all ARC size counters and stats.
 *
 * @param[in] victim    Directory entry to evict (must have valid qidx_node)
 * @param[in] src       Source cache queue (T1 or T2)
 * @param[in] dst       Destination ghost queue (B1 or B2)
 * @param[in] dst_list  ARC_B1 or ARC_B2 (determines which counters to update)
 * @param[in] fallback  true if this is a cross-queue fallback eviction (for logging)
 */
static void arc_evict_to_ghost(struct dir *victim, q_t *src, q_t *dst,
                               int dst_list, bool fallback)
{
    const char *src_name = (dst_list == ARC_B1) ? "T1" : "T2";
    const char *dst_name = (dst_list == ARC_B1) ? "B1" : "B2";
    arc_ensure_ghost_capacity(dst_list);

    if (queue_move_to_tail_of(src, dst, victim->qidx_node) == NULL) {
        LOG(log_error, logtype_afpd,
            "arc_replace: %s→%s migration failed", src_name, dst_name);
        AFP_PANIC("arc_replace ghost migration");
    }

    victim->d_flags |= DIRF_ARC_GHOST;
    victim->arc_list = (uint8_t)dst_list;

    if (dst_list == ARC_B1) {
        arc_cache.t1_size--;
        arc_cache.b1_size++;
        arc_cache.stats.evictions_t1++;
    } else {
        arc_cache.t2_size--;
        arc_cache.b2_size++;
        arc_cache.stats.evictions_t2++;
    }

    LOG(log_debug, logtype_afpd,
        "arc_replace: evicted from %s to %s (%sdid:%u)",
        src_name, dst_name, fallback ? "fallback, " : "",
        ntohl(victim->d_did));
}

/*!
 * @brief ARC REPLACE subroutine (from paper Figure 1)
 *
 * Evicts one entry from T1 or T2 and moves it to the corresponding ghost list (B1 or B2).
 * Decision based on:
 * - Current size of T1 relative to target p
 * - Whether the incoming request is in B2
 *
 * If the preferred queue only contains curdir (which must never be evicted),
 * falls back to the other queue. Returns -1 only when both queues are
 * exhausted, allowing the caller to proceed with a temporarily over-capacity
 * cache that self-heals on the next eviction cycle.
 *
 * From paper:
 * "if |T1| ≥ 1 and ((x ∈ B2 and |T1| = p) or |T1| > p):
 *      Move LRU of T1 to MRU of B1, remove from cache
 *  else:
 *      Move LRU of T2 to MRU of B2, remove from cache"
 *
 * @param[in] in_b2  1 if incoming request is in B2, 0 otherwise
 * @return 0 on successful eviction, -1 if no entry could be evicted
 */
static int arc_replace(int in_b2)
{
    struct dir *victim;
    int try_t1_first;
    /* Decide which list to evict from */
    try_t1_first = (arc_cache.t1_size >= 1 &&
                    ((in_b2 && arc_cache.t1_size == arc_cache.p) ||
                     (arc_cache.t1_size > arc_cache.p)));

    if (try_t1_first) {
        victim = arc_find_victim(arc_cache.t1, arc_cache.t1_size);

        if (victim) {
            arc_evict_to_ghost(victim, arc_cache.t1, arc_cache.b1,
                               ARC_B1, false);
        } else {
            /* T1 blocked by curdir or empty — fall back to T2 */
            victim = arc_find_victim(arc_cache.t2, arc_cache.t2_size);

            if (!victim) {
                LOG(log_warning, logtype_afpd,
                    "arc_replace: both queues blocked by curdir "
                    "(T1=%zu, T2=%zu, c=%zu)",
                    arc_cache.t1_size, arc_cache.t2_size, arc_cache.c);
                return -1;
            }

            arc_evict_to_ghost(victim, arc_cache.t2, arc_cache.b2,
                               ARC_B2, true);
        }
    } else {
        victim = arc_find_victim(arc_cache.t2, arc_cache.t2_size);

        if (victim) {
            arc_evict_to_ghost(victim, arc_cache.t2, arc_cache.b2,
                               ARC_B2, false);
        } else {
            /* T2 blocked by curdir or empty — fall back to T1 */
            victim = arc_find_victim(arc_cache.t1, arc_cache.t1_size);

            if (!victim) {
                LOG(log_warning, logtype_afpd,
                    "arc_replace: both queues blocked by curdir "
                    "(T1=%zu, T2=%zu, c=%zu)",
                    arc_cache.t1_size, arc_cache.t2_size, arc_cache.c);
                return -1;
            }

            arc_evict_to_ghost(victim, arc_cache.t1, arc_cache.b1,
                               ARC_B1, true);
        }
    }

    arc_verify_invariants();
    return 0;
}

/*!
 * @brief ARC Case I: Cache hit in T1 or T2
 *
 * From paper: "Move x to MRU of T2"
 *
 * Promotes entry from T1 to T2 (frequency list) on second+ access,
 * or moves within T2 if already frequent.
 *
 * @param[in] dir  Directory entry in T1 or T2
 */
static void arc_case_i(struct dir *dir)
{
    AFP_ASSERT(dir);
    int in_t1 = (dir->arc_list == ARC_T1);

    if (in_t1) {
        /* Hit in T1: promote to T2 (frequency list) */
        if (!dir->qidx_node) {
            LOG(log_error, logtype_afpd, "arc_case_i: NULL qidx_node in T1");
            return;
        }

        /* Move node from T1 to T2 without reallocation */
        if (queue_move_to_tail_of(arc_cache.t1, arc_cache.t2, dir->qidx_node) == NULL) {
            LOG(log_error, logtype_afpd, "arc_case_i: T1→T2 migration failed");
            AFP_PANIC("arc_case_i T1→T2 migration");
        }

        arc_cache.t1_size--;
        arc_cache.t2_size++;
        arc_cache.stats.hits_t1++;
        arc_cache.stats.promotions_t1_to_t2++;
        dir->arc_list = ARC_T2;  /* Update list membership */
        LOG(log_debug, logtype_afpd,
            "arc_case_i: promoted T1→T2 (did:%u)", ntohl(dir->d_did));
    } else {
        /* Hit in T2: move to MRU (tail) of T2 */
        if (!dir->qidx_node) {
            LOG(log_error, logtype_afpd, "arc_case_i: NULL qidx_node in T2");
            return;
        }

        /* Move node to tail without dequeue/enque (node recycling)
         * dir->qidx_node unchanged - new position */
        if (queue_move_to_tail(arc_cache.t2, dir->qidx_node) == NULL) {
            LOG(log_error, logtype_afpd, "arc_case_i: T2 move to tail failed");
            AFP_PANIC("arc_case_i T2 move_to_tail");
        }

        arc_cache.stats.hits_t2++;
        dir->arc_list = ARC_T2;  /* Stays in T2 */
        LOG(log_debug, logtype_afpd,
            "arc_case_i: T2 hit, moved to MRU (did:%u)", ntohl(dir->d_did));
    }

    arc_verify_invariants();
}

/*!
 * @brief ARC Case II: Ghost hit in B1
 *
 * From paper:
 * "Adapt p = min(c, p + max(|B2|/|B1|, 1))
 *  REPLACE(p)
 *  Move x to MRU of T2, load into cache"
 *
 * B1 hit indicates we evicted this entry too soon from T1.
 * Increase p to favor recency (grow T1 target size).
 *
 * Ghost entries in this implementation are full struct dir with DIRF_ARC_GHOST flag.
 * This function adapts p, calls REPLACE, and promotes ghost from B1 to T2 inline
 * using queue_move_to_tail_of() for zero-allocation transition.
 *
 * @param[in] ghost  Ghost entry (struct dir with DIRF_ARC_GHOST) in B1
 */
static void arc_case_ii_adapt_and_replace(struct dir *ghost)
{
    AFP_ASSERT(ghost);
    AFP_ASSERT(ghost->arc_list == ARC_B1);
    AFP_ASSERT(ghost->d_flags & DIRF_ARC_GHOST);
    /* Adapt p: increase T1 target size */
    size_t delta = 1;

    if (arc_cache.b1_size > 0 && arc_cache.b2_size > 0) {
        delta = (arc_cache.b2_size / arc_cache.b1_size) > 1 ?
                (arc_cache.b2_size / arc_cache.b1_size) : 1;
    }

    size_t old_p = arc_cache.p;
    arc_cache.p = (arc_cache.p + delta > arc_cache.c) ?
                  arc_cache.c : arc_cache.p + delta;

    if (arc_cache.p != old_p) {
        arc_cache.stats.adaptations++;
        arc_cache.stats.p_increases++;

        if (arc_cache.p > arc_cache.stats.p_max) {
            arc_cache.stats.p_max = arc_cache.p;
        }
    }

    if (!ghost->qidx_node) {
        LOG(log_error, logtype_afpd, "arc_case_ii: NULL qidx_node in B1");
        AFP_PANIC("arc_case_ii NULL qidx_node");
    }

    /* Protect ghost from arc_ensure_ghost_capacity() eviction:
     * Move ghost to MRU of B1 so it won't be the LRU victim when
     * arc_replace() → arc_evict_to_ghost() → arc_ensure_ghost_capacity()
     * needs to shrink B1 to maintain B1+B2 ≤ c. */
    queue_move_to_tail(arc_cache.b1, ghost->qidx_node);

    /* Call REPLACE to make room in cache (ghost is safe at MRU of B1) */
    if (arc_replace(0) != 0) {
        LOG(log_warning, logtype_afpd,
            "arc_case_ii: arc_replace failed, cache temporarily over capacity");
    }

    /* Promote ghost node from B1 to T2 */
    if (queue_move_to_tail_of(arc_cache.b1, arc_cache.t2,
                              ghost->qidx_node) == NULL) {
        LOG(log_error, logtype_afpd, "arc_case_ii: B1→T2 migration failed");
        AFP_PANIC("arc_case_ii B1→T2 migration");
    }

    /* Clear ghost flag - now a full cache entry in T2 */
    ghost->d_flags &= ~DIRF_ARC_GHOST;
    ghost->arc_list = ARC_T2;
    arc_cache.b1_size--;
    arc_cache.t2_size++;
    arc_cache.stats.ghost_hits_b1++;
    LOG(log_debug, logtype_afpd,
        "arc_case_ii: B1 ghost hit (did:%u), p: %zu→%zu, delta:%zu, promoted to T2",
        ntohl(ghost->d_did), old_p, arc_cache.p, delta);
    arc_verify_invariants();
}

/*!
 * @brief ARC Case III: Ghost hit in B2
 *
 * From paper:
 * "Adapt p = max(0, p - max(|B1|/|B2|, 1))
 *  REPLACE(p)
 *  Move x to MRU of T2, load into cache"
 *
 * B2 hit indicates we evicted this entry too soon from T2.
 * Decrease p to favor frequency (grow T2 target size).
 *
 * Ghost entries in this implementation are full struct dir with DIRF_ARC_GHOST flag.
 * This function adapts p, calls REPLACE, and promotes ghost from B2 to T2 inline
 * using queue_move_to_tail_of() for zero-allocation transition.
 *
 * @param[in] ghost  Ghost entry (struct dir with DIRF_ARC_GHOST) in B2
 */
static void arc_case_iii_adapt_and_replace(struct dir *ghost)
{
    AFP_ASSERT(ghost);
    AFP_ASSERT(ghost->arc_list == ARC_B2);
    AFP_ASSERT(ghost->d_flags & DIRF_ARC_GHOST);
    /* Adapt p: decrease T1 target size (increase T2) */
    size_t delta = 1;

    if (arc_cache.b1_size > 0 && arc_cache.b2_size > 0) {
        delta = (arc_cache.b1_size / arc_cache.b2_size) > 1 ?
                (arc_cache.b1_size / arc_cache.b2_size) : 1;
    }

    size_t old_p = arc_cache.p;
    arc_cache.p = (arc_cache.p > delta) ? arc_cache.p - delta : 0;

    if (arc_cache.p != old_p) {
        arc_cache.stats.adaptations++;
        arc_cache.stats.p_decreases++;

        if (arc_cache.p < arc_cache.stats.p_min) {
            arc_cache.stats.p_min = arc_cache.p;
        }
    }

    if (!ghost->qidx_node) {
        LOG(log_error, logtype_afpd, "arc_case_iii: NULL qidx_node in B2");
        AFP_PANIC("arc_case_iii NULL qidx_node");
    }

    /* Protect ghost from arc_ensure_ghost_capacity() eviction:
     * Move ghost to MRU of B2 so it won't be the LRU victim when
     * arc_replace() → arc_evict_to_ghost() → arc_ensure_ghost_capacity()
     * needs to shrink B2 to maintain B1+B2 ≤ c. */
    queue_move_to_tail(arc_cache.b2, ghost->qidx_node);

    /* Call REPLACE to make room in cache (ghost is safe at MRU of B2) */
    if (arc_replace(1) != 0) {  /* in_b2 = 1 */
        LOG(log_warning, logtype_afpd,
            "arc_case_iii: arc_replace failed, cache temporarily over capacity");
    }

    /* Move ghost node from B2 to T2 */
    if (queue_move_to_tail_of(arc_cache.b2, arc_cache.t2,
                              ghost->qidx_node) == NULL) {
        LOG(log_error, logtype_afpd, "arc_case_iii: B2→T2 migration failed");
        AFP_PANIC("arc_case_iii B2→T2 migration");
    }

    /* Clear ghost flag - now a full cache entry in T2 */
    ghost->d_flags &= ~DIRF_ARC_GHOST;
    ghost->arc_list = ARC_T2;
    arc_cache.b2_size--;
    arc_cache.t2_size++;
    arc_cache.stats.ghost_hits_b2++;
    LOG(log_debug, logtype_afpd,
        "arc_case_iii: B2 ghost hit (did:%u), p: %zu→%zu, delta:%zu, promoted to T2",
        ntohl(ghost->d_did), old_p, arc_cache.p, delta);
    arc_verify_invariants();
}

/*!
 * @brief ARC Case IV: Complete miss (not in any list)
 *
 * From paper:
 * "case (i): |L1| = c
 *      if |T1| < c then delete LRU of B1, REPLACE(p)
 *      else delete LRU of T1, remove from cache
 *  case (ii): |L1| < c and |L1| + |L2| ≥ c
 *      if |L1| + |L2| = 2c then delete LRU of B2
 *      REPLACE(p)
 *  Put x at MRU of T1, load into cache"
 *
 * Adds new entry to T1 (recency list).
 *
 * @param[in] dir  New directory entry to insert
 */
static void arc_case_iv(struct dir *dir)
{
    AFP_ASSERT(dir);
    size_t l1_size = arc_cache.t1_size + arc_cache.b1_size;
    size_t l2_size = arc_cache.t2_size + arc_cache.b2_size;

    /* Case (i): |L1| = c */
    if (l1_size == arc_cache.c) {
        if (arc_cache.t1_size < arc_cache.c) {
            /* Delete LRU of B1 (ghost), then REPLACE */
            struct dir *ghost = (struct dir *)dequeue(arc_cache.b1);

            if (ghost) {
                arc_cache.b1_size--;
                ghost->qidx_node = NULL;  /* Clear queue pointer after dequeue */
                /* Ghost is a full struct dir - remove from hash tables and free */
                dircache_remove(NULL, ghost, DIRCACHE | DIDNAME_INDEX);
                dir_free(ghost);
            }

            if (arc_replace(0) != 0) {
                LOG(log_warning, logtype_afpd,
                    "arc_case_iv(i): arc_replace failed, cache temporarily over capacity");
            }
        } else {
            /* |T1| = c: Delete LRU of T1 (no ghost created).
             * Use arc_find_victim to skip curdir — never free a live curdir pointer.
             * If T1 only has curdir, try T2 as fallback. */
            struct dir *victim = arc_find_victim(arc_cache.t1, arc_cache.t1_size);

            if (!victim) {
                /* T1 only has curdir, try T2 */
                victim = arc_find_victim(arc_cache.t2, arc_cache.t2_size);
            }

            if (victim) {
                /* Unlink victim's node from its queue (circular doubly-linked list) */
                qnode_t *node = victim->qidx_node;
                node->prev->next = node->next;
                node->next->prev = node->prev;
                free(node);

                if (victim->arc_list == ARC_T1) {
                    arc_cache.t1_size--;
                } else {
                    arc_cache.t2_size--;
                }

                victim->qidx_node = NULL;
                dircache_remove(NULL, victim, DIRCACHE | DIDNAME_INDEX);
                dir_free(victim);
            } else {
                LOG(log_warning, logtype_afpd,
                    "arc_case_iv(i): cannot evict, all entries are curdir "
                    "(T1=%zu, T2=%zu)", arc_cache.t1_size, arc_cache.t2_size);
            }
        }
    }
    /* Case (ii): |L1| < c and |L1| + |L2| ≥ c */
    else if (l1_size < arc_cache.c && (l1_size + l2_size) >= arc_cache.c) {
        /* If directory is full (2c), delete LRU of B2 (ghost) */
        if ((l1_size + l2_size) == 2 * arc_cache.c) {
            struct dir *ghost = (struct dir *)dequeue(arc_cache.b2);

            if (ghost) {
                arc_cache.b2_size--;
                ghost->qidx_node = NULL;  /* Clear queue pointer after dequeue */
                /* Ghost is a full struct dir - remove from hash tables and free */
                dircache_remove(NULL, ghost, DIRCACHE | DIDNAME_INDEX);
                dir_free(ghost);
            }
        }

        /* Call REPLACE */
        if (arc_replace(0) != 0) {
            LOG(log_warning, logtype_afpd,
                "arc_case_iv(ii): arc_replace failed, cache temporarily over capacity");
        }
    }

    /* Case (iii): Cache not full yet, just add */

    /* Add new entry to MRU (tail) of T1 */
    if ((dir->qidx_node = enqueue(arc_cache.t1, dir)) == NULL) {
        LOG(log_error, logtype_afpd, "arc_case_iv: T1 enqueue failed");
        AFP_PANIC("arc_case_iv T1 enqueue");
    }

    arc_cache.t1_size++;
    dir->arc_list = ARC_T1;  /* Mark as being in T1 */
    /* Track peak cached entries (parallel to LRU's queue_count_max tracking) */
    size_t total_cached = arc_cache.t1_size + arc_cache.t2_size;

    if (total_cached > queue_count_max) {
        queue_count_max = total_cached;
    }

    arc_verify_invariants();
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

static inline hash_val_t hash_didname(const void *p)
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

static inline int hash_comp_didname(const void *k1, const void *k2)
{
    const struct dir *key1 = (const struct dir *)k1;
    const struct dir *key2 = (const struct dir *)k2;
    return !(key1->d_vid == key2->d_vid
             && key1->d_pdid == key2->d_pdid
             && (bstrcmp(key1->d_u_name, key2->d_u_name) == 0));
}

/***************************
 * queue index on dircache */

static q_t *index_queue;
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
        /* Always validate if freq is 0 (invalid config) */
        return 1;
    }

    /* Use the fetched value + 1 (post-increment semantics) */
    return ((count + 1) % dircache_validation_freq == 0);
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

    while (i--) {
        if ((dir = (struct dir *)dequeue(index_queue)) == NULL) { /* 1 */
            dircache_dump();
            AFP_PANIC("dircache_evict");
        }

        queue_count--;
        dir->qidx_node = NULL;  /* Clear queue pointer after dequeue */

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
    LOG(log_debug, logtype_afpd,
        "dircache_evict: evictions complete (entries now=%lu)", queue_count);
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
 * @param[in] vol      pointer to struct vol
 * @param[in] cnid     CNID of the directory to search
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

    /* Handle CNID_INVALID (0) gracefully - can occur in race conditions
     * when another process deletes a directory while we're looking it up. */
    if (cnid == CNID_INVALID) {
        LOG(log_debug, logtype_afpd,
            "dircache_search_by_did: cnid is CNID_INVALID, returning NULL");
        return NULL;
    }

    /* Special AFP system DIDs (DIRDID_ROOT_PARENT=1, DIRDID_ROOT=2) are not cached */
    if (ntohl(cnid) == 1 || ntohl(cnid) == 2) {
        LOG(log_debug, logtype_afpd,
            "dircache_search_by_did: cnid %u is a special AFP system DID, returning NULL",
            ntohl(cnid));
        return NULL;
    }

    /* DIDs 3-16 are reserved but unused - should never occur in normal operation */
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
                ntohl(cnid), cdir->d_u_name ? cfrombstr(cdir->d_u_name) : "(null)");
            (void)dir_remove(vol, cdir, 1);  /* Invalid, Expunged during validation */
            dircache_stat.expunged++;
            dircache_stat.misses++;  /* Count expunge as miss */
            return NULL;
        }

        /* Periodic validation to detect external filesystem changes.
         * Internal netatalk operations invalidate cache explicitly via dir_remove(). */
        if (should_validate_cache_entry()) {
            /* Check if file still exists */
            if (ostat(cfrombstr(cdir->d_fullpath), &st, vol_syml_opt(vol)) != 0) {
                LOG(log_debug, logtype_afpd,
                    "dircache(cnid:%u): {missing:\"%s\"}",
                    ntohl(cnid), cfrombstr(cdir->d_fullpath));
                (void)dir_remove(vol, cdir, 1);  /* Invalid, Expunged during validation */
                dircache_stat.expunged++;
                dircache_stat.misses++;  /* Count expunge as miss */
                return NULL;
            }

            /* Direct validation: inode or ctime changed = refresh in-place.
             * Per POSIX (IEEE 1003.1-2017 §4.9 "File Times Update"), st_ctime
             * ("time of last file status change") is updated by any operation
             * that modifies file metadata: chmod (mode), chown (uid/gid),
             * write/truncate (size/mtime), link/unlink, rename, utimens, etc.
             * Therefore checking only inode + ctime is sufficient to detect
             * external changes to ALL cached stat fields.
             * This is guaranteed on all Netatalk target platforms: Linux
             * (ext4/XFS/Btrfs), macOS (APFS/HFS+), FreeBSD/NetBSD/OpenBSD
             * (UFS/FFS/ZFS) — all implement POSIX ctime semantics. */
            if (cdir->dcache_ino != st.st_ino || cdir->dcache_ctime != st.st_ctime) {
                LOG(log_debug, logtype_afpd,
                    "dircache(cnid:%u): {modified:\"%s\"} (ino:%llu->%llu, ctime:%ld->%ld)",
                    ntohl(cnid), cdir->d_u_name ? cfrombstr(cdir->d_u_name) : "(null)",
                    (unsigned long long)cdir->dcache_ino, (unsigned long long)st.st_ino,
                    (long)cdir->dcache_ctime, (long)st.st_ctime);
                /* Refresh stat in-place via dir_modify(DCMOD_STAT).
                 * Handles inode changes (CNID refresh + AD invalidation)
                 * and ctime changes (AD invalidation + stat update).
                 * Avoids destroying the entire entry and forcing a
                 * full rebuild from the CNID database. */
                dir_modify(vol, cdir, &(struct dir_modify_args) {
                    .flags = DCMOD_STAT,
                    .st = &st
                });

                /* dir_modify may evict the entry (inode change + CNID miss) */
                if (cdir->d_did == CNID_INVALID) {
                    dircache_stat.expunged++;
                    dircache_stat.misses++;
                    return NULL;
                }

                /* Entry refreshed in-place — fall through to ARC handling */
            }

            /* Refresh non-ctime/inode stat fields from fresh stat data.
             * Redundant after dir_modify (mismatch path) but needed
             * when ctime/inode matched (validation-passed path). */
            cdir->dcache_mode = st.st_mode;
            cdir->dcache_mtime = st.st_mtime;
            cdir->dcache_uid = st.st_uid;
            cdir->dcache_gid = st.st_gid;
            cdir->dcache_size = st.st_size;
            LOG(log_maxdebug, logtype_afpd,
                "dircache(cnid:%u): {validated: path:\"%s\"}",
                ntohl(cnid), cfrombstr(cdir->d_fullpath));
        } else {
            /* Skipped validation for performance */
            LOG(log_maxdebug, logtype_afpd,
                "dircache(cnid:%u): {cached (unvalidated): path:\"%s\"}",
                ntohl(cnid), cfrombstr(cdir->d_fullpath));
        }

        /* ARC mode: Handle cache hits or ghost hits */
        if (arc_cache.enabled) {
            if (cdir->d_flags & DIRF_ARC_GHOST) {
                /* Ghost hit (Case II or III)
                 * Per ARC paper: Ghost hit = CACHE MISS + ghost learning
                 * "a miss in ARC(c), a hit in DBL(2c)"
                 * Ghost entries keep ALL data so we can return them after promotion.
                 * Handle adaptation and REPLACE
                 */
                if (cdir->arc_list == ARC_B1) {
                    arc_case_ii_adapt_and_replace(cdir);  /* Case II - promotes inline */
                } else if (cdir->arc_list == ARC_B2) {
                    arc_case_iii_adapt_and_replace(cdir);  /* Case III - promotes inline */
                }

                /* Ghost promoted to T2 inside case_ii/case_iii (zero-allocation) */
                dircache_stat.ghost_hits++;
            } else {
                /* Regular cache hit (Case I) in T1 or T2 */
                arc_case_i(cdir);
                dircache_stat.hits++;
            }
        } else {
            /* LRU mode: All found entries are cache hits */
            dircache_stat.hits++;
        }
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
 * @param[in] vol      volume
 * @param[in] dir      directory
 * @param[in] name     name (server side encoding)
 * @param[in] len      strlen of name
 *
 * @returns pointer to struct dir if found in cache, else NULL
 */
struct dir *dircache_search_by_name(const struct vol *vol,
                                    const struct dir *dir,
                                    char *name,
                                    size_t len)
{
    struct dir *cdir = NULL;
    struct dir key;
    struct stat st;
    hnode_t *hn;
    AFP_ASSERT(vol);
    AFP_ASSERT(dir);
    AFP_ASSERT(name);
    AFP_ASSERT(len == strlen(name));
    AFP_ASSERT(len < 256);  /* Ensure safe conversion to int */
    struct tagbstring uname = {-1, (int)len, (unsigned char *)name};
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
        /* Periodic validation to detect external filesystem changes.
         * Internal netatalk operations invalidate cache explicitly via dir_remove(). */
        if (should_validate_cache_entry()) {
            /* Check if file still exists */
            if (ostat(cfrombstr(cdir->d_fullpath), &st, vol_syml_opt(vol)) != 0) {
                LOG(log_debug, logtype_afpd,
                    "dircache(did:%u,\"%s\"): {missing:\"%s\"}",
                    ntohl(dir->d_did), name, cfrombstr(cdir->d_fullpath));
                (void)dir_remove(vol, cdir, 1);  /* Invalid, Expunged during validation */
                dircache_stat.expunged++;
                dircache_stat.misses++;  /* Count expunge as miss */
                return NULL;
            }

            /* Direct validation: inode or ctime changed = refresh in-place.
             * Per POSIX (IEEE 1003.1-2017 §4.9 "File Times Update"), st_ctime
             * ("time of last file status change") is updated by any operation
             * that modifies file metadata: chmod (mode), chown (uid/gid),
             * write/truncate (size/mtime), link/unlink, rename, utimens, etc.
             * Therefore checking only inode + ctime is sufficient to detect
             * external changes to ALL cached stat fields.
             * This is guaranteed on all Netatalk target platforms: Linux
             * (ext4/XFS/Btrfs), macOS (APFS/HFS+), FreeBSD/NetBSD/OpenBSD
             * (UFS/FFS/ZFS) — all implement POSIX ctime semantics. */
            if (cdir->dcache_ino != st.st_ino || cdir->dcache_ctime != st.st_ctime) {
                LOG(log_debug, logtype_afpd,
                    "dircache(did:%u,\"%s\"): {modified} (ino:%llu->%llu, ctime:%ld->%ld)",
                    ntohl(dir->d_did), name,
                    (unsigned long long)cdir->dcache_ino, (unsigned long long)st.st_ino,
                    (long)cdir->dcache_ctime, (long)st.st_ctime);
                /* Refresh stat in-place via dir_modify(DCMOD_STAT).
                 * Handles inode changes (CNID refresh + AD invalidation)
                 * and ctime changes (AD invalidation + stat update).
                 * Avoids destroying the entire entry and forcing a
                 * full rebuild from the CNID database. */
                dir_modify(vol, cdir, &(struct dir_modify_args) {
                    .flags = DCMOD_STAT,
                    .st = &st
                });

                /* dir_modify may evict the entry (inode change + CNID miss) */
                if (cdir->d_did == CNID_INVALID) {
                    dircache_stat.expunged++;
                    dircache_stat.misses++;
                    return NULL;
                }

                /* Entry refreshed in-place — fall through to ARC handling */
            }

            /* Refresh non-ctime/inode stat fields from fresh stat data.
             * Redundant after dir_modify (mismatch path) but needed
             * when ctime/inode matched (validation-passed path). */
            cdir->dcache_mode = st.st_mode;
            cdir->dcache_mtime = st.st_mtime;
            cdir->dcache_uid = st.st_uid;
            cdir->dcache_gid = st.st_gid;
            cdir->dcache_size = st.st_size;
        }

        /* ARC mode: Handle cache hits or ghost hits */
        if (arc_cache.enabled) {
            if (cdir->d_flags & DIRF_ARC_GHOST) {
                /* Ghost hit (Case II or III) = cache miss per ARC paper */
                /* Handle adaptation and REPLACE */
                if (cdir->arc_list == ARC_B1) {
                    arc_case_ii_adapt_and_replace(cdir);  /* Case II - promotes inline */
                } else if (cdir->arc_list == ARC_B2) {
                    arc_case_iii_adapt_and_replace(cdir);  /* Case III - promotes inline */
                }

                /* Ghost promoted to T2 inside case_ii/case_iii (zero-allocation) */
                /* Ghost hit = instant return in our implementation (keeps full data) */
                dircache_stat.ghost_hits++;
            } else {
                /* Regular cache hit (Case I) in T1 or T2 */
                arc_case_i(cdir);
                /* Real cache hit */
                LOG(log_maxdebug, logtype_afpd,
                    "dircache(did:%u,\"%s\"): {found in cache}",
                    ntohl(dir->d_did), name);
                dircache_stat.hits++;
            }
        } else {
            /* LRU mode: All found entries are cache hits */
            LOG(log_maxdebug, logtype_afpd,
                "dircache(did:%u,\"%s\"): {found in cache}",
                ntohl(dir->d_did), name);
            dircache_stat.hits++;
        }
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
 * Supports both LRU mode (legacy) and ARC mode.
 *
 * @param[in] vol   pointer to volume
 * @param[in] dir   pointer to parent directory
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
            ntohl(dir->d_did), CNID_START,
            dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)");
    }

    AFP_ASSERT(ntohl(dir->d_did) >= CNID_START);
    AFP_ASSERT(dir->d_u_name);
    AFP_ASSERT(dir->d_vid);

    /* Hash table capacity check */
    if (arc_cache.enabled) {
        /* ARC can have up to 2c entries (c cached + c ghosts per paper) */
        AFP_ASSERT(dircache->hash_nodecount <= dircache_maxsize * 2);
    } else {
        /* LRU only has c entries, so max hash = max cache */
        AFP_ASSERT(dircache->hash_nodecount <= dircache_maxsize);
    }

    /*
     * Make sure we don't add duplicates
     */
    /* Search primary cache by CNID */
    key.d_vid = dir->d_vid;
    key.d_did = dir->d_did;

    if ((hn = hash_lookup(dircache, &key))) {
        /* Found an entry with the same CNID, delete it */
        struct dir *duplicate = hnode_get(hn);
        LOG(log_debug, logtype_afpd,
            "dircache_add: found duplicate CNID entry: did:%u, removing",
            ntohl(duplicate->d_did));
        dir_remove(vol, duplicate, 0);  /* Proactive duplicate cleanup */
        dircache_stat.expunged++;
    }

    key.d_vid = vol->v_vid;
    key.d_pdid = dir->d_pdid;
    key.d_u_name = dir->d_u_name;

    if ((hn = hash_lookup(index_didname, &key))) {
        /* Found an entry with the same DID/name, delete it */
        struct dir *duplicate = hnode_get(hn);
        LOG(log_debug, logtype_afpd,
            "dircache_add: found duplicate DID/name entry: pdid:%u name:'%s', removing",
            ntohl(duplicate->d_pdid),
            duplicate->d_u_name ? cfrombstr(duplicate->d_u_name) : "(null)");
        dir_remove(vol, duplicate, 0);  /* Proactive duplicate cleanup */
        dircache_stat.expunged++;
    }

    /* LRU mode: Check cache fullness and evict BEFORE adding to hash tables */
    if (!arc_cache.enabled && dircache->hash_nodecount >= dircache_maxsize) {
        LOG(log_debug, logtype_afpd,
            "dircache_add: cache full, evicting before adding did:%u",
            ntohl(dir->d_did));
        dircache_evict();
    }

    /* Add it to the main dircache hash table (both modes) */
    if (hash_alloc_insert(dircache, dir, dir) == 0) {
        LOG(log_error, logtype_afpd,
            "dircache_add: main hash insert failed for did:%u",
            ntohl(dir->d_did));
        dircache_dump();
        exit(EXITERR_SYS);
    }

    /* Add it to the did/name index (both modes) */
    if (hash_alloc_insert(index_didname, dir, dir) == 0) {
        /* insert failed; Rollback main hash to keep counts consistent */
        LOG(log_error, logtype_afpd,
            "dircache_add: didname hash insert failed for did:%u, rolling back",
            ntohl(dir->d_did));
        hn = hash_lookup(dircache, dir);

        if (hn) {
            hash_delete_free(dircache, hn);
        }

        dircache_dump();
        exit(EXITERR_SYS);
    }

    /* Add to queue index - dispatch based on mode */
    if (arc_cache.enabled) {
        /* ARC mode: Use Case IV (complete miss) */
        arc_case_iv(dir);
    } else {
        /* LRU mode: Enqueue the new entry */
        if ((dir->qidx_node = enqueue(index_queue, dir)) == NULL) {
            dircache_dump();
            exit(EXITERR_SYS);
        }

        queue_count++;

        /* Track peak entries for statistics */
        if (queue_count > queue_count_max) {
            queue_count_max = queue_count;
        }
    }

    dircache_stat.added++;
    LOG(log_debug, logtype_afpd, "dircache(did:%u,'%s'): {added}",
        ntohl(dir->d_did), dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)");

    if (!arc_cache.enabled) {
        AFP_ASSERT(queue_count == index_didname->hash_nodecount
                   && queue_count == dircache->hash_nodecount);
    }

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
        /* Remove from queue - dispatch based on mode */
        if (arc_cache.enabled) {
            /* ARC mode: Remove from appropriate ARC list */
            if (dir->qidx_node) {
                dequeue(dir->qidx_node->prev);

                /* Update list size based on which list it was in */
                switch (dir->arc_list) {
                case ARC_T1:
                    arc_cache.t1_size--;
                    break;

                case ARC_T2:
                    arc_cache.t2_size--;
                    break;

                case ARC_B1:
                    arc_cache.b1_size--;
                    break;

                case ARC_B2:
                    arc_cache.b2_size--;
                    break;

                case ARC_NONE:
                    /* Entry not in any ARC list */
                    break;

                default:
                    /* Should never reach here - all arc_list values covered */
                    LOG(log_error, logtype_afpd,
                        "dircache_remove: unknown arc_list value: %d", dir->arc_list);
                    break;
                }

                dir->arc_list = ARC_NONE;
                dir->qidx_node = NULL;  /* Clear queue pointer after dequeue */
            }
        } else {
            /* LRU mode: Remove from queue */
            if (!dir->qidx_node) {
                LOG(log_error, logtype_afpd,
                    "dircache_remove: NULL qidx_node for did:%u (orphan entry!)",
                    ntohl(dir->d_did));
            } else {
                dequeue(dir->qidx_node->prev);
                /* Clear queue pointer after dequeue */
                dir->qidx_node = NULL;
                queue_count--;
            }
        }
    }

    if (flags & DIDNAME_INDEX) {
        if ((hn = hash_lookup(index_didname, dir)) == NULL) {
            LOG(log_error, logtype_afpd,
                "dircache_remove: Cache entry not in didname index: did:%u",
                ntohl(dir->d_did));
        } else {
            hash_delete_free(index_didname, hn);
        }
    }

    if (flags & DIRCACHE) {
        if ((hn = hash_lookup(dircache, dir)) == NULL) {
            LOG(log_error, logtype_afpd,
                "dircache_remove: Cache entry not in dircache: did:%u",
                ntohl(dir->d_did));
        } else {
            hash_delete_free(dircache, hn);
        }
    }

    LOG(log_debug, logtype_afpd, "dircache(did:%u,\"%s\"): {removed}",
        ntohl(dir->d_did), dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)");
    dircache_stat.removed++;
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
 * @param[in] vol  volume
 * @param[in] dir  parent directory whose children should be removed
 * @returns 0 on success, -1 if curdir was removed and recovery failed
 */
int dircache_remove_children(const struct vol *vol, struct dir *dir)
{
    struct dir *entry;
    hnode_t *hn;
    hscan_t hs;
    /* Store dir pointers */
    struct dir **to_remove = NULL;
    int remove_count = 0;
    int remove_capacity = 0;

    if (!dir || !dir->d_fullpath) {
        return 0;
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
                    struct dir **tmp = realloc(to_remove, remove_capacity * sizeof(struct dir*));

                    if (!tmp) {
                        LOG(log_error, logtype_afpd,
                            "dircache_remove_children: out of memory");
                        free(to_remove);
                        return -1;
                    }

                    to_remove = tmp;
                }

                /* Store the dir pointer for removal */
                to_remove[remove_count] = entry;
                remove_count++;
                LOG(log_debug, logtype_afpd,
                    "dircache_remove_children: marking child \"%s\" (did:%u) for removal",
                    entry->d_u_name ? cfrombstr(entry->d_u_name) : "(null)", ntohl(entry->d_did));
            }
        }
    }

    /* Save curdir DID before loop in case dir_remove recovery fails */
    cnid_t saved_curdir_did = curdir ? curdir->d_did : CNID_INVALID;
    int recovery_status = 0;

    /* Second pass: remove collected entries from dircache */
    for (int i = 0; i < remove_count; i++) {
        entry = to_remove[i];
        LOG(log_debug, logtype_afpd,
            "dircache_remove_children: removing stale \"%s\" (did:%u) from dircache",
            entry->d_u_name ? cfrombstr(entry->d_u_name) : "(null)", ntohl(entry->d_did));

        /* dir_remove returns -1 if it removed curdir and recovery failed */
        if (dir_remove(vol, entry, 0) != 0) {
            /* Proactive cleanup of children after rename */
            recovery_status = -1;
        }
    }

    /* Defensive check: If curdir is NULL, dir_remove failed to recover */
    if (!curdir) {
        curdir = vol->v_root ? vol->v_root : &rootParent;
        LOG(log_debug, logtype_afpd,
            "dircache_remove_children: DEFENSIVE: curdir is NULL (was did:%u), fallback to %s",
            ntohl(saved_curdir_did), vol->v_root ? "volume root" : "rootParent");
        recovery_status = -1;
    }

    if (to_remove) {
        free(to_remove);
    }

    if (remove_count) {
        const char *status = recovery_status ? " (curdir recovery failed)" : "";
        LOG(log_debug, logtype_afpd,
            "dircache_remove_children: removed %d dircache entries of \"%s\"%s",
            remove_count, cfrombstr(dir->d_fullpath), status);
    } else {
        LOG(log_debug, logtype_afpd,
            "dircache_remove_children: removed %d dircache entries of \"%s\"",
            remove_count, cfrombstr(dir->d_fullpath));
    }

    return recovery_status;
}

/*!
 * @brief Re-insert entry into DID/name index after key change
 *
 * Called by dir_modify() after updating d_pdid / d_u_name.
 * The caller MUST have already removed the entry from the DIDNAME_INDEX
 * via dircache_remove(vol, dir, DIDNAME_INDEX) before calling this.
 *
 * @param[in] vol  Volume (required)
 * @param[in] dir  Entry with updated d_pdid / d_u_name (required)
 *
 * @returns 0 on success, -1 on hash insert failure
 */
int dircache_reindex_didname(const struct vol *vol _U_, struct dir *dir)
{
    AFP_ASSERT(dir);

    /* Caller has already called dircache_remove(vol, dir, DIDNAME_INDEX)
     * and updated dir->d_pdid / dir->d_u_name. Re-insert with new key. */
    if (hash_alloc_insert(index_didname, dir, dir) == 0) {
        LOG(log_error, logtype_afpd,
            "dircache_reindex_didname: re-insert failed for did:%u",
            ntohl(dir->d_did));
        return -1;
    }

    return 0;
}

/*!
 * @brief Promote a cache entry in ARC to signal recency
 *
 * Dispatches based on arc_list membership:
 *   T1/T2: arc_case_i() — move to MRU of T2
 *   B1:    arc_case_ii_adapt_and_replace() — promote ghost to T2
 *   B2:    arc_case_iii_adapt_and_replace() — promote ghost to T2
 *   LRU:   no-op (LRU is FIFO by design, R7 D-010)
 *
 * @param[in,out] dir  Cache entry to promote (required)
 */
void dircache_promote(struct dir *dir)
{
    AFP_ASSERT(dir);

    if (arc_cache.enabled) {
        switch (dir->arc_list) {
        case ARC_T1:
        case ARC_T2:
            arc_case_i(dir);       /* Case I: T1→T2 or T2 MRU */
            break;

        case ARC_B1:
            arc_case_ii_adapt_and_replace(dir);   /* Case II: B1→T2 */
            break;

        case ARC_B2:
            arc_case_iii_adapt_and_replace(dir);  /* Case III: B2→T2 */
            break;

        default:
            break;  /* ARC_NONE: not in any list */
        }
    }

    /* LRU mode: no-op — LRU is FIFO by design (R7 D-010) */
}

/*!
 * @brief Initialize the dircache and indexes
 *
 * This is called in child afpd initialization. The maximum cache size will be
 * max(DEFAULT_DIRCACHE_SIZE, min(size, MAX_DIRCACHE_SIZE)).
 * It initializes a hashtable which we use to store a directory cache in.
 * It also initializes two indexes:
 * - a DID/name index on the main dircache
 * - a queue index on the dircache (LRU mode) or four queues (ARC mode)
 *
 * @param[in] reqsize   requested maximum size from afp.conf
 *
 * @returns 0 on success, -1 on error
 */
int dircache_init(int reqsize)
{
    extern AFPObj *AFPobj;
    int use_arc = 0;
    unsigned int hash_size;

    /* Check if ARC mode dircache via configuration first */
    if (AFPobj && AFPobj->options.dircache_mode == 1) {
        use_arc = 1;
    }

    /* Initialize the main dircache with requested size
     * Bounds: MIN_DIRCACHE_SIZE (8K) to MAX_DIRCACHE_SIZE (2M)
     * Default: DEFAULT_DIRCACHE_SIZE (64K) if reqsize <= 0 or out of bounds */
    if (reqsize > 0 && reqsize >= MIN_DIRCACHE_SIZE
            && reqsize <= MAX_DIRCACHE_SIZE) {
        /* Use requested size, rounding up to next power of 2 if needed */
        dircache_maxsize = MIN_DIRCACHE_SIZE;

        while (dircache_maxsize < reqsize && dircache_maxsize < MAX_DIRCACHE_SIZE) {
            dircache_maxsize *= 2;
        }
    } else if (reqsize > MAX_DIRCACHE_SIZE) {
        /* Requested size too large, use maximum */
        dircache_maxsize = MAX_DIRCACHE_SIZE;
        LOG(log_warning, logtype_afpd,
            "dircache_init: requested size %d exceeds maximum %d, using maximum",
            reqsize, MAX_DIRCACHE_SIZE);
    } else {
        /* Use default (reqsize <= 0 or < MIN_DIRCACHE_SIZE) */
        dircache_maxsize = DEFAULT_DIRCACHE_SIZE;

        if (reqsize > 0 && reqsize < MIN_DIRCACHE_SIZE) {
            LOG(log_warning, logtype_afpd,
                "dircache_init: requested size %d below minimum %d, using default %d",
                reqsize, MIN_DIRCACHE_SIZE, DEFAULT_DIRCACHE_SIZE);
        }
    }

    /* Determine hash table size based on mode:
     * LRU: hash_size = c (only cached entries)
     * ARC: hash_size = 2c (c cached + c ghosts per ARC paper) */
    hash_size = use_arc ? (dircache_maxsize * 2) : dircache_maxsize;
    LOG(log_info, logtype_afpd,
        "dircache_init: cache size: %u, hash size: %u, mode: %s",
        dircache_maxsize, hash_size, use_arc ? "ARC" : "LRU");

    if ((dircache = hash_create(hash_size, hash_comp_vid_did,
                                hash_vid_did)) == NULL) {
        return -1;
    }

    /* Initialize did/name index hashtable with same size */
    if ((index_didname = hash_create(hash_size, hash_comp_didname,
                                     hash_didname)) == NULL) {
        return -1;
    }

    if (use_arc) {
        /* Initialize ARC cache structures */
        if (arc_init(dircache_maxsize) != 0) {
            LOG(log_error, logtype_afpd, "dircache_init: ARC initialization failed");
            return -1;
        }
    } else {
        /* Initialize LRU queue (legacy mode) */
        if ((index_queue = queue_init()) == NULL) {
            return -1;
        } else {
            queue_count = 0;
        }

        LOG(log_info, logtype_afpd,
            "dircache_init: LRU mode enabled (size: %u)", dircache_maxsize);
    }

    /* Initialize invalid entries queue (used by both modes) */
    if ((invalid_dircache_entries = queue_init()) == NULL) {
        if (arc_cache.enabled) {
            arc_destroy();
        }

        return -1;
    }

    /* As long as directory.c hasn't got its own initializer call, we do it here.
     * Most numeric fields are set by C11 designated initializers in directory.c.
     * d_did uses htonl() which is not a compile-time constant, set here at runtime. */
    rootParent.d_did = DIRDID_ROOT_PARENT;
    rootParent.d_fullpath = bfromcstr("ROOT_PARENT");
    rootParent.d_m_name = bfromcstr("ROOT_PARENT");
    rootParent.d_u_name = rootParent.d_m_name;

    /* Apply validation parameters from configuration */
    if (AFPobj && AFPobj->options.dircache_validation_freq > 0) {
        dircache_set_validation_params(
            (unsigned int)AFPobj->options.dircache_validation_freq);
    }

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
    /* Memory accounting: struct dir size × high-water mark entry count.
     * This is the fixed-size allocation per entry; excludes variable-length
     * heap data (d_fullpath, d_m_name, d_u_name bstrings, d_m_name_ucs2). */
    unsigned long cache_max_mem_kb =
        (queue_count_max * sizeof(struct dir)) / 1024;

    if (dircache_stat.lookups > 0) {
        hit_ratio = ((double)dircache_stat.hits / (double)dircache_stat.lookups) *
                    100.0;
    }

    /* validation_ratio = validations / (entries_returned_from_cache) * 100
     * For LRU: entries_returned = hits (no ghosts)
     */
    if (dircache_validation_freq > 0 && dircache_stat.hits > 0) {
        /* Thread-safe read of validation counter */
        uint64_t counter_value = __sync_fetch_and_add(&validation_counter, 0);
        validation_ratio = ((double)counter_value / (double)dircache_validation_freq) /
                           (double)dircache_stat.hits * 100.0;
    }

    /* Get username from AFPobj if available. AFPobj->username is an array,
     * not a pointer so only need to check if AFPobj is non-null. */
    extern AFPObj *AFPobj;
    const char *username = (AFPobj
                            && AFPobj->username[0]) ? AFPobj->username : "unknown";

    if (arc_cache.enabled) {
        /* ARC mode statistics - includes all LRU baseline metrics plus ARC-specific */
        size_t total_cached = arc_cache.t1_size + arc_cache.t2_size;
        size_t total_ghosts = arc_cache.b1_size + arc_cache.b2_size;
        uint64_t total_arc_evictions = arc_cache.stats.evictions_t1 +
                                       arc_cache.stats.evictions_t2;
        double freq_bias = (total_cached > 0) ?
                           ((double)arc_cache.t2_size / (double)total_cached) * 100.0 : 0.0;
        double miss_ratio = (dircache_stat.lookups > 0) ?
                            ((double)dircache_stat.misses / (double)dircache_stat.lookups) * 100.0 : 0.0;
        /* Safe read of validation counters
         * validation_ratio = validations / (entries_returned_from_cache) * 100
         * For ARC: entries_returned = hits + ghost_hits (both returned from cache)
         */
        uint64_t validations_performed = 0;
        uint64_t entries_returned = dircache_stat.hits + dircache_stat.ghost_hits;

        if (dircache_validation_freq > 0 && entries_returned > 0) {
            uint64_t counter_value = __sync_fetch_and_add(&validation_counter, 0);
            validations_performed = counter_value / dircache_validation_freq;
            validation_ratio = ((double)counter_value / (double)dircache_validation_freq) /
                               (double)entries_returned * 100.0;
        }

        /* Main statistics line - shows cache hits, ghost hits, and true misses separately */
        double ghost_hit_ratio = (dircache_stat.lookups > 0) ?
                                 ((double)dircache_stat.ghost_hits / (double)dircache_stat.lookups) * 100.0 :
                                 0.0;
        double total_beneficial = (dircache_stat.lookups > 0) ?
                                  ((double)(dircache_stat.hits + dircache_stat.ghost_hits) /
                                   (double)dircache_stat.lookups) * 100.0 : 0.0;
        LOG(log_info, logtype_afpd,
            "dircache statistics (ARC): (user: %s) "
            "cache_entries: %zu, ghost_entries: %zu, max_entries: %lu (%lu KB), config_max: %zu, "
            "lookups: %llu, cache_hits: %llu (%.1f%%), ghost_hits: %llu (%.1f%%), total_hits: (%.1f%%), true_misses: %llu (%.1f%%), "
            "validations: ~%llu (%.1f%%), "
            "added: %llu, removed: %llu, expunged: %llu, invalid_on_use: %llu, "
            "evicted: %llu, validation_freq: %u",
            username,
            total_cached,
            total_ghosts,
            queue_count_max,
            cache_max_mem_kb,
            arc_cache.c,
            dircache_stat.lookups,
            dircache_stat.hits, hit_ratio,
            dircache_stat.ghost_hits,
            ghost_hit_ratio,
            total_beneficial,
            dircache_stat.misses,
            miss_ratio,
            validations_performed,
            validation_ratio,
            dircache_stat.added,
            dircache_stat.removed,
            dircache_stat.expunged,
            dircache_stat.invalid_on_use,
            total_arc_evictions,
            dircache_validation_freq);
        /* ARC-specific details: ghost hits breakdown and learning metrics */
        LOG(log_info, logtype_afpd,
            "ARC ghost performance: (user: %s) ghost_hits: %llu (%.1f%%), "
            "ghost_hits(B1=%llu, B2=%llu), learning_benefit: (%.1f%%)",
            username,
            dircache_stat.ghost_hits, ghost_hit_ratio,
            arc_cache.stats.ghost_hits_b1,
            arc_cache.stats.ghost_hits_b2,
            ghost_hit_ratio);  /* Learning benefit (wouldn't have been cached in LRU) */
        /* ARC table state - current snapshot of all four lists */
        LOG(log_info, logtype_afpd,
            "ARC table state: (user: %s) T1=%zu (recent/cached), T2=%zu (frequent/cached), "
            "B1=%zu (recent/ghost), B2=%zu (frequent/ghost), "
            "total_cached=%zu/%zu, total_ghosts=%zu, freq_bias: (%.1f%%)",
            username,
            arc_cache.t1_size, arc_cache.t2_size,
            arc_cache.b1_size, arc_cache.b2_size,
            total_cached, arc_cache.c, total_ghosts,
            freq_bias);
        /* ARC adaptation parameters - shows how cache is tuning itself */
        LOG(log_info, logtype_afpd,
            "ARC adaptation: (user: %s) p=%zu/%zu (%.1f%% target for T1), "
            "p_range=[%zu,%zu], adaptations=%llu (increases=%llu, decreases=%llu)",
            username,
            arc_cache.p, arc_cache.c,
            (arc_cache.c > 0) ? ((double)arc_cache.p / arc_cache.c * 100.0) : 0.0,
            arc_cache.stats.p_min, arc_cache.stats.p_max,
            arc_cache.stats.adaptations,
            arc_cache.stats.p_increases,
            arc_cache.stats.p_decreases);
        /* ARC operations breakdown - detailed operation counters */
        LOG(log_info, logtype_afpd,
            "ARC operations: (user: %s) cache_hits(T1=%llu, T2=%llu), "
            "promotions(T1→T2=%llu), evictions(T1=%llu, T2=%llu)",
            username,
            arc_cache.stats.hits_t1,
            arc_cache.stats.hits_t2,
            arc_cache.stats.promotions_t1_to_t2,
            arc_cache.stats.evictions_t1,
            arc_cache.stats.evictions_t2);
    } else {
        /* LRU mode statistics (no ghost_hits in LRU, should always be 0) */
        double miss_ratio = (dircache_stat.lookups > 0) ?
                            ((double)dircache_stat.misses / (double)dircache_stat.lookups) * 100.0 : 0.0;
        LOG(log_info, logtype_afpd,
            "dircache statistics (LRU): (user: %s) "
            "entries: %lu, max_entries: %lu (%lu KB), config_max: %u, lookups: %llu, hits: %llu (%.1f%%), misses: %llu (%.1f%%), "
            "validations: ~%llu (%.1f%%), "
            "added: %llu, removed: %llu, expunged: %llu, invalid_on_use: %llu, evicted: %llu, "
            "validation_freq: %u",
            username,
            queue_count,
            queue_count_max,
            cache_max_mem_kb,
            dircache_maxsize,
            dircache_stat.lookups,
            dircache_stat.hits, hit_ratio,
            dircache_stat.misses,
            miss_ratio,
            dircache_validation_freq > 0 ? __sync_fetch_and_add(&validation_counter,
                    0) / dircache_validation_freq : 0, validation_ratio,
            dircache_stat.added,
            dircache_stat.removed,
            dircache_stat.expunged,
            dircache_stat.invalid_on_use,
            dircache_stat.evicted,
            dircache_validation_freq);
    }

    /* Cross-process dircache hint statistics */
    LOG(log_info, logtype_afpd,
        "dircache statistics (hints): (user: %s) sent: %llu, received: %llu, acted_on: %llu, no_match: %llu",
        username,
        ipc_get_hints_sent(),
        cache_hint_stat.hints_received,
        cache_hint_stat.hints_acted_on,
        cache_hint_stat.hints_no_match);
    /* AD cache statistics (Tier 1) — from ad_cache.c extern counters */
    double ad_hit_ratio = 0.0;
    unsigned long long ad_total = ad_cache_hits + ad_cache_misses + ad_cache_no_ad;

    if (ad_total > 0) {
        ad_hit_ratio = ((double)(ad_cache_hits + ad_cache_no_ad) /
                        (double)ad_total) * 100.0;
    }

    LOG(log_info, logtype_afpd,
        "dircache statistics (AD): (user: %s) hits: %llu, misses: %llu, no_ad: %llu, hit_ratio: %.1f%%",
        username, ad_cache_hits, ad_cache_misses, ad_cache_no_ad,
        ad_hit_ratio);
}

/*!
 * @brief Dump dircache to /tmp/dircache.PID
 */
void dircache_dump(void)
{
    char tmpnam[MAXPATHLEN + 1];
    FILE *dump;
    qnode_t *n = NULL;
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

    /* Header with mode information */
    if (arc_cache.enabled) {
        fprintf(dump, "Directory Cache Mode: ARC (Adaptive Replacement Cache)\n");
        fprintf(dump, "Cache size (c): %zu\n", arc_cache.c);
        fprintf(dump, "Target T1 size (p): %zu (%.1f%%)\n",
                arc_cache.p,
                arc_cache.c > 0 ? ((double)arc_cache.p / (double)arc_cache.c) * 100.0 : 0.0);
        fprintf(dump, "T1: %zu, T2: %zu, B1: %zu, B2: %zu\n",
                arc_cache.t1_size, arc_cache.t2_size,
                arc_cache.b1_size, arc_cache.b2_size);
        fprintf(dump, "Total cached: %zu, Total ghosts: %zu\n\n",
                arc_cache.t1_size + arc_cache.t2_size,
                arc_cache.b1_size + arc_cache.b2_size);
    } else {
        fprintf(dump, "Directory Cache Mode: LRU (Least Recently Used)\n");
        fprintf(dump, "Number of cache entries in LRU queue: %lu\n", queue_count);
        fprintf(dump, "Configured maximum cache size: %u\n\n", dircache_maxsize);
    }

    fprintf(dump, "Primary CNID index:\n");
    fprintf(dump, "       VID     DID    CNID STAT PATH\n");
    fprintf(dump,
            "====================================================================\n");
    hash_scan_begin(&hs, dircache);
    i = 1;

    while ((hn = hash_scan_next(&hs))) {
        dir = hnode_get(hn);
        const char *ad_cache_state;

        if (dir->dcache_rlen >= 0) {
            ad_cache_state = "cached";
        } else if (dir->dcache_rlen == (off_t) -1) {
            ad_cache_state = "unloaded";
        } else {
            ad_cache_state = "absent";
        }

        fprintf(dump, "%05u: %3u  %6u  %6u %s  rlen:%-6lld ad:%-8s  %s\n",
                i++,
                ntohs(dir->d_vid),
                ntohl(dir->d_pdid),
                ntohl(dir->d_did),
                dir->d_flags & DIRF_ISFILE ? "f" : "d",
                (long long)dir->dcache_rlen,
                ad_cache_state,
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

    if (arc_cache.enabled) {
        /* ARC mode: Show T1, T2, B1, B2 lists separately */
        fprintf(dump, "\nT1 List (Recent, size=%zu, target_p=%zu):\n",
                arc_cache.t1_size, arc_cache.p);
        fprintf(dump, "       VID     DID    CNID STAT PATH\n");
        fprintf(dump,
                "====================================================================\n");
        n = arc_cache.t1->next;
        i = 1;

        while (n != arc_cache.t1) {
            dir = (struct dir *)n->data;
            fprintf(dump, "%05u: %3u  %6u  %6u %s    %s\n",
                    i++,
                    ntohs(dir->d_vid),
                    ntohl(dir->d_pdid),
                    ntohl(dir->d_did),
                    dir->d_flags & DIRF_ISFILE ? "f" : "d",
                    dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(ghost)");
            n = n->next;
        }

        fprintf(dump, "\nT2 List (Frequent, size=%zu):\n", arc_cache.t2_size);
        fprintf(dump, "       VID     DID    CNID STAT PATH\n");
        fprintf(dump,
                "====================================================================\n");
        n = arc_cache.t2->next;
        i = 1;

        while (n != arc_cache.t2) {
            dir = (struct dir *)n->data;
            fprintf(dump, "%05u: %3u  %6u  %6u %s    %s\n",
                    i++,
                    ntohs(dir->d_vid),
                    ntohl(dir->d_pdid),
                    ntohl(dir->d_did),
                    dir->d_flags & DIRF_ISFILE ? "f" : "d",
                    dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(ghost)");
            n = n->next;
        }

        fprintf(dump, "\nB1 List (Ghost/Recent, size=%zu):\n", arc_cache.b1_size);
        fprintf(dump, "       VID     DID    CNID STAT NAME\n");
        fprintf(dump,
                "====================================================================\n");
        n = arc_cache.b1->next;
        i = 1;

        while (n != arc_cache.b1) {
            dir = (struct dir *)n->data;
            fprintf(dump, "%05u: %3u  %6u  %6u %s    %s (GHOST)\n",
                    i++,
                    ntohs(dir->d_vid),
                    ntohl(dir->d_pdid),
                    ntohl(dir->d_did),
                    dir->d_flags & DIRF_ISFILE ? "f" : "d",
                    dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)");
            n = n->next;
        }

        fprintf(dump, "\nB2 List (Ghost/Frequent, size=%zu):\n", arc_cache.b2_size);
        fprintf(dump, "       VID     DID    CNID STAT NAME\n");
        fprintf(dump,
                "====================================================================\n");
        n = arc_cache.b2->next;
        i = 1;

        while (n != arc_cache.b2) {
            dir = (struct dir *)n->data;
            fprintf(dump, "%05u: %3u  %6u  %6u %s    %s (GHOST)\n",
                    i++,
                    ntohs(dir->d_vid),
                    ntohl(dir->d_pdid),
                    ntohl(dir->d_did),
                    dir->d_flags & DIRF_ISFILE ? "f" : "d",
                    dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)");
            n = n->next;
        }
    } else {
        /* LRU mode: Show single queue */
        fprintf(dump, "\nLRU Queue:\n");
        fprintf(dump, "       VID     DID    CNID STAT PATH\n");
        fprintf(dump,
                "====================================================================\n");
        n = index_queue ? index_queue->next : NULL;
        i = 1;

        while (n != NULL && n != index_queue && i <= (int)queue_count) {
            dir = (struct dir *)n->data;
            fprintf(dump, "%05u: %3u  %6u  %6u %s    %s\n",
                    i,
                    ntohs(dir->d_vid),
                    ntohl(dir->d_pdid),
                    ntohl(dir->d_did),
                    dir->d_flags & DIRF_ISFILE ? "f" : "d",
                    cfrombstr(dir->d_fullpath));
            n = n->next;
            i++;
        }
    }

    fprintf(dump, "\n");
    fflush(dump);
    fclose(dump);
    return;
}

/*!
 * @brief Set directory cache validation frequency
 *
 * Allows runtime configuration of cache validation behavior for performance tuning.
 * Lower validation frequency improves performance but may delay detection of
 * external filesystem changes.
 *
 * @param[in] freq  validation frequency (1 = validate every access, 100 = every 100th access)
 *
 * @returns 0 on success, -1 on invalid parameters
 */
int dircache_set_validation_params(unsigned int freq)
{
    if (freq == 0 || freq > 100) {
        LOG(log_error, logtype_afpd,
            "dircache_set_validation_params: invalid frequency %u (must be 1-100)", freq);
        return -1;
    }

    dircache_validation_freq = freq;
    /* Thread-safe reset of validation counter using compiler builtins */
    __sync_lock_test_and_set(&validation_counter, 0);
    LOG(log_info, logtype_afpd,
        "dircache: validation parameters updated (freq=%u), counter reset", freq);
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

/***********************************************************************************
 * Cross-process dircache hint receiver (parent→child via dedicated hint pipe)
 ***********************************************************************************/

/*!
 * @brief Read one cache hint from the dedicated hint pipe (non-blocking)
 *
 * Reads from obj->hint_fd — the dedicated pipe for parent→child hints.
 * POSIX pipe atomicity: write() of ≤ PIPE_BUF bytes is atomic, so each
 * read() returns either a complete message or nothing.
 *
 * @param[in]  obj   AFPObj with hint_fd (pipe read end)
 * @param[out] hint  Populated on success
 * @returns    1 if hint read, 0 if no data available, -1 on error
 */
static int ipc_recv_cache_hint(AFPObj *obj, struct ipc_cache_hint_payload *hint)
{
    char buf[IPC_MAXMSGSIZE];
    ssize_t ret;

    if (obj->hint_fd < 0) {
        return -1;
    }

    /* Read complete hint message from pipe — atomic delivery guaranteed */
    ret = read(obj->hint_fd, buf,
               IPC_HEADERLEN + sizeof(struct ipc_cache_hint_payload));

    if (ret == 0) {
        /* EOF — parent process crashed (pipe write end closed).
         * Close and disable to prevent POLLHUP spin loop. */
        LOG(log_error, logtype_afpd,
            "ipc_recv_cache_hint: hint pipe closed, parent lost, service restart required");
        close(obj->hint_fd);
        obj->hint_fd = -1;
        return -1;
    }

    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;    /* No data available */
        }

        return -1;     /* Unexpected error */
    }

    if (ret != IPC_HEADERLEN + (ssize_t)sizeof(struct ipc_cache_hint_payload)) {
        /* Should never happen for atomic pipe writes, but defensive */
        LOG(log_warning, logtype_afpd,
            "ipc_recv_cache_hint: unexpected read size %zd (expected %zu)",
            ret, IPC_HEADERLEN + sizeof(struct ipc_cache_hint_payload));
        cache_hint_stat.hints_no_match++;
        return 0;
    }

    /* Extract payload from after the IPC header */
    memcpy(hint, buf + IPC_HEADERLEN, sizeof(*hint));
    return 1;
}

/*!
 * @brief Process cross-process dircache invalidation hints
 *
 * Called from the DSI command loop after each AFP command completes.
 * Uses direct hash_lookup() on the CNID hash table to support BOTH
 * files (DIRF_ISFILE) and directories — dircache_search_by_did() is
 * unsuitable because it actively removes file entries.
 *
 * Dispatches on hint type:
 * - CACHE_HINT_REFRESH: ostat() + dir_modify() — stat refresh, AD invalidation
 * - CACHE_HINT_DELETE: direct dir_remove() — no ostat needed
 * - CACHE_HINT_DELETE_CHILDREN: dircache_remove_children() + parent cleanup
 */
void process_cache_hints(AFPObj *obj)
{
    struct ipc_cache_hint_payload hint = {0};
    struct stat st;
    int hints_processed = 0;
#define MAX_HINTS_PER_CYCLE 32

    while (hints_processed < MAX_HINTS_PER_CYCLE &&
            ipc_recv_cache_hint(obj, &hint) > 0) {
        cache_hint_stat.hints_received++;
        /* Resolve volume from vid (already network byte order).
         * Cannot be const — passed to dir_remove/dir_modify which take non-const vol. */
        struct vol *vol = getvolbyvid(hint.vid); // NOSONAR

        if (!vol) {
            cache_hint_stat.hints_no_match++;
            continue;
        }

        /* Validate CNID */
        if (hint.cnid == CNID_INVALID || ntohl(hint.cnid) < CNID_START) {
            cache_hint_stat.hints_no_match++;
            continue;
        }

        /* Direct hash lookup to find entry in our local dircache.
         * Uses hash_lookup() instead of dircache_search_by_did() because
         * the latter removes file entries (DIRF_ISFILE check). */
        struct dir key = { .d_vid = vol->v_vid, .d_did = hint.cnid };
        hnode_t *hn = hash_lookup(dircache, &key);

        if (!hn) {
            cache_hint_stat.hints_no_match++;
            continue;  /* Not in our cache — nothing to invalidate */
        }

        struct dir *entry = hnode_get(hn);

        cache_hint_stat.hints_acted_on++;
        /* Save fields for logging BEFORE dispatch — dir_remove() may free entry */
        cnid_t log_cnid = hint.cnid;
        int log_is_ghost = (entry->d_flags & DIRF_ARC_GHOST) ? 1 : 0;

        switch (hint.event) {
        case CACHE_HINT_REFRESH:

            /* ostat + dir_modify — unified handling for files and directories.
             * DCMOD_AD_INV unconditionally invalidates AD cache because the hint
             * is an authoritative signal metadata changed. ctime has second
             * granularity and misses sub-second cross-client changes.
             * DCMOD_NO_PROMOTE prevents ARC promotion from cross-process hints. */
            if (ostat(cfrombstr(entry->d_fullpath), &st, vol_syml_opt(vol)) != 0) {
                /* Entry gone or renamed — old path invalid, remove from cache */
                dir_remove(vol, entry, 0);
            } else {
                dir_modify(vol, entry, &(struct dir_modify_args) {
                    .flags = DCMOD_STAT | DCMOD_AD_INV | DCMOD_NO_PROMOTE,
                    .st = &st
                });
            }

            break;

        case CACHE_HINT_DELETE:
            /* Direct removal — no ostat needed for deleted entries */
            dir_remove(vol, entry, 0);
            break;

        case CACHE_HINT_DELETE_CHILDREN:

            /* Remove all children first, then handle parent entry */
            if (!(entry->d_flags & DIRF_ISFILE)) {
                dircache_remove_children(vol, entry);
            }

            /* Then remove or refresh the parent itself */
            if (ostat(cfrombstr(entry->d_fullpath), &st, vol_syml_opt(vol)) != 0) {
                dir_remove(vol, entry, 0);
            } else {
                dir_modify(vol, entry, &(struct dir_modify_args) {
                    .flags = DCMOD_STAT | DCMOD_NO_PROMOTE,
                    .st = &st
                });
            }

            break;

        default:
            LOG(log_warning, logtype_afpd,
                "process_cache_hints: unknown hint event %u for did:%u",
                hint.event, ntohl(hint.cnid));
            cache_hint_stat.hints_no_match++;
            break;
        }

        LOG(log_debug, logtype_afpd,
            "process_cache_hints: %s did:%u%s from sibling",
            hint.event == CACHE_HINT_REFRESH ? "refresh" :
            hint.event == CACHE_HINT_DELETE ? "delete" :
            hint.event == CACHE_HINT_DELETE_CHILDREN ? "delete-children" : "unknown",
            ntohl(log_cnid),
            log_is_ghost ? " (ghost)" : "");
        hints_processed++;
    }

    if (hints_processed > 0) {
        LOG(log_debug, logtype_afpd,
            "process_cache_hints: processed %d hints this cycle", hints_processed);
    }
}

/*!
 * @brief Report that a cache entry was invalid when actually used
 *
 * This function should be called when a cached directory entry that was
 * returned without validation (for performance) turns out to be invalid
 * when actually accessed (e.g., file doesn't exist, has been modified, etc).
 * This helps track the effectiveness of the validation frequency setting.
 *
 * @param[in] dir  The directory entry that was found to be invalid
 */
void dircache_report_invalid_entry(struct dir *dir)
{
    if (dir) {
        /* Increment for every cache refresh event */
        dircache_stat.invalid_on_use++;
        LOG(log_debug, logtype_afpd,
            "dircache: cache refresh required for \"%s\"",
            dir->d_u_name ? cfrombstr(dir->d_u_name) : "(null)");
    }
}

