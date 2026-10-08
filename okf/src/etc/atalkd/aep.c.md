---
type: C Source File
title: "etc/atalkd/aep.c"
description: "1 function, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/aep.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/aep.h](../../include/atalk/aep.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atserv.h](atserv.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `string.h`, `sys/socket.h`, `sys/types.h`

# Functions

### aep_packet

```c
int aep_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len)
```

Defined at lines 22 to 46.

Dispatched via: [atserv](main.c.md#atserv)
