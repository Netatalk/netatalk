---
type: C Source File
title: "libatalk/asp/asp_init.c"
description: "2 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_init.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 48 to 74. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Allocate the [ASP](../../include/atalk/asp.h.md#struct-asp) server object on an ATP socket.

The object starts as a session listening socket with a wildcard peer address, no status, and no IPC descriptors.

Parameters:
* `atp`: ATP socket of the server

Returns: the server object, or NULL when calloc fails

Called by: [configinit](../../etc/afpd/afp_config.c.md#configinit)

### asp_setstatus

```c
void asp_setstatus(ASP, char *, const int)
```

Defined at lines 85 to 89. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Set the status block that status requests get.

The object keeps the pointer, so the block has to outlive it.

Parameters:
* `asp`: server object
* `status`: status block
* `slen`: length of status

Called by: [status_init](../../etc/afpd/status.c.md#status_init)
