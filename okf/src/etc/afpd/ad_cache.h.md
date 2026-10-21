---
type: C Header File
title: "etc/afpd/ad_cache.h"
description: "AppleDouble metadata and resource fork cache interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/ad_cache.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T20:46:18+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
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

# Types

### struct ea_cache_stats

Defined at line 34.
* `unsigned long long hits`: names or a value served from an entry
* `unsigned long long misses`: lookups the entry could not serve
* `unsigned long long stored`: names lists and values stored
* `unsigned long long oversize`: lists over the cap, values out of room
* `unsigned long long dropped`: populated caches released
* `unsigned long long absent`: names a cached list proved missing
