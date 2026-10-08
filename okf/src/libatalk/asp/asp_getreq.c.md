---
type: C Source File
title: "libatalk/asp/asp_getreq.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_getreq.c"
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

### asp_getrequest

```c
int asp_getrequest(ASP)
```

Defined at lines 48 to 100. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Calls: [atp_rreq_try](../atp/atp_rreq.c.md#atp_rreq_try)

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp)
