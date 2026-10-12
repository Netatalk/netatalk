---
type: C Source File
title: "libatalk/asp/asp_shutdown.c"
description: "ASP CloseSession requests to the workstation."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_shutdown.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `string.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### asp_shutdown

```c
int asp_shutdown(ASP)
```

Defined at lines 34 to 72. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Ask the workstation to close a session with an [ASP](../../include/atalk/asp.h.md#struct-asp) CloseSession request.

The request goes to the session socket of the workstation as an exactly once transaction, with 5 tries 2 seconds apart. The call waits for the reply.

Parameters:
* `asp`: session, its command buffer reused for the request

Returns: 0, or -1 when the request or the reply fails

Calls: [atp_rresp](../atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../atp/atp_sreq.c.md#atp_sreq)

Called by: [afp_asp_die_now](../../etc/afpd/afp_asp.c.md#afp_asp_die_now)
