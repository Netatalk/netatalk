---
type: C Source File
title: "libatalk/nbp/nbp_lkup.c"
description: "2 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/nbp/nbp_lkup.c"
tags: ["libatalk/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/nbp](../nbp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [nbp_conf.h](nbp_conf.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [netatalk/ddp.h](../../sys/netatalk/ddp.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `netdb.h`, `signal.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`

# Functions

### nbp_lookup

```c
int nbp_lookup(const char *, const char *, const char *, struct nbpnve *, const int, const struct at_addr *)
```

Defined at lines 34 to 39. Declared in [include/atalk/nbp.h](../../include/atalk/nbp.h.md).

Calls: [nbp_do_lookup_op](nbp_lkup.c.md#nbp_do_lookup_op)

Called by: [main](../../bin/aecho/aecho.c.md#main), [main](../../bin/pap/pap.c.md#main), [main](../../bin/pap/papstatus.c.md#main), [nbp_rgstr](nbp_rgstr.c.md#nbp_rgstr)

### nbp_do_lookup_op

```c
int nbp_do_lookup_op(const char *, const char *, const char *, struct nbpnve *, const int, const struct at_addr *, const struct at_addr *, uint8_t)
```

Defined at lines 41 to 255. Declared in [include/atalk/nbp.h](../../include/atalk/nbp.h.md).

Calls: [nbp_match](nbp_util.c.md#nbp_match), [nbp_parse](nbp_util.c.md#nbp_parse), [netddp_close](../../include/atalk/netddp.h.md#netddp_close), [netddp_open](../netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [main](../../bin/nbp/nbplkup.c.md#main), [nbp_lookup](nbp_lkup.c.md#nbp_lookup)

Uses file-scope variables: `nbp_id` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_port` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_recv` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_send` in [libatalk/nbp/nbp_util.c](nbp_util.c.md)

# Macros

* Undocumented: `SOCKLEN_T`
