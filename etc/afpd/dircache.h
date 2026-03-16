/*
 * Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>
 * Copyright (c) 2025-2026 Andy Lemin (@andylemin)
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

#ifndef DIRCACHE_H
#define DIRCACHE_H

#include <sys/types.h>

#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/volume.h>

/* Dircache size bounds */
#define MIN_DIRCACHE_SIZE 1024             /* 1K minimum (testing/constrained systems) */
#define DEFAULT_DIRCACHE_SIZE 65536        /* 64K default (production) */
#define MAX_DIRCACHE_SIZE 2097152          /* 2M maximum (high-memory servers) */
#define DIRCACHE_FREE_QUANTUM 256

/* Max dircache entries removed per hash chain per idle worker iteration.
 * 16 is compromise between latency (~1μs per dir_free × 16 = ~16μs per batch)
 * and is_idle polling frequency (instant yield to AFP commands) */
#define DEFERRED_CHAIN_BATCH 16

/* flags for dircache_remove */
#define DIRCACHE      (1 << 0)
#define DIDNAME_INDEX (1 << 1)
#define QUEUE_INDEX   (1 << 2)
#define DIRCACHE_NOSHRINK (1 << 3)  /* Skip hash table shrink */
#define DIRCACHE_ALL  (DIRCACHE|DIDNAME_INDEX|QUEUE_INDEX)

extern int        dircache_init(int reqsize);
extern int        dircache_add(const struct vol *, struct dir *);
extern void       dircache_remove(const struct vol *, struct dir *, int flag);
extern struct dir *dircache_search_by_did(const struct vol *vol, cnid_t did);
extern struct dir *dircache_search_by_name(const struct vol *,
        const struct dir *dir, char *name, size_t len);
extern void       dircache_dump(void);
extern void       log_dircache_stat(void);
extern int        dircache_set_validation_params(unsigned int freq);
extern void       dircache_reset_validation_counter(void);
extern void       dircache_report_invalid_entry(struct dir *dir);
extern int        dircache_remove_children(const struct vol *vol,
        struct dir *dir);
extern int        dircache_reindex_didname(const struct vol *vol,
        struct dir *dir);
extern void       dircache_promote(struct dir *dir);
extern void       process_cache_hints(AFPObj *obj);
extern void       dircache_remove_children_defer(const struct vol *vol,
        struct dir *dir);
extern void       dircache_flush_deferred_for_vol(uint16_t vid);
extern int        dircache_has_deferred_work(void);
extern int        dircache_process_deferred_chain(void);
extern void       dircache_rfork_shutdown(void);

/* Tier 2: Resource Fork data cache */
extern size_t rfork_cache_used;
extern size_t rfork_cache_budget;
extern size_t rfork_max_entry_size;
extern unsigned int rfork_lru_count;
extern q_t *rfork_lru;

/* Tier 2: Resource Fork data cache statistics */
extern unsigned long long rfork_stat_lookups;
extern unsigned long long rfork_stat_hits;
extern unsigned long long rfork_stat_misses;
extern unsigned long long rfork_stat_added;
extern unsigned long long rfork_stat_evicted;
extern unsigned long long rfork_stat_invalidated;
extern size_t rfork_stat_used_max;

#endif /* DIRCACHE_H */
