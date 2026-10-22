---
type: C Source File
title: "etc/afpd/virtual_icon.c"
description: "The virtual Icon file of a volume root."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/virtual_icon.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-22T19:27:15+11:00 }
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

Defined at lines 63 to 69.

Write a 32-bit value in big-endian order.

Parameters:
* `buf`: 4-byte destination
* `val`: value

Called by: [build_resource_fork](virtual_icon.c.md#build_resource_fork)

### put_u16

```c
static void put_u16(unsigned char *buf, uint16_t val)
```

Defined at lines 77 to 81.

Write a 16-bit value in big-endian order.

Parameters:
* `buf`: 2-byte destination
* `val`: value

Called by: [build_resource_fork](virtual_icon.c.md#build_resource_fork)

### build_resource_fork

```c
static void build_resource_fork(struct vol *vol, const unsigned char *icn_hash, const unsigned char *icl4, const unsigned char *icl8)
```

Defined at lines 98 to 236.

Build the resource fork of the virtual volume icon file from 3 icon resources.

The fork holds a 256-byte header, the data of ICN#, icl4, and icl8 with 4-byte length prefixes, and a resource map. All 3 resources use ID -16455, the custom icon ID of the Finder. A failed allocation leaves the volume without a virtual icon.

Parameters:
* `vol`: volume that receives v_icon_rfork and v_icon_rfork_len
* `icn_hash`: 256-byte ICN# resource with bitmap and mask
* `icl4`: 512-byte icl4 resource, 4-bit color
* `icl8`: 1024-byte icl8 resource, 8-bit color

Calls: [put_u16](virtual_icon.c.md#put_u16), [put_u32](virtual_icon.c.md#put_u32)

Called by: [virtual_icon_init](virtual_icon.c.md#virtual_icon_init)

### virtual_icon_enabled

```c
int virtual_icon_enabled(const struct vol *vol)
```

Defined at lines 246 to 249.

Tell whether a volume serves a virtual icon file.

Parameters:
* `vol`: volume

Returns: nonzero when the volume has a built icon resource fork

Called by: [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_openfork](fork.c.md#afp_openfork), [enumerate](enumerate.c.md#enumerate), [getdirparams](directory.c.md#getdirparams), [is_virtual_icon](virtual_icon.c.md#is_virtual_icon), [of_closefork](ofork.c.md#of_closefork)

### real_icon_exists

```c
int real_icon_exists(const struct vol *vol)
```

Defined at lines 259 to 265.

Tell whether the volume root holds a real icon file.

A real file takes precedence over the virtual one.

Parameters:
* `vol`: volume

Returns: nonzero when lstat finds VIRTUAL_ICON_NAME in the volume root

Called by: [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [enumerate](enumerate.c.md#enumerate), [getdirparams](directory.c.md#getdirparams), [is_virtual_icon](virtual_icon.c.md#is_virtual_icon)

### is_virtual_icon_name

```c
int is_virtual_icon_name(const char *name)
```

Defined at lines 273 to 276.

Tell whether a name is the name of the virtual icon file.

Parameters:
* `name`: file name

Returns: nonzero for VIRTUAL_ICON_NAME

Called by: [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_openfork](fork.c.md#afp_openfork), [is_virtual_icon](virtual_icon.c.md#is_virtual_icon), [of_closefork](ofork.c.md#of_closefork)

### is_virtual_icon

```c
int is_virtual_icon(const struct vol *vol, cnid_t did, const char *name)
```

Defined at lines 289 to 295.

Tell whether a name in a folder is the virtual icon file.

The volume root holds it while the volume serves a virtual icon and no real file takes the name.

Parameters:
* `vol`: volume
* `did`: folder of the name, in network byte order
* `name`: file name on disk

Returns: nonzero for the virtual icon file

Calls: [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Called by: [afp_delete](filedir.c.md#afp_delete), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_rename](filedir.c.md#afp_rename), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [moveandrename](filedir.c.md#moveandrename)

### virtual_icon_init

```c
void virtual_icon_init(struct vol *vol)
```

Defined at lines 305 to 360.

Build the virtual icon of a volume from its legacy icon option.

The option names one of the built-in icons. An empty option builds nothing, and an unknown name logs a warning. A volume with a built icon keeps it.

Parameters:
* `vol`: volume

Calls: [build_resource_fork](virtual_icon.c.md#build_resource_fork)

Called by: [afp_openvol](volume.c.md#afp_openvol)

Uses file-scope variables: `daemon_icon` in [etc/afpd/icon.c](icon.c.md), `daemon_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `daemon_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `declogo_icon` in [etc/afpd/icon.c](icon.c.md), `declogo_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `declogo_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `fileserver_icon` in [etc/afpd/icon.c](icon.c.md), `fileserver_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `fileserver_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `globe_icon` in [etc/afpd/icon.c](icon.c.md), `globe_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `globe_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `nas_icon` in [etc/afpd/icon.c](icon.c.md), `nas_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `nas_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `sdcard_icon` in [etc/afpd/icon.c](icon.c.md), `sdcard_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `sdcard_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `sunlogo_icon` in [etc/afpd/icon.c](icon.c.md), `sunlogo_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `sunlogo_icon_icl8` in [etc/afpd/icon.c](icon.c.md), `viking_icon` in [etc/afpd/icon.c](icon.c.md), `viking_icon_icl4` in [etc/afpd/icon.c](icon.c.md), `viking_icon_icl8` in [etc/afpd/icon.c](icon.c.md)

### virtual_icon_get_rfork

```c
const unsigned char * virtual_icon_get_rfork(const struct vol *vol, size_t *outlen)
```

Defined at lines 369 to 379.

Return the resource fork of the virtual icon file.

Parameters:
* `vol`: volume
* `outlen`: receives the fork length, 0 without a virtual icon

Returns: the fork, or NULL without a virtual icon

Called by: [afp_openfork](fork.c.md#afp_openfork)

### virtual_icon_getfilparams

```c
int virtual_icon_getfilparams(const AFPObj *obj, struct vol *vol, uint16_t bitmap, uint16_t open_bit, char *buf, size_t *buflen)
```

Defined at lines 399 to 548.

Build the AFP file parameters of the virtual icon file.

The file is invisible, sits in the volume root with CNID VIRTUAL_ICON_CNID, and has an empty data fork. The resource fork length is the length of the built fork. The dates are AD_DATE_START, and the UNIX privileges show a read-only file of uid and gid 0.

Parameters:
* `obj`: session object
* `vol`: volume
* `bitmap`: file bitmap of the request
* `open_bit`: ATTRBIT_DOPEN or ATTRBIT_ROPEN of the open fork the reply describes, or 0
* `buf`: reply buffer
* `buflen`: length of the parameters on return

Returns: AFP_OK, AFPERR_MISC without a virtual icon, or AFPERR_BITMAP for an unknown bit

Calls: [set_name](file.c.md#set_name)

Called by: [enumerate](enumerate.c.md#enumerate), [getforkparams](fork.c.md#getforkparams), [virtual_icon_reply](filedir.c.md#virtual_icon_reply)

# Macros

* Undocumented: `ICON_RES_ID`
