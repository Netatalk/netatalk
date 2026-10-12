---
type: C Source File
title: "libatalk/asp/asp_tickle.c"
description: "ASP tickles."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_tickle.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `errno.h`, `string.h`, `sys/socket.h`, `sys/types.h`

# Functions

### asp_tickle

```c
int asp_tickle(ASP, const uint8_t, struct sockaddr_at *)
```

Defined at lines 33 to 52. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Send one [ASP](../../include/atalk/asp.h.md#struct-asp) tickle for a session, with a single try and no wait for a reply.

Parameters:
* `asp`: server object whose ATP socket sends the tickle
* `sid`: session ID
* `sat`: address of the workstation

Returns: 1, or 0 when the send fails

Calls: [atp_sreq](../atp/atp_sreq.c.md#atp_sreq)

Called by: [tickle_handler](asp_getsess.c.md#tickle_handler)
