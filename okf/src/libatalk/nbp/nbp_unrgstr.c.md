---
type: C Source File
title: "libatalk/nbp/nbp_unrgstr.c"
description: "NBP name removal through atalkd."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/nbp/nbp_unrgstr.c"
tags: ["libatalk/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 51 to 179. Declared in [include/atalk/nbp.h](../../include/atalk/nbp.h.md).

Remove an NBP name from atalkd.

The NBPOP_UNRGSTR request goes to the NBP port at addr, and the call waits 2 seconds for the first reply.

Parameters:
* `obj`: object name of up to NBPSTRLEN bytes, or NULL for an empty one
* `type`: type name of up to NBPSTRLEN bytes, or NULL for an empty one
* `zone`: zone name of up to NBPSTRLEN bytes, or NULL for an empty one
* `addr`: node that holds the name, or NULL

Returns: 0 when atalkd answers NBPOP_OK, or -1 for a name that is too long, no reply with ETIMEDOUT, or another answer

Calls: [netddp_close](../../include/atalk/netddp.h.md#netddp_close), [netddp_open](../netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [asp_cleanup](../../etc/afpd/main.c.md#asp_cleanup), [configinit](../../etc/afpd/afp_config.c.md#configinit), [main](../../bin/nbp/nbpunrgstr.c.md#main), [papd_cleanup](../../etc/papd/main.c.md#papd_cleanup)

Uses file-scope variables: `nbp_id` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_port` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_recv` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_send` in [libatalk/nbp/nbp_util.c](nbp_util.c.md)

# Macros

* Undocumented: `SOCKLEN_T`
