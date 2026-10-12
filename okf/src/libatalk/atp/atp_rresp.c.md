---
type: C Source File
title: "libatalk/atp/atp_rresp.c"
description: "Receiving ATP responses."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_rresp.c"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/atp](../atp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atp_internals.h](atp_internals.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### atp_rresp

```c
int atp_rresp(ATP, struct atp_block *)
```

Defined at lines 66 to 123. Declared in [include/atalk/atp.h](../../include/atalk/atp.h.md).

Wait for the complete response to the pending ATP request and hand out its packets.

The data of each response packet, without its ATP header, goes into the response vector entry of its sequence number. In atpb, atp_saddr names the address to accept. The vector atp_rresiov holds 1 to 8 entries with their sizes and receives the data and the lengths, and atp_rresiovcnt receives the packet count.

Parameters:
* `ah`: ATP endpoint
* `atpb`: parameter block

Returns: 0, the atp_rsel result other than ATP_TRESP, or -1 with EINVAL or EMSGSIZE

Calls: [atp_free_buf](atp_bufs.c.md#atp_free_buf), [atp_print_bufuse](atp_internals.h.md#atp_print_bufuse), [atp_rsel](atp_rsel.c.md#atp_rsel), [bprint](../util/bprint.c.md#bprint)

Called by: [asp_attention](../asp/asp_attn.c.md#asp_attention), [asp_shutdown](../asp/asp_shutdown.c.md#asp_shutdown), [asp_wrtcont](../asp/asp_write.c.md#asp_wrtcont), [do_atp_lookup](../../bin/getzones/getzones.c.md#do_atp_lookup), [getstatus](../../bin/pap/papstatus.c.md#getstatus), [main](../../bin/pap/pap.c.md#main), [send_file](../../bin/pap/pap.c.md#send_file), [session](../../etc/papd/session.c.md#session)
