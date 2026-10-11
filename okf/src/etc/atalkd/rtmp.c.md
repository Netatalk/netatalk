---
type: C Source File
title: "etc/atalkd/rtmp.c"
description: "13 functions, includes 12 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/rtmp.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/rtmp.h](../../include/atalk/rtmp.h.md)
* [atserv.h](atserv.h.md)
* [gate.h](gate.h.md)
* [interface.h](interface.h.md)
* [list.h](list.h.md)
* [main.h](main.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [route.h](route.h.md)
* [rtmp.h](rtmp.h.md)
* [zip.h](zip.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `net/if.h`, `net/route.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/param.h`, `sys/socket.h`, `sys/types.h`

# Functions

### rtmp_delzonemap

```c
void rtmp_delzonemap(struct rtmptab *)
```

Defined at lines 49 to 107. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Drop the zone list of a route and the route from each of its zones.

When the route was the last one of a zone, the zone leaves the zone table and the call frees it. The caller clears RTMPTAB_HASZONES.

Parameters:
* `rtmp`: route, its zone list ends up empty

Called by: [as_timer](main.c.md#as_timer), [rtmp_delinuse](rtmp.c.md#rtmp_delinuse)

### rtmp_config

```c
static int rtmp_config(struct rtmp_head *rh, struct interface *iface)
```

Defined at lines 124 to 182.

Complete the configuration of a phase 1 interface from the RTMP packet of a router.

A phase 2 interface ignores the data. When the router reports another net, the interface moves to it, but a seed interface refuses a net other than its configured one. The call then adds the loopback route, marks the interface configured, and boots the next interface.

Parameters:
* `rh`: RTMP header of the router packet
* `iface`: interface to configure

Returns: 0, 1 for a net that conflicts with the seed, or -1 when the loopback route fails

Calls: [bootaddr](main.c.md#bootaddr), [looproute](rtmp.c.md#looproute), [setaddr](main.c.md#setaddr)

Uses file-scope variables: `ciface` in [etc/atalkd/main.c](main.c.md), `stabletimer` in [etc/atalkd/main.c](main.c.md)

### rtmp_delinuse

```c
static void rtmp_delinuse(struct rtmptab *rtmp)
```

Defined at lines 193 to 243.

Take a gateway route out of use.

The call unlinks the route from the in-use list of its interface, drops its zone map, and deletes its kernel route. A route without a gateway or interface only logs.

Parameters:
* `rtmp`: route in use

Calls: [gateroute](rtmp.c.md#gateroute), [rtmp_delzonemap](rtmp.c.md#rtmp_delzonemap)

Called by: [rtmp_free](rtmp.c.md#rtmp_free), [rtmp_new](rtmp.c.md#rtmp_new), [rtmp_replace](rtmp.c.md#rtmp_replace)

### rtmp_addinuse

```c
static void rtmp_addinuse(struct rtmptab *rtmp)
```

Defined at lines 254 to 292.

Put a gateway route in use.

The call adds the kernel route and links the route at the head of the in-use list of its interface. It does not check whether the route is in use already, and a route without a gateway or interface only logs.

Parameters:
* `rtmp`: route to use

Calls: [gateroute](rtmp.c.md#gateroute)

Called by: [rtmp_new](rtmp.c.md#rtmp_new), [rtmp_replace](rtmp.c.md#rtmp_replace)

### rtmp_copyzones

```c
static int rtmp_copyzones(struct rtmptab *to, struct rtmptab *from)
```

Defined at lines 307 to 353.

Move the zone list of a route to its replacement.

The zones and RTMPTAB_HASZONES pass from one route to the other, and each zone then points at the new route. The call trusts the route to zone map and the zone to route map to agree. A zone without a reference back to the old route stops it midway with -1.

Parameters:
* `to`: route that receives the zones
* `from`: route that gives them up

Returns: 0, or -1 for a NULL route or a zone without the back reference

Called by: [rtmp_new](rtmp.c.md#rtmp_new), [rtmp_replace](rtmp.c.md#rtmp_replace)

### rtmp_free

```c
void rtmp_free(struct rtmptab *)
```

Defined at lines 364 to 395. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Take a gateway route out of use, unlink it from its gateway, and free it.

For a route in use, the call first drops its zone map and its kernel route.

Parameters:
* `rtmp`: route to free

Calls: [rtmp_delinuse](rtmp.c.md#rtmp_delinuse)

Called by: [as_timer](main.c.md#as_timer), [rtmp_replace](rtmp.c.md#rtmp_replace)

### rtmp_replace

```c
int rtmp_replace(struct rtmptab **)
```

Defined at lines 412 to 482. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Pick the route with the fewest hops for the net range of a route in use.

The call searches the gateways on the interface of the route and on each router interface for routes with the same net range. The first one with the fewest hops goes in use, and when that is another route, it takes over the zones. The call frees the old route when its state is RTMPTAB_BAD. When the route itself stays in use with RTMPHOPS_POISON hops, the call deletes its kernel route.

Parameters:
* `replace_ptr`: route in use, set to NULL when the call frees it

Returns: 0 after a switch, 1 when the route stays, or -1 for an error

Calls: [gateroute](rtmp.c.md#gateroute), [rtmp_addinuse](rtmp.c.md#rtmp_addinuse), [rtmp_copyzones](rtmp.c.md#rtmp_copyzones), [rtmp_delinuse](rtmp.c.md#rtmp_delinuse), [rtmp_free](rtmp.c.md#rtmp_free)

Called by: [as_timer](main.c.md#as_timer)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

### rtmp_new

```c
static int rtmp_new(struct rtmptab *rtmp)
```

Defined at lines 498 to 562.

Decide whether a new gateway route goes in use.

The call sets newrtmpdata, then looks for a route in use that overlaps the range, on the receiving interface or a router interface. Without one, the new route goes in use. A route in use with another range, or with fewer hops, keeps its place. Otherwise the new route takes over its zones and replaces it.

Parameters:
* `rtmp`: candidate route of a gateway

Returns: 0 when the route goes in use, 1 when it does not, or -1 when the zone move fails

Calls: [rtmp_addinuse](rtmp.c.md#rtmp_addinuse), [rtmp_copyzones](rtmp.c.md#rtmp_copyzones), [rtmp_delinuse](rtmp.c.md#rtmp_delinuse)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

### rtmp_packet

```c
int rtmp_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len)
```

Defined at lines 594 to 1045. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Process an RTMP packet from the RTMP port of an interface.

Linux 2.6 hands packets for every node to one socket, so the call picks the interface whose net range holds the sender. It skips packets from the address of that interface. RTMP data from a router depends on the interface state:

* With an address but no configuration, the data ends the no-router state. The configuration goes on with rtmp_config in phase 1 or a ZIP GetNetInfo in phase 2.
* Without an address and configuration, the data goes unused.
* On a configured interface, the data refreshes the gateway, created on first contact, and merges each tuple into the routes of the gateway.

A known route gets a fresh state and its new number of hops. A route in use that got longer looks for a replacement with rtmp_replace. A new route goes to rtmp_new, unless it is more than RTMPHOPS_MAX hops away. A first tuple that does not match the interface, or a range conflict, ends the call with 1.

A configured router interface with zones answers an RTMP request with its address and, in phase 2, its net range. Route data requests only log.

Parameters:
* `ap`: RTMP port of the receiving interface
* `from`: sender of the packet
* `data`: packet from the DDP type byte on
* `len`: length of data

Returns: 0, 1 for a malformed or conflicting packet, or -1 for a failure that stops atalkd

Calls: [zip_getnetinfo](zip.c.md#zip_getnetinfo)

Called by: [as_timer](main.c.md#as_timer), [rtmp_request](rtmp.c.md#rtmp_request)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

Dispatched via: [atserv](main.c.md#atserv)

### rtmp_request

```c
int rtmp_request(struct interface *)
```

Defined at lines 1056 to 1096. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Send an RTMP request from an interface to all nodes of its net.

The request leaves the RTMP port of the interface for node ATADDR_BCAST on the current net of the interface.

Parameters:
* `iface`: interface to ask on

Returns: 0, or -1 without an RTMP port or when the send fails

Calls: [rtmp_packet](rtmp.c.md#rtmp_packet)

Called by: [as_timer](main.c.md#as_timer), [bootaddr](main.c.md#bootaddr)

### looproute

```c
int looproute(struct interface *, unsigned int)
```

Defined at lines 1110 to 1167. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Add or delete the loopback host route for the address of an interface.

The route sends packets for the interface address through the loopback interface. IFACE_LOOP tracks whether the route exists.

Parameters:
* `iface`: interface, its IFACE_LOOP flag follows the route
* `cmd`: RTMP_ADD or RTMP_DEL

Returns: 0, -1 when the route already exists or is missing, or 1 when [route()](route.c.md#route) fails

Calls: [route](route.c.md#route)

Called by: [as_down](main.c.md#as_down), [as_timer](main.c.md#as_timer), [rtmp_config](rtmp.c.md#rtmp_config), [zip_packet](zip.c.md#zip_packet)

### gateroute

```c
int gateroute(unsigned int, struct rtmptab *)
```

Defined at lines 1183 to 1269. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Add or delete the kernel routes for the net range of a gateway route.

The kernel gets one route per net of the range through the gateway. A gateway that reports net 0 gets the first net of the range instead. RTMPTAB_ROUTE tracks whether the routes exist, and a net whose [route()](route.c.md#route) call fails only logs.

Parameters:
* `command`: RTMP_ADD or RTMP_DEL
* `rtmp`: gateway route, its RTMPTAB_ROUTE flag follows the routes

Returns: 0, or -1 for a NULL route or gateway, or when the routes already exist or are missing

Calls: [route](route.c.md#route)

Called by: [as_down](main.c.md#as_down), [rtmp_addinuse](rtmp.c.md#rtmp_addinuse), [rtmp_delinuse](rtmp.c.md#rtmp_delinuse), [rtmp_replace](rtmp.c.md#rtmp_replace)

### newrt

```c
struct rtmptab * newrt(const struct interface *)
```

Defined at lines 1278 to 1288. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Allocate a zeroed route entry for an interface.

Parameters:
* `iface`: interface the route belongs to

Returns: the route, or NULL when calloc fails

Called by: [addr](config.c.md#addr), [main](main.c.md#main), [net](config.c.md#net)
