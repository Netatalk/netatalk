---
type: C Source File
title: "libatalk/asp/asp_init.c"
description: "2 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_init.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `stdio.h`, `stdlib.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### asp_init

```c
ASP asp_init(ATP)
```

Defined at lines 39 to 65. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Called by: [configinit](../../etc/afpd/afp_config.c.md#configinit)

### asp_setstatus

```c
void asp_setstatus(ASP, char *, const int)
```

Defined at lines 67 to 71. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Called by: [status_init](../../etc/afpd/status.c.md#status_init)
