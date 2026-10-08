---
type: C Header File
title: "etc/atalkd/rtmp.h"
description: "3 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/rtmp.h"
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
* [etc/atalkd/nbp.c](nbp.c.md)
* [etc/atalkd/route.c](route.c.md)
* [etc/atalkd/rtmp.c](rtmp.c.md)
* [etc/atalkd/zip.c](zip.c.md)

# Types

### struct rtmp_head

Defined at line 43.
* `unsigned short rh_net`
* `unsigned char rh_nodelen`
* `unsigned char rh_node`

### struct rtmp_tuple

Defined at line 49.
* `unsigned short rt_net`
* `unsigned char rt_dist`

### struct rtmptab

Defined at line 30.
* `struct rtmptab * rt_next`
* `struct rtmptab * rt_prev`
* `struct rtmptab * rt_inext`
* `struct rtmptab * rt_iprev`
* `unsigned short rt_firstnet`
* `unsigned short rt_lastnet`
* `unsigned char rt_hops`
* `unsigned char rt_state`
* `unsigned char rt_flags`
* `unsigned char rt_nzq`
* `struct gate * rt_gate`
* `struct list * rt_zt`
* `const struct interface * rt_iface`

# Macros

* Undocumented: `OS_STARTUP_FIRSTNET`, `RTMPTAB_BAD`, `RTMPTAB_EXTENDED`, `RTMPTAB_GOOD`, `RTMPTAB_HASZONES`, `RTMPTAB_PERM`, `RTMPTAB_ROUTE`, `RTMPTAB_SUSP1`, `RTMPTAB_SUSP2`, `RTMPTAB_ZIPQUERY`, `RTMP_ADD`, `RTMP_DEL`, `STARTUP_FIRSTNET`, `STARTUP_LASTNET`, `SZ_RTMPTUPLE`
