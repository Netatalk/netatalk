---
type: C Header File
title: "etc/afpd/desktop.h"
description: "1 type, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/desktop.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/globals.h](../../include/atalk/globals.h.md)
* [volume.h](volume.h.md)

# Included by

* [etc/afpd/acls.c](acls.c.md)
* [etc/afpd/appl.c](appl.c.md)
* [etc/afpd/catsearch.c](catsearch.c.md)
* [etc/afpd/desktop.c](desktop.c.md)
* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/enumerate.c](enumerate.c.md)
* [etc/afpd/extattrs.c](extattrs.c.md)
* [etc/afpd/fce_api.c](fce_api.c.md)
* [etc/afpd/fce_util.c](fce_util.c.md)
* [etc/afpd/file.c](file.c.md)
* [etc/afpd/filedir.c](filedir.c.md)
* [etc/afpd/fork.c](fork.c.md)
* [etc/afpd/mangle.c](mangle.c.md)
* [etc/afpd/ofork.c](ofork.c.md)
* [etc/afpd/switch.c](switch.c.md)

# Types

### struct savedt

Defined at line 33.
* `uint8_t sdt_creator`
* `int sdt_fd`
* `int sdt_index`
* `short sdt_vid`

# Typedefs and enums

* `typedef unsigned char CreatorType`

# Macros

* Undocumented: `APPLEDESKTOP`
