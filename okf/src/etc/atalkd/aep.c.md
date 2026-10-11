---
type: C Source File
title: "etc/atalkd/aep.c"
description: "1 function, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/aep.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 35 to 59.

Answer an AppleTalk Echo Protocol request by sending it back as a reply.

The call turns the AEPOP_REQUEST byte into AEPOP_REPLY in place and sends the packet back to its sender.

Parameters:
* `ap`: AEP port of the interface
* `from`: sender of the request
* `data`: DDP payload, rewritten to the reply
* `len`: length of data

Returns: 0, or 1 for a malformed packet or a failed send

Dispatched via: [atserv](main.c.md#atserv)
