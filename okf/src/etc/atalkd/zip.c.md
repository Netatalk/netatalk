---
type: C Source File
title: "etc/atalkd/zip.c"
description: "6 functions, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/zip.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 43 to 82.

Calls: [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp)

Called by: [zip_packet](zip.c.md#zip_packet)

### zip_packet

```c
int zip_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len)
```

Defined at lines 85 to 972. Declared in [etc/atalkd/zip.h](zip.h.md).

Calls: [addzone](zip.c.md#addzone), [bootaddr](main.c.md#bootaddr), [looproute](rtmp.c.md#looproute), [setaddr](main.c.md#setaddr), [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp), [zone_bcast](multicast.c.md#zone_bcast), [zonecheck](zip.c.md#zonecheck)

Called by: [as_timer](main.c.md#as_timer), [zip_getnetinfo](zip.c.md#zip_getnetinfo)

Uses file-scope variables: `ciface` in [etc/atalkd/main.c](main.c.md), `interfaces` in [etc/atalkd/main.c](main.c.md), `stabletimer` in [etc/atalkd/main.c](main.c.md)

Dispatched via: [atserv](main.c.md#atserv)

### zip_getnetinfo

```c
int zip_getnetinfo(struct interface *)
```

Defined at lines 974 to 1026. Declared in [etc/atalkd/zip.h](zip.h.md).

Calls: [zip_packet](zip.c.md#zip_packet)

Called by: [as_timer](main.c.md#as_timer), [bootaddr](main.c.md#bootaddr), [rtmp_packet](rtmp.c.md#rtmp_packet)

### newzt

```c
struct ziptab * newzt(const int, const char *)
```

Defined at lines 1028 to 1050. Declared in [etc/atalkd/zip.h](zip.h.md).

Called by: [addzone](zip.c.md#addzone), [zone](config.c.md#zone)

### add_list

```c
static int add_list(struct list **head, void *data)
```

Defined at lines 1057 to 1088.

Called by: [addzone](zip.c.md#addzone)

### addzone

```c
int addzone(struct rtmptab *, int, char *)
```

Defined at lines 1090 to 1155. Declared in [etc/atalkd/zip.h](zip.h.md).

Calls: [add_list](zip.c.md#add_list), [newzt](zip.c.md#newzt), [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp)

Called by: [as_timer](main.c.md#as_timer), [zip_packet](zip.c.md#zip_packet)

Uses file-scope variables: `ziplast`

# File-scope variables

`ziplast`, `ziptab`
