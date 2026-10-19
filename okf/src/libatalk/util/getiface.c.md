---
type: C Source File
title: "libatalk/util/getiface.c"
description: "Lists of the network interface names of the system."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/getiface.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-20T00:03:30+11:00 }
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

Defined at lines 48 to 59.

Append an interface name to a NULL-terminated list.

The list needs room for the name and the NULL after it.

Parameters:
* `list`: name list
* `i`: number of names in list, incremented on success
* `name`: interface name

Returns: 0, or -1 when strdup fails

Called by: [getifaces](getiface.c.md#getifaces)

### getifaces

```c
static int getifaces(const int sockfd, char ***list)
```

Defined at lines 73 to 162.

Build a NULL-terminated list of the interface names of the system.

The call reads the names from if_nameindex, or from SIOCGIFCONF on sockfd where if_nameindex is missing. A failed strdup ends the list early.

Parameters:
* `sockfd`: socket for SIOCGIFCONF, unused with if_nameindex
* `list`: receives the list, and with a return of 0 it holds no names and needs only free

Returns: the number of names, 0 without names or after a failure

Calls: [addname](getiface.c.md#addname)

Called by: [getifacelist](getiface.c.md#getifacelist)

### getifacelist

```c
char ** getifacelist(void)
```

Defined at lines 172 to 189.

Return the names of the network interfaces of the system.

The list ends with a NULL entry, and freeifacelist releases it.

Returns: the list, or NULL without a socket or without any names

Calls: [getifaces](getiface.c.md#getifaces)

Called by: [getifconf](../../etc/atalkd/config.c.md#getifconf), [guess_interface](../dsi/dsi_tcp.c.md#guess_interface)

### freeifacelist

```c
void freeifacelist(char **ifacelist)
```

Defined at lines 197 to 210.

Free a list from getifacelist and its names.

Parameters:
* `ifacelist`: list, or NULL

Called by: [getifconf](../../etc/atalkd/config.c.md#getifconf), [guess_interface](../dsi/dsi_tcp.c.md#guess_interface)

# Macros

* Undocumented: `IFACE_NUM`
