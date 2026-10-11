---
type: C Source File
title: "etc/atalkd/zip.c"
description: "6 functions, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/zip.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/zip.h](../../include/atalk/zip.h.md)
* [atserv.h](atserv.h.md)
* [gate.h](gate.h.md)
* [interface.h](interface.h.md)
* [list.h](list.h.md)
* [main.h](main.h.md)
* [multicast.h](multicast.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [rtmp.h](rtmp.h.md)
* [zip.h](zip.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `net/if.h`, `net/route.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`

# Functions

### zonecheck

```c
static int zonecheck(struct rtmptab *rtmp, struct interface *iface)
```

Defined at lines 54 to 93.

Check the zones a seed interface learned from the network against its configured zones.

Each configured zone has to be in the zone list of the route, and the two lists have to be the same length. Interfaces without -seed always pass.

Parameters:
* `rtmp`: route of the interface with the learned zones
* `iface`: interface with the configured zones

Returns: 0 when the zones agree, or -1 after a log line for a mismatch

Calls: [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp)

Called by: [zip_packet](zip.c.md#zip_packet)

### zip_packet

```c
int zip_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len)
```

Defined at lines 128 to 1015. Declared in [etc/atalkd/zip.h](zip.h.md).

Process a ZIP packet, or a ZIP request over ATP, from the ZIP port of an interface.

ZIP packets carry these ops:

* ZIPOP_QUERY answers with the zones of each requested net that has a route in use. A zone list that does not fit in one packet goes out in ZIPOP_EREPLY packets.
* ZIPOP_REPLY and ZIPOP_EREPLY from a known gateway add the zones to the route with a pending query. The zones of the interface route go through zonecheck, and a mismatch on a seed interface stops atalkd.
* ZIPOP_GNI, on a configured router interface with zones, answers with the net range and the zone multicast address. A requested zone that is not valid gets the default zone in the reply.
* ZIPOP_GNIREPLY completes the configuration of a booting phase 2 interface. The interface takes the default zone and the net range, and a seed interface stops atalkd on a mismatch.
* ZIPOP_NOTIFY only logs.

Over ATP the call answers GetMyZone, GetZoneList, and GetLocalZones with the zones that fit in one response. A GetMyZone or GetLocalZones request from outside the net range of the interface gets no answer, and requests on loopback take the first interface.

Parameters:
* `ap`: ZIP port of the receiving interface
* `from`: sender, which a ZIPOP_GNI from outside the net range turns into net 0 and ATADDR_BCAST
* `data`: packet from the DDP type byte on
* `len`: length of data

Returns: 0, 1 for a malformed or unexpected packet, or -1 for a failure that stops atalkd

Calls: [addzone](zip.c.md#addzone), [bootaddr](main.c.md#bootaddr), [looproute](rtmp.c.md#looproute), [setaddr](main.c.md#setaddr), [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp), [zone_bcast](multicast.c.md#zone_bcast), [zonecheck](zip.c.md#zonecheck)

Called by: [as_timer](main.c.md#as_timer), [zip_getnetinfo](zip.c.md#zip_getnetinfo)

Uses file-scope variables: `ciface` in [etc/atalkd/main.c](main.c.md), `stabletimer` in [etc/atalkd/main.c](main.c.md)

Dispatched via: [atserv](main.c.md#atserv)

### zip_getnetinfo

```c
int zip_getnetinfo(struct interface *)
```

Defined at lines 1028 to 1080. Declared in [etc/atalkd/zip.h](zip.h.md).

Send a ZIP GetNetInfo for the net range and default zone of an interface.

The request leaves the ZIP port of the interface for net 0 and node ATADDR_BCAST. Its net range is 0-0 and its zone name empty, so the reply carries the default zone.

Parameters:
* `iface`: interface to ask on

Returns: 0, or -1 without a ZIP port or when the send fails

Calls: [zip_packet](zip.c.md#zip_packet)

Called by: [as_timer](main.c.md#as_timer), [bootaddr](main.c.md#bootaddr), [rtmp_packet](rtmp.c.md#rtmp_packet)

### newzt

```c
struct ziptab * newzt(const int, const char *)
```

Defined at lines 1092 to 1114. Declared in [etc/atalkd/zip.h](zip.h.md).

Allocate a zone entry that holds its name in a new buffer.

The name keeps its length in zt_len and has no NUL terminator.

Parameters:
* `len`: name length, 1 to MAX_ZONE_LENGTH
* `name`: zone name, not NUL-terminated

Returns: the zone, or NULL after a failed allocation or with errno EINVAL for a bad length or name

Called by: [addzone](zip.c.md#addzone), [zone](config.c.md#zone)

### add_list

```c
static int add_list(struct list **head, void *data)
```

Defined at lines 1125 to 1156.

Append a pointer to a list unless the list holds it already.

Parameters:
* `head`: list head
* `data`: pointer to store

Returns: 0 after an append, 1 when the list holds the pointer, or -1 when malloc fails

Called by: [addzone](zip.c.md#addzone)

### addzone

```c
int addzone(struct rtmptab *, int, char *)
```

Defined at lines 1173 to 1238. Declared in [etc/atalkd/zip.h](zip.h.md).

Map a route and a zone to each other, adding the zone to the zone table when new.

The zone name matches without regard to case, through the Mac Roman upper case map. The route joins the route list of the zone, and the zone joins the zone list of the route. A mapping that exists on one side only means the tables disagree, and the call fails.

Parameters:
* `rt`: route
* `len`: zone name length, 1 to MAX_ZONE_LENGTH
* `zone`: zone name, not NUL-terminated

Returns: 0, also when both mappings exist, or -1 for a bad name, a failed allocation, or a one-sided mapping

Calls: [add_list](zip.c.md#add_list), [newzt](zip.c.md#newzt), [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp)

Called by: [as_timer](main.c.md#as_timer), [zip_packet](zip.c.md#zip_packet)

Uses file-scope variables: `ziplast`

# File-scope variables

`ziplast`, `ziptab`
