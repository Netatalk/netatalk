---
type: C Header File
title: "etc/afpd/pfd_cache.h"
description: "1 type, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/pfd_cache.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/directory.h](../../include/atalk/directory.h.md)
* [volume.h](volume.h.md)
* System headers: `sys/stat.h`

# Included by

* [etc/afpd/afp_dsi.c](afp_dsi.c.md)
* [etc/afpd/dircache.c](dircache.c.md)
* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/pfd_cache.c](pfd_cache.c.md)
* [etc/afpd/volume.c](volume.c.md)

# Types

### struct pfd_stats

Defined at line 17.
* `unsigned long long hits`
* `unsigned long long opens`
* `unsigned long long sync_refreshes`
* `unsigned long long probe_refreshes`
* `unsigned long long regrounds`
* `unsigned long long fallbacks`
* `unsigned long long purges`
* `unsigned long long path_repairs`

# Macros

* Undocumented: `PFD_REGROUND_INTERVAL`
