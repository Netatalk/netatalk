---
type: C Source File
title: "sys/netatalk/ddp_output.c"
description: "3 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp_output.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [at.h](at.h.md)
* [at_var.h](at_var.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [ddp.h](ddp.h.md)
* [ddp_var.h](ddp_var.h.md)
* System headers: `errno.h`, `net/if.h`, `net/route.h`, `netinet/if_ether.h`, `netinet/in.h`, `sys/mbuf.h`, `sys/param.h`, `sys/socket.h`, `sys/types.h`

# Functions

### at_cksum

```c
unsigned short at_cksum()
```

Declared at sys/netatalk/ddp_output.c line 29; no definition in the scanned sources.

Called by: [ddp_output](ddp_output.c.md#ddp_output)

### ddp_output

```c
ddp_output(struct ddpcb *ddp, struct mbuf *m)
```

Defined at lines 32 to 93. Declared in [sys/netatalk/at_proto.c](at_proto.c.md).

Calls: [at_cksum](ddp_output.c.md#at_cksum), [ddp_route](ddp_output.c.md#ddp_route)

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

Uses file-scope variables: `ddp_cksum`

Dispatched via: [atalksw](at_proto.c.md#atalksw)

### ddp_route

```c
ddp_route(struct mbuf *m, struct route *ro)
```

Defined at lines 128 to 233.

Called by: [ddp_input](ddp_input.c.md#ddp_input), [ddp_output](ddp_output.c.md#ddp_output)

# Macros

* Undocumented: `align`

# File-scope variables

`ddp_cksum`
