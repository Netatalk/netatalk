---
type: C Source File
title: "libatalk/asp/asp_cmdreply.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_cmdreply.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* System headers: `string.h`, `sys/socket.h`, `sys/types.h`, `sys/uio.h`

# Functions

### asp_cmdreply

```c
int asp_cmdreply(ASP, int)
```

Defined at lines 36 to 82. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Calls: [atp_sresp](../atp/atp_sresp.c.md#atp_sresp)

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp)
