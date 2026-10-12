---
type: C Source File
title: "libatalk/asp/asp_close.c"
description: "Closing an ASP session in reply to the CloseSession request of the workstation."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_close.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `stdlib.h`, `string.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### asp_close

```c
int asp_close(ASP)
```

Defined at lines 52 to 74. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Answer the pending close request of a session, close its ATP socket, and free it.

Parameters:
* `asp`: session, freed on return

Returns: 0, or -1 when the reply or the close fails

Calls: [atp_close](../atp/atp_close.c.md#atp_close), [atp_sresp](../atp/atp_sresp.c.md#atp_sresp)

Called by: [afp_asp_close](../../etc/afpd/afp_asp.c.md#afp_asp_close), [configinit](../../etc/afpd/afp_config.c.md#configinit)
