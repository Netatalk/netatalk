---
type: C Source File
title: "libatalk/atp/atp_rreq.c"
description: "2 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_rreq.c"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/atp](../atp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atp_internals.h](atp_internals.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `netinet/in.h`, `stdlib.h`, `string.h`, `sys/time.h`, `sys/types.h`, `unistd.h`

# Functions

### atp_rreq_try

```c
int atp_rreq_try(ATP, struct atp_block *)
```

Defined at lines 64 to 129. Declared in [include/atalk/atp.h](../../include/atalk/atp.h.md).

1 got, 0 none yet, -1 error

1 got, 0 none yet, -1 error

A caller that saw the socket turn readable never blocks here and keeps control of its wait. A readable socket does not promise a request, since the datagram may belong to another transaction or another node. The waiting form, atp_rreq, loops over this call.

The endpoint records the transaction ID of a request and, for an exactly once request, its release time. In atpb, atp_saddr names the address to accept and receives the sender. The atp_rreqdata buffer and its size in atp_rreqdlen receive the data and its length, and atp_bitmap the bitmap.

Parameters:
* `ah`: ATP endpoint
* `atpb`: parameter block

Returns: 1 for a request, 0 for a datagram without a new request, or -1 for an error, EMSGSIZE when the data does not fit

Calls: [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf), [atp_free_buf](atp_bufs.c.md#atp_free_buf), [atp_print_bufuse](atp_internals.h.md#atp_print_bufuse), [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rsel](atp_rsel.c.md#atp_rsel)

Called by: [asp_getrequest](../asp/asp_getreq.c.md#asp_getrequest), [atp_rreq](atp_rreq.c.md#atp_rreq)

### atp_rreq

```c
int atp_rreq(ATP, struct atp_block *)
```

Defined at lines 138 to 147. Declared in [include/atalk/atp.h](../../include/atalk/atp.h.md).

Wait for an ATP request, reading datagrams until one arrives.

Parameters:
* `ah`: ATP endpoint
* `atpb`: parameter block, filled as by atp_rreq_try

Returns: 0 for a request, or -1 for an error

Calls: [atp_rreq_try](atp_rreq.c.md#atp_rreq_try)

Called by: [asp_getsession](../asp/asp_getsess.c.md#asp_getsession), [main](../../etc/papd/main.c.md#main), [send_file](../../bin/pap/pap.c.md#send_file), [session](../../etc/papd/session.c.md#session)
