---
type: C Source File
title: "libatalk/atp/atp_sresp.c"
description: "1 function, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_sresp.c"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/atp](../atp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atp_internals.h](atp_internals.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### atp_sresp

```c
int atp_sresp(ATP, struct atp_block *)
```

Defined at lines 53 to 176. Declared in [include/atalk/atp.h](../../include/atalk/atp.h.md).

send a transaction response

Parameters:
* `ah`: open atp handle
* `atpb`: parameter block

Calls: [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf), [atp_build_resp_packet](atp_packet.c.md#atp_build_resp_packet), [atp_free_buf](atp_bufs.c.md#atp_free_buf), [atp_print_bufuse](atp_internals.h.md#atp_print_bufuse), [bprint](../util/bprint.c.md#bprint), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [asp_close](../asp/asp_close.c.md#asp_close), [asp_cmdreply](../asp/asp_cmdreply.c.md#asp_cmdreply), [asp_getsession](../asp/asp_getsess.c.md#asp_getsession), [main](../../etc/papd/main.c.md#main), [send_file](../../bin/pap/pap.c.md#send_file), [session](../../etc/papd/session.c.md#session)
