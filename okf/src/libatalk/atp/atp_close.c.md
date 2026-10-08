---
type: C Source File
title: "libatalk/atp/atp_close.c"
description: "1 function, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_close.c"
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
* System headers: `stdlib.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`

# Functions

### atp_close

```c
int atp_close(ATP)
```

Defined at lines 23 to 75. Declared in [include/atalk/atp.h](../../include/atalk/atp.h.md).

Calls: [atp_free_buf](atp_bufs.c.md#atp_free_buf), [atp_print_bufuse](atp_internals.h.md#atp_print_bufuse), [netddp_close](../../include/atalk/netddp.h.md#netddp_close)

Called by: [asp_close](../asp/asp_close.c.md#asp_close), [asp_getsession](../asp/asp_getsess.c.md#asp_getsession), [configfree](../../etc/afpd/afp_config.c.md#configfree), [configinit](../../etc/afpd/afp_config.c.md#configinit), [do_atp_lookup](../../bin/getzones/getzones.c.md#do_atp_lookup), [main](../../bin/pap/papstatus.c.md#main), [main](../../etc/papd/main.c.md#main)
