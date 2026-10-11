---
type: C Source File
title: "libatalk/atp/atp_packet.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_packet.c"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 111 to 129. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Build an ATP request packet: the DDP type, the ATP header, and the request data.

Parameters:
* `pktbuf`: buffer that receives the packet and its length
* `tid`: transaction ID
* `ctrl`: control information, ATP_TREQ with its flags
* `atpb`: parameter block with the bitmap and the request data

Called by: [atp_sreq](atp_sreq.c.md#atp_sreq)

### atp_build_resp_packet

```c
void atp_build_resp_packet(struct atpbuf *, uint16_t, uint8_t, struct atp_block *, uint8_t)
```

Defined at lines 142 to 163. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Build one ATP response packet from the response vector entry with a sequence number.

Parameters:
* `pktbuf`: buffer that receives the packet and its length
* `tid`: transaction ID of the request
* `ctrl`: control information, ATP_TRESP with ATP_EOM on the last packet
* `atpb`: parameter block with the response vector
* `seqnum`: sequence number, the index into the response vector

Called by: [atp_sresp](atp_sresp.c.md#atp_sresp)

### atp_queue_push

```c
void atp_queue_push(ATP, struct atpbuf *)
```

Defined at lines 179 to 205. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Push a packet onto the queue of unmatched received packets, dropping the oldest one when the queue is full.

Only a matching receive or atp_close removes an entry, so a stale response or traffic for another node can stay in the queue. The queue therefore holds no more than ATP_MAXQUEUE packets and drops the oldest one. It never drops the packet just received, which a caller may still wait for. Both callers push through this call, so they agree on the packet to drop.

Parameters:
* `ah`: ATP endpoint
* `buf`: received packet, owned by the queue afterwards

Calls: [atp_free_buf](atp_bufs.c.md#atp_free_buf)

Called by: [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rsel](atp_rsel.c.md#atp_rsel)

### atp_recv_atp

```c
int atp_recv_atp(ATP, struct sockaddr_at *, uint8_t *, uint16_t, char *, int)
```

Defined at lines 229 to 409. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Receive an ATP packet of a function and transaction ID from an address, from the queue or the socket.

The queue of unmatched packets comes first. On the socket, a packet without function bits goes away, and a TREL frees the saved response of its transaction. Any other packet that does not match joins the queue. A wildcard address, ATP_TIDANY, or more than one function bit widens the match.

Parameters:
* `ah`: ATP endpoint
* `fromaddr`: address to accept, receives the sender
* `func`: function bits to accept, receives the function of the packet
* `tid`: transaction ID, or ATP_TIDANY
* `rbuf`: buffer of ATP_BUFSIZ bytes that receives the packet
* `wait`: nonzero to read until a packet matches

Returns: the packet length, 0 without a match when not waiting, or -1 when a read or an allocation fails

Calls: [at_addr_eq](atp_packet.c.md#at_addr_eq), [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf), [atp_free_buf](atp_bufs.c.md#atp_free_buf), [atp_print_bufuse](atp_internals.h.md#atp_print_bufuse), [atp_queue_push](atp_packet.c.md#atp_queue_push), [bprint](../util/bprint.c.md#bprint), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom)

Called by: [atp_rreq_try](atp_rreq.c.md#atp_rreq_try), [atp_rsel](atp_rsel.c.md#atp_rsel)

### at_addr_eq

```c
int at_addr_eq(struct sockaddr_at *, struct sockaddr_at *)
```

Defined at lines 421 to 435. Declared in [libatalk/atp/atp_internals.h](atp_internals.h.md).

Compare two AppleTalk addresses, with wildcards in the first.

ATADDR_ANYPORT, ATADDR_ANYNET, and ATADDR_ANYNODE in paddr match any value.

Parameters:
* `paddr`: address with possible wildcards
* `saddr`: address to test

Returns: nonzero when they match, or 0

Called by: [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rsel](atp_rsel.c.md#atp_rsel)

# Macros

* Undocumented: `SOCKLEN_T`
