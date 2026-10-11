---
type: C Header File
title: "etc/afpd/virtual_icon.h"
description: "includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/virtual_icon.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/globals.h](../../include/atalk/globals.h.md)
* [volume.h](volume.h.md)

# Included by

* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/enumerate.c](enumerate.c.md)
* [etc/afpd/filedir.c](filedir.c.md)
* [etc/afpd/fork.c](fork.c.md)
* [etc/afpd/ofork.c](ofork.c.md)
* [etc/afpd/virtual_icon.c](virtual_icon.c.md)
* [etc/afpd/volume.c](volume.c.md)

# Macros

* `RFORK_DATA_LEN_OFF`: Offset within the resource fork header of the data-length field.
* `RFORK_HEADER_SIZE`: Size of the classic Mac resource fork header (data offset, map offset, data length, map length — four big-endian uint32s).
* `VIRTUAL_ICON_CNID`: Reserved CNID for the virtual Icon file (below CNID_START=17).
* `VIRTUAL_ICON_NAME`: The Mac filename: "Icon" followed by carriage return (0x0D).
