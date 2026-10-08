---
type: C Source File
title: "libatalk/asp/asp_shutdown.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_shutdown.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 18 to 56. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Calls: [atp_rresp](../atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../atp/atp_sreq.c.md#atp_sreq)

Called by: [afp_asp_die_now](../../etc/afpd/afp_asp.c.md#afp_asp_die_now)
