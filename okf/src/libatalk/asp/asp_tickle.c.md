---
type: C Source File
title: "libatalk/asp/asp_tickle.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_tickle.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 20 to 39. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

send off a tickle

Calls: [atp_sreq](../atp/atp_sreq.c.md#atp_sreq)

Called by: [tickle_handler](asp_getsess.c.md#tickle_handler)
