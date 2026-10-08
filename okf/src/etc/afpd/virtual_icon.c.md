---
type: C Source File
title: "etc/afpd/virtual_icon.c"
description: "9 functions, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/virtual_icon.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [icon.h](icon.h.md)
* [virtual_icon.h](virtual_icon.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `stdlib.h`, `string.h`, `sys/stat.h`

# Functions

### put_u32

```c
static void put_u32(unsigned char *buf, uint32_t val)
```

Defined at lines 54 to 60.

Write a big-endian uint32_t to buf

Called by: [build_resource_fork](virtual_icon.c.md#build_resource_fork)

### put_u16

```c
static void put_u16(unsigned char *buf, uint16_t val)
```

Defined at lines 65 to 69.

Write a big-endian uint16_t to buf

Called by: [build_resource_fork](virtual_icon.c.md#build_resource_fork)

### build_resource_fork

```c
static void build_resource_fork(struct vol *vol, const unsigned char *icn_hash, const unsigned char *icl4, const unsigned char *icl8)
```

Defined at lines 78 to 216.

Build the Macintosh resource fork binary data containing ICN#, icl4, and icl8.

icn_hash: 256-byte ICN# resource (icon bitmap + mask) icl4: 512-byte icl4 resource (32x32 4-bit color icon) icl8: 1024-byte icl8 resource (32x32 8-bit color icon)

Calls: [put_u16](virtual_icon.c.md#put_u16), [put_u32](virtual_icon.c.md#put_u32)

Called by: [virtual_icon_init](virtual_icon.c.md#virtual_icon_init)

### virtual_icon_enabled

```c
int virtual_icon_enabled(const struct vol *vol)
```

Defined at lines 220 to 223.

Called by: [afp_delete](filedir.c.md#afp_delete), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_openfork](fork.c.md#afp_openfork), [afp_rename](filedir.c.md#afp_rename), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [enumerate](enumerate.c.md#enumerate), [getdirparams](directory.c.md#getdirparams), [of_closefork](ofork.c.md#of_closefork)

### real_icon_exists

```c
int real_icon_exists(const struct vol *vol)
```

Defined at lines 225 to 231.

Called by: [afp_delete](filedir.c.md#afp_delete), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_rename](filedir.c.md#afp_rename), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [enumerate](enumerate.c.md#enumerate), [getdirparams](directory.c.md#getdirparams)

### is_virtual_icon_name

```c
int is_virtual_icon_name(const char *name)
```

Defined at lines 233 to 236.

Called by: [afp_delete](filedir.c.md#afp_delete), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_openfork](fork.c.md#afp_openfork), [afp_rename](filedir.c.md#afp_rename), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [of_closefork](ofork.c.md#of_closefork)

### virtual_icon_init

```c
void virtual_icon_init(struct vol *vol)
```

Defined at lines 238 to 293.

Calls: [build_resource_fork](virtual_icon.c.md#build_resource_fork)

Called by: [afp_openvol](volume.c.md#afp_openvol)

Uses file-scope variables: `daemon_icon` in [etc/afpd/icon.c](icon.c.md), `daemon_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `daemon_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `declogo_icon` in [etc/afpd/icon.c](icon.c.md), `declogo_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `declogo_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `fileserver_icon` in [etc/afpd/icon.c](icon.c.md), `fileserver_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `fileserver_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `globe_icon` in [etc/afpd/icon.c](icon.c.md), `globe_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `globe_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `nas_icon` in [etc/afpd/icon.c](icon.c.md), `nas_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `nas_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `sdcard_icon` in [etc/afpd/icon.c](icon.c.md), `sdcard_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `sdcard_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `sunlogo_icon` in [etc/afpd/icon.c](icon.c.md), `sunlogo_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `sunlogo_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `viking_icon` in [etc/afpd/icon.c](icon.c.md), `viking_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `viking_icon_icl8` in [etc/afpd/icon.c](icon.c.md)

### virtual_icon_get_rfork

```c
const unsigned char * virtual_icon_get_rfork(const struct vol *vol, size_t *outlen)
```

Defined at lines 295 to 305.

Called by: [afp_openfork](fork.c.md#afp_openfork), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon)

### virtual_icon_getfilparams

```c
int virtual_icon_getfilparams(const AFPObj *obj, struct vol *vol, uint16_t bitmap, char *buf, size_t *buflen)
```

Defined at lines 313 to 463.

Synthesize AFP file parameters for the virtual Icon\r file.

This fills the reply buffer with parameters matching the requested bitmap, just as getmetadata/getfilparams would for a real file.

Calls: [set_name](file.c.md#set_name)

Called by: [afp_openfork](fork.c.md#afp_openfork), [enumerate](enumerate.c.md#enumerate), [getforkparams](fork.c.md#getforkparams), [virtual_icon_reply](filedir.c.md#virtual_icon_reply)

# Macros

* Undocumented: `ICON_RES_ID`
