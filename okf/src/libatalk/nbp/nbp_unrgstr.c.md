---
type: C Source File
title: "libatalk/nbp/nbp_unrgstr.c"
description: "1 function, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/nbp/nbp_unrgstr.c"
tags: ["libatalk/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/nbp](../nbp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [nbp_conf.h](nbp_conf.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `netdb.h`, `signal.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`

# Functions

### nbp_unrgstr

```c
int nbp_unrgstr(const char *, const char *, const char *, const struct at_addr *)
```

Defined at lines 32 to 160. Declared in [include/atalk/nbp.h](../../include/atalk/nbp.h.md).

Calls: [netddp_close](../../include/atalk/netddp.h.md#netddp_close), [netddp_open](../netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [asp_cleanup](../../etc/afpd/main.c.md#asp_cleanup), [configinit](../../etc/afpd/afp_config.c.md#configinit), [main](../../bin/nbp/nbpunrgstr.c.md#main), [papd_cleanup](../../etc/papd/main.c.md#papd_cleanup)

Uses file-scope variables: `nbp_id` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_port` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_recv` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_send` in [libatalk/nbp/nbp_util.c](nbp_util.c.md)

# Macros

* Undocumented: `SOCKLEN_T`
