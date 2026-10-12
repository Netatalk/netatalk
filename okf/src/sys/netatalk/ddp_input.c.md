---
type: C Source File
title: "sys/netatalk/ddp_input.c"
description: "DDP input: packet delivery and forwarding in the BSD kernel stack."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp_input.c"
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
* System headers: `ctype.h`, `net/if.h`, `net/route.h`, `sys/mbuf.h`, `sys/param.h`, `sys/socket.h`, `sys/socketvar.h`, `sys/types.h`

# Functions

### at_cksum

```c
unsigned short at_cksum()
```

Declared at sys/netatalk/ddp_input.c line 32; no definition in the scanned sources.

Called by: [ddp_input](ddp_input.c.md#ddp_input)

### atintr

```c
atintr()
```

Defined at lines 42 to 131.

Drain the AppleTalk input queues and pass each packet to ddp_input.

The phase 2 queue comes first, for interfaces with a phase 2 address. On the phase 1 queue, an interface needs a phase 1 address, and the call strips the 3-byte LLAP header. An extended DDP type then takes the long header path, and any other type the short one.

Calls: [ddp_input](ddp_input.c.md#ddp_input)

### ddp_input

```c
ddp_input(struct mbuf *m, struct ifnet *ifp, struct elaphdr *elh, int phase)
```

Defined at lines 156 to 398.

Deliver an incoming DDP packet to its socket, or forward it.

A short header, which comes with an LLAP header in phase 1, takes the nodes from the LLAP header. A long header carries full addresses and an optional checksum. The packet goes to a local address that matches the destination net and node, or ATADDR_BCAST on that net. The startup range 0xff00 to 0xfffe counts as local as well. The call trims link padding and drops packets shorter than their DDP length.

Without a local address, and with ddp_forward set, the packet goes out through ddp_route with one more hop. It does not when it reached DDP_MAXHOPS or the firewall refuses its route. A local packet whose checksum does not match goes away, and the rest lands in the receive buffer of the socket that ddp_search finds.

Parameters:
* `m`: packet
* `ifp`: receiving interface
* `elh`: LLAP header of a short header packet, or NULL
* `phase`: 1 or 2

Calls: [at_cksum](ddp_input.c.md#at_cksum), [ddp_route](ddp_output.c.md#ddp_route), [ddp_search](ddp_usrreq.c.md#ddp_search)

Called by: [atintr](ddp_input.c.md#atintr)

Uses file-scope variables: `ddp_cksum` in [sys/netatalk/ddp_output.c](ddp_output.c.md), `ddp_firewall`, `ddp_forward`, `forwro`

### m_printm

```c
m_printm(struct mbuf *m)
```

Defined at lines 405 to 411.

Print an mbuf chain as a hex dump, one bprint call per mbuf.

Parameters:
* `m`: first mbuf of the chain

Calls: [bprint](ddp_input.c.md#bprint)

### bprint

```c
bprint(char *data, int len)
```

Defined at lines 422 to 463.

Print a hex dump of a buffer with the kernel printf, 15 bytes a line, then an end line.

Print a hex dump of a buffer to stdout, 16 bytes a line.

Called by: [m_printm](ddp_input.c.md#m_printm)

# Macros

* Undocumented: `BPALEN`, `BPXLEN`

# File-scope variables

`ddp_firewall`, `ddp_forward`, `forwro`, `hexdig`
