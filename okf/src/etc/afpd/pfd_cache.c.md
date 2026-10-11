---
type: C Source File
title: "etc/afpd/pfd_cache.c"
description: "12 functions, 1 type, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/pfd_cache.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [pfd_cache.h](pfd_cache.h.md)
* [volume.h](volume.h.md)
* System headers: `errno.h`, `fcntl.h`, `string.h`, `sys/stat.h`, `unistd.h`

# Functions

### pfd_init_once

```c
static void pfd_init_once(void)
```

Defined at lines 93 to 102.

Mark every slot of the parent descriptor cache empty on first use.

Called by: [pfd_get](pfd_cache.c.md#pfd_get)

Uses file-scope variables: `pfd_initialized`, `pfd_slots`

### slot_retire

```c
static void slot_retire(struct pfd_slot *s)
```

Defined at lines 109 to 119.

Close the descriptor of a cache slot and mark the slot empty.

Parameters:
* `s`: slot to retire

Called by: [pfd_get](pfd_cache.c.md#pfd_get), [pfd_purge](pfd_cache.c.md#pfd_purge), [pfd_purge_vol](pfd_cache.c.md#pfd_purge_vol), [pfd_shutdown](pfd_cache.c.md#pfd_shutdown)

### slot_fill

```c
static int slot_fill(struct pfd_slot *s, const struct vol *vol, const struct dir *parent)
```

Defined at lines 134 to 158.

Open the directory of a dircache entry into a cache slot and record its identity.

The slot remembers the device and inode of the opened directory. It also keeps the dcache_ino of the entry at fill time, so later lookups can tell which side learned of a replacement.

Parameters:
* `s`: slot to fill
* `vol`: volume of the directory
* `parent`: dircache entry of the directory

Returns: the descriptor, or -1 with the slot left empty

Called by: [pfd_get](pfd_cache.c.md#pfd_get)

Uses file-scope variables: `pfd_stat`

### slot_move_to_front

```c
static void slot_move_to_front(int i)
```

Defined at lines 165 to 172.

Move slot i to position 0, the front of the use order.

Parameters:
* `i`: slot index

Called by: [pfd_get](pfd_cache.c.md#pfd_get)

Uses file-scope variables: `pfd_slots`

### pfd_get

```c
static int pfd_get(const struct vol *vol, const struct dir *parent)
```

Defined at lines 189 to 268.

Directory descriptor of a dircache entry from the slot cache.

A hit first tests whether the dcache_ino of the entry changed since the fill. An unchanged value serves the slot. A value that matches the slot inode only re-baselines, and any other value refills the slot. Every 256 hits, the PFD_REGROUND_INTERVAL, a full-path stat catches an external rename-away and recreate of the directory. A miss fills an empty slot or evicts the slot at the end of the use order. Only the main thread calls it.

Parameters:
* `vol`: volume of the directory
* `parent`: dircache entry of the directory

Returns: the descriptor, or -1 when the open failed and the caller falls back to the full path

Calls: [ostat](../../libatalk/util/unix.c.md#ostat), [pfd_init_once](pfd_cache.c.md#pfd_init_once), [slot_fill](pfd_cache.c.md#slot_fill), [slot_move_to_front](pfd_cache.c.md#slot_move_to_front), [slot_retire](pfd_cache.c.md#slot_retire)

Called by: [pfd_ostat](pfd_cache.c.md#pfd_ostat)

Uses file-scope variables: `pfd_slots`, `pfd_stat`

### pfd_repair_path

```c
static void pfd_repair_path(struct dir *entry, const struct dir *parent)
```

Defined at lines 284 to 313.

Rebuild the full path of a dircache entry from its parent after a relative stat.

The relative stat proved that the parent descriptor and the name reach the live object. An internal rename re-paths the parent at once but defers its children, so a child can still carry the old path. When the stored path does not equal the parent path plus the name, the call replaces it. The intact case costs one length compare, one byte load, and one short memcmp. When the allocation fails, the stale path stays for the next validation.

Parameters:
* `entry`: dircache entry whose path to check
* `parent`: dircache entry of its parent

Calls: [fullpath_join_blk](directory.c.md#fullpath_join_blk)

Called by: [pfd_ostat](pfd_cache.c.md#pfd_ostat)

Uses file-scope variables: `pfd_stat`

### pfd_ostat

```c
int pfd_ostat(const struct vol *vol, struct dir *entry, struct stat *st, int options)
```

Defined at lines 331 to 367.

Stat the full path of a dircache entry through the cached parent descriptor.

The result and errno match ostat on the full path. The call stats the name relative to the cached descriptor of the parent. It falls back to the full path when the parent has no usable entry or the open fails. A successful relative stat also repairs the stored path of the entry through pfd_repair_path.

Parameters:
* `vol`: volume of the entry
* `entry`: dircache entry to stat
* `st`: receives the stat
* `options`: ostat options, O_NOFOLLOW to stat a symlink itself

Returns: 0, or -1 with errno set

Calls: [dircache_lookup_parent](dircache.c.md#dircache_lookup_parent), [ostat](../../libatalk/util/unix.c.md#ostat), [ostatat](../../libatalk/util/unix.c.md#ostatat), [pfd_get](pfd_cache.c.md#pfd_get), [pfd_repair_path](pfd_cache.c.md#pfd_repair_path)

Called by: [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `pfd_stat`

### pfd_purge

```c
void pfd_purge(uint16_t vid, cnid_t did)
```

Defined at lines 379 to 393.

Retire the cache slot of a directory.

The lookup uses the raw volume ID, because dircache_remove purges from the d_vid of the entry and its callers can pass no volume. Both the main thread and the idle worker call it, never at the same time.

Parameters:
* `vid`: volume ID, as stored in struct dir
* `did`: directory ID

Calls: [slot_retire](pfd_cache.c.md#slot_retire)

Called by: [dir_modify](directory.c.md#dir_modify), [dircache_remove](dircache.c.md#dircache_remove), [process_cache_hints](dircache.c.md#process_cache_hints)

Uses file-scope variables: `pfd_initialized`, `pfd_slots`, `pfd_stat`

### pfd_purge_vol

```c
void pfd_purge_vol(uint16_t vid)
```

Defined at lines 400 to 412.

Retire every cache slot of a volume, as the volume closes.

Parameters:
* `vid`: volume ID, as stored in struct dir

Calls: [slot_retire](pfd_cache.c.md#slot_retire)

Called by: [closevol](volume.c.md#closevol)

Uses file-scope variables: `pfd_initialized`, `pfd_slots`, `pfd_stat`

### pfd_shutdown

```c
void pfd_shutdown(void)
```

Defined at lines 417 to 426.

Close every cached descriptor when the session ends.

Calls: [slot_retire](pfd_cache.c.md#slot_retire)

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

Uses file-scope variables: `pfd_initialized`, `pfd_slots`

### pfd_stats_get

```c
void pfd_stats_get(struct pfd_stats *out)
```

Defined at lines 433 to 436.

Snapshot of the cache counters, for the unit tests.

Parameters:
* `out`: receives the counters

Uses file-scope variables: `pfd_stat`

### pfd_log_stats

```c
void pfd_log_stats(void)
```

Defined at lines 444 to 456.

Log the cache counters at the end of the session.

Refresh counters that approach the hit count mean that the slots rebuild on every call.

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

Uses file-scope variables: `pfd_stat`

# Types

### struct pfd_slot

Defined at line 66.
* `cnid_t did`
* `uint16_t vid`
* `int fd`
* `dev_t dev`
* `ino_t ino`
* `ino_t fill_dcache_ino`
* `uint32_t uses`

# Macros

* Undocumented: `PFD_OPEN_FLAGS`, `PFD_SLOTS`

# File-scope variables

`pfd_initialized`, `pfd_slots`, `pfd_stat`
