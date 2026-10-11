---
type: C Source File
title: "sys/netatalk/at_control.c"
description: "7 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/at_control.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [aarp.h](aarp.h.md)
* [at.h](at.h.md)
* [at_var.h](at_var.h.md)
* [phase2.h](phase2.h.md)
* System headers: `errno.h`, `net/af.h`, `net/if.h`, `net/route.h`, `netinet/if_ether.h`, `netinet/in.h`, `sys/ioctl.h`, `sys/kernel.h`, `sys/mbuf.h`, `sys/param.h`, `sys/socket.h`, `sys/socketvar.h`, `sys/systm.h`, `sys/types.h`, `sys/user.h`

# Functions

### atalk_hash

```c
void atalk_hash(struct sockaddr_at *sat, struct afhash *hp)
```

Defined at lines 47 to 52.

Compute the route hashes of an AppleTalk address.

The net hash is the net, and the host hash is the net shifted by 8 bits plus the node.

Parameters:
* `sat`: AppleTalk address
* `hp`: receives the hashes

### atalk_netmatch

```c
int atalk_netmatch(struct sockaddr_at *sat1, struct sockaddr_at *sat2)
```

Defined at lines 65 to 81.

Match the net of an address against the net or net range of another.

When sat1 is the address of a local interface, any net in the range of that interface matches. That way ifa_ifwithnet() works without an address entry for each net of the range.

Parameters:
* `sat1`: address, possibly of a local interface
* `sat2`: address to test

Returns: nonzero for a match, or 0

### at_control

```c
at_control(int cmd, caddr_t data, struct ifnet *ifp)
```

Defined at lines 103 to 342.

Run an AppleTalk interface ioctl: set or read the address of a phase.

SIOCSIFADDR, for the super-user only, takes the phase from the net range in the address. It creates the AppleTalk address of that phase on the interface, or deletes the routes of the existing one, then runs at_ifinit. A new address joins [at_ifaddr](at_var.h.md#struct-at_ifaddr) behind the first entry, unless the first entry is loopback, so the first address stays a real interface.

SIOCGIFADDR returns the address of the phase. BSD 4.4 systems also add and delete addresses with SIOCAIFADDR and SIOCDIFADDR. Any other request goes to the ioctl of the interface.

Parameters:
* `cmd`: ioctl request
* `data`: struct ifreq with the address
* `ifp`: interface

Returns: 0, or an errno value such as EPERM, ENOBUFS, EADDRNOTAVAIL, or EOPNOTSUPP

Calls: [at_ifinit](at_control.c.md#at_ifinit), [at_scrub](at_control.c.md#at_scrub)

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_scrub

```c
at_scrub(struct ifnet *ifp, struct at_ifaddr *aa)
```

Defined at lines 354 to 406.

Delete the routes of an AppleTalk interface address.

Loopback has a host route. A net range of 0 to fffe uses the default route, and any other range has one route per net.

Parameters:
* `ifp`: interface
* `aa`: interface address, without AFA_ROUTE afterwards

Returns: 0, or the rtinit error on BSD 4.4 systems

Called by: [aa_clean](at_control.c.md#aa_clean), [at_control](at_control.c.md#at_control)

### at_ifinit

```c
at_ifinit(struct ifnet *ifp, struct at_ifaddr *aa, struct sockaddr_at *sat)
```

Defined at lines 431 to 635.

Give an AppleTalk interface address its net range and a free address, then add its routes.

The net range comes from the netrange in the address. Loopback takes the address as given. Any other interface probes with AARP, from the requested net and node or from a choice based on the time. Each try runs a series of 10 probes 200 ms apart. A conflict moves on to the next node, then to the next net of the range.

After the ioctl of the interface accepts the address, loopback gets a host route. Phase 1 gets the net route, and phase 2 the default route for the range 0 to fffe or a route per net. Any failure restores the old address and range, and an interrupted probe returns EINTR.

Parameters:
* `ifp`: interface
* `aa`: interface address
* `sat`: requested address, with the net range in its sat_zero

Returns: 0, or EINVAL for a net outside the range, EINTR, EADDRINUSE without a free address, or an ioctl or route error

Called by: [at_control](at_control.c.md#at_control)

Uses file-scope variables: `time`

### at_broadcast

```c
at_broadcast(struct sockaddr_at *sat)
```

Defined at lines 646 to 668.

Tell whether an AppleTalk address reaches every node of a local net.

The node has to be ATADDR_BCAST. Net 0 always counts. Any other net has to fall within the net range that an interface with IFF_BROADCAST holds.

Parameters:
* `sat`: AppleTalk address

Returns: 1 for such an address, or 0

Called by: [aarpresolve](aarp.c.md#aarpresolve)

### aa_clean

```c
aa_clean()
```

Defined at lines 676 to 702.

Delete the routes of every AppleTalk interface address and unlink each address.

An address missing from the address list of its interface panics.

Calls: [at_scrub](at_control.c.md#at_scrub)

# File-scope variables

`time`
