---
type: C Source File
title: "etc/afpd/pfd_cache.c"
description: "12 functions, 1 type, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/pfd_cache.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 90 to 99.

Called by: [pfd_get](pfd_cache.c.md#pfd_get)

Uses file-scope variables: `pfd_initialized`, `pfd_slots`

### slot_retire

```c
static void slot_retire(struct pfd_slot *s)
```

Defined at lines 101 to 111.

Called by: [pfd_get](pfd_cache.c.md#pfd_get), [pfd_purge](pfd_cache.c.md#pfd_purge), [pfd_purge_vol](pfd_cache.c.md#pfd_purge_vol), [pfd_shutdown](pfd_cache.c.md#pfd_shutdown)

### slot_fill

```c
static int slot_fill(struct pfd_slot *s, const struct vol *vol, const struct dir *parent)
```

Defined at lines 115 to 139.

Called by: [pfd_get](pfd_cache.c.md#pfd_get)

Uses file-scope variables: `pfd_stat`

### slot_move_to_front

```c
static void slot_move_to_front(int i)
```

Defined at lines 141 to 148.

Called by: [pfd_get](pfd_cache.c.md#pfd_get)

Uses file-scope variables: `pfd_slots`

### pfd_get

```c
static int pfd_get(const struct vol *vol, const struct dir *parent)
```

Defined at lines 152 to 231.

Calls: [ostat](../../libatalk/util/unix.c.md#ostat), [pfd_init_once](pfd_cache.c.md#pfd_init_once), [slot_fill](pfd_cache.c.md#slot_fill), [slot_move_to_front](pfd_cache.c.md#slot_move_to_front), [slot_retire](pfd_cache.c.md#slot_retire)

Called by: [pfd_ostat](pfd_cache.c.md#pfd_ostat)

Uses file-scope variables: `pfd_slots`, `pfd_stat`

### pfd_repair_path

```c
static void pfd_repair_path(struct dir *entry, const struct dir *parent)
```

Defined at lines 245 to 274.

Calls: [fullpath_join_blk](directory.c.md#fullpath_join_blk)

Called by: [pfd_ostat](pfd_cache.c.md#pfd_ostat)

Uses file-scope variables: `pfd_stat`

### pfd_ostat

```c
int pfd_ostat(const struct vol *vol, struct dir *entry, struct stat *st, int options)
```

Defined at lines 288 to 324.

ostat(cfrombstr(entry->d_fullpath), st, options), accelerated.

Resolves entry's parent to a cached dir fd and fstatat()s the leaf name; any impediment (no parent entry, ghost parent, open failure) falls back to the full-path ostat internally. Callers cannot observe which path ran: return value and errno are exactly what ostat would produce.

On success the entry's d_fullpath is additionally repaired against the parent's — the relative stat can outlive a rename the path string hasn't caught up with (see pfd_repair_path).

Calls: [dircache_lookup_parent](dircache.c.md#dircache_lookup_parent), [ostat](../../libatalk/util/unix.c.md#ostat), [ostatat](../../libatalk/util/unix.c.md#ostatat), [pfd_get](pfd_cache.c.md#pfd_get), [pfd_repair_path](pfd_cache.c.md#pfd_repair_path)

Called by: [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name)

Uses file-scope variables: `pfd_stat`

### pfd_purge

```c
void pfd_purge(uint16_t vid, cnid_t did)
```

Defined at lines 330 to 344.

Calls: [slot_retire](pfd_cache.c.md#slot_retire)

Called by: [dir_modify](directory.c.md#dir_modify), [dircache_remove](dircache.c.md#dircache_remove), [process_cache_hints](dircache.c.md#process_cache_hints)

Uses file-scope variables: `pfd_initialized`, `pfd_slots`, `pfd_stat`

### pfd_purge_vol

```c
void pfd_purge_vol(uint16_t vid)
```

Defined at lines 346 to 358.

Calls: [slot_retire](pfd_cache.c.md#slot_retire)

Called by: [closevol](volume.c.md#closevol)

Uses file-scope variables: `pfd_initialized`, `pfd_slots`, `pfd_stat`

### pfd_shutdown

```c
void pfd_shutdown(void)
```

Defined at lines 360 to 369.

Calls: [slot_retire](pfd_cache.c.md#slot_retire)

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

Uses file-scope variables: `pfd_initialized`, `pfd_slots`

### pfd_stats_get

```c
void pfd_stats_get(struct pfd_stats *out)
```

Defined at lines 371 to 374.

Uses file-scope variables: `pfd_stat`

### pfd_log_stats

```c
void pfd_log_stats(void)
```

Defined at lines 376 to 388.

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
