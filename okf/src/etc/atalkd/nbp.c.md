---
type: C Source File
title: "etc/atalkd/nbp.c"
description: "2 functions, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/nbp.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atserv.h](atserv.h.md)
* [gate.h](gate.h.md)
* [interface.h](interface.h.md)
* [list.h](list.h.md)
* [multicast.h](multicast.h.md)
* [nbp.h](nbp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [rtmp.h](rtmp.h.md)
* [zip.h](zip.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `net/if.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/param.h`, `sys/socket.h`, `sys/types.h`

# Functions

### nbp_ack

```c
static void nbp_ack(int fd, int nh_op, int nh_id, struct sockaddr_at *to)
```

Defined at lines 53 to 69.

Send an NBP reply that carries only an op and a request ID.

A failed send only logs.

Parameters:
* `fd`: socket of the NBP port
* `nh_op`: NBPOP_OK or NBPOP_ERROR
* `nh_id`: NBP ID of the request
* `to`: requester address

Called by: [nbp_packet](nbp.c.md#nbp_packet)

### nbp_packet

```c
int nbp_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len)
```

Defined at lines 108 to 732. Declared in [etc/atalkd/nbp.h](nbp.h.md).

Process an NBP packet from the NBP port of an interface.

The packet has to carry one tuple whose object, type, and zone fit in 32 bytes each, with nothing after the zone. An empty zone or `*` stands for the default zone, the first zone of the first interface. The ops work as follows:

* NBPOP_RGSTR adds the name and the interface of the port to nbptab. The first name in a zone adds the zone multicast address to the phase 2 interfaces of the zone.
* NBPOP_UNRGSTR removes the first entry with the same object, type, and zone. The multicast address of the zone stays.
* NBPOP_BRRQ sends a lookup to each net of the zone. Every node of a local net gets an NBPOP_LKUP, and a router on a remote net gets an NBPOP_FWD. When atalkd knows no zone, the NBPOP_LKUP goes to the local net.
* With -t, every node of a nonextended net gets an NBPOP_LKUP instead.
* On a router interface, NBPOP_FWD turns into an NBPOP_LKUP to every node of the local net.
* NBPOP_LKUP answers from nbptab, with `=` matching any object or type, in NBPOP_LKUPREPLY packets of up to 15 tuples. The replies go to the address in the request tuple.

A BRRQ that covers a local net, and a FWD, also answer from nbptab. Entries of an interface that does not route answer only on that interface, and loopback never answers. RGSTR and UNRGSTR answer NBPOP_OK, or NBPOP_ERROR when the zone is unknown or not local, or the entry is missing. A BRRQ for an unknown zone gets NBPOP_ERROR as well.

Parameters:
* `ap`: NBP port of the receiving interface
* `from`: sender, which an NBPOP_FWD replaces with net 0 and the node ATADDR_BCAST
* `data`: packet from the DDP type byte on, its NBP op rewritten for forwarding
* `len`: length of data

Returns: 0, 1 for a malformed packet or an unknown op, or -1 for a failure that stops atalkd

Calls: [addmulti](multicast.c.md#addmulti), [nbp_ack](nbp.c.md#nbp_ack), [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp), [zone_bcast](multicast.c.md#zone_bcast)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md), `transition` in [etc/atalkd/main.c](main.c.md)

Dispatched via: [atserv](main.c.md#atserv)

# File-scope variables

`nbptab`
