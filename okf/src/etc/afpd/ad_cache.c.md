---
type: C Source File
title: "etc/afpd/ad_cache.c"
description: "6 functions, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/ad_cache.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Store the AppleDouble metadata of an object in its dircache entry.

Every ad_metadata disk read with a dircache entry at hand ends here. The entry keeps the Finder info, the dates, the AFP file info, and the resource fork length. The stored modification date is the later of the AppleDouble date and the stat mtime, as FILPBIT_MDATE serves it. An entry that the AppleDouble version lacks stays zero, a valid default.

Parameters:
* `adp`: AppleDouble with its metadata loaded
* `cached`: dircache entry of the object

Calls: [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry)

Called by: [ad_getcomment](desktop.c.md#ad_getcomment), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [adl_lkup](catsearch.c.md#adl_lkup), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_listextattr](extattrs.c.md#afp_listextattr), [dir_modify](directory.c.md#dir_modify), [getfilparams](file.c.md#getfilparams), [getmetadata](file.c.md#getmetadata), [moveandrename](filedir.c.md#moveandrename)

### ad_rebuild_from_cache

```c
void ad_rebuild_from_cache(struct adouble *adp, const struct dir *cached)
```

Defined at lines 100 to 132.

Fill an AppleDouble from the metadata cached in a dircache entry.

The cached Finder info, dates, and AFP file info land at the entry offsets of adp. The ad_getattr, ad_getdate, and [ad_entry](../../include/atalk/adouble.h.md#struct-ad_entry) calls then work as after a disk read. The ad_init call sets up those offsets, so the caller runs it first.

Parameters:
* `adp`: AppleDouble initialized with ad_init
* `cached`: dircache entry with cached metadata

Called by: [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached)

### ad_metadata_cached

```c
int ad_metadata_cached(const char *name, int flags, struct adouble *adp, const struct vol *vol, struct dir *dir, bool strict, struct stat *recent_st)
```

Defined at lines 154 to 287.

Read the AppleDouble metadata of an object through the dircache.

The call closes the metadata before it returns, so callers that write open it separately. Without strict the cache serves first, as enumeration and FPGetFileDirParms need. With strict the ctime and inode must match before the cache serves, as rename and delete need. A mismatch refreshes the entry through dir_modify and reads from disk. A cached absence of metadata answers ENOENT without a disk read.

Parameters:
* `name`: file or directory
* `flags`: ADFLAGS_DIR and ADFLAGS_CHECK_OF, as for ad_metadata
* `adp`: initialized AppleDouble receiving the metadata
* `vol`: volume of the object
* `dir`: dircache entry of the object, or NULL
* `strict`: true to test ctime and inode before a cache hit
* `recent_st`: fresh stat of the object, or NULL

Returns: 0, or -1 with errno set, ENOENT when the object has no metadata

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_metadata](../../libatalk/adouble/ad_open.c.md#ad_metadata), [ad_rebuild_from_cache](ad_cache.c.md#ad_rebuild_from_cache), [ad_reso_size](../../libatalk/adouble/ad_open.c.md#ad_reso_size), [ad_rlen_meta_absent](../../include/atalk/directory.h.md#ad_rlen_meta_absent), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [dir_modify](directory.c.md#dir_modify), [ostat](../../libatalk/util/unix.c.md#ostat), [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [adl_lkup](catsearch.c.md#adl_lkup), [afp_listextattr](extattrs.c.md#afp_listextattr), [deletecurdir](directory.c.md#deletecurdir), [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams), [moveandrename](filedir.c.md#moveandrename)

Uses file-scope variables: `ad_cache_hits`, `ad_cache_misses`, `ad_cache_no_ad`, `rfork_stat_invalidated` in [etc/afpd/dircache.c](dircache.c.md)

### rfork_cache_free

```c
void rfork_cache_free(struct dir *entry)
```

Defined at lines 307 to 362.

Free the cached resource fork data of a dircache entry.

The buffer leaves the resource fork LRU list, and its size, dcache_rlen, comes off the budget counter. A negative dcache_rlen with a buffer breaks the invariant, so the call logs an error and frees the buffer without touching the counter. A buffer without an LRU node still frees correctly. The AppleDouble metadata of the entry stays valid.

Parameters:
* `entry`: dircache entry with a cached resource fork

Called by: [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [dir_free](directory.c.md#dir_free), [dir_modify](directory.c.md#dir_modify), [read_fork](fork.c.md#read_fork), [rfork_cache_evict_to_budget](ad_cache.c.md#rfork_cache_evict_to_budget), [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd)

Uses file-scope variables: `rfork_cache_used` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru_count` in [etc/afpd/dircache.c](dircache.c.md)

### rfork_cache_evict_to_budget

```c
void rfork_cache_evict_to_budget(size_t needed)
```

Defined at lines 374 to 388.

Free cached resource forks from the oldest end of the LRU list until needed bytes fit.

The head of the list holds the oldest buffer and the tail the newest. The rfork_cache_store_from_fd call runs it before it caches a new fork, and the cost grows with the number of evicted buffers only.

Parameters:
* `needed`: bytes about to join the cache

Calls: [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd)

Uses file-scope variables: `rfork_cache_budget` in [etc/afpd/dircache.c](dircache.c.md), `rfork_cache_used` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_evicted` in [etc/afpd/dircache.c](dircache.c.md)

### rfork_cache_store_from_fd

```c
int rfork_cache_store_from_fd(struct dir *entry, struct adouble *adp, int eid)
```

Defined at lines 406 to 504.

Cache the entire resource fork of a file by reading it from the open fork.

The ad_read call covers every storage format, so the call reads dcache_rlen bytes through it. It refuses a closed resource fork, an empty or unknown length, and a fork above rfork_max_entry_size. It also refuses while the cache holds over 2 times its budget. Older buffers leave first when the budget lacks room. A short read means the fork changed since its metadata entered the cache, so the call invalidates the cached metadata as well.

Parameters:
* `entry`: dircache entry of the file
* `adp`: AppleDouble with the resource fork open
* `eid`: ADEID_RFORK

Returns: 0, or -1 when the fork is not cacheable or the read fell short

Calls: [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [enqueue](../../libatalk/util/queue.c.md#enqueue), [rfork_cache_evict_to_budget](ad_cache.c.md#rfork_cache_evict_to_budget), [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [read_fork](fork.c.md#read_fork)

Uses file-scope variables: `rfork_cache_budget` in [etc/afpd/dircache.c](dircache.c.md), `rfork_cache_used` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru_count` in [etc/afpd/dircache.c](dircache.c.md), `rfork_max_entry_size` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_used_max` in [etc/afpd/dircache.c](dircache.c.md)

# File-scope variables

`ad_cache_hits`, `ad_cache_misses`, `ad_cache_no_ad`
