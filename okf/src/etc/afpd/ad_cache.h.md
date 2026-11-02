---
type: C Header File
title: "etc/afpd/ad_cache.h"
description: "AppleDouble metadata and resource fork cache interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/ad_cache.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-11-02T11:32:44+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `stdbool.h`, `sys/stat.h`

# Included by

* [etc/afpd/acls.c](acls.c.md)
* [etc/afpd/ad_cache.c](ad_cache.c.md)
* [etc/afpd/catsearch.c](catsearch.c.md)
* [etc/afpd/desktop.c](desktop.c.md)
* [etc/afpd/dircache.c](dircache.c.md)
* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/extattrs.c](extattrs.c.md)
* [etc/afpd/file.c](file.c.md)
* [etc/afpd/filedir.c](filedir.c.md)
* [etc/afpd/fork.c](fork.c.md)
* [etc/afpd/ofork.c](ofork.c.md)

# Functions

### rfork_stamp

```c
static struct timespec rfork_stamp(const struct stat *st)
```

Defined at lines 36 to 39.

The stamp a resource fork's stat gives its cached copy: its ctime.

Calls: [atalk_stat_ctime_timespec](../../include/atalk/compat.h.md#atalk_stat_ctime_timespec)

Called by: [fork_sidecar_measure](fork.c.md#fork_sidecar_measure), [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd), [rfork_size](fork.c.md#rfork_size)

Mentioned in the documentation of: [fork_sidecar_measure](fork.c.md#fork_sidecar_measure), [rfork_buf_holds](fork.c.md#rfork_buf_holds), [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd), [rfork_size](fork.c.md#rfork_size)

# Types

### struct ea_cache_stats

Defined at line 41.
* `unsigned long long hits`: names or a value served from an entry
* `unsigned long long misses`: lookups the entry could not serve
* `unsigned long long stored`: names lists and values stored
* `unsigned long long oversize`: lists over the cap, values out of room
* `unsigned long long dropped`: populated caches released
* `unsigned long long absent`: names a cached list proved missing
