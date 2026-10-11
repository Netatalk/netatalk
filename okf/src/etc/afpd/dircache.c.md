---
type: C Source File
title: "etc/afpd/dircache.c"
description: "Directory Cache."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/dircache.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 140 to 143.

First node of one chain of a hash table.

The deferred child scans walk the dircache one chain at a time with it.

Parameters:
* `hash`: hash table
* `chain`: chain index

Returns: the first node, or NULL for an empty chain or an index out of range

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

### hash_vid_did

```c
static hash_val_t hash_vid_did(const void *key)
```

Defined at lines 353 to 370.

FNV-1a hash of the volume ID and directory ID of a dircache entry.

Parameters:
* `key`: struct dir whose d_vid and d_did to hash

Returns: the hash value

Called by: [dircache_init](dircache.c.md#dircache_init)

### hash_comp_vid_did

```c
static int hash_comp_vid_did(const void *key1, const void *key2)
```

Defined at lines 379 to 384.

Compare two dircache keys by volume ID and directory ID.

Parameters:
* `key1`: first struct dir
* `key2`: second struct dir

Returns: 0 when both keys carry the same d_vid and d_did, 1 otherwise

Called by: [dircache_init](dircache.c.md#dircache_init)

### arc_init

```c
static int arc_init(size_t cache_size)
```

Defined at lines 396 to 423.

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

Defined at lines 428 to 461.

Destroy ARC cache structures.

Calls: [queue_destroy](../../libatalk/util/queue.c.md#queue_destroy)

Called by: [dircache_init](dircache.c.md#dircache_init)

Uses file-scope variables: `arc_cache`

### arc_verify_invariants

```c
static void arc_verify_invariants(void)
```

Defined at lines 469 to 493.

Verify ARC invariants (debug mode)

Checks that ARC list sizes satisfy all constraints from the paper. Called after each operation in debug builds.

Called by: [arc_case_i](dircache.c.md#arc_case_i), [arc_case_ii_adapt_and_replace](dircache.c.md#arc_case_ii_adapt_and_replace), [arc_case_iii_adapt_and_replace](dircache.c.md#arc_case_iii_adapt_and_replace), [arc_case_iv_insert](dircache.c.md#arc_case_iv_insert), [arc_replace](dircache.c.md#arc_replace)

Uses file-scope variables: `arc_cache`, `arc_promoting_ghost`

### dircache_purge_vol

```c
void dircache_purge_vol(const struct vol *vol)
```

Defined at lines 511 to 552.

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

Defined at lines 568 to 579.

Queue a dircache entry that left every index for a later free.

The caller first removes the entry from both hash indexes and every ARC and LRU queue. The entry gets CNID_INVALID as its DID, so pointers that this request handed out earlier stay valid and their holders see the entry as stale. ARC ghost eviction can reach entries returned during the same request, so an immediate free could leave a dangling pointer. dir_free_invalid_q frees the queue at the end of every AFP request, and the idle worker drains it while afpd waits in poll. When enqueue cannot allocate a queue node, the entry leaks, as in dir_remove.

Parameters:
* `dir`: entry to retire

Calls: [enqueue](../../libatalk/util/queue.c.md#enqueue), [iw_note_work](idle_worker.c.md#iw_note_work)

Called by: [arc_case_iv_make_room](dircache.c.md#arc_case_iv_make_room), [arc_ensure_ghost_capacity](dircache.c.md#arc_ensure_ghost_capacity), [dircache_evict](dircache.c.md#dircache_evict), [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_remove_children](dircache.c.md#dircache_remove_children)

Uses file-scope variables: `invalid_dircache_entries` in [etc/afpd/directory.c](directory.c.md)

### arc_ghost_trim_candidate

```c
qnode_t * arc_ghost_trim_candidate(q_t *q, const struct dir *skip)
```

Defined at lines 593 to 610.

Pick the ghost to release from a list at capacity.

Non-static so the unit tests can drive selection directly.

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

Defined at lines 630 to 670.

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

Defined at lines 684 to 711.

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

Defined at lines 725 to 755.

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

Defined at lines 779 to 834.

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

Defined at lines 846 to 892.

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

Defined at lines 911 to 970.

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

Defined at lines 989 to 1047.

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

Defined at lines 1064 to 1138.

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

Defined at lines 1145 to 1164.

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

Defined at lines 1188 to 1236.

Hash of the volume ID, parent DID, and name of a dircache entry.

The didname index uses it. The mixing follows the SuperFastHash algorithm by Paul Hsieh over the bytes of d_u_name, seeded with d_pdid plus d_vid.

Parameters:
* `p`: struct dir whose d_vid, d_pdid, and d_u_name to hash

Returns: the hash value

Called by: [dircache_init](dircache.c.md#dircache_init)

Uses file-scope variables: `p`

### hash_comp_didname

```c
static int hash_comp_didname(const void *k1, const void *k2)
```

Defined at lines 1245 to 1252.

Compare two dircache entries by volume ID, parent DID, and name.

Parameters:
* `k1`: first struct dir
* `k2`: second struct dir

Returns: 0 when volume ID, parent DID, and name all match, 1 otherwise

Called by: [dircache_init](dircache.c.md#dircache_init)

### validation_freq

```c
static unsigned int validation_freq(void)
```

Defined at lines 1269 to 1276.

Validation frequency in effect.

Read live: the coherency defaults are applied per volume, after [dircache_init()](dircache.c.md#dircache_init). The single reader for the validation decision and the statistics, so both always report the same frequency.

Returns: configured validation frequency, never 0 (divisor)

Called by: [log_dircache_stat](dircache.c.md#log_dircache_stat), [should_validate_cache_entry](dircache.c.md#should_validate_cache_entry)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### should_validate_cache_entry

```c
static int should_validate_cache_entry(void)
```

Defined at lines 1288 to 1294.

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

Defined at lines 1307 to 1339.

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

Defined at lines 1358 to 1363.

Cache hint event to send when dir disappears.

Files map to CACHE_HINT_DELETE and directories to CACHE_HINT_DELETE_CHILDREN. Receivers treat CACHE_HINT_DELETE as file-only when they decide whether a pfd slot may exist. The mapping therefore has a single definition.

Parameters:
* `dir`: dircache entry that disappeared

Returns: CACHE_HINT_DELETE for a file, CACHE_HINT_DELETE_CHILDREN for a directory

Called by: [validation_expunge_and_hint](dircache.c.md#validation_expunge_and_hint), [validation_refresh_and_hint](dircache.c.md#validation_refresh_and_hint)

### validation_expunge_and_hint

```c
static void validation_expunge_and_hint(const struct vol *vol, struct dir *cdir)
```

Defined at lines 1377 to 1387.

Expunge an entry that validation found gone and tell the sibling afpd processes.

One hint message saves every sibling afpd a separate full-path stat. The hint reports the evidence. A recreated object of the same name does no harm, because hints only invalidate and receivers rebuild. The call counts the expunge as a cache miss.

Parameters:
* `vol`: volume of the entry
* `cdir`: entry to expunge, invalid afterwards

Calls: [dir_remove](directory.c.md#dir_remove), [gone_event_for](dircache.c.md#gone_event_for), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint)

Called by: [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### validation_refresh_and_hint

```c
static int validation_refresh_and_hint(const struct vol *vol, struct dir *cdir, struct stat *st)
```

Defined at lines 1409 to 1442.

Refresh an entry whose inode or ctime changed and tell the sibling afpd processes.

An inode change means another object replaced the entry. The call then sends CACHE_HINT_REFRESH under the current DID. When dir_modify gave the entry a new DID, a second hint goes out under the old DID that siblings still cache. When dir_modify evicted the entry instead, the gone event of the entry goes out under the old DID.

A ctime-only change sends nothing. Fork writes advance ctime on every flush, and a hint per write would rebuild the sibling AppleDouble and resource fork caches. Each sibling picks up pure metadata changes when it validates the entry itself.

Parameters:
* `vol`: volume of the entry
* `cdir`: entry to refresh
* `st`: fresh stat of the entry

Returns: 0 when the entry survived, -1 when dir_modify evicted it

Calls: [dir_modify](directory.c.md#dir_modify), [gone_event_for](dircache.c.md#gone_event_for), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint)

Called by: [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### dircache_lookup_parent

```c
struct dir * dircache_lookup_parent(const struct vol *vol, cnid_t did)
```

Defined at lines 1456 to 1480.

Look up a directory entry by DID for pfd_cache parent resolution.

The probe never validates, changes, or promotes the entry. It skips ARC ghosts. A ghost keeps frozen, unvalidated identity fields, so accepting one as a live parent would let the pfd sync test pass against stale identity. It also skips file entries, because a parent is a directory.

Parameters:
* `vol`: volume of the entry
* `did`: directory ID

Returns: the entry, or NULL for an invalid DID, a miss, a ghost, or a file

Calls: [hash_lookup](hash.c.md#hash_lookup), [hnode_get](hash.c.md#hnode_get)

Called by: [pfd_ostat](pfd_cache.c.md#pfd_ostat)

Uses file-scope variables: `dircache`

### dircache_search_by_did

```c
struct dir * dircache_search_by_did(const struct vol *vol, cnid_t cnid)
```

Defined at lines 1500 to 1633.

Search the dircache via a CNID for a directory.

Found cache entries are expunged if both the parent directory st_ctime and the objects st_ctime are modified. This func builds on the fact, that all our code only ever needs to and does search the dircache by CNID expecting directories to be returned, but not files. Thus (1) if we find a file for a given CNID we (1a) remove it from the cache (1b) return NULL indicating nothing found (2) we can then use d_fullpath to stat the directory

Parameters:
* `vol`: pointer to struct vol
* `cnid`: CNID of the directory to search

Returns: Pointer to struct dir if found, else NULL

Calls: [arc_case_i](dircache.c.md#arc_case_i), [arc_case_ii_adapt_and_replace](dircache.c.md#arc_case_ii_adapt_and_replace), [arc_case_iii_adapt_and_replace](dircache.c.md#arc_case_iii_adapt_and_replace), [dir_remove](directory.c.md#dir_remove), [hash_lookup](hash.c.md#hash_lookup), [hnode_get](hash.c.md#hnode_get), [pfd_ostat](pfd_cache.c.md#pfd_ostat), [should_validate_cache_entry](dircache.c.md#should_validate_cache_entry), [validation_expunge_and_hint](dircache.c.md#validation_expunge_and_hint), [validation_refresh_and_hint](dircache.c.md#validation_refresh_and_hint)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [dirlookup_internal](directory.c.md#dirlookup_internal), [dirlookup_internal_retry](directory.c.md#dirlookup_internal_retry)

Uses file-scope variables: `arc_cache`, `dircache`

### dircache_search_by_name

```c
struct dir * dircache_search_by_name(const struct vol *vol, const struct dir *dir, char *name, size_t len)
```

Defined at lines 1648 to 1763.

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

Defined at lines 1776 to 1793.

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

Defined at lines 1806 to 1936.

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

Defined at lines 1951 to 1965.

Abort afpd because an index node names an entry other than its owner.

Every node starts with its entry as data and goes away only when that entry leaves the index. A mismatch therefore means memory corruption or a foreign delete. Freeing an entry that a table still references would corrupt the cache, so the call logs both entries, dumps the dircache, and panics.

Parameters:
* `index_name`: name of the index, for the log line
* `dir`: entry that dircache_remove works on
* `node`: node that the entry stores, or NULL
* `found`: entry that the node names, or NULL

Calls: [dircache_dump](dircache.c.md#dircache_dump)

Called by: [dircache_remove](dircache.c.md#dircache_remove)

### dircache_remove

```c
void dircache_remove(const struct vol *vol, struct dir *dir, int flags)
```

Defined at lines 1983 to 2103.

Remove an entry from the dircache indexes and queues that flags name.

The call removes only the nodes that the entry stores, so it never costs another entry its node. An index that holds no node for the entry stays untouched. A node that names a different entry panics before anything changes, and so does a DIRF_INDEXED entry without a dircache node. Removing a directory from the dircache also purges its pfd slot. Callers outside [dircache.c](dircache.c.md) pass QUEUE_INDEX, DIDNAME_INDEX, and DIRCACHE.

Parameters:
* `vol`: unused
* `dir`: entry to remove
* `flags`: QUEUE_INDEX, DIDNAME_INDEX, DIRCACHE, and DIRCACHE_NOSHRINK to remove hash nodes without shrinking the tables during a scan

Calls: [dircache_mismatch_panic](dircache.c.md#dircache_mismatch_panic), [hash_delete_free](hash.c.md#hash_delete_free), [hash_scan_delfree](hash.c.md#hash_scan_delfree), [hnode_get](hash.c.md#hnode_get), [pfd_purge](pfd_cache.c.md#pfd_purge), [queue_remove](../../libatalk/util/queue.c.md#queue_remove)

Called by: [arc_case_iv_make_room](dircache.c.md#arc_case_iv_make_room), [arc_ensure_ghost_capacity](dircache.c.md#arc_ensure_ghost_capacity), [dir_modify](directory.c.md#dir_modify), [dir_remove](directory.c.md#dir_remove), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [dircache_evict](dircache.c.md#dircache_evict), [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_remove_children](dircache.c.md#dircache_remove_children)

Uses file-scope variables: `arc_cache`, `dircache`, `index_didname`, `queue_count`, `vid_entry_count`

Mentioned in the documentation of: [arc_ensure_ghost_capacity](dircache.c.md#arc_ensure_ghost_capacity)

### dircache_remove_children

```c
int dircache_remove_children(const struct vol *vol, struct dir *dir)
```

Defined at lines 2125 to 2201.

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

### dircache_reindex_didname

```c
int dircache_reindex_didname(const struct vol *vol, struct dir *dir)
```

Defined at lines 2215 to 2245.

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

Defined at lines 2263 to 2291.

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

Defined at lines 2304 to 2331.

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

Defined at lines 2348 to 2439.

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

Defined at lines 2449 to 2658.

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

Defined at lines 2666 to 2686.

Shutdown the rfork cache — free remaining RFork LRU nodes and sentinel.

Called from the child shutdown path after [log_dircache_stat()](dircache.c.md#log_dircache_stat). Frees the qnodes (rfork data buffers are reclaimed by exit()) and the LRU sentinel.

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

Uses file-scope variables: `rfork_lru`, `rfork_lru_count`

### dircache_dump

```c
void dircache_dump(void)
```

Defined at lines 2691 to 2897.

Dump dircache to /tmp/dircache.PID.

Calls: [hash_scan_begin](hash.c.md#hash_scan_begin), [hash_scan_next](hash.c.md#hash_scan_next), [hnode_get](hash.c.md#hnode_get), [tmpdir](../../libatalk/util/unix.c.md#tmpdir)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [dircache_add](dircache.c.md#dircache_add), [dircache_evict](dircache.c.md#dircache_evict), [dircache_mismatch_panic](dircache.c.md#dircache_mismatch_panic)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `arc_cache`, `dircache`, `dircache_maxsize`, `index_didname`, `index_queue`, `queue_count`, `rfork_cache_budget`, `rfork_cache_used`, `rfork_lru_count`, `rfork_max_entry_size`

### dircache_reset_validation_counter

```c
void dircache_reset_validation_counter(void)
```

Defined at lines 2905 to 2910.

Reset validation counter for consistent testing.

Resets the global validation counter to ensure predictable validation patterns between test runs or configuration changes.

Called by: [dircache_init](dircache.c.md#dircache_init)

Uses file-scope variables: `validation_counter`

### process_cache_hints

```c
void process_cache_hints(AFPObj *obj)
```

Defined at lines 2945 to 3135.

Apply the dircache invalidation hints that sibling afpd processes sent.

The [DSI](../../include/atalk/dsi.h.md#struct-dsi) command loop calls it after every AFP command. Each call reads the hint pipe without blocking and applies up to 32 hints, the MAX_HINTS_PER_CYCLE limit. A partial message waits for the next call. End of file means the parent afpd closed the pipe, so the call closes its end and later calls return at once.

The lookup uses hash_lookup on the DID index, because dircache_search_by_did drops file entries and hints cover files too. CACHE_HINT_REFRESH stats the entry and refreshes it with dir_modify, or removes it when the stat fails. CACHE_HINT_DELETE removes the entry. CACHE_HINT_DELETE_CHILDREN defers removing the children and then refreshes or removes the entry itself. A miss still purges a pfd slot for the directory events.

CACHE_HINT_VOLUME_RESET matches an open volume by its CNID tag and ends the session with SIGTERM, so the client reconnects to the rebuilt CNID table.

Parameters:
* `obj`: session object holding the hint pipe

Calls: [cnid_volume_tag](volume.c.md#cnid_volume_tag), [dir_modify](directory.c.md#dir_modify), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [hash_lookup](hash.c.md#hash_lookup), [hnode_get](hash.c.md#hnode_get), [ostat](../../libatalk/util/unix.c.md#ostat), [pfd_purge](pfd_cache.c.md#pfd_purge)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp), [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `cache_hint_stat`, `dircache`

### dircache_report_invalid_entry

```c
void dircache_report_invalid_entry(struct dir *dir)
```

Defined at lines 3147 to 3156.

Report that a cache entry was invalid when actually used.

This function should be called when a cached directory entry that was returned without validation (for performance) turns out to be invalid when actually accessed (e.g., file doesn't exist, has been modified, etc). This helps track the effectiveness of the validation frequency setting.

Parameters:
* `dir`: The directory entry that was found to be invalid

Called by: [dir_remove](directory.c.md#dir_remove)

### dircache_remove_children_defer

```c
void dircache_remove_children_defer(const struct vol *vol, struct dir *dir)
```

Defined at lines 3175 to 3247.

Queue a deferred dircache_remove_children scan for dir.

The enqueue costs constant time, and the idle worker runs the hash scan while afpd waits in poll. The job stores the full path of dir byte for byte, with its volume, depth, and enqueue sequence number. Without an active idle worker, with a full queue, or when malloc fails, the call runs dircache_remove_children at once instead. A NULL dir or one without a full path needs nothing.

Parameters:
* `vol`: volume of dir
* `dir`: directory whose children leave the cache, or NULL

Calls: [dircache_remove_children](dircache.c.md#dircache_remove_children), [iw_is_active](idle_worker.c.md#iw_is_active), [iw_note_work](idle_worker.c.md#iw_note_work)

Called by: [afp_delete](filedir.c.md#afp_delete), [afp_openfork](fork.c.md#afp_openfork), [deletecurdir](directory.c.md#deletecurdir), [dirlookup_internal_retry](directory.c.md#dirlookup_internal_retry), [dirlookup_strict](directory.c.md#dirlookup_strict), [moveandrename](filedir.c.md#moveandrename), [movecwd](directory.c.md#movecwd), [process_cache_hints](dircache.c.md#process_cache_hints)

Uses file-scope variables: `deferred_count`, `deferred_depth_max`, `deferred_depth_min`, `deferred_queue`, `deferred_seq`, `deferred_tail`

### dircache_flush_deferred_for_vol

```c
void dircache_flush_deferred_for_vol(uint16_t vid)
```

Defined at lines 3264 to 3327.

Finish every deferred child scan of a closing volume at once.

The afp_closevol handler calls it before the volume structures go away, while the idle worker is dormant. The call drops the active job cursor and the covering memo when they belong to the volume. It also drops any batch in progress, because the jobs it retires invalidate the batch member index. It then scans the dircache for each job of the volume and frees the entries below the job path, except curdir.

Parameters:
* `vid`: v_vid of the closing volume

Calls: [deferred_compact_head](dircache.c.md#deferred_compact_head), [deferred_job_kill](dircache.c.md#deferred_job_kill), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [hash_scan_begin](hash.c.md#hash_scan_begin), [hash_scan_next](hash.c.md#hash_scan_next), [hnode_get](hash.c.md#hnode_get)

Called by: [afp_closevol](volume.c.md#afp_closevol)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `deferred_active`, `deferred_batch_active`, `deferred_batch_n`, `deferred_count`, `deferred_cover_len`, `deferred_cover_path`, `deferred_cover_vid`, `deferred_head`, `deferred_queue`, `dircache`

### dircache_has_deferred_work

```c
int dircache_has_deferred_work(void)
```

Defined at lines 3336 to 3339.

Whether deferred child scans wait in the queue.

The idle worker calls it to decide whether it has work.

Returns: nonzero when the queue holds jobs, 0 otherwise

Called by: [iw_main](idle_worker.c.md#iw_main), [iw_work_pending](idle_worker.c.md#iw_work_pending)

Uses file-scope variables: `deferred_count`

### deferred_job_kill

```c
static void deferred_job_kill(int idx)
```

Defined at lines 3350 to 3355.

Retire a deferred job slot without running its scan.

The callers retire jobs of a closing volume and jobs that a finished scan already covers. The slot keeps its place in the ring until the head moves past it.

Parameters:
* `idx`: slot index in the deferred queue

Called by: [deferred_batch_complete](dircache.c.md#deferred_batch_complete), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_queue`

### deferred_compact_head

```c
static void deferred_compact_head(void)
```

Defined at lines 3360 to 3368.

Advance deferred_head past dead slots, keeping count consistent.

Called by: [deferred_batch_complete](dircache.c.md#deferred_batch_complete), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_count`, `deferred_head`, `deferred_queue`

### path_is_strict_child

```c
static int path_is_strict_child(const char *path, size_t len, const char *parent_path, size_t parent_len)
```

Defined at lines 3382 to 3388.

Whether path lies strictly below parent_path.

The test compares bytes and requires a slash right after the parent prefix, so "/A/BC" does not count as a child of "/A/B".

Parameters:
* `path`: candidate path, or NULL
* `len`: bytes in path
* `parent_path`: parent path
* `parent_len`: bytes in parent_path

Returns: 1 for a strict descendant, 0 otherwise

Called by: [deferred_cover_drops](dircache.c.md#deferred_cover_drops), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

### deferred_cover_set

```c
static void deferred_cover_set(const struct deferred_cleanup *done, unsigned int scan_seq)
```

Defined at lines 3400 to 3416.

Remember a finished scan as the covering memo.

Jobs popped afterwards for the same path or a path below it need no separate scan when they entered the queue before the scan started. When malloc fails, no memo remains.

Parameters:
* `done`: job whose scan finished
* `scan_seq`: enqueue sequence number at the start of the scan

Called by: [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_cover_len`, `deferred_cover_path`, `deferred_cover_seq`, `deferred_cover_vid`

### deferred_cover_drops

```c
static int deferred_cover_drops(const struct deferred_cleanup *j)
```

Defined at lines 3430 to 3441.

Whether the covering memo makes a popped job redundant.

The job must belong to the memo volume, carry the memo path or a path strictly below it, and predate the memo scan. The sequence test counts an unsigned difference of 0x80000000 or more as earlier, so the test stays defined across wraparound.

Parameters:
* `j`: popped job

Returns: 1 when the memo scan already purged the entries of the job, 0 otherwise

Calls: [path_is_strict_child](dircache.c.md#path_is_strict_child)

Called by: [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_cover_len`, `deferred_cover_path`, `deferred_cover_seq`, `deferred_cover_vid`

### deferred_compact_tail

```c
static void deferred_compact_tail(void)
```

Defined at lines 3449 to 3468.

Compact deferred_tail backwards past dead slots.

LIFO consumption retires slots at the tail; head compaction still handles slots killed by the volume flush.

Called by: [deferred_batch_complete](dircache.c.md#deferred_batch_complete), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `deferred_count`, `deferred_depth_max`, `deferred_depth_min`, `deferred_queue`, `deferred_tail`

### deferred_batch_cmp

```c
static int deferred_batch_cmp(const void *a, const void *b)
```

Defined at lines 3479 to 3498.

Order two batch members by volume ID, then by path bytes.

A shorter path that is a prefix of the other sorts first.

Parameters:
* `a`: pointer to the deferred queue index of the first member
* `b`: pointer to the deferred queue index of the second member

Returns: a negative value, 0, or a positive value, as qsort expects

Called by: [deferred_batch_start](dircache.c.md#deferred_batch_start)

Uses file-scope variables: `c`, `deferred_queue`

### deferred_batch_match

```c
static int deferred_batch_match(uint16_t vid, const char *ep, size_t elen)
```

Defined at lines 3513 to 3567.

Find the batch member that covers an entry path.

Every batch member sits at the same depth, so the only candidate ancestor is the entry path truncated at slash number deferred_batch_depth + 1. A binary search over the sorted member index compares bytes only. A path at or above the member depth matches nothing.

Parameters:
* `vid`: volume of the entry
* `ep`: entry path
* `elen`: bytes in ep

Returns: the position in the member index, or -1 without a match

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain)

Uses file-scope variables: `c`, `deferred_batch_depth`, `deferred_batch_idx`, `deferred_batch_n`, `deferred_queue`

### deferred_batch_start

```c
static int deferred_batch_start(void)
```

Defined at lines 3575 to 3598.

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

Defined at lines 3606 to 3623.

Finish the flat batch scan: retire every pre-batch member.

Mid-batch enqueues (seq >= batch seq) stay pending — their entries may sit in chains the batch already passed.

Calls: [deferred_compact_head](dircache.c.md#deferred_compact_head), [deferred_compact_tail](dircache.c.md#deferred_compact_tail), [deferred_job_kill](dircache.c.md#deferred_job_kill)

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain)

Uses file-scope variables: `deferred_batch_active`, `deferred_batch_idx`, `deferred_batch_n`, `deferred_batch_seq`, `deferred_queue`

### deferred_batch_chain

```c
static int deferred_batch_chain(void)
```

Defined at lines 3631 to 3695.

Scan one hash chain for the flat batch: purge every entry whose unique candidate ancestor is a batch member.

Returns: 1 if more chains remain, 0 when the batch completed

Calls: [deferred_batch_complete](dircache.c.md#deferred_batch_complete), [deferred_batch_match](dircache.c.md#deferred_batch_match), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [hash_chain_head](dircache.c.md#hash_chain_head), [hash_size](hash.c.md#hash_size), [hnode_get](hash.c.md#hnode_get), [iw_grant_active](idle_worker.c.md#iw_grant_active)

Called by: [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `deferred_batch_chain_idx`, `deferred_count`, `dircache`

### dircache_process_deferred_chain

```c
int dircache_process_deferred_chain(void)
```

Defined at lines 3710 to 3840.

Process one unit of deferred cleanup.

Called by idle worker under a validated grant. Jobs are consumed LIFO: AFP deletes bottom-up, so a tree's covering root-most job arrives last and its single scan purges the whole subtree; the covering memo then drops the tree's remaining jobs in O(1) as they pop. When every pending job sits at one depth, one merged batch scan serves all of them. Every removal re-checks iw_can_work; scans resume via their chain cursors across granted cycles.

Returns: 1 if more work remains, 0 if all deferred work is complete

Calls: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [deferred_batch_start](dircache.c.md#deferred_batch_start), [deferred_compact_head](dircache.c.md#deferred_compact_head), [deferred_compact_tail](dircache.c.md#deferred_compact_tail), [deferred_cover_drops](dircache.c.md#deferred_cover_drops), [deferred_cover_set](dircache.c.md#deferred_cover_set), [deferred_job_kill](dircache.c.md#deferred_job_kill), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [hash_chain_head](dircache.c.md#hash_chain_head), [hash_size](hash.c.md#hash_size), [hnode_get](hash.c.md#hnode_get), [iw_grant_active](idle_worker.c.md#iw_grant_active), [path_is_strict_child](dircache.c.md#path_is_strict_child)

Called by: [iw_main](idle_worker.c.md#iw_main)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `deferred_active`, `deferred_batch_active`, `deferred_count`, `deferred_depth_max`, `deferred_depth_min`, `deferred_queue`, `deferred_scan_started_seq`, `deferred_seq`, `deferred_tail`, `dircache`

# Types

### struct deferred_cleanup

Defined at line 151.
* `char * parent_path`
* `size_t parent_len`
* `uint16_t v_vid`
* `hashcount_t chain_idx`
* `unsigned int seq`
* `unsigned int depth`

### struct dircache_stat

Defined at line 215.
* `unsigned long long lookups`
* `unsigned long long hits`: Hits in T1/T2 (cached data, instant return)
* `unsigned long long ghost_hits`: Hits in B1/B2 (ghost entries, instant return in our impl)
* `unsigned long long misses`: True misses (not in any ARC list)
* `unsigned long long added`
* `unsigned long long removed`
* `unsigned long long expunged`
* `unsigned long long evicted`
* `unsigned long long invalid_on_use`: entries that failed when used
* `unsigned long long covered_cancelled`: deferred jobs cancelled as covered by a completed ancestor scan

# Typedefs and enums

* `enum arc_list_t`: `ARC_NONE`, `ARC_T1`, `ARC_T2`, `ARC_B1`, `ARC_B2`

# Macros

* Undocumented: `HINT_READ_BUF_SIZE`, `MAX_DEFERRED_CLEANUPS`, `MAX_HINTS_PER_CYCLE`, `get16bits`

# File-scope variables

`adaptations`, `arc_cache`, `arc_promoting_ghost`, `b1`, `b1_size`, `b2`, `b2_size`, `c`, `cache_hint_stat`, `deferred_active`, `deferred_batch_active`, `deferred_batch_chain_idx`, `deferred_batch_depth`, `deferred_batch_idx`, `deferred_batch_n`, `deferred_batch_seq`, `deferred_count`, `deferred_cover_len`, `deferred_cover_path`, `deferred_cover_seq`, `deferred_cover_vid`, `deferred_depth_max`, `deferred_depth_min`, `deferred_head`, `deferred_queue`, `deferred_scan_started_seq`, `deferred_seq`, `deferred_tail`, `dircache`, `dircache_maxsize`, `dircache_stat`, `enabled`, `evictions_t1`, `evictions_t2`, `ghost_hits_b1`, `ghost_hits_b2`, `hints_acted_on`, `hints_no_match`, `hints_received`, `hits_t1`, `hits_t2`, `index_didname`, `index_queue`, `p`, `p_decreases`, `p_increases`, `p_max`, `p_min`, `promotions_t1_to_t2`, `queue_count`, `queue_count_max`, `rfork_cache_budget`, `rfork_cache_used`, `rfork_lru`, `rfork_lru_count`, `rfork_max_entry_size`, `rfork_stat_added`, `rfork_stat_evicted`, `rfork_stat_hits`, `rfork_stat_invalidated`, `rfork_stat_lookups`, `rfork_stat_misses`, `rfork_stat_used_max`, `stats`, `t1`, `t1_size`, `t2`, `t2_size`, `validation_counter`, `vid_entry_count`
