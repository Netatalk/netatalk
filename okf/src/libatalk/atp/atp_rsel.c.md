---
type: C Source File
title: "libatalk/atp/atp_rsel.c"
description: "3 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_rsel.c"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/atp](../atp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atp_internals.h](atp_internals.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `signal.h`, `string.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### resend_request

```c
static int resend_request(ATP ah)
```

Defined at lines 37 to 71.

Calls: [bprint](../util/bprint.c.md#bprint), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [atp_rsel](atp_rsel.c.md#atp_rsel)

### atp_resp_incomplete

```c
static int atp_resp_incomplete(ATP ah)
```

Defined at lines 81 to 89.

Called by: [atp_rsel](atp_rsel.c.md#atp_rsel)

### atp_rsel

```c
int atp_rsel(ATP, struct sockaddr_at *, int)
```

Defined at lines 97 to 407. Declared in [include/atalk/atp.h](../../include/atalk/atp.h.md).

Receive ATP packets.

Parameters:
* `ah`: open atp handle
* `faddr`: address to receive from
* `func`: which function(s) to wait for; 0 means request or response

Calls: [at_addr_eq](atp_packet.c.md#at_addr_eq), [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf), [atp_free_buf](atp_bufs.c.md#atp_free_buf), [atp_print_bufuse](atp_internals.h.md#atp_print_bufuse), [atp_queue_push](atp_packet.c.md#atp_queue_push), [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_resp_incomplete](atp_rsel.c.md#atp_resp_incomplete), [bprint](../util/bprint.c.md#bprint), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto), [resend_request](atp_rsel.c.md#resend_request)

Called by: [atp_rreq_try](atp_rreq.c.md#atp_rreq_try), [atp_rresp](atp_rresp.c.md#atp_rresp), [main](../../etc/papd/main.c.md#main), [send_file](../../bin/pap/pap.c.md#send_file), [session](../../etc/papd/session.c.md#session)
