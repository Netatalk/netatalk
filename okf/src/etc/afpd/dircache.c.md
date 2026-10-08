---
type: C Source File
title: "etc/afpd/dircache.c"
description: "Directory Cache."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/dircache.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/queue.h](../../include/atalk/queue.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [hash.h](hash.h.md)
* [idle_worker.h](idle_worker.h.md)
* [pfd_cache.h](pfd_cache.h.md)
* System headers: `assert.h`, `bstrlib.h`, `errno.h`, `signal.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `time.h`

# Functions

### hash_chain_head

```c
static hnode_t * hash_chain_head(hash_t *hash, hashcount_t chain)
```

Defined at lines 133 to 136.

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

### hash_vid_did

```c
static hash_val_t hash_vid_did(const void *key)
```

Defined at lines 341 to 358.

Called by: [dircache_init](dircache.c.md#dircache_init)

### hash_comp_vid_did

```c
static int hash_comp_vid_did(const void *key1, const void *key2)
```

Defined at lines 360 to 365.

Called by: [dircache_init](dircache.c.md#dircache_init)

### arc_init

```c
static int arc_init(size_t cache_size)
```

Defined at lines 377 to 404.

Initialize ARC cache structures.

Parameters:
* `cache_size`: Total cache size (c)

Returns: 0 on success, -1 on error

Calls: [queue_init](../../libatalk/util/queue.c.md#queue_init)

Called by: [dircache_init](dircache.c.md#dircache_init)

Uses file-scope variables: `arc_cache`

### arc_destroy

```c
static void arc_destroy(void)
```

Defined at lines 409 to 442.

Destroy ARC cache structures.

Calls: [queue_destroy](../../libatalk/util/queue.c.md#queue_destroy)

Called by: [dircache_init](dircache.c.md#dircache_init)

Uses file-scope variables: `arc_cache`

### arc_verify_invariants

```c
static void arc_verify_invariants(void)
```

Defined at lines 450 to 474.

Verify ARC invariants (debug mode)

Checks that ARC list sizes satisfy all constraints from the paper. Called after each operation in debug builds.

Called by: [arc_case_i](dircache.c.md#arc_case_i), [arc_case_ii_adapt_and_replace](dircache.c.md#arc_case_ii_adapt_and_replace), [arc_case_iii_adapt_and_replace](dircache.c.md#arc_case_iii_adapt_and_replace), [arc_case_iv_insert](dircache.c.md#arc_case_iv_insert), [arc_replace](dircache.c.md#arc_replace)

Uses file-scope variables: `arc_cache`, `arc_promoting_ghost`

### dircache_purge_vol

```c
void dircache_purge_vol(const struct vol *vol)
```

Defined at lines 492 to 533.

Release every cache entry belonging to a closing volume.

Nothing else reclaims by volume, so entries left behind are found as valid when the vid is reused. DIRCACHE_NOSHRINK keeps the scan valid: each entry deletes only its own just-returned node and the table never rehashes mid-scan.

Nothing is exempt: [closevol()](../../bin/nad/nad_util.c.md#closevol) frees v_root next, so an entry kept back would outlive its volume under a reusable vid. A curdir still inside the volume is dropped to rootParent rather than left cached.

Parameters:
* `vol`: Volume being closed (required)

Calls: [dircache_defer_free](dircache.c.md#dircache_defer_free), [dircache_remove](dircache.c.md#dircache_remove), [hash_scan_begin](hash.c.md#hash_scan_begin), [hash_scan_next](hash.c.md#hash_scan_next), [hnode_get](hash.c.md#hnode_get)

Called by: [closevol](volume.c.md#closevol)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `dircache`, `rootParent` in [etc/afpd/directory.c](directory.c.md), `vid_entry_count`

### dircache_defer_free

```c
static void dircache_defer_free(struct dir *dir)
```

Defined at lines 546 to 557.

Hash-removed entry: defer its free to end-of-request.

Caller must have removed the entry from both hash indexes and every ARC/LRU queue. Deferring keeps pointers handed out earlier in this request valid (consumers gate on d_did != CNID_INVALID) — ARC ghost eviction can reach entries that WERE returned this request, so an inline free here could dangle a held pointer. On qnode OOM (effectively unreachable: the kernel OOM killer fires first) leak the entry instead; dir_remove makes the same choice.

Calls: [enqueue](../../libatalk/util/queue.c.md#enqueue), [iw_note_work](idle_worker.c.md#iw_note_work)

Called by: [arc_case_iv_make_room](dircache.c.md#arc_case_iv_make_room), [arc_ensure_ghost_capacity](dircache.c.md#arc_ensure_ghost_capacity), [dircache_evict](dircache.c.md#dircache_evict), [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_remove_children](dircache.c.md#dircache_remove_children)

Uses file-scope variables: `invalid_dircache_entries` in [etc/afpd/directory.c](directory.c.md)

### arc_ghost_trim_candidate

```c
qnode_t * arc_ghost_trim_candidate(q_t *q, const struct dir *skip)
```

Defined at lines 571 to 588.

Pick the ghost to release from a list at capacity.

LRU first, skipping the excluded entry; a single-node list is both MRU and LRU, so MRU ordering alone cannot exclude it.

Parameters:
* `q`: Ghost queue (B1 or B2)
* `skip`: Entry that must not be selected (ghost mid-promotion), or NULL

Returns: node to release, or NULL to leave the list alone

Called by: [arc_ensure_ghost_capacity](dircache.c.md#arc_ensure_ghost_capacity)

### arc_ensure_ghost_capacity

```c
static void arc_ensure_ghost_capacity(arc_list_t target_list)
```

Defined at lines 608 to 648.

Ensure the ghost lists have room for one more entry.

Maintains the ARC invariant: B1 + B2 ≤ c

Called before operations that will add a ghost entry to B1 or B2. If the ghost lists are at or over capacity, deletes the LRU ghost from the specified target list to make room.

This handles the edge case where entries are removed from the cache via [dircache_remove()](dircache.c.md#dircache_remove) (bypassing ghost lists), causing the ghost lists to stay at capacity while the cache shrinks.

A ghost mid-promotion is skipped, leaving the ghost lists one over capacity until that promotion moves it out.

Parameters:
* `target_list`: Which ghost list will receive new entry (ARC_B1 or ARC_B2)

Calls: [arc_ghost_trim_candidate](dircache.c.md#arc_ghost_trim_candidate), [dircache_defer_free](dircache.c.md#dircache_defer_free), [dircache_remove](dircache.c.md#dircache_remove), [queue_remove](../../libatalk/util/queue.c.md#queue_remove)

Called by: [arc_evict_to_ghost](dircache.c.md#arc_evict_to_ghost)

Uses file-scope variables: `arc_cache`, `arc_promoting_ghost`

### arc_find_victim

```c
static struct dir * arc_find_victim(q_t *queue, size_t queue_size)
```

Defined at lines 662 to 689.

Find an evictable victim in a cache queue, skipping curdir.

Only searches T1 and T2 (the real cache queues). B1/B2 are ghost lists with full data.

Tries the LRU entry first; if it's curdir, tries the second-LRU.

Parameters:
* `queue`: The ARC queue to search (T1 or T2)
* `queue_size`: Number of entries in the queue

Returns: evictable dir entry, or NULL if queue is empty/only contains curdir

Called by: [arc_case_iv_make_room](dircache.c.md#arc_case_iv_make_room), [arc_replace](dircache.c.md#arc_replace)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### arc_evict_to_ghost

```c
static void arc_evict_to_ghost(struct dir *victim, q_t *src, q_t *dst, int dst_list, bool fallback)
```

Defined at lines 703 to 733.

Evict a victim from a cache queue to the corresponding ghost list.

Moves victim's queue node from src to dst, sets ghost flag, and updates all ARC size counters and stats.

Parameters:
* `victim`: Directory entry to evict (must have valid qidx_node)
* `src`: Source cache queue (T1 or T2)
* `dst`: Destination ghost queue (B1 or B2)
* `dst_list`: ARC_B1 or ARC_B2 (determines which counters to update)
* `fallback`: true if this is a cross-queue fallback eviction (for logging)

Calls: [arc_ensure_ghost_capacity](dircache.c.md#arc_ensure_ghost_capacity), [queue_move_to_tail_of](../../libatalk/util/queue.c.md#queue_move_to_tail_of)

Called by: [arc_replace](dircache.c.md#arc_replace)

Uses file-scope variables: `arc_cache`

### arc_replace

```c
static int arc_replace(int in_b2)
```

Defined at lines 757 to 812.

ARC REPLACE subroutine (from paper Figure 1)

Evicts one entry from T1 or T2 and moves it to the corresponding ghost list (B1 or B2). Decision based on:

* Current size of T1 relative to target p
* Whether the incoming request is in B2

If the preferred queue only contains curdir (which must never be evicted), falls back to the other queue. Returns -1 only when both queues are exhausted, allowing the caller to proceed with a temporarily over-capacity cache that self-heals on the next eviction cycle.

From paper: "if |T1| ≥ 1 and ((x ∈ B2 and |T1| = p) or |T1| > p): Move LRU of T1 to MRU of B1, remove from cache else: Move LRU of T2 to MRU of B2, remove from cache"

Parameters:
* `in_b2`: 1 if incoming request is in B2, 0 otherwise

Returns: 0 on successful eviction, -1 if no entry could be evicted

Calls: [arc_evict_to_ghost](dircache.c.md#arc_evict_to_ghost), [arc_find_victim](dircache.c.md#arc_find_victim), [arc_verify_invariants](dircache.c.md#arc_verify_invariants)

Called by: [arc_case_ii_adapt_and_replace](dircache.c.md#arc_case_ii_adapt_and_replace), [arc_case_iii_adapt_and_replace](dircache.c.md#arc_case_iii_adapt_and_replace), [arc_case_iv_make_room](dircache.c.md#arc_case_iv_make_room)

Uses file-scope variables: `arc_cache`

### arc_case_i

```c
static void arc_case_i(struct dir *dir)
```

Defined at lines 824 to 870.

ARC Case I: Cache hit in T1 or T2.

From paper: "Move x to MRU of T2"

Promotes entry from T1 to T2 (frequency list) on second+ access, or moves within T2 if already frequent.

Parameters:
* `dir`: Directory entry in T1 or T2

Calls: [arc_verify_invariants](dircache.c.md#arc_verify_invariants), [queue_move_to_tail](../../libatalk/util/queue.c.md#queue_move_to_tail), [queue_move_to_tail_of](../../libatalk/util/queue.c.md#queue_move_to_tail_of)

Called by: [dircache_promote](dircache.c.md#dircache_promote), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `arc_cache`

Mentioned in the documentation of: [dircache_promote](dircache.c.md#dircache_promote)

### arc_case_ii_adapt_and_replace

```c
static void arc_case_ii_adapt_and_replace(struct dir *ghost)
```

Defined at lines 889 to 948.

ARC Case II: Ghost hit in B1.

From paper: "Adapt p = min(c, p + max(|B2|/|B1|, 1)) REPLACE(p) Move x to MRU of T2, load into cache"

B1 hit indicates we evicted this entry too soon from T1. Increase p to favor recency (grow T1 target size).

Ghost entries in this implementation are full struct dir with DIRF_ARC_GHOST flag. This function adapts p, calls REPLACE, and promotes ghost from B1 to T2 inline using [queue_move_to_tail_of()](../../libatalk/util/queue.c.md#queue_move_to_tail_of) for zero-allocation transition.

Parameters:
* `ghost`: Ghost entry (struct dir with DIRF_ARC_GHOST) in B1

Calls: [arc_replace](dircache.c.md#arc_replace), [arc_verify_invariants](dircache.c.md#arc_verify_invariants), [queue_move_to_tail_of](../../libatalk/util/queue.c.md#queue_move_to_tail_of)

Called by: [dircache_promote](dircache.c.md#dircache_promote), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `arc_cache`, `arc_promoting_ghost`

Mentioned in the documentation of: [dircache_promote](dircache.c.md#dircache_promote)

### arc_case_iii_adapt_and_replace

```c
static void arc_case_iii_adapt_and_replace(struct dir *ghost)
```

Defined at lines 967 to 1025.

ARC Case III: Ghost hit in B2.

From paper: "Adapt p = max(0, p - max(|B1|/|B2|, 1)) REPLACE(p) Move x to MRU of T2, load into cache"

B2 hit indicates we evicted this entry too soon from T2. Decrease p to favor frequency (grow T2 target size).

Ghost entries in this implementation are full struct dir with DIRF_ARC_GHOST flag. This function adapts p, calls REPLACE, and promotes ghost from B2 to T2 inline using [queue_move_to_tail_of()](../../libatalk/util/queue.c.md#queue_move_to_tail_of) for zero-allocation transition.

Parameters:
* `ghost`: Ghost entry (struct dir with DIRF_ARC_GHOST) in B2

Calls: [arc_replace](dircache.c.md#arc_replace), [arc_verify_invariants](dircache.c.md#arc_verify_invariants), [queue_move_to_tail_of](../../libatalk/util/queue.c.md#queue_move_to_tail_of)

Called by: [dircache_promote](dircache.c.md#dircache_promote), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `arc_cache`, `arc_promoting_ghost`

Mentioned in the documentation of: [dircache_promote](dircache.c.md#dircache_promote)

### arc_case_iv_make_room

```c
static void arc_case_iv_make_room(void)
```

Defined at lines 1042 to 1116.

ARC Case IV, eviction half: make room for a complete miss.

From paper: "case (i): |L1| = c if |T1| < c then delete LRU of B1, REPLACE(p) else delete LRU of T1, remove from cache case (ii): |L1| < c and |L1| + |L2| ≥ c if |L1| + |L2| = 2c then delete LRU of B2 REPLACE(p)"

Runs before the new entry's hash inserts: whenever the dircache holds 2c entries this deletes one from the hash, so [hash_insert()](hash.c.md#hash_insert) never sees a full table (its nodecount < maxcount assert holds).

Calls: [arc_find_victim](dircache.c.md#arc_find_victim), [arc_replace](dircache.c.md#arc_replace), [dequeue](../../libatalk/util/queue.c.md#dequeue), [dircache_defer_free](dircache.c.md#dircache_defer_free), [dircache_remove](dircache.c.md#dircache_remove), [queue_remove](../../libatalk/util/queue.c.md#queue_remove)

Called by: [dircache_add](dircache.c.md#dircache_add)

Uses file-scope variables: `arc_cache`

### arc_case_iv_insert

```c
static void arc_case_iv_insert(struct dir *dir)
```

Defined at lines 1123 to 1142.

ARC Case IV, insertion half: "Put x at MRU of T1, load into cache".

Parameters:
* `dir`: New directory entry, already in both hash indexes

Calls: [arc_verify_invariants](dircache.c.md#arc_verify_invariants), [enqueue](../../libatalk/util/queue.c.md#enqueue)

Called by: [dircache_add](dircache.c.md#dircache_add)

Uses file-scope variables: `arc_cache`, `queue_count_max`

### hash_didname

```c
static hash_val_t hash_didname(const void *p)
```

Defined at lines 1157 to 1205.

Called by: [dircache_init](dircache.c.md#dircache_init)

Uses file-scope variables: `p`

### hash_comp_didname

```c
static int hash_comp_didname(const void *k1, const void *k2)
```

Defined at lines 1207 to 1214.

Called by: [dircache_init](dircache.c.md#dircache_init)

### validation_freq

```c
static unsigned int validation_freq(void)
```

Defined at lines 1231 to 1238.

Validation frequency in effect.

Read live: the coherency defaults are applied per volume, after [dircache_init()](dircache.c.md#dircache_init). The single reader for the validation decision and the statistics, so both always report the same frequency.

Returns: configured validation frequency, never 0 (divisor)

Called by: [log_dircache_stat](dircache.c.md#log_dircache_stat), [should_validate_cache_entry](dircache.c.md#should_validate_cache_entry)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### should_validate_cache_entry

```c
static int should_validate_cache_entry(void)
```

Defined at lines 1250 to 1256.

Determine if cache entry should be validated against filesystem.

Uses probabilistic validation to reduce filesystem calls while still detecting external changes. Internal netatalk operations use explicit cache invalidation via [dir_remove()](directory.c.md#dir_remove) calls, so frequent validation is only needed to detect external filesystem changes.

Returns: 1 if validation should be performed, 0 otherwise

Calls: [validation_freq](dircache.c.md#validation_freq)

Called by: [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `validation_counter`

### dircache_evict

```c
static void dircache_evict(void)
```

Defined at lines 1269 to 1301.

Remove a fixed number of (oldest) entries from the cache and indexes.

The default is to remove the 256 oldest entries from the cache.

1. Get the oldest entry
1. If it's in use i.e. open forks reference it or it's curdir requeue it, don't remove it
1. Remove the dir from the main cache and the didname index
1. Free the struct dir structure and all its members

Calls: [dequeue](../../libatalk/util/queue.c.md#dequeue), [dircache_defer_free](dircache.c.md#dircache_defer_free), [dircache_dump](dircache.c.md#dircache_dump), [dircache_remove](dircache.c.md#dircache_remove), [enqueue](../../libatalk/util/queue.c.md#enqueue)

Called by: [dircache_add](dircache.c.md#dircache_add)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `dircache`, `index_queue`, `queue_count`

### gone_event_for

```c
static uint8_t gone_event_for(const struct dir *dir)
```

Defined at lines 1311 to 1316.

Called by: [validation_expunge_and_hint](dircache.c.md#validation_expunge_and_hint), [validation_refresh_and_hint](dircache.c.md#validation_refresh_and_hint)

### validation_expunge_and_hint

```c
static void validation_expunge_and_hint(const struct vol *vol, struct dir *cdir)
```

Defined at lines 1325 to 1335.

Validation discovered the entry gone: expunge + hint siblings.

One pipe message replaces each sibling re-discovering the change by its own full-path stat. The hint reports evidence (gone); a same-named recreate is harmless — hints only invalidate, receivers rebuild.

Calls: [dir_remove](directory.c.md#dir_remove), [gone_event_for](dircache.c.md#gone_event_for), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint)

Called by: [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### validation_refresh_and_hint

```c
static int validation_refresh_and_hint(const struct vol *vol, struct dir *cdir, struct stat *st)
```

Defined at lines 1350 to 1383.

Validation saw an ino/ctime change: refresh in-place + hint.

Hints: an inode change (object replaced) broadcasts REFRESH under the entry's current DID and, when dir_modify re-keyed it, under the old DID too — siblings cache the object under the old key. A ctime-only change sends nothing: fork writes advance ctime on every flush, and broadcasting those would rebuild sibling AD/rfork caches per write; pure metadata changes propagate via each sibling's own validation.

Returns: 0 if the entry survived (refreshed in-place), -1 if dir_modify evicted it (inode change + CNID miss) — hint sent either way.

Calls: [dir_modify](directory.c.md#dir_modify), [gone_event_for](dircache.c.md#gone_event_for), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint)

Called by: [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### dircache_lookup_parent

```c
struct dir * dircache_lookup_parent(const struct vol *vol, cnid_t did)
```

Defined at lines 1393 to 1417.

Plain index probe for pfd_cache parent resolution.

Never validates, never mutates, never promotes. Filters ARC ghosts (their identity fields are frozen and unvalidated — treating one as a live parent would let the pfd sync-check pass against stale identity) and file entries (a parent must be a directory).

Calls: [hash_lookup](hash.c.md#hash_lookup), [hnode_get](hash.c.md#hnode_get)

Called by: [pfd_ostat](pfd_cache.c.md#pfd_ostat)

Uses file-scope variables: `dircache`

### dircache_search_by_did

```c
struct dir * dircache_search_by_did(const struct vol *vol, cnid_t cnid)
```

Defined at lines 1437 to 1570.

Search the dircache via a CNID for a directory.

Found cache entries are expunged if both the parent directory st_ctime and the objects st_ctime are modified. This func builds on the fact, that all our code only ever needs to and does search the dircache by CNID expecting directories to be returned, but not files. Thus (1) if we find a file for a given CNID we (1a) remove it from the cache (1b) return NULL indicating nothing found (2) we can then use d_fullpath to stat the directory

Parameters:
* `vol`: pointer to struct vol
* `cnid`: CNID of the directory to search

Returns: Pointer to struct dir if found, else NULL

Calls: [arc_case_i](dircache.c.md#arc_case_i), [arc_case_ii_adapt_and_replace](dircache.c.md#arc_case_ii_adapt_and_replace), [arc_case_iii_adapt_and_replace](dircache.c.md#arc_case_iii_adapt_and_replace), [dir_remove](directory.c.md#dir_remove), [hash_lookup](hash.c.md#hash_lookup), [hnode_get](hash.c.md#hnode_get), [pfd_ostat](pfd_cache.c.md#pfd_ostat), [should_validate_cache_entry](dircache.c.md#should_validate_cache_entry), [validation_expunge_and_hint](dircache.c.md#validation_expunge_and_hint), [validation_refresh_and_hint](dircache.c.md#validation_refresh_and_hint)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [dirlookup_internal](directory.c.md#dirlookup_internal), [dirlookup_internal_retry](directory.c.md#dirlookup_internal_retry)

Uses file-scope variables: `arc_cache`, `dircache`

Mentioned in the documentation of: [process_cache_hints](dircache.c.md#process_cache_hints)

### dircache_search_by_name

```c
struct dir * dircache_search_by_name(const struct vol *vol, const struct dir *dir, char *name, size_t len)
```

Defined at lines 1585 to 1700.

Search the cache via did/name hashtable.

Found cache entries are expunged if both the parent directory st_ctime and the objects st_ctime are modified.

Parameters:
* `vol`: volume
* `dir`: directory
* `name`: name (server side encoding)
* `len`: strlen of name

Returns: pointer to struct dir if found in cache, else NULL

Calls: [arc_case_i](dircache.c.md#arc_case_i), [arc_case_ii_adapt_and_replace](dircache.c.md#arc_case_ii_adapt_and_replace), [arc_case_iii_adapt_and_replace](dircache.c.md#arc_case_iii_adapt_and_replace), [hash_lookup](hash.c.md#hash_lookup), [hnode_get](hash.c.md#hnode_get), [pfd_ostat](pfd_cache.c.md#pfd_ostat), [should_validate_cache_entry](dircache.c.md#should_validate_cache_entry), [validation_expunge_and_hint](dircache.c.md#validation_expunge_and_hint), [validation_refresh_and_hint](dircache.c.md#validation_refresh_and_hint)

Called by: [ad_addcomment](desktop.c.md#ad_addcomment), [ad_getcomment](desktop.c.md#ad_getcomment), [ad_rmvcomment](desktop.c.md#ad_rmvcomment), [adl_lkup](catsearch.c.md#adl_lkup), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [afp_exchangefiles](file.c.md#afp_exchangefiles), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setacl](acls.c.md#afp_setacl), [afp_setextattr](extattrs.c.md#afp_setextattr), [afp_setfilparams](file.c.md#afp_setfilparams), [catsearch](catsearch.c.md#catsearch), [cname](directory.c.md#cname), [deletefile](file.c.md#deletefile), [dir_add](directory.c.md#dir_add), [enumerate](enumerate.c.md#enumerate), [moveandrename](filedir.c.md#moveandrename), [of_closefork](ofork.c.md#of_closefork), [path_resolve_cached_file](file.c.md#path_resolve_cached_file), [read_fork](fork.c.md#read_fork), [rfork_invalidate_for_ofork](fork.c.md#rfork_invalidate_for_ofork), [setfilparams](file.c.md#setfilparams)

Uses file-scope variables: `arc_cache`, `index_didname`

### dircache_expunge_duplicate

```c
static void dircache_expunge_duplicate(const struct vol *vol, struct dir *dup, int *took_curdir)
```

Defined at lines 1713 to 1730.

Retire a cached entry that a pending insert is about to supersede.

Not [dir_remove()](directory.c.md#dir_remove): its curdir recovery calls [dirlookup()](directory.c.md#dirlookup), which re-enters [dircache_add()](dircache.c.md#dircache_add) and re-publishes the key the caller is clearing. curdir is handed back instead, for the caller to re-point once its entry is published.

Parameters:
* `vol`: Volume the entry belongs to
* `dup`: Entry to retire
* `took_curdir`: set if the retired entry was curdir

Calls: [dircache_defer_free](dircache.c.md#dircache_defer_free), [dircache_remove](dircache.c.md#dircache_remove)

Called by: [dircache_add](dircache.c.md#dircache_add), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### dircache_add

```c
int dircache_add(const struct vol *vol, struct dir *dir)
```

Defined at lines 1743 to 1873.

create struct dir from struct path

Add a struct dir to the cache and its indexes. Supports both LRU mode (legacy) and ARC mode.

Parameters:
* `vol`: pointer to volume
* `dir`: pointer to parent directory

Returns: 0; a failed hash insert aborts the process

Calls: [arc_case_iv_insert](dircache.c.md#arc_case_iv_insert), [arc_case_iv_make_room](dircache.c.md#arc_case_iv_make_room), [dircache_dump](dircache.c.md#dircache_dump), [dircache_evict](dircache.c.md#dircache_evict), [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [enqueue](../../libatalk/util/queue.c.md#enqueue), [hash_alloc_insert_node](hash.c.md#hash_alloc_insert_node), [hash_delete_free](hash.c.md#hash_delete_free), [hash_lookup](hash.c.md#hash_lookup), [hnode_get](hash.c.md#hnode_get)

Called by: [afp_createfile](file.c.md#afp_createfile), [dir_add](directory.c.md#dir_add), [dir_modify](directory.c.md#dir_modify), [dirlookup_internal](directory.c.md#dirlookup_internal), [getmetadata](file.c.md#getmetadata)

Uses file-scope variables: `arc_cache`, `curdir` in [etc/afpd/directory.c](directory.c.md), `dircache`, `dircache_maxsize`, `index_didname`, `index_queue`, `queue_count`, `queue_count_max`, `rootParent` in [etc/afpd/directory.c](directory.c.md), `vid_entry_count`

Mentioned in the documentation of: [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate)

### dircache_mismatch_panic

```c
static void dircache_mismatch_panic(const char *index_name, const struct dir *dir, const void *node, const struct dir *found)
```

Defined at lines 1882 to 1896.

A stored index node does not name its entry: unrecoverable.

Every node is created with its entry as data and freed only through the entry's own removal, so a mismatch means memory corruption or a foreign delete. Freeing what a table still points at cannot be survived.

Calls: [dircache_dump](dircache.c.md#dircache_dump)

Called by: [dircache_remove](dircache.c.md#dircache_remove)

### dircache_remove

```c
void dircache_remove(const struct vol *vol, struct dir *dir, int flags)
```

Defined at lines 1909 to 2029.

Remove an entry from the dircache.

Deletes the entry's own stored nodes, so removal can never cost another entry its node. A NULL node means the entry is not in that index and it is skipped; a node naming a different entry panics before anything is mutated or followed.

Callers outside of [dircache.c](dircache.c.md) should call this with flags = QUEUE_INDEX | DIDNAME_INDEX | DIRCACHE.

Calls: [dircache_mismatch_panic](dircache.c.md#dircache_mismatch_panic), [hash_delete_free](hash.c.md#hash_delete_free), [hash_scan_delfree](hash.c.md#hash_scan_delfree), [hnode_get](hash.c.md#hnode_get), [pfd_purge](pfd_cache.c.md#pfd_purge), [queue_remove](../../libatalk/util/queue.c.md#queue_remove)

Called by: [arc_case_iv_make_room](dircache.c.md#arc_case_iv_make_room), [arc_ensure_ghost_capacity](dircache.c.md#arc_ensure_ghost_capacity), [dir_modify](directory.c.md#dir_modify), [dir_remove](directory.c.md#dir_remove), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [dircache_evict](dircache.c.md#dircache_evict), [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_remove_children](dircache.c.md#dircache_remove_children)

Uses file-scope variables: `arc_cache`, `dircache`, `index_didname`, `queue_count`, `vid_entry_count`

Mentioned in the documentation of: [arc_ensure_ghost_capacity](dircache.c.md#arc_ensure_ghost_capacity)

### dircache_remove_children

```c
int dircache_remove_children(const struct vol *vol, struct dir *dir)
```

Defined at lines 2051 to 2127.

Remove all child entries of a directory from the dircache.

When a directory is renamed or moved, the full paths stored in the dircache become invalid for all child entries of the renamed dir. This function prunes orphaned child dircache entries of given dir. CNID entries use parent DIDs and name, and requre recursion to get the full path, therefore parent changes do not invalidate the CNIDs.

Removes as it scans: DIRCACHE_NOSHRINK deletes the entry's own just-returned node without rehashing the table, and the free is deferred, so nothing the scan is walking moves. [dir_remove()](directory.c.md#dir_remove) cannot be used here — its curdir recovery calls [dirlookup()](directory.c.md#dirlookup), and an insert mid-scan can grow the table. A removed curdir is re-resolved once the scan is over instead: a rename keeps the CNID, so the lookup rebuilds the entry on the new path.

Parameters:
* `vol`: volume
* `dir`: parent directory whose children should be removed

Returns: 0 on success, -1 if a removed curdir could not be re-resolved

Calls: [dircache_defer_free](dircache.c.md#dircache_defer_free), [dircache_remove](dircache.c.md#dircache_remove), [dirlookup](directory.c.md#dirlookup), [hash_scan_begin](hash.c.md#hash_scan_begin), [hash_scan_next](hash.c.md#hash_scan_next), [hnode_get](hash.c.md#hnode_get)

Called by: [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `dircache`, `rootParent` in [etc/afpd/directory.c](directory.c.md)

Mentioned in the documentation of: [process_cache_hints](dircache.c.md#process_cache_hints)

### dircache_reindex_didname

```c
int dircache_reindex_didname(const struct vol *vol, struct dir *dir)
```

Defined at lines 2141 to 2171.

Re-insert entry into DID/name index after key change.

Called by [dir_modify()](directory.c.md#dir_modify) after updating d_pdid / d_u_name. The caller MUST have already removed the entry from the DIDNAME_INDEX via dircache_remove(vol, dir, DIDNAME_INDEX) before calling this.

Parameters:
* `vol`: Volume (required)
* `dir`: Entry with updated d_pdid / d_u_name (required)

Returns: 0 on success, -1 on hash insert failure

Calls: [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [hash_alloc_insert_node](hash.c.md#hash_alloc_insert_node), [hash_lookup](hash.c.md#hash_lookup), [hnode_get](hash.c.md#hnode_get)

Called by: [dir_modify](directory.c.md#dir_modify)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `index_didname`, `rootParent` in [etc/afpd/directory.c](directory.c.md)

### dircache_promote

```c
void dircache_promote(struct dir *dir)
```

Defined at lines 2189 to 2217.

Promote a cache entry to signal recency.

Dispatches based on cache mode:

ARC mode (arc_list membership): T1/T2: [arc_case_i()](dircache.c.md#arc_case_i) — move to MRU of T2 B1: [arc_case_ii_adapt_and_replace()](dircache.c.md#arc_case_ii_adapt_and_replace) — promote ghost to T2 B2: [arc_case_iii_adapt_and_replace()](dircache.c.md#arc_case_iii_adapt_and_replace) — promote ghost to T2

LRU mode: Move entry to MRU position of the LRU queue, so recently accessed entries are evicted last. Prevents actively-used entries from being evicted.

Parameters:
* `dir`: Cache entry to promote (required)

Calls: [arc_case_i](dircache.c.md#arc_case_i), [arc_case_ii_adapt_and_replace](dircache.c.md#arc_case_ii_adapt_and_replace), [arc_case_iii_adapt_and_replace](dircache.c.md#arc_case_iii_adapt_and_replace), [queue_move_to_tail](../../libatalk/util/queue.c.md#queue_move_to_tail)

Called by: [dir_modify](directory.c.md#dir_modify)

Uses file-scope variables: `arc_cache`, `index_queue`

### dircache_resolve_size

```c
unsigned int dircache_resolve_size(int reqsize)
```

Defined at lines 2230 to 2257.

Resolve a requested dircache size to the effective maximum.

Unset or below-minimum sizes use DEFAULT_DIRCACHE_SIZE, in-range sizes round up to the next power of two, oversize requests clamp to MAX_DIRCACHE_SIZE.

Parameters:
* `reqsize`: requested maximum size from afp.conf

Returns: the effective maximum cache size

Called by: [dircache_init](dircache.c.md#dircache_init)

### dircache_init

```c
int dircache_init(int reqsize)
```

Defined at lines 2274 to 2365.

Initialize the dircache and indexes.

This is called in child afpd initialization. Unset or below-minimum sizes use DEFAULT_DIRCACHE_SIZE, in-range sizes round up to the next power of two, oversize requests clamp to MAX_DIRCACHE_SIZE. It initializes a hashtable which we use to store a directory cache in. It also initializes two indexes:

* a DID/name index on the main dircache
* a queue index on the dircache (LRU mode) or four queues (ARC mode)

Parameters:
* `reqsize`: requested maximum size from afp.conf

Returns: 0 on success, -1 on error

Calls: [arc_destroy](dircache.c.md#arc_destroy), [arc_init](dircache.c.md#arc_init), [dircache_reset_validation_counter](dircache.c.md#dircache_reset_validation_counter), [dircache_resolve_size](dircache.c.md#dircache_resolve_size), [hash_comp_didname](dircache.c.md#hash_comp_didname), [hash_comp_vid_did](dircache.c.md#hash_comp_vid_did), [hash_create](hash.c.md#hash_create), [hash_didname](dircache.c.md#hash_didname), [hash_size](hash.c.md#hash_size), [hash_vid_did](dircache.c.md#hash_vid_did), [queue_init](../../libatalk/util/queue.c.md#queue_init)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp), [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `arc_cache`, `dircache`, `dircache_maxsize`, `index_didname`, `index_queue`, `invalid_dircache_entries` in [etc/afpd/directory.c](directory.c.md), `queue_count`, `rfork_cache_budget`, `rfork_lru`, `rfork_max_entry_size`, `rootParent` in [etc/afpd/directory.c](directory.c.md)

Mentioned in the documentation of: [validation_freq](dircache.c.md#validation_freq)

### log_dircache_stat

```c
void log_dircache_stat(void)
```

Defined at lines 2375 to 2584.

Log dircache statistics.

Includes hit ratio percentage for monitoring cache effectiveness, validation-specific metrics to monitor performance impact of the optimization changes, and username for tracking per-user stats. Shows both expunged (caught by validation) and invalid_on_use (missed by validation).

Calls: [ipc_get_hints_dropped](../../libatalk/util/server_ipc.c.md#ipc_get_hints_dropped), [ipc_get_hints_sent](../../libatalk/util/server_ipc.c.md#ipc_get_hints_sent), [validation_freq](dircache.c.md#validation_freq)

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `ad_cache_hits` in [etc/afpd/ad_cache.c](ad_cache.c.md), `ad_cache_misses` in [etc/afpd/ad_cache.c](ad_cache.c.md), `ad_cache_no_ad` in [etc/afpd/ad_cache.c](ad_cache.c.md), `arc_cache`, `cache_hint_stat`, `dircache_maxsize`, `queue_count`, `queue_count_max`, `rfork_cache_budget`, `rfork_lru_count`, `rfork_stat_added`, `rfork_stat_evicted`, `rfork_stat_hits`, `rfork_stat_invalidated`, `rfork_stat_lookups`, `rfork_stat_misses`, `rfork_stat_used_max`, `validation_counter`

Mentioned in the documentation of: [dircache_rfork_shutdown](dircache.c.md#dircache_rfork_shutdown)

### dircache_rfork_shutdown

```c
void dircache_rfork_shutdown(void)
```

Defined at lines 2592 to 2612.

Shutdown the rfork cache — free remaining RFork LRU nodes and sentinel.

Called from the child shutdown path after [log_dircache_stat()](dircache.c.md#log_dircache_stat). Frees the qnodes (rfork data buffers are reclaimed by exit()) and the LRU sentinel.

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

Uses file-scope variables: `rfork_lru`, `rfork_lru_count`

### dircache_dump

```c
void dircache_dump(void)
```

Defined at lines 2617 to 2823.

Dump dircache to /tmp/dircache.PID.

Calls: [hash_scan_begin](hash.c.md#hash_scan_begin), [hash_scan_next](hash.c.md#hash_scan_next), [hnode_get](hash.c.md#hnode_get), [tmpdir](../../libatalk/util/unix.c.md#tmpdir)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [dircache_add](dircache.c.md#dircache_add), [dircache_evict](dircache.c.md#dircache_evict), [dircache_mismatch_panic](dircache.c.md#dircache_mismatch_panic)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `arc_cache`, `dircache`, `dircache_maxsize`, `index_didname`, `index_queue`, `queue_count`, `rfork_cache_budget`, `rfork_cache_used`, `rfork_lru_count`, `rfork_max_entry_size`

### dircache_reset_validation_counter

```c
void dircache_reset_validation_counter(void)
```

Defined at lines 2831 to 2836.

Reset validation counter for consistent testing.

Resets the global validation counter to ensure predictable validation patterns between test runs or configuration changes.

Called by: [dircache_init](dircache.c.md#dircache_init)

Uses file-scope variables: `validation_counter`

### process_cache_hints

```c
void process_cache_hints(AFPObj *obj)
```

Defined at lines 2862 to 3052.

Process cross-process dircache invalidation hints.

Called from the [DSI](../../include/atalk/dsi.h.md#struct-dsi) command loop after each AFP command completes. Uses direct [hash_lookup()](hash.c.md#hash_lookup) on the CNID hash table to support BOTH files (DIRF_ISFILE) and directories — [dircache_search_by_did()](dircache.c.md#dircache_search_by_did) is unsuitable because it actively removes file entries.

Dispatches on hint type:

* CACHE_HINT_REFRESH: [ostat()](../../libatalk/util/unix.c.md#ostat) + [dir_modify()](directory.c.md#dir_modify) — stat refresh, AD invalidation
* CACHE_HINT_DELETE: direct [dir_remove()](directory.c.md#dir_remove) — no ostat needed
* CACHE_HINT_DELETE_CHILDREN: [dircache_remove_children()](dircache.c.md#dircache_remove_children) + parent cleanup

Calls: [cnid_volume_tag](volume.c.md#cnid_volume_tag), [dir_modify](directory.c.md#dir_modify), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [hash_lookup](hash.c.md#hash_lookup), [hnode_get](hash.c.md#hnode_get), [ostat](../../libatalk/util/unix.c.md#ostat), [pfd_purge](pfd_cache.c.md#pfd_purge)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp), [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `cache_hint_stat`, `dircache`

### dircache_report_invalid_entry

```c
void dircache_report_invalid_entry(struct dir *dir)
```

Defined at lines 3064 to 3073.

Report that a cache entry was invalid when actually used.

This function should be called when a cached directory entry that was returned without validation (for performance) turns out to be invalid when actually accessed (e.g., file doesn't exist, has been modified, etc). This helps track the effectiveness of the validation frequency setting.

Parameters:
* `dir`: The directory entry that was found to be invalid

Called by: [dir_remove](directory.c.md#dir_remove)

### dircache_remove_children_defer

```c
void dircache_remove_children_defer(const struct vol *vol, struct dir *dir)
```

Defined at lines 3086 to 3158.

Enqueue a deferred dircache_remove_children operation.

O(1) enqueue — the idle worker performs the actual hash scan during poll() idle periods. Falls back to synchronous removal if the worker is not active or the queue is full.

Calls: [dircache_remove_children](dircache.c.md#dircache_remove_children), [iw_is_active](idle_worker.c.md#iw_is_active), [iw_note_work](idle_worker.c.md#iw_note_work)

Called by: [afp_delete](filedir.c.md#afp_delete), [afp_openfork](fork.c.md#afp_openfork), [deletecurdir](directory.c.md#deletecurdir), [dirlookup_internal_retry](directory.c.md#dirlookup_internal_retry), [dirlookup_strict](directory.c.md#dirlookup_strict), [moveandrename](filedir.c.md#moveandrename), [movecwd](directory.c.md#movecwd), [process_cache_hints](dircache.c.md#process_cache_hints)

Uses file-scope variables: `deferred_count`, `deferred_depth_max`, `deferred_depth_min`, `deferred_queue`, `deferred_seq`, `deferred_tail`

### dircache_flush_deferred_for_vol

```c
void dircache_flush_deferred_for_vol(uint16_t vid)
```

Defined at lines 3170 to 3233.

Process deferred cleanup entries for a closing volume synchronously.

Precondition: : Worker is dormant (called during AFP command processing). Called from [afp_closevol()](volume.c.md#afp_closevol) before volume structures are freed. Drops the preselected active-job cursor if it points at this volume.

Calls: [deferred_compact_head](dircache.c.md#deferred_compact_head), [deferred_job_kill](dircache.c.md#deferred_job_kill), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [hash_scan_begin](hash.c.md#hash_scan_begin), [hash_scan_next](hash.c.md#hash_scan_next), [hnode_get](hash.c.md#hnode_get)

Called by: [afp_closevol](volume.c.md#afp_closevol)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `deferred_active`, `deferred_batch_active`, `deferred_batch_n`, `deferred_count`, `deferred_cover_len`, `deferred_cover_path`, `deferred_cover_vid`, `deferred_head`, `deferred_queue`, `dircache`

Mentioned in the documentation of: [dir_remove_and_free](directory.c.md#dir_remove_and_free)

### dircache_has_deferred_work

```c
int dircache_has_deferred_work(void)
```

Defined at lines 3236 to 3239.

Uses file-scope variables: `deferred_count`

### deferred_job_kill

```c
static void deferred_job_kill(int idx)
```

Defined at lines 3244 to 3249.

Free a job slot without completing it (volume gone / covered).

Called by: [deferred_batch_complete](dircache.c.md#deferred_batch_complete), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_queue`

### deferred_compact_head

```c
static void deferred_compact_head(void)
```

Defined at lines 3254 to 3262.

Advance deferred_head past dead slots, keeping count consistent.

Called by: [deferred_batch_complete](dircache.c.md#deferred_batch_complete), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_count`, `deferred_head`, `deferred_queue`

### path_is_strict_child

```c
static int path_is_strict_child(const char *path, size_t len, const char *parent_path, size_t parent_len)
```

Defined at lines 3271 to 3277.

Test path strictly below parent: byte-prefix at a '/' boundary.

The boundary byte check rejects sibling prefixes ("/A/BC" under "/A/B").

Returns: 1 if path is a strict descendant of parent_path, else 0

Called by: [deferred_cover_drops](dircache.c.md#deferred_cover_drops), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

### deferred_cover_set

```c
static void deferred_cover_set(const struct deferred_cleanup *done, unsigned int scan_seq)
```

Defined at lines 3282 to 3298.

Remember a completed scan as the covering memo.

Called by: [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_cover_len`, `deferred_cover_path`, `deferred_cover_seq`, `deferred_cover_vid`

### deferred_cover_drops

```c
static int deferred_cover_drops(const struct deferred_cleanup *j)
```

Defined at lines 3311 to 3322.

Test whether a popped job is covered by the memoized scan.

Covered iff enqueued before that scan started (serial-number compare: the unsigned seq difference exceeds half the counter space iff the job predates the scan, fully defined across wraparound), same volume, and its byte path equals or sits strictly below the memoized path at a '/' boundary.

Returns: 1 if the job's entries were already purged, else 0

Calls: [path_is_strict_child](dircache.c.md#path_is_strict_child)

Called by: [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_cover_len`, `deferred_cover_path`, `deferred_cover_seq`, `deferred_cover_vid`

### deferred_compact_tail

```c
static void deferred_compact_tail(void)
```

Defined at lines 3330 to 3349.

Compact deferred_tail backwards past dead slots.

LIFO consumption retires slots at the tail; head compaction still handles slots killed by the volume flush.

Called by: [deferred_batch_complete](dircache.c.md#deferred_batch_complete), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_count`, `deferred_depth_max`, `deferred_depth_min`, `deferred_queue`, `deferred_tail`

### deferred_batch_cmp

```c
static int deferred_batch_cmp(const void *a, const void *b)
```

Defined at lines 3354 to 3373.

qsort comparator for batch members: vid, then path bytes.

Called by: [deferred_batch_start](dircache.c.md#deferred_batch_start)

Uses file-scope variables: `c`, `deferred_queue`

### deferred_batch_match

```c
static int deferred_batch_match(uint16_t vid, const char *ep, size_t elen)
```

Defined at lines 3384 to 3438.

Test whether an entry path is covered by a batch member.

The candidate ancestor is unique at uniform depth: the entry path truncated at its (batch_depth+1)-th slash. Binary search over the sorted member index; byte comparisons only.

Returns: member index-array position, or -1

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain)

Uses file-scope variables: `c`, `deferred_batch_depth`, `deferred_batch_idx`, `deferred_batch_n`, `deferred_queue`

### deferred_batch_start

```c
static int deferred_batch_start(void)
```

Defined at lines 3446 to 3469.

Start a flat batch scan over every live pending job.

Precondition: deferred_depth_min == deferred_depth_max and no batch is active.

Returns: 1 if the batch was armed, 0 if no live members exist

Calls: [deferred_batch_cmp](dircache.c.md#deferred_batch_cmp)

Called by: [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_batch_active`, `deferred_batch_chain_idx`, `deferred_batch_depth`, `deferred_batch_idx`, `deferred_batch_n`, `deferred_batch_seq`, `deferred_count`, `deferred_depth_min`, `deferred_head`, `deferred_queue`, `deferred_seq`

### deferred_batch_complete

```c
static void deferred_batch_complete(void)
```

Defined at lines 3477 to 3494.

Finish the flat batch scan: retire every pre-batch member.

Mid-batch enqueues (seq >= batch seq) stay pending — their entries may sit in chains the batch already passed.

Calls: [deferred_compact_head](dircache.c.md#deferred_compact_head), [deferred_compact_tail](dircache.c.md#deferred_compact_tail), [deferred_job_kill](dircache.c.md#deferred_job_kill)

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain)

Uses file-scope variables: `deferred_batch_active`, `deferred_batch_idx`, `deferred_batch_n`, `deferred_batch_seq`, `deferred_queue`

### deferred_batch_chain

```c
static int deferred_batch_chain(void)
```

Defined at lines 3502 to 3566.

Scan one hash chain for the flat batch: purge every entry whose unique candidate ancestor is a batch member.

Returns: 1 if more chains remain, 0 when the batch completed

Calls: [deferred_batch_complete](dircache.c.md#deferred_batch_complete), [deferred_batch_match](dircache.c.md#deferred_batch_match), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [hash_chain_head](dircache.c.md#hash_chain_head), [hash_size](hash.c.md#hash_size), [hnode_get](hash.c.md#hnode_get), [iw_grant_active](idle_worker.c.md#iw_grant_active)

Called by: [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `deferred_batch_chain_idx`, `deferred_count`, `dircache`

### dircache_process_deferred_chain

```c
int dircache_process_deferred_chain(void)
```

Defined at lines 3581 to 3711.

Process one unit of deferred cleanup.

Called by idle worker under a validated grant. Jobs are consumed LIFO: AFP deletes bottom-up, so a tree's covering root-most job arrives last and its single scan purges the whole subtree; the covering memo then drops the tree's remaining jobs in O(1) as they pop. When every pending job sits at one depth, one merged batch scan serves all of them. Every removal re-checks iw_can_work; scans resume via their chain cursors across granted cycles.

Returns: 1 if more work remains, 0 if all deferred work is complete

Calls: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [deferred_batch_start](dircache.c.md#deferred_batch_start), [deferred_compact_head](dircache.c.md#deferred_compact_head), [deferred_compact_tail](dircache.c.md#deferred_compact_tail), [deferred_cover_drops](dircache.c.md#deferred_cover_drops), [deferred_cover_set](dircache.c.md#deferred_cover_set), [deferred_job_kill](dircache.c.md#deferred_job_kill), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [hash_chain_head](dircache.c.md#hash_chain_head), [hash_size](hash.c.md#hash_size), [hnode_get](hash.c.md#hnode_get), [iw_grant_active](idle_worker.c.md#iw_grant_active), [path_is_strict_child](dircache.c.md#path_is_strict_child)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `deferred_active`, `deferred_batch_active`, `deferred_count`, `deferred_depth_max`, `deferred_depth_min`, `deferred_queue`, `deferred_scan_started_seq`, `deferred_seq`, `deferred_tail`, `dircache`

# Types

### struct deferred_cleanup

Defined at line 144.
* `char * parent_path`
* `size_t parent_len`
* `uint16_t v_vid`
* `hashcount_t chain_idx`
* `unsigned int seq`
* `unsigned int depth`

### struct dircache_stat

Defined at line 208.
* `unsigned long long lookups`
* `unsigned long long hits`
* `unsigned long long ghost_hits`
* `unsigned long long misses`
* `unsigned long long added`
* `unsigned long long removed`
* `unsigned long long expunged`
* `unsigned long long evicted`
* `unsigned long long invalid_on_use`
* `unsigned long long covered_cancelled`

# Typedefs and enums

* `enum arc_list_t`: `ARC_NONE`, `ARC_T1`, `ARC_T2`, `ARC_B1`, `ARC_B2`

# Macros

* Undocumented: `HINT_READ_BUF_SIZE`, `MAX_DEFERRED_CLEANUPS`, `MAX_HINTS_PER_CYCLE`, `get16bits`

# File-scope variables

`adaptations`, `arc_cache`, `arc_promoting_ghost`, `b1`, `b1_size`, `b2`, `b2_size`, `c`, `cache_hint_stat`, `deferred_active`, `deferred_batch_active`, `deferred_batch_chain_idx`, `deferred_batch_depth`, `deferred_batch_idx`, `deferred_batch_n`, `deferred_batch_seq`, `deferred_count`, `deferred_cover_len`, `deferred_cover_path`, `deferred_cover_seq`, `deferred_cover_vid`, `deferred_depth_max`, `deferred_depth_min`, `deferred_head`, `deferred_queue`, `deferred_scan_started_seq`, `deferred_seq`, `deferred_tail`, `dircache`, `dircache_maxsize`, `dircache_stat`, `enabled`, `evictions_t1`, `evictions_t2`, `ghost_hits_b1`, `ghost_hits_b2`, `hints_acted_on`, `hints_no_match`, `hints_received`, `hits_t1`, `hits_t2`, `index_didname`, `index_queue`, `p`, `p_decreases`, `p_increases`, `p_max`, `p_min`, `promotions_t1_to_t2`, `queue_count`, `queue_count_max`, `rfork_cache_budget`, `rfork_cache_used`, `rfork_lru`, `rfork_lru_count`, `rfork_max_entry_size`, `rfork_stat_added`, `rfork_stat_evicted`, `rfork_stat_hits`, `rfork_stat_invalidated`, `rfork_stat_lookups`, `rfork_stat_misses`, `rfork_stat_used_max`, `stats`, `t1`, `t1_size`, `t2`, `t2_size`, `validation_counter`, `vid_entry_count`
