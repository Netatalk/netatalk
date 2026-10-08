---
type: C Source File
title: "etc/afpd/ad_cache.c"
description: "6 functions, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/ad_cache.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [unix.h](unix.h.md)
* System headers: `errno.h`, `stdbool.h`, `stdlib.h`, `string.h`, `sys/stat.h`

# Functions

### ad_store_to_cache

```c
void ad_store_to_cache(struct adouble *adp, struct dir *cached)
```

Defined at lines 56 to 87.

Store AD metadata results into struct dir cache fields.

Called after every [ad_metadata()](../../libatalk/adouble/ad_open.c.md#ad_metadata) disk read when a cached entry is available. Pre-computes the served FILPBIT_MDATE value as max(ad_mdate, dcache_mtime).

[ad_entry()](../../include/atalk/adouble.h.md#struct-ad_entry) returns NULL if the entry doesn't exist in this AD version. This is fine — zero-initialized cache fields are valid defaults: zero FinderInfo = no custom attributes zero FileDatesI = overridden by stat mtime fallback zero AFPFileI = no attributes

Called by: [ad_getcomment](desktop.c.md#ad_getcomment), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [adl_lkup](catsearch.c.md#adl_lkup), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_listextattr](extattrs.c.md#afp_listextattr), [dir_modify](directory.c.md#dir_modify), [getfilparams](file.c.md#getfilparams), [getmetadata](file.c.md#getmetadata), [moveandrename](filedir.c.md#moveandrename)

### ad_rebuild_from_cache

```c
void ad_rebuild_from_cache(struct adouble *adp, const struct dir *cached)
```

Defined at lines 99 to 131.

Populate struct adouble from struct dir cache fields.

Sets up the entry directory in adp so that [ad_getattr()](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_getdate()](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_entry()](../../include/atalk/adouble.h.md#struct-ad_entry) all work normally — downstream code is completely unchanged.

Precondition: [ad_init()](../../libatalk/adouble/ad_open.c.md#ad_init) must have been called on adp to set up ad_eid[] offsets and valid_data_len via [ad_init_offsets()](../../libatalk/adouble/ad_open.c.md#ad_init_offsets). As of E-006, [ad_init()](../../libatalk/adouble/ad_open.c.md#ad_init) now calls [ad_init_offsets()](../../libatalk/adouble/ad_open.c.md#ad_init_offsets) internally.

Called by: [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached)

### ad_metadata_cached

```c
int ad_metadata_cached(const char *name, int flags, struct adouble *adp, const struct vol *vol, struct dir *dir, bool strict, struct stat *recent_st)
```

Defined at lines 143 to 276.

Unified AD metadata access with integrated cache management.

Read-only metadata accessor with internal [ad_close()](../../libatalk/adouble/ad_flush.c.md#ad_close). Callers needing a writable fd (e.g., for ad_setid/ad_flush) must call [ad_open()](../../libatalk/adouble/ad_open.c.md#ad_open) separately.

strict=false: cache-first (enumerate, FPGetFileDirParms). strict=true: validate ctime/inode before serving (moveandrename, deletecurdir). recent_st: optional stat to avoid duplicate [ostat()](../../libatalk/util/unix.c.md#ostat) in strict path.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_metadata](../../libatalk/adouble/ad_open.c.md#ad_metadata), [ad_rebuild_from_cache](ad_cache.c.md#ad_rebuild_from_cache), [ad_reso_size](../../libatalk/adouble/ad_open.c.md#ad_reso_size), [ad_rlen_meta_absent](../../include/atalk/directory.h.md#ad_rlen_meta_absent), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [dir_modify](directory.c.md#dir_modify), [ostat](../../libatalk/util/unix.c.md#ostat), [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [adl_lkup](catsearch.c.md#adl_lkup), [afp_listextattr](extattrs.c.md#afp_listextattr), [deletecurdir](directory.c.md#deletecurdir), [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams), [moveandrename](filedir.c.md#moveandrename)

Uses file-scope variables: `ad_cache_hits`, `ad_cache_misses`, `ad_cache_no_ad`, `rfork_stat_invalidated` in [etc/afpd/dircache.c](dircache.c.md)

### rfork_cache_free

```c
void rfork_cache_free(struct dir *entry)
```

Defined at lines 297 to 352.

Free a single entry's rfork buffer, remove from rfork LRU, update counter.

Uses dcache_rlen for the buffer size. INVARIANT: dcache_rlen >= 0 when dcache_rfork_buf != NULL. Uses production-safe fallback: if dcache_rlen < 0 (invariant violation), logs error, frees buffer, but does NOT touch budget counter (unknown size). Handles rfork_lru_node == NULL gracefully (orphaned buffer from enqueue failure — budget is still decremented correctly). Decrements rfork_lru_count when removing from LRU. Does NOT reset dcache_rlen — the AD metadata remains valid.

Called by: [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [dir_free](directory.c.md#dir_free), [dir_modify](directory.c.md#dir_modify), [read_fork](fork.c.md#read_fork), [rfork_cache_evict_to_budget](ad_cache.c.md#rfork_cache_evict_to_budget), [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd)

Uses file-scope variables: `rfork_cache_used` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru_count` in [etc/afpd/dircache.c](dircache.c.md)

### rfork_cache_evict_to_budget

```c
void rfork_cache_evict_to_budget(size_t needed)
```

Defined at lines 362 to 376.

Evict rfork buffers from rfork LRU head (oldest/LRU) until under budget.

Queue convention: sentinel->next = head = LRU (oldest), sentinel->prev = tail = MRU (newest). Called by [rfork_cache_store_from_fd()](ad_cache.c.md#rfork_cache_store_from_fd) when budget would be exceeded. O(k) where k = entries evicted.

Calls: [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd)

Uses file-scope variables: `rfork_cache_budget` in [etc/afpd/dircache.c](dircache.c.md), `rfork_cache_used` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_evicted` in [etc/afpd/dircache.c](dircache.c.md)

### rfork_cache_store_from_fd

```c
int rfork_cache_store_from_fd(struct dir *entry, struct adouble *adp, int eid)
```

Defined at lines 391 to 489.

Store resource fork data by reading directly from the ad fd.

Allocates dcache_rlen bytes and does ad_read(adp, eid, 0, buf, dcache_rlen). [ad_read()](../../libatalk/adouble/ad_read.c.md#ad_read) handles all storage formats (EA, macOS xattr, AD v2 sidecar). Guards: returns -1 if !ad_rsrc_open(adp) (fd not open), dcache_rlen <= 0, or rlen > rfork_max_entry_size. Validates that ad_read returns exactly dcache_rlen bytes — if not, the fork size changed since Tier 1 metadata was cached: invalidates ALL cached AD metadata (Tier 1) and returns -1 (self-healing).

Returns: 0 on success, -1 if not cacheable, short read, or ad_read failed.

Calls: [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [enqueue](../../libatalk/util/queue.c.md#enqueue), [rfork_cache_evict_to_budget](ad_cache.c.md#rfork_cache_evict_to_budget), [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [read_fork](fork.c.md#read_fork)

Uses file-scope variables: `rfork_cache_budget` in [etc/afpd/dircache.c](dircache.c.md), `rfork_cache_used` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru_count` in [etc/afpd/dircache.c](dircache.c.md), `rfork_max_entry_size` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_used_max` in [etc/afpd/dircache.c](dircache.c.md)

Mentioned in the documentation of: [rfork_cache_evict_to_budget](ad_cache.c.md#rfork_cache_evict_to_budget)

# File-scope variables

`ad_cache_hits`, `ad_cache_misses`, `ad_cache_no_ad`
