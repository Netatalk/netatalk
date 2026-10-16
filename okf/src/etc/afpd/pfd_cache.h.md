---
type: C Header File
title: "etc/afpd/pfd_cache.h"
description: "Parent directory descriptor cache and statistics interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/pfd_cache.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
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

Defined at line 22.
* `unsigned long long hits`: fd served
* `unsigned long long opens`: slot fills (full path resolutions)
* `unsigned long long sync_refreshes`: dircache-learned mismatches, refilled
* `unsigned long long probe_refreshes`: re-ground probe mismatches, refilled
* `unsigned long long regrounds`: absolute-path probes issued
* `unsigned long long fallbacks`: fell back to full-path ostat
* `unsigned long long purges`: slots retired by purge hooks
* `unsigned long long path_repairs`: stale d_fullpath rebuilt from parent

# Macros

* Undocumented: `PFD_REGROUND_INTERVAL`
