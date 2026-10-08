---
type: C Header File
title: "etc/atalkd/zip.h"
description: "1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/zip.h"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `sys/types.h`

# Included by

* [etc/atalkd/config.c](config.c.md)
* [etc/atalkd/main.c](main.c.md)
* [etc/atalkd/multicast.c](multicast.c.md)
* [etc/atalkd/multicast.h](multicast.h.md)
* [etc/atalkd/nbp.c](nbp.c.md)
* [etc/atalkd/rtmp.c](rtmp.c.md)
* [etc/atalkd/zip.c](zip.c.md)

# Types

### struct ziptab

Defined at line 11.
* `struct ziptab * zt_next`
* `struct ziptab * zt_prev`
* `unsigned char zt_len`
* `char * zt_name`
* `unsigned char * zt_bcast`
* `struct list * zt_rt`

# File-scope variables

`ziplast`
