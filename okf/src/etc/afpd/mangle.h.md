---
type: C Header File
title: "etc/afpd/mangle.h"
description: "AFP filename mangling and demangling interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/mangle.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [directory.h](directory.h.md)
* [volume.h](volume.h.md)
* System headers: `stdlib.h`, `string.h`, `unistd.h`

# Included by

* [etc/afpd/desktop.c](desktop.c.md)
* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/mangle.c](mangle.c.md)
* [etc/afpd/volume.c](volume.c.md)

# Macros

* `MANGLE_LENGTH`: #ffffffff This really can't be changed.
* `MAX_EXT_LENGTH`: XXX This cannot be greater than 27.
* Undocumented: `MANGLE_CHAR`, `MAX_LENGTH`, `MAX_MANGLE_SUFFIX_LENGTH`
