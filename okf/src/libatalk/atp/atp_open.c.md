---
type: C Source File
title: "libatalk/atp/atp_open.c"
description: "1 function, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_open.c"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/atp](../atp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [atp_internals.h](atp_internals.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `atalk/ddp.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `unistd.h`

# Functions

### atp_open

```c
ATP atp_open(uint8_t, const struct at_addr *)
```

Defined at lines 47 to 85. Declared in [include/atalk/atp.h](../../include/atalk/atp.h.md).

Calls: [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf), [netddp_close](../../include/atalk/netddp.h.md#netddp_close), [netddp_open](../netddp/netddp_open.c.md#netddp_open)

Called by: [asp_getsession](../asp/asp_getsess.c.md#asp_getsession), [configinit](../../etc/afpd/afp_config.c.md#configinit), [do_atp_lookup](../../bin/getzones/getzones.c.md#do_atp_lookup), [main](../../bin/pap/pap.c.md#main), [main](../../bin/pap/papstatus.c.md#main), [main](../../etc/papd/main.c.md#main)
