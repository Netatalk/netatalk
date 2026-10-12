---
type: C Source File
title: "etc/atalkd/multicast.c"
description: "Zone multicast addresses of AppleTalk interfaces."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/multicast.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 307 to 318.

Add an AppleTalk multicast address to an interface.

Parameters:
* `name`: interface name
* `data`: 6-byte Ethernet multicast address, or NULL for 09:00:07:ff:ff:ff, which reaches every AppleTalk node

Returns: 0, or -1 when SIOCADDMULTI fails

Calls: [ifconfig](main.c.md#ifconfig)

Called by: [getifconf](config.c.md#getifconf), [nbp_packet](nbp.c.md#nbp_packet), [readconf](config.c.md#readconf)

Uses file-scope variables: `ethermulti`

### atalk_cksum

```c
static uint16_t atalk_cksum(unsigned char *data, int len)
```

Defined at lines 331 to 351.

Compute the DDP checksum of a buffer, which hashes zone names here.

Each byte adds to the sum, which then rotates left by one bit within 16 bits. A result of 0 becomes 0xffff, since 0 means no checksum in DDP.

Parameters:
* `data`: bytes to sum
* `len`: number of bytes

Returns: the 16-bit checksum, never 0

Called by: [zone_bcast](multicast.c.md#zone_bcast)

### zone_bcast

```c
int zone_bcast(struct ziptab *zt)
```

Defined at lines 365 to 393.

Compute and store the Ethernet multicast address of a zone.

The DDP checksum of the zone name in upper case, modulo 253, picks one of the 253 AppleTalk zone multicast addresses. The table holds Ethernet addresses, which FDDI shares, and the call knows no other link type such as Token Ring.

Parameters:
* `zt`: zone, receives the address in zt_bcast

Returns: 0, or -1 for an invalid zone or a failed allocation

Calls: [atalk_cksum](multicast.c.md#atalk_cksum)

Called by: [nbp_packet](nbp.c.md#nbp_packet), [zip_packet](zip.c.md#zip_packet)

Uses file-scope variables: `ethermulti`, `ethermultitab`

# Macros

* Undocumented: `elements`

# File-scope variables

`ethermulti`, `ethermultitab`
