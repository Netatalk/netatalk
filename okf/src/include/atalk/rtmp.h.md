---
type: C Header File
title: "include/atalk/rtmp.h"
description: "2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/rtmp.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `stdint.h`

# Included by

* [bin/rtmpqry/rtmpqry.c](../../bin/rtmpqry/rtmpqry.c.md)
* [etc/atalkd/main.c](../../etc/atalkd/main.c.md)
* [etc/atalkd/rtmp.c](../../etc/atalkd/rtmp.c.md)

# Types

### struct rtmpent

Defined at line 33.
* `uint16_t re_net`
* `uint8_t re_hops`

### struct rtmprdhdr

Defined at line 41.
* `uint16_t rrdh_snet`
* `uint8_t rrdh_idlen`
* `uint8_t rrdh_id`

# Macros

* Undocumented: `RTMPHOPS_MAX`, `RTMPHOPS_POISON`, `RTMPROP_RDR`, `RTMPROP_RDR_NOSH`, `RTMPROP_REQUEST`
