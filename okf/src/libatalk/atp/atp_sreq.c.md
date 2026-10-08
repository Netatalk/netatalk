---
type: C Source File
title: "libatalk/atp/atp_sreq.c"
description: "1 function, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_sreq.c"
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
* System headers: `errno.h`, `netinet/in.h`, `signal.h`, `stdlib.h`, `string.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### atp_sreq

```c
int atp_sreq(ATP, struct atp_block *, int, uint8_t)
```

Defined at lines 56 to 148. Declared in [include/atalk/atp.h](../../include/atalk/atp.h.md).

Send ATP transaction service request.

Parameters:
* `ah`: open atp handle
* `atpb`: parameter block
* `respcount`: buffers available for response
* `flags`: ATP_XO, ATP_TREL

Calls: [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf), [atp_build_req_packet](atp_packet.c.md#atp_build_req_packet), [atp_free_buf](atp_bufs.c.md#atp_free_buf), [atp_print_bufuse](atp_internals.h.md#atp_print_bufuse), [bprint](../util/bprint.c.md#bprint), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [asp_attention](../asp/asp_attn.c.md#asp_attention), [asp_shutdown](../asp/asp_shutdown.c.md#asp_shutdown), [asp_tickle](../asp/asp_tickle.c.md#asp_tickle), [asp_wrtcont](../asp/asp_write.c.md#asp_wrtcont), [do_atp_lookup](../../bin/getzones/getzones.c.md#do_atp_lookup), [getstatus](../../bin/pap/papstatus.c.md#getstatus), [main](../../bin/pap/pap.c.md#main), [send_file](../../bin/pap/pap.c.md#send_file), [session](../../etc/papd/session.c.md#session)
