---
type: C Source File
title: "sys/netatalk/aarp.c"
description: "10 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/aarp.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [aarp.h](aarp.h.md)
* [at.h](at.h.md)
* [at_var.h](at_var.h.md)
* [ddp_var.h](ddp_var.h.md)
* [phase2.h](phase2.h.md)
* System headers: `net/af.h`, `net/if.h`, `net/route.h`, `netinet/if_ether.h`, `netinet/in.h`, `sys/kernel.h`, `sys/mbuf.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `syslog.h`

# Functions

### aarptimer

```c
void aarptimer(void)
```

Defined at lines 79 to 100.

Age the AARP table once a minute and free the entries that ran out.

A complete entry lives for AARPT_KILLC minutes without use and an incomplete one for AARPT_KILLI minutes. Permanent entries stay. The call schedules itself again.

Calls: [aarptfree](aarp.c.md#aarptfree)

Called by: [aarp_clean](aarp.c.md#aarp_clean), [aarptnew](aarp.c.md#aarptnew)

### at_ifawithnet

```c
struct ifaddr * at_ifawithnet(struct sockaddr_at *sat, struct ifaddr *ifa)
```

Defined at lines 112 to 145.

Find the AppleTalk address of an interface whose net range holds the net of an address.

On BSD 4.4 systems the net has to equal the net of the interface address.

Parameters:
* `sat`: address to place
* `ifa`: first entry of the address list of the interface

Returns: the interface address, or NULL

Called by: [aarpresolve](aarp.c.md#aarpresolve), [aarpwhohas](aarp.c.md#aarpwhohas), [at_aarpinput](aarp.c.md#at_aarpinput)

### aarpwhohas

```c
void aarpwhohas(struct arpcom *ac, struct sockaddr_at *sat)
```

Defined at lines 159 to 244.

Send an AARP request for an AppleTalk address out of an Ethernet interface.

A phase 2 net gets the request at the AppleTalk multicast address, behind an 802.2 SNAP header and with full net.node addresses. A phase 1 net gets it at ff:ff:ff:ff:ff:ff with node numbers only. Without an address of the interface on that net, nothing goes out.

Parameters:
* `ac`: Ethernet interface
* `sat`: address to resolve

Calls: [at_ifawithnet](aarp.c.md#at_ifawithnet)

Called by: [aarpresolve](aarp.c.md#aarpresolve)

Uses file-scope variables: `aarp_org_code`, `atmulticastaddr`, `etherbroadcastaddr`

### aarpresolve

```c
int aarpresolve(struct arpcom *ac, struct mbuf *m, struct sockaddr_at *destsat, unsigned char *desten)
```

Defined at lines 264 to 327.

Find the Ethernet address of an AppleTalk destination for an outgoing packet.

A destination that at_broadcast accepts maps to the AppleTalk multicast address in phase 2 and to ff:ff:ff:ff:ff:ff in phase 1. A complete AARP entry gives its Ethernet address. Otherwise the entry holds the packet, in place of one held before, and an AARP request goes out. A bucket without a free entry panics.

Parameters:
* `ac`: Ethernet interface
* `m`: packet to send, which the entry holds or the call frees when it returns 0
* `destsat`: AppleTalk destination
* `desten`: receives the Ethernet address

Returns: 1 with desten filled in, or 0 when the packet waits for the reply or goes away

Calls: [aarptnew](aarp.c.md#aarptnew), [aarpwhohas](aarp.c.md#aarpwhohas), [at_broadcast](at_control.c.md#at_broadcast), [at_ifawithnet](aarp.c.md#at_ifawithnet)

Uses file-scope variables: `atmulticastaddr`, `etherbroadcastaddr`

### aarpinput

```c
void aarpinput(struct arpcom *ac, struct mbuf *m)
```

Defined at lines 339 to 377.

Check an incoming AARP packet and pass an AppleTalk one to at_aarpinput.

An interface with IFF_NOARP, another hardware type, or a short packet drops the packet.

Parameters:
* `ac`: Ethernet interface
* `m`: packet, which the call frees or passes on

Calls: [at_aarpinput](aarp.c.md#at_aarpinput)

### at_aarpinput

```c
void at_aarpinput(struct arpcom *ac, struct mbuf *m)
```

Defined at lines 396 to 597.

Process an AppleTalk AARP packet: learn the sender, defend the local address, and answer.

Packets from the local hardware address or from ff:ff:ff:ff:ff:ff go away. A packet that claims the local address wakes a probe in progress, so it picks another address, and otherwise logs a duplicate address. The sender gets a complete cache entry, and a held packet for it goes out. A probe for the address of the sender frees its entry.

A request or probe for the local address gets a response, except while the interface probes.

Parameters:
* `ac`: Ethernet interface
* `m`: packet, reused for the response or freed

Calls: [aarpprobe](aarp.c.md#aarpprobe), [aarptfree](aarp.c.md#aarptfree), [aarptnew](aarp.c.md#aarptnew), [at_ifawithnet](aarp.c.md#at_ifawithnet)

Called by: [aarpinput](aarp.c.md#aarpinput)

Uses file-scope variables: `aarp_org_code`, `etherbroadcastaddr`

### aarptfree

```c
void aarptfree(struct aarptab *aat)
```

Defined at lines 604 to 614.

Free the held packet of an AARP entry and clear the entry.

Parameters:
* `aat`: AARP entry

Called by: [aarptimer](aarp.c.md#aarptimer), [aarptnew](aarp.c.md#aarptnew), [at_aarpinput](aarp.c.md#at_aarpinput)

### aarptnew

```c
struct aarptab * aarptnew(struct at_addr *addr)
```

Defined at lines 626 to 665.

Take an AARP entry for an address from its hash bucket.

A free entry comes first, and otherwise the oldest entry that is not permanent gives way. The first call starts aarptimer.

Parameters:
* `addr`: AppleTalk address

Returns: the entry, in use, or NULL when the bucket holds only permanent entries

Calls: [aarptfree](aarp.c.md#aarptfree), [aarptimer](aarp.c.md#aarptimer)

Called by: [aarpresolve](aarp.c.md#aarpresolve), [at_aarpinput](aarp.c.md#at_aarpinput)

### aarpprobe

```c
void aarpprobe(struct arpcom *ac)
```

Defined at lines 676 to 776.

Send the next AARP probe for the address that an interface probes for.

Each call sends one probe and schedules the next one 200 ms later, until aa_probcnt runs out. The call then clears AFA_PROBING and wakes the waiting caller.

Parameters:
* `ac`: Ethernet interface

Called by: [at_aarpinput](aarp.c.md#at_aarpinput)

Uses file-scope variables: `aarp_org_code`, `atmulticastaddr`, `etherbroadcastaddr`

### aarp_clean

```c
void aarp_clean(void)
```

Defined at lines 781 to 792.

Stop aarptimer and free the packets that AARP entries hold.

Calls: [aarptimer](aarp.c.md#aarptimer)

# Macros

* Undocumented: `AARPTAB_BSIZ`, `AARPTAB_HASH`, `AARPTAB_LOOK`, `AARPTAB_NB`, `AARPTAB_SIZE`, `AARPT_AGE`, `AARPT_KILLC`, `AARPT_KILLI`

# File-scope variables

`aarp_org_code`, `aarptab`, `aarptab_size`, `at_org_code`, `atmulticastaddr`, `etherbroadcastaddr`
