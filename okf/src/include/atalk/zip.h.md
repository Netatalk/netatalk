---
type: C Header File
title: "include/atalk/zip.h"
description: "2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/zip.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [bin/getzones/getzones.c](../../bin/getzones/getzones.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/atalkd/main.c](../../etc/atalkd/main.c.md)
* [etc/atalkd/multicast.c](../../etc/atalkd/multicast.c.md)
* [etc/atalkd/zip.c](../../etc/atalkd/zip.c.md)

# Types

### struct ziphdr

Defined at line 30.
* `uint8_t zh_op`
* `uint8_t zh_cnt`

### struct zipreplent

Defined at line 38.
* `uint16_t zre_net`
* `uint8_t zre_zonelen`

# Macros

* Undocumented: `MAX_ZONE_LENGTH`, `ZIPGNI_INVALID`, `ZIPGNI_ONEZONE`, `ZIPGNI_USEBROADCAST`, `ZIPOP_BRINGUP`, `ZIPOP_EREPLY`, `ZIPOP_GETLOCALZONES`, `ZIPOP_GETMYZONE`, `ZIPOP_GETZONELIST`, `ZIPOP_GNI`, `ZIPOP_GNIREPLY`, `ZIPOP_NOTIFY`, `ZIPOP_QUERY`, `ZIPOP_REPLY`, `ZIPOP_TAKEDOWN`, `zh_count`, `zh_flags`, `zh_zero`
