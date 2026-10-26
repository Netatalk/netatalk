---
type: C Source File
title: "etc/afpd/ad_cache.c"
description: "AppleDouble metadata and resource fork caching in the directory cache."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/ad_cache.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-26T14:42:48+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [fork.h](fork.h.md)
* [unix.h](unix.h.md)
* System headers: `errno.h`, `fcntl.h`, `stdbool.h`, `stdint.h`, `stdlib.h`, `string.h`, `strings.h`, `sys/stat.h`

# Functions

### ad_store_to_cache

```c
void ad_store_to_cache(struct adouble *adp, struct dir *cached)
```

Defined at lines 67 to 98.

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

Defined at lines 111 to 143.

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

Defined at lines 166 to 249.

Read the AppleDouble metadata of an object through the dircache.

The call closes the metadata before it returns, so callers that write open it separately. The cache serves while the entry's ctime and inode match the caller's recent stat; strict, as rename and delete need, takes a stat of its own when the caller has none. A mismatch brings the entry to the stat with [dir_takes_stat()](directory.c.md#dir_takes_stat) and reads from disk; an entry that does not then hold the stat takes nothing from the read. A cached absence of metadata answers ENOENT without a disk read.

Parameters:
* `name`: file or directory
* `flags`: ADFLAGS_DIR and ADFLAGS_CHECK_OF, as for ad_metadata
* `adp`: initialized AppleDouble receiving the metadata
* `vol`: volume of the object
* `dir`: dircache entry of the object, or NULL
* `strict`: true to stat the object when recent_st is NULL
* `recent_st`: fresh stat of the object, or NULL

Returns: 0, or -1 with errno set, ENOENT when the object has no metadata

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_metadata](../../libatalk/adouble/ad_open.c.md#ad_metadata), [ad_rebuild_from_cache](ad_cache.c.md#ad_rebuild_from_cache), [ad_reso_size](../../libatalk/adouble/ad_open.c.md#ad_reso_size), [ad_rlen_meta_absent](../../include/atalk/directory.h.md#ad_rlen_meta_absent), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [dir_stat_differs](../../include/atalk/directory.h.md#dir_stat_differs), [dir_takes_stat](directory.c.md#dir_takes_stat), [ostat](../../libatalk/util/unix.c.md#ostat), [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [adl_lkup](catsearch.c.md#adl_lkup), [afp_listextattr](extattrs.c.md#afp_listextattr), [deletecurdir](directory.c.md#deletecurdir), [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams), [moveandrename](filedir.c.md#moveandrename)

Uses file-scope variables: `ad_cache_hits`, `ad_cache_misses`, `ad_cache_no_ad`, `rfork_stat_invalidated` in [etc/afpd/dircache.c](dircache.c.md)

### rfork_cache_free

```c
void rfork_cache_free(struct dir *entry)
```

Defined at lines 269 to 324.

Free the cached resource fork data of a dircache entry.

The buffer leaves the resource fork LRU list, and its size, dcache_rlen, comes off the budget counter. A negative dcache_rlen with a buffer breaks the invariant, so the call logs an error and frees the buffer without touching the counter. A buffer without an LRU node still frees correctly. The AppleDouble metadata of the entry stays valid.

Parameters:
* `entry`: dircache entry with a cached resource fork

Called by: [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [dir_free](directory.c.md#dir_free), [dir_modify](directory.c.md#dir_modify), [rfork_cache_evict_to_budget](ad_cache.c.md#rfork_cache_evict_to_budget), [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd)

Uses file-scope variables: `rfork_cache_used` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru_count` in [etc/afpd/dircache.c](dircache.c.md)

### rfork_cache_evict_to_budget

```c
void rfork_cache_evict_to_budget(size_t needed)
```

Defined at lines 336 to 350.

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

Defined at lines 370 to 476.

Cache the entire resource fork of a file by reading it from the open fork.

The ad_read call covers every storage format, so the call reads dcache_rlen bytes through it. It refuses a closed resource fork, an empty or unknown length, a fork above rfork_max_entry_size, a fork another session has open for writing, and a symlink's, whose opens no lock shows. It also refuses while the cache holds over 2 times its budget. Older buffers leave first when the budget lacks room. A short read means the fork changed since its metadata entered the cache, so the call invalidates the cached metadata as well.

Parameters:
* `entry`: dircache entry of the file
* `adp`: AppleDouble with the resource fork open
* `eid`: ADEID_RFORK

Returns: 0, or -1 when the fork is not cacheable or the read fell short

Calls: [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_testlock_range](../../libatalk/adouble/ad_lock.c.md#ad_testlock_range), [enqueue](../../libatalk/util/queue.c.md#enqueue), [rfork_cache_evict_to_budget](ad_cache.c.md#rfork_cache_evict_to_budget), [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [read_fork](fork.c.md#read_fork)

Uses file-scope variables: `rfork_cache_budget` in [etc/afpd/dircache.c](dircache.c.md), `rfork_cache_used` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru` in [etc/afpd/dircache.c](dircache.c.md), `rfork_lru_count` in [etc/afpd/dircache.c](dircache.c.md), `rfork_max_entry_size` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_used_max` in [etc/afpd/dircache.c](dircache.c.md)

### ea_cache_entry

```c
struct dir * ea_cache_entry(struct vol *vol, struct path *path, int oflag, const char *name)
```

Defined at lines 521 to 561.

The entry whose EA cache applies to this request, its stat brought up to the request's fresh stat, or NULL.

[cname()](directory.c.md#cname) has stat'ed a file; a directory addressed by its DID is stat'ed here, past the bypass tests, so a request the cache does not serve costs no stat. A get names its attribute, a list passes NULL: the names the list filter hides are afpd's own, written behind the entry by fork writes and FinderInfo stores, so a request under one takes no entry. Shared volumes never cache, so their requests skip the lookup. A symlink's own EAs and its target's differ, so a symlink is never cached; with follow symlinks = yes the stat cannot tell one, so kXAttrNoFollow requests bypass the cache there. An entry whose inode is not the object's is left to validation. A ctime change the stat shows below the second, where the filesystem keeps nanoseconds, drops the cache that [dir_stat_refresh()](directory.c.md#dir_stat_refresh) cannot see. When [of_statdir()](ofork.c.md#of_statdir)'s EACCES fallback has moved the cwd to the parent, from this stat or from the list handler's own, u_name takes the directory's own name, whatever that stat found, so the backend and the cache address it from there on every volume; the volume root has no parent directory and keeps ".".

Parameters:
* `vol`: volume of the request
* `path`: the request's path as [cname()](directory.c.md#cname) left it; a directory's stat is taken here, a file's d_cached memo updated
* `oflag`: O_NOFOLLOW for a kXAttrNoFollow request, else 0
* `name`: attribute name of a get, NULL for a names list

Returns: the entry, or NULL for a request the cache does not serve

Calls: [atalk_stat_ctime_timespec](../../include/atalk/compat.h.md#atalk_stat_ctime_timespec), [dir_stat_refresh](directory.c.md#dir_stat_refresh), [ea_cache_free](ad_cache.c.md#ea_cache_free), [of_statdir](ofork.c.md#of_statdir), [path_entry](file.c.md#path_entry), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [sys_ea_protected](../../libatalk/vfs/ea_sys.c.md#sys_ea_protected)

Called by: [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_listextattr](extattrs.c.md#afp_listextattr)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### ea_cache_names

```c
const char * ea_cache_names(struct dir *entry, size_t *len)
```

Defined at lines 570 to 580.

The cached names, or NULL when the entry has none cached.

Parameters:
* `entry`: dircache entry of the object
* `len`: bytes in the names on a hit

Returns: the names as the backend listed them, or NULL

Called by: [afp_listextattr](extattrs.c.md#afp_listextattr)

Uses file-scope variables: `ea_cache_stat`

Mentioned in the documentation of: [ea_cache_store_names](ad_cache.c.md#ea_cache_store_names)

### ea_cache_store_names

```c
void ea_cache_store_names(struct dir *entry, const char *names, size_t len)
```

Defined at lines 592 to 611.

Cache the object's EA names, after [ea_cache_names()](ad_cache.c.md#ea_cache_names) found none.

One byte over len, so an object without EAs holds a pointer too: the pointer is what says the names are valid.

Parameters:
* `entry`: dircache entry of the object
* `names`: the names as the backend listed them
* `len`: bytes in names; over EA_CACHE_NAMES nothing is kept

Called by: [afp_listextattr](extattrs.c.md#afp_listextattr)

Uses file-scope variables: `ea_cache_stat`

### ea_cache_free

```c
void ea_cache_free(struct dir *entry)
```

Defined at lines 618 to 630.

Drop the entry's EA cache.

Parameters:
* `entry`: dircache entry whose EA names and values are released

Called by: [arc_evict_to_ghost](dircache.c.md#arc_evict_to_ghost), [dir_free](directory.c.md#dir_free), [dir_modify](directory.c.md#dir_modify), [ea_cache_entry](ad_cache.c.md#ea_cache_entry), [ea_written](extattrs.c.md#ea_written)

Uses file-scope variables: `ea_cache_stat`

### ea_cache_value

```c
const char * ea_cache_value(struct dir *entry, const char *name, size_t *len)
```

Defined at lines 640 to 670.

The cached value of name, or NULL when it is not cached.

Parameters:
* `entry`: dircache entry of the object
* `name`: attribute name in the unix charset
* `len`: bytes in the value on a hit

Returns: the value as the backend delivered it, or NULL

Called by: [afp_getextattr](extattrs.c.md#afp_getextattr)

Uses file-scope variables: `ea_cache_stat`

### ea_cache_store_value

```c
void ea_cache_store_value(struct dir *entry, const char *name, const char *value, size_t len)
```

Defined at lines 681 to 711.

Cache one value the object was read for.

Parameters:
* `entry`: dircache entry of the object
* `name`: attribute name, at most UINT8_MAX bytes
* `value`: the value as the backend delivered it
* `len`: bytes in value; a record that does not fit within EA_CACHE_VALUES is not kept

Called by: [afp_getextattr](extattrs.c.md#afp_getextattr)

Uses file-scope variables: `ea_cache_stat`

### ea_cache_absent

```c
bool ea_cache_absent(const struct dir *entry, const char *name)
```

Defined at lines 725 to 755.

Does the entry's cached list prove an attribute missing?

The list holds the names as the client saw them, which an ASCII name matches after its conversion, case apart, since some attribute stores fold it; any other name, and an uncached list, leave the answer to the backend.

Parameters:
* `entry`: dircache entry of the object
* `name`: attribute name in the unix charset

Returns: true when the list is cached and lacks the name

Calls: [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_getextattr](extattrs.c.md#afp_getextattr)

Uses file-scope variables: `ea_cache_stat`

# Macros

* Undocumented: `EA_CACHE_NAMES`, `EA_CACHE_VALUES`

# File-scope variables

`ad_cache_hits`, `ad_cache_misses`, `ad_cache_no_ad`, `ea_cache_stat`
