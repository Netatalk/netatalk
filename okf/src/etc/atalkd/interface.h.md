---
type: C Header File
title: "etc/atalkd/interface.h"
description: "1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/interface.h"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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
* `int i_group`: for isolated appletalk domains
* `struct sockaddr_at i_addr`
* `struct sockaddr_at i_caddr`
* `struct ziptab * i_czt`
* `struct rtmptab * i_rt`
* `struct gate * i_gate`
* `struct atport * i_ports`

# Macros

* `IFACE_ADDR`: has an address set
* `IFACE_CONFIG`: has been configured
* `IFACE_DONTROUTE`: don't route this interface
* `IFACE_ERROR`: sendto returned an error
* `IFACE_ISROUTER`: act as a router.
* `IFACE_LOOP`: has a loopback route
* `IFACE_LOOPBACK`: is the loopback interface
* `IFACE_NOROUTER`: no router on interface
* `IFACE_RSEED`: almost the same as seed.
* `IFACE_SEED`: act as seed
* `IFBASE`: base number of interfaces
* Undocumented: `IFACE_PHASE1`, `IFACE_PHASE2`, `LOOPIFACE`, `STABLE`, `STABLEANYWAY`, `UNSTABLE`
