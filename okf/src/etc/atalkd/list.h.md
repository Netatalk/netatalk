---
type: C Header File
title: "etc/atalkd/list.h"
description: "1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/list.h"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [etc/atalkd/config.c](config.c.md)
* [etc/atalkd/main.c](main.c.md)
* [etc/atalkd/nbp.c](nbp.c.md)
* [etc/atalkd/rtmp.c](rtmp.c.md)
* [etc/atalkd/zip.c](zip.c.md)

# Types

### struct list

Defined at line 6.
* `void * l_data`
* `struct list * l_next`
* `struct list * l_prev`
