---
type: C Source File
title: "libatalk/adouble/ad_lock.c"
description: "Byte range and share mode locks on AppleDouble forks."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_lock.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-31T16:21:52+11:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `inttypes.h`, `limits.h`, `stdio.h`, `stdlib.h`, `string.h`

# Functions

### shmdstrfromoff

```c
static const char * shmdstrfromoff(off_t off)
```

Defined at lines 39 to 75.

Name the share-mode lock slot that a lock-band offset stands for, for log output.

Parameters:
* `off`: byte offset of a lock in the share-mode band

Returns: a constant string such as "OPEN_WR_DATA", or "-" for an offset outside the band

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_testlock](ad_lock.c.md#ad_testlock), [ad_testlock_range](ad_lock.c.md#ad_testlock_range), [ad_tmplock](ad_lock.c.md#ad_tmplock), [set_lock](ad_lock.c.md#set_lock)

### set_lock

```c
static int set_lock(int fd, int cmd, struct flock *lock)
```

Defined at lines 89 to 112.

Apply an fcntl lock operation to a fork descriptor.

The AD_SYMLINK pseudo-descriptor holds no locks: F_GETLK reports F_UNLCK and F_SETLK succeeds without a system call.

Parameters:
* `fd`: fork descriptor, or AD_SYMLINK
* `cmd`: F_SETLK or F_GETLK
* `lock`: lock to set or probe, which F_GETLK overwrites with the blocking lock

Returns: 0, or -1 with errno from fcntl

Calls: [shmdstrfromoff](ad_lock.c.md#shmdstrfromoff)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_testlock_range](ad_lock.c.md#ad_testlock_range), [ad_testlock_whole](ad_lock.c.md#ad_testlock_whole), [adf_lock_gaps](ad_lock.c.md#adf_lock_gaps), [testlock](ad_lock.c.md#testlock)

### XLATE_FCNTL_LOCK

```c
static int XLATE_FCNTL_LOCK(int type)
```

Defined at lines 120 to 134.

Translate an ADLOCK_* lock type into the fcntl lock type.

Parameters:
* `type`: ADLOCK_RD, ADLOCK_WR, or ADLOCK_CLR

Returns: F_RDLCK, F_WRLCK, or F_UNLCK, or -1 for any other value

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_tmplock](ad_lock.c.md#ad_tmplock)

### OVERLAP

```c
static int OVERLAP(off_t a, off_t alen, off_t b, off_t blen)
```

Defined at lines 150 to 155.

Tell whether two byte ranges overlap, where a zero length runs to the end of the file.

Subtracting the starts instead of adding the lengths keeps the comparison within off_t for ranges at or past byte zero, one at the largest offset included.

Parameters:
* `a`: start of the first range
* `alen`: length of the first range, or 0 for the rest of the file
* `b`: start of the second range
* `blen`: length of the second range, or 0 for the rest of the file

Returns: non-zero when the ranges share at least one byte

Called by: [adf_findlock](ad_lock.c.md#adf_findlock), [adf_findxlock](ad_lock.c.md#adf_findxlock), [testlock](ad_lock.c.md#testlock)

### adf_lock_fits

```c
static int adf_lock_fits(off_t start, off_t len)
```

Defined at lines 174 to 177.

Tell whether a range of positive length ends at or below the largest offset.

Parameters:
* `start`: first byte of the range, at or past byte zero
* `len`: length of the range, positive

Returns: non-zero when the range's last byte fits in off_t

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_tmplock](ad_lock.c.md#ad_tmplock)

### adf_lock_last

```c
static off_t adf_lock_last(off_t start, off_t len)
```

Defined at lines 187 to 190.

Last byte of a range that fits, where a zero length reaches the end of the file.

Parameters:
* `start`: first byte of the range
* `len`: length of the range, or 0 to the end of the file

Returns: the last byte, the largest offset for a zero length

Called by: [adf_lock_gaps](ad_lock.c.md#adf_lock_gaps), [adf_lock_reach](ad_lock.c.md#adf_lock_reach), [adf_lock_window](ad_lock.c.md#adf_lock_window)

### adf_lock_lower

```c
static int adf_lock_lower(const struct ad_fd *adf, off_t target, int by_reach)
```

Defined at lines 204 to 222.

First index of the lock array whose entry starts, or with by_reach reaches, at or past target.

Entries are in order of start, and their reach, the farthest last byte of an entry and the ones before it, rises with the index.

Parameters:
* `adf`: fork descriptor
* `target`: the offset to search for
* `by_reach`: nonzero to search the entries' reach, zero their start

Returns: the index, or adf_lockcount when no entry qualifies

Called by: [ad_lock](ad_lock.c.md#ad_lock), [adf_lock_window](ad_lock.c.md#adf_lock_window)

### adf_lock_window

```c
static void adf_lock_window(const struct ad_fd *adf, off_t off, off_t len, int *lo, int *hi)
```

Defined at lines 241 to 248.

Index window of the lock array that can hold entries overlapping a byte range.

An entry before the first one whose reach is at or past off ends before the range starts, and an entry at or past the first one that starts past the range's last byte begins after it ends. The range has passed the callers' checks: it starts at or past byte zero and its last byte fits in off_t. A range of zero length reaches the end of the file, and no entry starts past the largest offset, so both take the array's end as their upper bound.

Parameters:
* `adf`: fork descriptor
* `off`: start of the range
* `len`: length of the range, or 0 to the end of the file
* `lo`: first index that can overlap the range
* `hi`: index after the last entry that can overlap the range

Calls: [adf_lock_last](ad_lock.c.md#adf_lock_last), [adf_lock_lower](ad_lock.c.md#adf_lock_lower)

Called by: [adf_findlock](ad_lock.c.md#adf_findlock), [adf_findxlock](ad_lock.c.md#adf_findxlock), [adf_lock_gaps](ad_lock.c.md#adf_lock_gaps)

### adf_lock_reach

```c
static void adf_lock_reach(struct ad_fd *adf, int from)
```

Defined at lines 256 to 270.

Recompute the reach of the entries from index from to the end.

Parameters:
* `adf`: fork descriptor
* `from`: first index to recompute

Calls: [adf_lock_last](ad_lock.c.md#adf_lock_last)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [adf_freelock](ad_lock.c.md#adf_freelock)

### adf_lock_gaps

```c
static int adf_lock_gaps(const struct ad_fd *adf, const struct flock *r)
```

Defined at lines 291 to 341.

Apply a lock operation to the bytes of a range that no recorded entry covers.

The window's entries are in order of start, so the uncovered bytes are the stretches between them. An entry covers its bytes whatever its type: below the band the conflict scan has refused another fork's conflicting entry, and a read lock is recorded there only on a read-only descriptor, where a write request degrades to a read lock too; in the band the fork's own share-mode entries are read locks, so a write reaching them takes no write lock there. A take that fails on a stretch releases the stretches before it; a release that fails on one logs it and goes on.

Parameters:
* `adf`: fork descriptor
* `r`: the range, past the callers' checks, with l_type F_RDLCK or F_WRLCK to take it or F_UNLCK to release it

Returns: 0, or -1 with errno from fcntl for a refused take, or -1 after a failed release

Calls: [adf_lock_last](ad_lock.c.md#adf_lock_last), [adf_lock_window](ad_lock.c.md#adf_lock_window), [set_lock](ad_lock.c.md#set_lock)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_tmplock](ad_lock.c.md#ad_tmplock), [adf_freelock](ad_lock.c.md#adf_freelock)

### adf_freelock

```c
static void adf_freelock(struct ad_fd *ad, const int i)
```

Defined at lines 354 to 382.

Drop lock i of a fork descriptor and compact the lock array.

The release covers the bytes of the entry's range that no surviving entry holds, so the survivors keep their locks whatever the kernel coalesced. The array shrinks once more than 100 slots stand unused, the ARRAY_FREE_DELTA margin.

Parameters:
* `ad`: fork descriptor holding the lock array
* `i`: index of the lock to drop

Calls: [adf_lock_gaps](ad_lock.c.md#adf_lock_gaps), [adf_lock_reach](ad_lock.c.md#adf_lock_reach)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [adf_lock_free](ad_lock.c.md#adf_lock_free), [adf_unlock](ad_lock.c.md#adf_unlock)

Mentioned in the documentation of: [adf_unlock](ad_lock.c.md#adf_unlock)

### adf_unlock

```c
static void adf_unlock(struct adouble *ad, struct ad_fd *adf, const int fork)
```

Defined at lines 399 to 410.

Release every lock one fork holds on a fork descriptor.

The AFP specification releases all locks of a user when the user closes the fork. A user here is one OForkRefNum, the fork of lock[i].user. Only this fork's locks go. A sibling fork on the same shared [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) keeps its locks. The locks live in an array that [adf_freelock()](ad_lock.c.md#adf_freelock) compacts. The scan runs from the top down, so a removal moves only the entries above it, and re-reads the array pointer after each one, which realloc may have moved.

Parameters:
* `ad`: unused
* `adf`: fork descriptor whose lock array shrinks
* `fork`: OForkRefNum whose locks go

Calls: [adf_freelock](ad_lock.c.md#adf_freelock)

Called by: [ad_unlock](ad_lock.c.md#ad_unlock)

### adf_lock_init

```c
void adf_lock_init(struct ad_fd *adf)
```

Defined at lines 421 to 431.

Initialize lock tracking for an [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) structure.

Initializes the lock tracking fields in an [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) structure to their default empty state. This should be called when initializing a new [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) or after freeing all locks.

Parameters:
* `adf`: File descriptor structure to initialize

Called by: [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [adf_lock_free](ad_lock.c.md#adf_lock_free)

### adf_lock_free

```c
void adf_lock_free(struct ad_fd *adf)
```

Defined at lines 444 to 465.

Free all locks in an [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) structure.

Releases every lock in the [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) lock array, each over the bytes no other entry holds, and frees the array.

This function is called when closing file descriptors to ensure all locks are properly released and memory is freed.

Parameters:
* `adf`: File descriptor structure containing locks to free

Calls: [adf_freelock](ad_lock.c.md#adf_freelock), [adf_lock_init](ad_lock.c.md#adf_lock_init)

Called by: [ad_close](ad_flush.c.md#ad_close)

### adf_findlock

```c
static int adf_findlock(struct ad_fd *ad, const int fork, const int type, const off_t off, const off_t len)
```

Defined at lines 477 to 496.

Find a lock of one open fork that overlaps a byte range.

Parameters:
* `ad`: fork descriptor holding the lock array
* `fork`: reference number of the open fork that owns the lock
* `type`: ADLOCK_RD, ADLOCK_WR, or both
* `off`: start of the range
* `len`: length of the range

Returns: the index of the first matching lock, or -1

Calls: [OVERLAP](ad_lock.c.md#overlap), [adf_lock_window](ad_lock.c.md#adf_lock_window)

Called by: [ad_lock](ad_lock.c.md#ad_lock)

### adf_findxlock

```c
static int adf_findxlock(struct ad_fd *ad, const int fork, const int type, const off_t off, const off_t len)
```

Defined at lines 509 to 531.

Find a lock of any other open fork that overlaps a byte range.

Parameters:
* `ad`: fork descriptor holding the lock array
* `fork`: reference number of the open fork whose locks to skip
* `type`: ADLOCK_RD, ADLOCK_WR, or both
* `off`: start of the range
* `len`: length of the range

Returns: the index of the first matching lock, or -1

Calls: [OVERLAP](ad_lock.c.md#overlap), [adf_lock_window](ad_lock.c.md#adf_lock_window)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_tmplock](ad_lock.c.md#ad_tmplock)

### rf2off

```c
static off_t rf2off(off_t off)
```

Defined at lines 542 to 559.

Map a data fork band lock offset to the matching resource fork band offset.

Offsets outside the five data fork band offsets pass through unchanged.

Parameters:
* `off`: lock offset

Returns: the resource fork band offset, or off

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_testlock](ad_lock.c.md#ad_testlock)

Mentioned in the documentation of: [ad_testlock_range](ad_lock.c.md#ad_testlock_range)

### testlock

```c
static int testlock(const struct ad_fd *adf, off_t off, off_t len)
```

Defined at lines 574 to 605.

Test a lock.

1. Test against our own locks array
1. Test fcntl lock, locks from other processes

Parameters:
* `adf`: handle
* `off`: offset
* `len`: length

Returns: 1 if there's an existing lock, 0 if there's no lock, -1 in case any error occured

Calls: [OVERLAP](ad_lock.c.md#overlap), [set_lock](ad_lock.c.md#set_lock)

Called by: [ad_openforks](ad_lock.c.md#ad_openforks), [ad_testlock](ad_lock.c.md#ad_testlock)

Mentioned in the documentation of: [ad_testlock_range](ad_lock.c.md#ad_testlock_range)

### locktypetostr

```c
static const char * locktypetostr(int type)
```

Defined at lines 615 to 664.

Render an ADLOCK_* type mask as text, for log output.

Parameters:
* `type`: lock type mask

Returns: a static buffer holding "CLR", or the set bits as "RD", "WR", "UPG", and "FILELOCK" joined by "|"

Calls: [strlcat](../compat/strlcpy.c.md#strlcat)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_tmplock](ad_lock.c.md#ad_tmplock)

### ad_lock

```c
int ad_lock(struct adouble *ad, uint32_t eid, int locktype, off_t off, off_t len, int fork)
```

Defined at lines 701 to 865.

Take or release a byte-range or share-mode lock on a fork for one AFP fork.

Locks are process-wide fcntl locks, so the descriptor's array records the ranges each fork holds, in order of start, and is checked before the kernel. A write lock on a read-only descriptor degrades to a read lock, which still blocks a deny on the file. Resource fork ranges shift by the resource fork entry offset; with ADLOCK_FILELOCK they move into the data fork's share-mode band instead. BYTELOCK_MAX locks to the end of the file. Overlapping read locks of different forks are each recorded; the kernel merges them, and a release frees the bytes no other entry holds. A lock the kernel took but the array could not record is released again.

Parameters:
* `ad`: AppleDouble whose forks are open
* `eid`: ADEID_DFORK or ADEID_RFORK
* `locktype`: ADLOCK_RD, ADLOCK_WR, or ADLOCK_CLR, plus ADLOCK_UPGRADE or ADLOCK_FILELOCK
* `off`: first byte of the range, or the share-mode slot with ADLOCK_FILELOCK
* `len`: length of the range, BYTELOCK_MAX for the rest of the file, 1 with ADLOCK_FILELOCK
* `fork`: OForkRefNum the lock belongs to

Returns: 0, or -1 with errno EACCES for a conflicting lock of another fork; EINVAL for a range below byte zero, one the entry offset moves past the largest offset, one without length, a release of a range no entry of the fork holds as it is, an upgrade whose range differs from its entry's, or a take without ADLOCK_UPGRADE over bytes an entry of the fork already holds; EOVERFLOW for a take whose last byte passes the largest offset; else the errno of fcntl or realloc

Calls: [XLATE_FCNTL_LOCK](ad_lock.c.md#xlate_fcntl_lock), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [adf_findlock](ad_lock.c.md#adf_findlock), [adf_findxlock](ad_lock.c.md#adf_findxlock), [adf_freelock](ad_lock.c.md#adf_freelock), [adf_lock_fits](ad_lock.c.md#adf_lock_fits), [adf_lock_gaps](ad_lock.c.md#adf_lock_gaps), [adf_lock_lower](ad_lock.c.md#adf_lock_lower), [adf_lock_reach](ad_lock.c.md#adf_lock_reach), [locktypetostr](ad_lock.c.md#locktypetostr), [rf2off](ad_lock.c.md#rf2off), [set_lock](ad_lock.c.md#set_lock), [shmdstrfromoff](ad_lock.c.md#shmdstrfromoff)

Called by: [byte_lock](../../etc/afpd/fork.c.md#byte_lock), [fork_setmode](../../etc/afpd/fork.c.md#fork_setmode)

Mentioned in the documentation of: [ad_testlock_range](ad_lock.c.md#ad_testlock_range)

### ad_tmplock

```c
int ad_tmplock(struct adouble *ad, uint32_t eid, int locktype, off_t off, off_t len, int fork)
```

Defined at lines 893 to 966.

Take or release a temporary lock that the lock array does not record.

A temporary lock guards a read, a write, a truncate or the conversion of an AppleDouble file and goes away with its F_UNLCK. It covers only the bytes of its range no recorded entry holds, so the recorded ranges stay as they are. A missing resource fork counts as success before any check, and a write lock on a read-only descriptor degrades to a read lock. ADLOCK_FILELOCK locks the resource fork's AppleDouble file from byte zero instead of the fork's entry offset; the conversion takes it over the whole file with fork 0, on a descriptor that records no lock, so nothing is left out of it.

Parameters:
* `ad`: AppleDouble whose forks are open
* `eid`: ADEID_DFORK or ADEID_RFORK
* `locktype`: ADLOCK_RD, ADLOCK_WR, or ADLOCK_CLR, plus ADLOCK_FILELOCK
* `off`: first byte of the range
* `len`: length of the range, or 0 to the end of the file
* `fork`: OForkRefNum asking, or 0 to skip the check against other forks' locks

Returns: 0, or -1 with errno EINVAL for a range below byte zero, one whose start the resource fork's entry offset moves past the largest offset or one of negative length, EOVERFLOW for a last byte past the largest offset, EACCES when another fork holds a conflicting lock, or the errno of fcntl

Calls: [XLATE_FCNTL_LOCK](ad_lock.c.md#xlate_fcntl_lock), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [adf_findxlock](ad_lock.c.md#adf_findxlock), [adf_lock_fits](ad_lock.c.md#adf_lock_fits), [adf_lock_gaps](ad_lock.c.md#adf_lock_gaps), [locktypetostr](ad_lock.c.md#locktypetostr), [shmdstrfromoff](ad_lock.c.md#shmdstrfromoff)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams), [createfile_truncate](../../etc/afpd/file.c.md#createfile_truncate), [read_fork](../../etc/afpd/fork.c.md#read_fork), [write_fork](../../etc/afpd/fork.c.md#write_fork)

### ad_unlock

```c
void ad_unlock(struct adouble *ad, const int fork)
```

Defined at lines 975 to 988.

Release every lock one fork holds on the data and resource forks of an AppleDouble.

Parameters:
* `ad`: AppleDouble whose forks may be open
* `fork`: OForkRefNum whose locks go

Calls: [adf_unlock](ad_lock.c.md#adf_unlock)

Called by: [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

### ad_testlock

```c
int ad_testlock(struct adouble *ad, int eid, const off_t off)
```

Defined at lines 1000 to 1018.

Test for a share mode lock.

Parameters:
* `ad`: handle
* `eid`: datafork or resource fork
* `off`: sharemode lock to test

Returns: 1 if there's an existing lock, 0 if there's no lock, -1 in case any error occured

Calls: [rf2off](ad_lock.c.md#rf2off), [shmdstrfromoff](ad_lock.c.md#shmdstrfromoff), [testlock](ad_lock.c.md#testlock)

Called by: [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [fork_setmode](../../etc/afpd/fork.c.md#fork_setmode)

Mentioned in the documentation of: [ad_testlock_range](ad_lock.c.md#ad_testlock_range)

### ad_testlock_range

```c
int ad_testlock_range(struct adouble *ad, int eid, off_t off, off_t len)
```

Defined at lines 1059 to 1125.

GET-only range probe: does another process hold a conflicting lock?

Tests whether another process holds a conflicting lock on [off, off+len) of fork `eid`. Unlike [ad_testlock()](ad_lock.c.md#ad_testlock)/testlock() this does not scan our own adf_lock[] array first: it is kernel F_GETLK only. By POSIX, F_GETLK never reports the calling process's own locks, so this can be called through a fd of a fork we hold open without self-reporting our own band/byte entries — exactly the conflict read a delete needs (it must see only other holders). It never issues F_SETLK, so it has no acquire/release lifetime and cannot strand a lock.

Key behaviours:

* No adf_lock[] array scan — kernel F_GETLK only (no self-report).
* Always probes with an explicit F_WRLCK. A write probe conflicts with a peer's read or write lock, so it sees the F_RDLCK share-mode band entries. F_GETLK only tests, so the probe type is independent of the fd's O_RDWR/O_RDONLY mode, as in [testlock()](ad_lock.c.md#testlock).
* eid dispatch: ADEID_DFORK uses the data fd at the literal offset, which also covers the whole share-mode band (OPEN, DENY and RSRC mirrors), since the band lives on the data fd via [rf2off()](ad_lock.c.md#rf2off). ADEID_RFORK uses the resource fork's own fd (ad_rfp) at off + [ad_getentryoff()](ad_open.c.md#ad_getentryoff), matching [ad_lock()](ad_lock.c.md#ad_lock)'s rfork non-FILELOCK branch. Band offsets are never passed with ADEID_RFORK; a band offset is recognised as off >= AD_FILELOCK_BASE.
* Zone clamp on data-zone requests only (off < AD_FILELOCK_BASE): a len == 0 ("whole data zone") or over-long request is bounded to the data zone, never a POSIX l_len == 0 ("to infinity") that would sweep the share-mode band. An explicit band probe (off >= AD_FILELOCK_BASE) passes through unclamped.

Parameters:
* `ad`: handle
* `eid`: ADEID_DFORK (data + band) or ADEID_RFORK (rfork content)
* `off`: offset (content) or a band offset (>= AD_FILELOCK_BASE)
* `len`: length; 0 == whole data zone (data-zone requests only)

Returns: 1 = a conflicting lock exists (incl. F_GETLK EACCES/EAGAIN, reported as locked/indeterminate — fail safe, matching [testlock()](ad_lock.c.md#testlock)); 0 = no conflict (incl. no fd to probe); -1 = hard error (caller must refuse the operation, never proceed).

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [set_lock](ad_lock.c.md#set_lock), [shmdstrfromoff](ad_lock.c.md#shmdstrfromoff)

Called by: [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [rfork_cache_store_from_fd](../../etc/afpd/ad_cache.c.md#rfork_cache_store_from_fd)

### ad_testlock_whole

```c
int ad_testlock_whole(struct adouble *ad, int eid)
```

Defined at lines 1154 to 1174.

Probe with one F_GETLK whether another process holds any lock on a fork descriptor.

The negative-case fast path. One F_GETLK spans the entire descriptor of fork eid, unclamped: l_start 0 and l_len 0 run to infinity. On the data descriptor it covers the data content zone and the entire share-mode band, a superset of every ad_testlock_range dimension there. A clear result proves the descriptor holds nothing and lets the caller skip the per-dimension content and band probes. A hit forces the per-dimension resolution.

This is the one probe that must span the band, so unlike ad_testlock_range it is not zone-clamped. That is sound only because its sole use is the all-or-nothing fast path, never a per-dimension result. Like ad_testlock_range it asks only the kernel and probes with an explicit F_WRLCK so a peer's F_RDLCK band shows. It fails safe: F_GETLK failing with EACCES or EAGAIN counts as a lock.

Callers pass ADEID_DFORK. The resource fork descriptor carries only a content range and no band, so a whole-descriptor probe there would force the range probe anyway. ad_testlock_range reads it directly.

Parameters:
* `ad`: AppleDouble whose forks are open
* `eid`: ADEID_DFORK, or ADEID_RFORK

Returns: 1 when a lock exists, 0 when the descriptor is clear or absent, -1 on a hard error the caller fails closed on

Calls: [set_lock](ad_lock.c.md#set_lock)

Called by: [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks)

### ad_openforks

```c
uint16_t ad_openforks(struct adouble *ad, uint16_t attrbits)
```

Defined at lines 1188 to 1221.

Return if a file is open by another process.

Optimized for the common case:

* there's no locks held by another process (clients)
* or we already know the answer and don't need to test (attrbits)

Parameters:
* `ad`: handle
* `attrbits`: forks opened by us

Returns: bitflags ATTRBIT_DOPEN | ATTRBIT_ROPEN if other process has fork of file opened

Calls: [testlock](ad_lock.c.md#testlock)

Called by: [ad_open](ad_open.c.md#ad_open)

# Macros

* Undocumented: `ARRAY_BLOCK_SIZE`, `ARRAY_FREE_DELTA`, `LOCK_TOP`, `LTYPE2STRBUFSIZ`
