---
type: C Source File
title: "libatalk/util/getiface.c"
description: "4 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/getiface.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `net/if.h`, `netinet/in.h`, `netinet/tcp.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/socket.h`, `sys/types.h`

# Functions

### addname

```c
static int addname(char **list, int *i, const char *name)
```

Defined at lines 34 to 45.

we leave all of the ioctl's to the application

Called by: [getifaces](getiface.c.md#getifaces)

### getifaces

```c
static int getifaces(const int sockfd, char ***list)
```

Defined at lines 48 to 129.

Calls: [addname](getiface.c.md#addname)

Called by: [getifacelist](getiface.c.md#getifacelist)

### getifacelist

```c
char ** getifacelist(void)
```

Defined at lines 136 to 153.

Get interfaces from the kernel.

Note: we keep an extra null entry to signify the end of the interface list.

Calls: [getifaces](getiface.c.md#getifaces)

Called by: [getifconf](../../etc/atalkd/config.c.md#getifconf), [guess_interface](../dsi/dsi_tcp.c.md#guess_interface)

### freeifacelist

```c
void freeifacelist(char **ifacelist)
```

Defined at lines 157 to 170.

go through and free the interface list

Called by: [getifconf](../../etc/atalkd/config.c.md#getifconf), [guess_interface](../dsi/dsi_tcp.c.md#guess_interface)

# Macros

* Undocumented: `IFACE_NUM`
