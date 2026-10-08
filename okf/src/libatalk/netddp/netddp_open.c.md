---
type: C Source File
title: "libatalk/netddp/netddp_open.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/netddp/netddp_open.c"
tags: ["libatalk/netddp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/netddp](../netddp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `stdio.h`, `string.h`, `sys/socket.h`, `sys/types.h`

# Functions

### netddp_open

```c
int netddp_open(struct sockaddr_at *, struct sockaddr_at *)
```

Defined at lines 22 to 56. Declared in [include/atalk/netddp.h](../../include/atalk/netddp.h.md).

Called by: [atp_open](../atp/atp_open.c.md#atp_open), [do_getnetinfo](../../bin/getzones/getzones.c.md#do_getnetinfo), [do_query](../../bin/getzones/getzones.c.md#do_query), [main](../../bin/aecho/aecho.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [nbp_do_lookup_op](../nbp/nbp_lkup.c.md#nbp_do_lookup_op), [nbp_rgstr](../nbp/nbp_rgstr.c.md#nbp_rgstr), [nbp_unrgstr](../nbp/nbp_unrgstr.c.md#nbp_unrgstr), [setup_ddp_socket](../../bin/rtmpqry/rtmpqry.c.md#setup_ddp_socket)
