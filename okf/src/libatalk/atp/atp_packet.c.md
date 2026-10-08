---
type: C Source File
title: "libatalk/atp/atp_packet.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_packet.c"
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
* System headers: `atalk/ddp.h`, `netinet/in.h`, `string.h`, `sys/param.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### atp_build_req_packet

```c
void atp_build_req_packet(struct atpbuf *, uint16_t, uint8_t, struct atp_block *)
```

Defined at lines 102 to 120. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Called by: [atp_sreq](atp_sreq.c.md#atp_sreq)

### atp_build_resp_packet

```c
void atp_build_resp_packet(struct atpbuf *, uint16_t, uint8_t, struct atp_block *, uint8_t)
```

Defined at lines 122 to 143. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Called by: [atp_sresp](atp_sresp.c.md#atp_sresp)

### atp_queue_push

```c
void atp_queue_push(ATP, struct atpbuf *)
```

Defined at lines 158 to 184. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Calls: [atp_free_buf](atp_bufs.c.md#atp_free_buf)

Called by: [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rsel](atp_rsel.c.md#atp_rsel)

### atp_recv_atp

```c
int atp_recv_atp(ATP, struct sockaddr_at *, uint8_t *, uint16_t, char *, int)
```

Defined at lines 188 to 368. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Calls: [at_addr_eq](atp_packet.c.md#at_addr_eq), [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf), [atp_free_buf](atp_bufs.c.md#atp_free_buf), [atp_print_bufuse](atp_internals.h.md#atp_print_bufuse), [atp_queue_push](atp_packet.c.md#atp_queue_push), [bprint](../util/bprint.c.md#bprint), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom)

Called by: [atp_rreq_try](atp_rreq.c.md#atp_rreq_try), [atp_rsel](atp_rsel.c.md#atp_rsel)

### at_addr_eq

```c
int at_addr_eq(struct sockaddr_at *, struct sockaddr_at *)
```

Defined at lines 371 to 385. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Called by: [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rsel](atp_rsel.c.md#atp_rsel)

# Macros

* Undocumented: `SOCKLEN_T`
