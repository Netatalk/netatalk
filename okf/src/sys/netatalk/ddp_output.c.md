---
type: C Source File
title: "sys/netatalk/ddp_output.c"
description: "DDP output and routing in the BSD kernel stack."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp_output.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [at.h](at.h.md)
* [at_var.h](at_var.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [ddp.h](ddp.h.md)
* [ddp_var.h](ddp_var.h.md)
* System headers: `errno.h`, `net/if.h`, `net/route.h`, `netinet/if_ether.h`, `netinet/in.h`, `sys/mbuf.h`, `sys/param.h`, `sys/socket.h`, `sys/types.h`

# Functions

### at_cksum

```c
unsigned short at_cksum()
```

Declared at sys/netatalk/ddp_output.c line 34; no definition in the scanned sources.

Called by: [ddp_output](ddp_output.c.md#ddp_output)

### ddp_output

```c
ddp_output(struct ddpcb *ddp, struct mbuf *m)
```

Defined at lines 49 to 110. Declared in [sys/netatalk/at_proto.c](at_proto.c.md).

Send a packet from a DDP socket to its connected address.

The call prepends a long DDP header with 0 hops. The header names the foreign address of the socket as the destination and the local address as the source. With ddp_cksum set the header carries a checksum. Then ddp_route sends the packet along the route of the socket.

Parameters:
* `ddp`: DDP socket control block
* `m`: data

Returns: 0, ENOBUFS, or the ddp_route error

Calls: [at_cksum](ddp_output.c.md#at_cksum), [ddp_route](ddp_output.c.md#ddp_route)

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

Uses file-scope variables: `ddp_cksum`

Dispatched via: [atalksw](at_proto.c.md#atalksw)

### ddp_route

```c
ddp_route(struct mbuf *m, struct route *ro)
```

Defined at lines 169 to 274.

Send a DDP packet out of the interface of a route.

The route needs an interface with an AppleTalk address whose range holds the gateway net. A phase 2 interface pulls up to MLEN bytes of the packet into its first mbuf, so the header starts on a word boundary. A phase 1 interface puts an LLAP header in front, with the destination node on a local net and the gateway node otherwise. The link layer address is the destination on a local net, or the gateway.

Parameters:
* `m`: packet with its DDP header, freed on an error
* `ro`: route, its use count raised

Returns: the if_output result, EINVAL without a usable route, or ENOBUFS

Called by: [ddp_input](ddp_input.c.md#ddp_input), [ddp_output](ddp_output.c.md#ddp_output)

# Macros

* Undocumented: `align`

# File-scope variables

`ddp_cksum`
