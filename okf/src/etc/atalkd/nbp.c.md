---
type: C Source File
title: "etc/atalkd/nbp.c"
description: "2 functions, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/nbp.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atserv.h](atserv.h.md)
* [gate.h](gate.h.md)
* [interface.h](interface.h.md)
* [list.h](list.h.md)
* [multicast.h](multicast.h.md)
* [nbp.h](nbp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [rtmp.h](rtmp.h.md)
* [zip.h](zip.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `net/if.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/param.h`, `sys/socket.h`, `sys/types.h`

# Functions

### nbp_ack

```c
static void nbp_ack(int fd, int nh_op, int nh_id, struct sockaddr_at *to)
```

Defined at lines 43 to 59.

Called by: [nbp_packet](nbp.c.md#nbp_packet)

### nbp_packet

```c
int nbp_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len)
```

Defined at lines 61 to 685. Declared in [etc/atalkd/nbp.h](nbp.h.md).

Calls: [addmulti](multicast.c.md#addmulti), [nbp_ack](nbp.c.md#nbp_ack), [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp), [zone_bcast](multicast.c.md#zone_bcast)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md), `nbptab`, `transition` in [etc/atalkd/main.c](main.c.md)

Dispatched via: [atserv](main.c.md#atserv)

# File-scope variables

`nbptab`
