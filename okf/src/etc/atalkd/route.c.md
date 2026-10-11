---
type: C Source File
title: "etc/atalkd/route.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/route.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 33 to 41.

Add or delete a kernel route with an ioctl on the route socket rtfd.

Parameters:
* `message`: RTMP_ADD or RTMP_DEL, which are SIOCADDRT and SIOCDELRT here
* `dst`: destination address
* `gate`: gateway address
* `flags`: route flags, RTF_UP with RTF_GATEWAY or RTF_HOST

Returns: the ioctl result, 0 or -1

Called by: [gateroute](rtmp.c.md#gateroute), [looproute](rtmp.c.md#looproute)

Uses file-scope variables: `rtfd` in [etc/atalkd/main.c](main.c.md)

Mentioned in the documentation of: [gateroute](rtmp.c.md#gateroute), [looproute](rtmp.c.md#looproute)
