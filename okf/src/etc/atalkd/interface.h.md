---
type: C Header File
title: "etc/atalkd/interface.h"
description: "1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/interface.h"
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
* [etc/atalkd/nbp.c](nbp.c.md)
* [etc/atalkd/rtmp.c](rtmp.c.md)
* [etc/atalkd/zip.c](zip.c.md)

# Types

### struct interface

Defined at line 11.
* `struct interface * i_next`
* `char i_name`
* `int i_flags`
* `int i_time`
* `int i_group`
* `struct sockaddr_at i_addr`
* `struct sockaddr_at i_caddr`
* `struct ziptab * i_czt`
* `struct rtmptab * i_rt`
* `struct gate * i_gate`
* `struct atport * i_ports`

# Macros

* Undocumented: `IFACE_ADDR`, `IFACE_CONFIG`, `IFACE_DONTROUTE`, `IFACE_ERROR`, `IFACE_ISROUTER`, `IFACE_LOOP`, `IFACE_LOOPBACK`, `IFACE_NOROUTER`, `IFACE_PHASE1`, `IFACE_PHASE2`, `IFACE_RSEED`, `IFACE_SEED`, `IFBASE`, `LOOPIFACE`, `STABLE`, `STABLEANYWAY`, `UNSTABLE`
