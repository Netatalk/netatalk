---
type: C Source File
title: "etc/atalkd/rtmp.c"
description: "13 functions, includes 12 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/rtmp.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 41 to 99. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Called by: [as_timer](main.c.md#as_timer), [rtmp_delinuse](rtmp.c.md#rtmp_delinuse)

### rtmp_config

```c
static int rtmp_config(struct rtmp_head *rh, struct interface *iface)
```

Defined at lines 105 to 163.

Complete configuration for phase 1 interface using RTMP information.

Calls: [bootaddr](main.c.md#bootaddr), [looproute](rtmp.c.md#looproute), [setaddr](main.c.md#setaddr)

Called by: [rtmp_packet](rtmp.c.md#rtmp_packet)

Uses file-scope variables: `ciface` in [etc/atalkd/main.c](main.c.md), `stabletimer` in [etc/atalkd/main.c](main.c.md)

### rtmp_delinuse

```c
static void rtmp_delinuse(struct rtmptab *rtmp)
```

Defined at lines 169 to 219.

Delete rtmp from the per-interface in-use table, remove all zone references, and remove the route from the kernel.

Calls: [gateroute](rtmp.c.md#gateroute), [rtmp_delzonemap](rtmp.c.md#rtmp_delzonemap)

Called by: [rtmp_free](rtmp.c.md#rtmp_free), [rtmp_new](rtmp.c.md#rtmp_new), [rtmp_replace](rtmp.c.md#rtmp_replace)

### rtmp_addinuse

```c
static void rtmp_addinuse(struct rtmptab *rtmp)
```

Defined at lines 225 to 263.

Add rtmp to the per-interface in-use table.

Note: No verification is done...

Calls: [gateroute](rtmp.c.md#gateroute)

Called by: [rtmp_new](rtmp.c.md#rtmp_new), [rtmp_replace](rtmp.c.md#rtmp_replace)

### rtmp_copyzones

```c
static int rtmp_copyzones(struct rtmptab *to, struct rtmptab *from)
```

Defined at lines 275 to 321.

Change the zone mapping to replace "from" with "to".

Note: This code assumes the consistency of both the route -> zone map and the zone -> route map. This is probably a bad idea. How can we insure that the data is good at this point? What do we do if we get several copies of a route in an RTMP packet?

Called by: [rtmp_new](rtmp.c.md#rtmp_new), [rtmp_replace](rtmp.c.md#rtmp_replace)

### rtmp_free

```c
void rtmp_free(struct rtmptab *)
```

Defined at lines 328 to 359. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Remove rtmp from the in-use table and the per-gate table. Free any associated space.

Calls: [rtmp_delinuse](rtmp.c.md#rtmp_delinuse)

Called by: [as_timer](main.c.md#as_timer), [rtmp_replace](rtmp.c.md#rtmp_replace)

### rtmp_replace

```c
int rtmp_replace(struct rtmptab **)
```

Defined at lines 368 to 438. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Find a replacement for the structure pointed to by "replace_ptr".

Returns: If we can't find a replacement, return 1.

Returns: If we do find a replacement, return 0.

Returns: If we encounter an error, return -1.

Calls: [gateroute](rtmp.c.md#gateroute), [rtmp_addinuse](rtmp.c.md#rtmp_addinuse), [rtmp_copyzones](rtmp.c.md#rtmp_copyzones), [rtmp_delinuse](rtmp.c.md#rtmp_delinuse), [rtmp_free](rtmp.c.md#rtmp_free)

Called by: [as_timer](main.c.md#as_timer), [rtmp_packet](rtmp.c.md#rtmp_packet)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

### rtmp_new

```c
static int rtmp_new(struct rtmptab *rtmp)
```

Defined at lines 441 to 505.

Calls: [rtmp_addinuse](rtmp.c.md#rtmp_addinuse), [rtmp_copyzones](rtmp.c.md#rtmp_copyzones), [rtmp_delinuse](rtmp.c.md#rtmp_delinuse)

Called by: [rtmp_packet](rtmp.c.md#rtmp_packet)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

### rtmp_packet

```c
int rtmp_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len)
```

Defined at lines 508 to 959. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Calls: [gateroute](rtmp.c.md#gateroute), [looproute](rtmp.c.md#looproute), [newrt](rtmp.c.md#newrt), [rtmp_config](rtmp.c.md#rtmp_config), [rtmp_new](rtmp.c.md#rtmp_new), [rtmp_replace](rtmp.c.md#rtmp_replace), [zip_getnetinfo](zip.c.md#zip_getnetinfo)

Called by: [as_timer](main.c.md#as_timer), [rtmp_request](rtmp.c.md#rtmp_request)

Uses file-scope variables: `interfaces` in [etc/atalkd/main.c](main.c.md)

Dispatched via: [atserv](main.c.md#atserv)

### rtmp_request

```c
int rtmp_request(struct interface *)
```

Defined at lines 961 to 1001. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Calls: [rtmp_packet](rtmp.c.md#rtmp_packet)

Called by: [as_timer](main.c.md#as_timer), [bootaddr](main.c.md#bootaddr)

### looproute

```c
int looproute(struct interface *, unsigned int)
```

Defined at lines 1004 to 1061. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Calls: [route](route.c.md#route)

Called by: [as_down](main.c.md#as_down), [as_timer](main.c.md#as_timer), [rtmp_config](rtmp.c.md#rtmp_config), [rtmp_packet](rtmp.c.md#rtmp_packet), [zip_packet](zip.c.md#zip_packet)

### gateroute

```c
int gateroute(unsigned int, struct rtmptab *)
```

Defined at lines 1063 to 1149. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Calls: [route](route.c.md#route)

Called by: [as_down](main.c.md#as_down), [rtmp_addinuse](rtmp.c.md#rtmp_addinuse), [rtmp_delinuse](rtmp.c.md#rtmp_delinuse), [rtmp_packet](rtmp.c.md#rtmp_packet), [rtmp_replace](rtmp.c.md#rtmp_replace)

### newrt

```c
struct rtmptab * newrt(const struct interface *)
```

Defined at lines 1152 to 1162. Declared in [etc/atalkd/rtmp.h](rtmp.h.md).

Called by: [addr](config.c.md#addr), [main](main.c.md#main), [net](config.c.md#net), [rtmp_packet](rtmp.c.md#rtmp_packet)
