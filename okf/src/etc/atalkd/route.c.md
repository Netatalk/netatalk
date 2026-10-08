---
type: C Source File
title: "etc/atalkd/route.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/route.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [route.h](route.h.md)
* [rtmp.h](rtmp.h.md)
* System headers: `net/route.h`, `string.h`, `sys/ioctl.h`, `sys/param.h`, `sys/socket.h`, `sys/types.h`, `unistd.h`

# Functions

### route

```c
int route(int message, struct sockaddr *dst, struct sockaddr *gate, int flags)
```

Defined at lines 23 to 31.

Called by: [gateroute](rtmp.c.md#gateroute), [looproute](rtmp.c.md#looproute)

Uses file-scope variables: `rtfd` in [etc/atalkd/main.c](main.c.md)
