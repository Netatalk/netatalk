---
type: C Source File
title: "libatalk/nbp/nbp_rgstr.c"
description: "NBP name registration through atalkd."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/nbp/nbp_rgstr.c"
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

### nbp_rgstr

```c
int nbp_rgstr(struct sockaddr_at *, const char *, const char *, const char *)
```

Defined at lines 53 to 183. Declared in [include/atalk/nbp.h](../../include/atalk/nbp.h.md).

Register an NBP name for a socket with the local atalkd.

A name that a lookup already finds fails with EADDRINUSE. The NBPOP_RGSTR request goes to the NBP port of the local node, and the call waits 2 seconds for the first reply. No reply sets ETIMEDOUT.

Parameters:
* `sat`: socket address to register
* `obj`: object name of up to NBPSTRLEN bytes, or NULL for an empty one
* `type`: type name of up to NBPSTRLEN bytes, or NULL for an empty one
* `zone`: zone name of up to NBPSTRLEN bytes, or NULL for the default zone `*`

Returns: 0 when atalkd answers NBPOP_OK, or -1 for a name in use or too long, no reply, or another answer

Calls: [nbp_lookup](nbp_lkup.c.md#nbp_lookup), [netddp_close](../../include/atalk/netddp.h.md#netddp_close), [netddp_open](../netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [configinit](../../etc/afpd/afp_config.c.md#configinit), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../etc/papd/main.c.md#main)

Uses file-scope variables: `nbp_id` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_port` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_recv` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_send` in [libatalk/nbp/nbp_util.c](nbp_util.c.md)

# Macros

* Undocumented: `SOCKLEN_T`
