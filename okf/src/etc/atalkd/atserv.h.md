---
type: C Header File
title: "etc/atalkd/atserv.h"
description: "2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/atserv.h"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [etc/atalkd/aep.c](aep.c.md)
* [etc/atalkd/main.c](main.c.md)
* [etc/atalkd/nbp.c](nbp.c.md)
* [etc/atalkd/rtmp.c](rtmp.c.md)
* [etc/atalkd/zip.c](zip.c.md)

# Types

### struct atport

Defined at line 8.
* `int ap_fd`
* `struct atport * ap_next`
* `struct interface * ap_iface`
* `unsigned char ap_port`
* `int(* ap_packet`: Called through by [main](main.c.md#main).

### struct atserv

Defined at line 17.
* `char * as_name`
* `unsigned char as_port`
* `int(* as_packet`
