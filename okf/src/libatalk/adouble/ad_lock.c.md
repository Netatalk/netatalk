---
type: C Source File
title: "libatalk/adouble/ad_lock.c"
description: "21 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_lock.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `inttypes.h`, `stdio.h`, `stdlib.h`, `string.h`

# Functions

### shmdstrfromoff

```c
static const char * shmdstrfromoff(off_t off)
```

Defined at lines 31 to 67.

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_testlock](ad_lock.c.md#ad_testlock), [ad_testlock_range](ad_lock.c.md#ad_testlock_range), [ad_tmplock](ad_lock.c.md#ad_tmplock), [set_lock](ad_lock.c.md#set_lock)

### set_lock

```c
static int set_lock(int fd, int cmd, struct flock *lock)
```

Defined at lines 70 to 93.

Calls: [shmdstrfromoff](ad_lock.c.md#shmdstrfromoff)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_testlock_range](ad_lock.c.md#ad_testlock_range), [ad_testlock_whole](ad_lock.c.md#ad_testlock_whole), [ad_tmplock](ad_lock.c.md#ad_tmplock), [adf_freelock](ad_lock.c.md#adf_freelock), [adf_relockrange](ad_lock.c.md#adf_relockrange), [testlock](ad_lock.c.md#testlock)

### XLATE_FCNTL_LOCK

```c
static int XLATE_FCNTL_LOCK(int type)
```

Defined at lines 96 to 110.

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_tmplock](ad_lock.c.md#ad_tmplock)

### OVERLAP

```c
static int OVERLAP(off_t a, off_t alen, off_t b, off_t blen)
```

Defined at lines 113 to 118.

Called by: [adf_findlock](ad_lock.c.md#adf_findlock), [adf_findxlock](ad_lock.c.md#adf_findxlock), [adf_relockrange](ad_lock.c.md#adf_relockrange), [testlock](ad_lock.c.md#testlock)

### adf_freelock

```c
static void adf_freelock(struct ad_fd *ad, const int i)
```

Defined at lines 130 to 192.

remove a lock and compact space if necessary

Calls: [adf_relockrange](ad_lock.c.md#adf_relockrange), [set_lock](ad_lock.c.md#set_lock)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [adf_unlock](ad_lock.c.md#adf_unlock)

### adf_unlock

```c
static void adf_unlock(struct adouble *ad, struct ad_fd *adf, const int fork)
```

Defined at lines 202 to 215.

Calls: [adf_freelock](ad_lock.c.md#adf_freelock)

Called by: [ad_unlock](ad_lock.c.md#ad_unlock)

### adf_lock_init

```c
void adf_lock_init(struct ad_fd *adf)
```

Defined at lines 226 to 236.

Initialize lock tracking for an [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) structure.

Initializes the lock tracking fields in an [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) structure to their default empty state. This should be called when initializing a new [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) or after freeing all locks.

Parameters:
* `adf`: File descriptor structure to initialize

Called by: [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [adf_lock_free](ad_lock.c.md#adf_lock_free)

### adf_lock_free

```c
void adf_lock_free(struct ad_fd *adf)
```

Defined at lines 250 to 283.

Free all locks in an [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) structure.

Iterates through all locks in the [ad_fd](../../include/atalk/adouble.h.md#struct-ad_fd) lock array, releasing fcntl locks and freeing memory. Manages refcounted locks properly by only freeing refcount memory when the last reference is released.

This function is called when closing file descriptors to ensure all locks are properly released and memory is freed.

Parameters:
* `adf`: File descriptor structure containing locks to free

Calls: [adf_lock_init](ad_lock.c.md#adf_lock_init)

Called by: [ad_close](ad_flush.c.md#ad_close)

### adf_relockrange

```c
static void adf_relockrange(struct ad_fd *ad, int fd, off_t off, off_t len)
```

Defined at lines 287 to 308.

relock any byte lock that overlaps off/len. unlock everything else.

Calls: [OVERLAP](ad_lock.c.md#overlap), [set_lock](ad_lock.c.md#set_lock)

Called by: [ad_tmplock](ad_lock.c.md#ad_tmplock), [adf_freelock](ad_lock.c.md#adf_freelock)

### adf_findlock

```c
static int adf_findlock(struct ad_fd *ad, const int fork, const int type, const off_t off, const off_t len)
```

Defined at lines 312 to 330.

find a byte lock that overlaps off/len for a particular open fork

Calls: [OVERLAP](ad_lock.c.md#overlap)

Called by: [ad_lock](ad_lock.c.md#ad_lock)

### adf_findxlock

```c
static int adf_findxlock(struct ad_fd *ad, const int fork, const int type, const off_t off, const off_t len)
```

Defined at lines 334 to 355.

search other fork lock lists

Calls: [OVERLAP](ad_lock.c.md#overlap)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_tmplock](ad_lock.c.md#ad_tmplock)

### rf2off

```c
static off_t rf2off(off_t off)
```

Defined at lines 373 to 390.

translate a resource fork lock to an offset

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_testlock](ad_lock.c.md#ad_testlock)

Mentioned in the documentation of: [ad_testlock_range](ad_lock.c.md#ad_testlock_range)

### testlock

```c
static int testlock(const struct ad_fd *adf, off_t off, off_t len)
```

Defined at lines 405 to 435.

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

Defined at lines 438 to 487.

Calls: [strlcat](../compat/strlcpy.c.md#strlcat)

Called by: [ad_lock](ad_lock.c.md#ad_lock), [ad_tmplock](ad_lock.c.md#ad_tmplock)

### ad_lock

```c
int ad_lock(struct adouble *ad, uint32_t eid, int locktype, off_t off, off_t len, int fork)
```

Defined at lines 493 to 689.

Calls: [XLATE_FCNTL_LOCK](ad_lock.c.md#xlate_fcntl_lock), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [adf_findlock](ad_lock.c.md#adf_findlock), [adf_findxlock](ad_lock.c.md#adf_findxlock), [adf_freelock](ad_lock.c.md#adf_freelock), [locktypetostr](ad_lock.c.md#locktypetostr), [rf2off](ad_lock.c.md#rf2off), [set_lock](ad_lock.c.md#set_lock), [shmdstrfromoff](ad_lock.c.md#shmdstrfromoff)

Called by: [byte_lock](../../etc/afpd/fork.c.md#byte_lock), [fork_setmode](../../etc/afpd/fork.c.md#fork_setmode)

Mentioned in the documentation of: [ad_testlock_range](ad_lock.c.md#ad_testlock_range)

### ad_tmplock

```c
int ad_tmplock(struct adouble *ad, uint32_t eid, int locktype, off_t off, off_t len, int fork)
```

Defined at lines 691 to 761.

Calls: [XLATE_FCNTL_LOCK](ad_lock.c.md#xlate_fcntl_lock), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [adf_findxlock](ad_lock.c.md#adf_findxlock), [adf_relockrange](ad_lock.c.md#adf_relockrange), [locktypetostr](ad_lock.c.md#locktypetostr), [set_lock](ad_lock.c.md#set_lock), [shmdstrfromoff](ad_lock.c.md#shmdstrfromoff)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams), [read_fork](../../etc/afpd/fork.c.md#read_fork), [write_fork](../../etc/afpd/fork.c.md#write_fork)

### ad_unlock

```c
void ad_unlock(struct adouble *ad, const int fork)
```

Defined at lines 764 to 777.

Calls: [adf_unlock](ad_lock.c.md#adf_unlock)

Called by: [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

### ad_testlock

```c
int ad_testlock(struct adouble *ad, int eid, const off_t off)
```

Defined at lines 789 to 807.

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

Defined at lines 849 to 915.

GET-only range probe: does another process hold a conflicting lock?

Tests whether another process holds a conflicting lock on [off, off+len) of fork `eid`. Unlike [ad_testlock()](ad_lock.c.md#ad_testlock)/testlock() this does not scan our own adf_lock[] array first: it is kernel F_GETLK only. By POSIX, F_GETLK never reports the calling process's own locks, so this can be called through a fd of a fork we hold open without self-reporting our own band/byte entries — exactly the conflict read a delete needs (it must see only other holders). It never issues F_SETLK, so it has no acquire/release lifetime and cannot strand a lock.

Key behaviours:

* No adf_lock[] array scan — kernel F_GETLK only (no self-report).
* Always probes with an explicit F_WRLCK. A write probe conflicts with a peer's read or write lock, so it sees the F_RDLCK share-mode band entries. F_GETLK only tests, so the probe type is independent of the fd's O_RDWR/O_RDONLY mode (cf. [testlock()](ad_lock.c.md#testlock), which picks F_RDLCK on an RO fd and would miss the band).
* eid dispatch: ADEID_DFORK uses the data fd at the literal offset, which also covers the whole share-mode band (OPEN, DENY and RSRC mirrors), since the band lives on the data fd via [rf2off()](ad_lock.c.md#rf2off). ADEID_RFORK uses the resource fork's own fd (ad_rfp) at off + [ad_getentryoff()](ad_open.c.md#ad_getentryoff), matching [ad_lock()](ad_lock.c.md#ad_lock)'s rfork non-FILELOCK branch. Band offsets are never passed with ADEID_RFORK; a band offset is recognised as off >= AD_FILELOCK_BASE.
* Zone clamp on data-zone requests only (off < AD_FILELOCK_BASE): a len == 0 ("whole data zone") or over-long request is bounded to the data zone, never a POSIX l_len == 0 ("to infinity") that would sweep the share-mode band. An explicit band probe (off >= AD_FILELOCK_BASE) passes through unclamped.

Parameters:
* `ad`: handle
* `eid`: ADEID_DFORK (data + band) or ADEID_RFORK (rfork content)
* `off`: offset (content) or a band offset (>= AD_FILELOCK_BASE)
* `len`: length; 0 == whole data zone (data-zone requests only)

Returns: 1 = a conflicting lock exists (incl. F_GETLK EACCES/EAGAIN, reported as locked/indeterminate — fail safe, matching [testlock()](ad_lock.c.md#testlock)); 0 = no conflict (incl. no fd to probe); -1 = hard error (caller must refuse the operation, never proceed).

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [set_lock](ad_lock.c.md#set_lock), [shmdstrfromoff](ad_lock.c.md#shmdstrfromoff)

Called by: [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks)

### ad_testlock_whole

```c
int ad_testlock_whole(struct adouble *ad, int eid)
```

Defined at lines 943 to 963.

GET-only whole-fd probe: does another process hold any lock on the fd?

The negative-case fast path. Issues one F_GETLK over the entire fd of fork `eid` — deliberately unclamped (l_start = 0, l_len = 0 = "to infinity"), so on the data fd it spans the data content zone and the whole share-mode band, a strict superset of every ad_testlock_range dimension on that fd. A clear (0) result proves the fd holds nothing, letting a caller skip the per-dimension content + per-bit band probes; a hit (1) forces the fidelity-preserving per-dimension resolution.

This is the one probe that must span the band, so unlike ad_testlock_range it is not zone-clamped. That is sound only because its sole use is the all-or-nothing fast path, never a per-dimension result. Like ad_testlock_range it is kernel F_GETLK only (no array scan), probes with an explicit F_WRLCK, and shares the same fail-safe contract.

Callers pass ADEID_DFORK: the rfork fd carries only a content range and no band, so a whole-fd probe there would collapse 1->1 and force the real range probe anyway (a net extra syscall) — the rfork is read with ad_testlock_range directly.

Returns: 1 = some lock exists (incl. F_GETLK EACCES/EAGAIN — fail safe); 0 = wholly clear (incl. no fd to probe); -1 = hard error (caller must fail closed).

Calls: [set_lock](ad_lock.c.md#set_lock)

Called by: [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks)

### ad_openforks

```c
uint16_t ad_openforks(struct adouble *ad, uint16_t attrbits)
```

Defined at lines 977 to 1010.

Return if a file is open by another process.

Optimized for the common case:

* there's no locks held by another process (clients)
* or we already know the answer and don't need to test (attrbits)

Parameters:
* `ad`: handle
* `attrbits`: forks opened by us

Returns: bitflags ATTRBIT_DOPEN | ATTRBIT_ROPEN if other process has fork of file opened

Calls: [testlock](ad_lock.c.md#testlock)

Called by: [ad_open](ad_open.c.md#ad_open), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

# Macros

* Undocumented: `ARRAY_BLOCK_SIZE`, `ARRAY_FREE_DELTA`, `LTYPE2STRBUFSIZ`
