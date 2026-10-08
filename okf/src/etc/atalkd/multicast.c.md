---
type: C Source File
title: "etc/atalkd/multicast.c"
description: "3 functions, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/multicast.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/zip.h](../../include/atalk/zip.h.md)
* [main.h](main.h.md)
* [multicast.h](multicast.h.md)
* [rtmp.h](rtmp.h.md)
* [zip.h](zip.h.md)
* System headers: `errno.h`, `net/if.h`, `stdint.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/param.h`, `sys/socket.h`, `sys/types.h`

# Functions

### addmulti

```c
int addmulti(const char *name, const unsigned char *data)
```

Defined at lines 295 to 306.

configure multicast for a given named interface

Calls: [ifconfig](main.c.md#ifconfig)

Called by: [getifconf](config.c.md#getifconf), [nbp_packet](nbp.c.md#nbp_packet), [readconf](config.c.md#readconf)

Uses file-scope variables: `ethermulti`

### atalk_cksum

```c
static uint16_t atalk_cksum(unsigned char *data, int len)
```

Defined at lines 309 to 329.

Called by: [zone_bcast](multicast.c.md#zone_bcast)

### zone_bcast

```c
int zone_bcast(struct ziptab *zt)
```

Defined at lines 340 to 368.

Fill in multicast for zone.

Note: There is a general issue here: how can we tell the type of interface we're configuring for? E.g. Is it ethernet, tokenring, or FDDI? (Of course, FDDI and Ethernet look just alike.)

Calls: [atalk_cksum](multicast.c.md#atalk_cksum)

Called by: [nbp_packet](nbp.c.md#nbp_packet), [zip_packet](zip.c.md#zip_packet)

Uses file-scope variables: `ethermulti`, `ethermultitab`

# Macros

* Undocumented: `elements`

# File-scope variables

`ethermulti`, `ethermultitab`
