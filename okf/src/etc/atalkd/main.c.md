---
type: C Source File
title: "etc/atalkd/main.c"
description: "11 functions, includes 16 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/main.c"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/rtmp.h](../../include/atalk/rtmp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/zip.h](../../include/atalk/zip.h.md)
* [atserv.h](atserv.h.md)
* [gate.h](gate.h.md)
* [interface.h](interface.h.md)
* [list.h](list.h.md)
* [main.h](main.h.md)
* [nbp.h](nbp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [rtmp.h](rtmp.h.md)
* [zip.h](zip.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `fcntl.h`, `net/if.h`, `net/route.h`, `netdb.h`, `netinet/in.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/file.h`, `sys/ioctl.h`, `sys/param.h`, `sys/resource.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/wait.h`, `unistd.h`

# Function tables

### atserv

Initialized at line 73. Dispatches to: [aep_packet](aep.c.md#aep_packet), [nbp_packet](nbp.c.md#nbp_packet), [rtmp_packet](rtmp.c.md#rtmp_packet), [zip_packet](zip.c.md#zip_packet)

# Functions

### atalkd_exit

```c
static void atalkd_exit(const int i)
```

Defined at lines 106 to 129.

Calls: [ifconfig](main.c.md#ifconfig)

Called by: [as_down](main.c.md#as_down), [as_timer](main.c.md#as_timer), [bootaddr](main.c.md#bootaddr), [consistency](main.c.md#consistency), [main](main.c.md#main), [setaddr](main.c.md#setaddr)

Uses file-scope variables: `interfaces`, `pidfile`

### sendto_iface

```c
static ssize_t sendto_iface(struct interface *iface, int sockfd, const void *buf, size_t len, const struct sockaddr_at *dest_addr)
```

Defined at lines 135 to 157.

Called by: [as_timer](main.c.md#as_timer)

### as_timer

```c
static void as_timer(int sig)
```

Defined at lines 159 to 687.

Calls: [addzone](zip.c.md#addzone), [atalkd_exit](main.c.md#atalkd_exit), [bootaddr](main.c.md#bootaddr), [consistency](main.c.md#consistency), [looproute](rtmp.c.md#looproute), [rtmp_delzonemap](rtmp.c.md#rtmp_delzonemap), [rtmp_free](rtmp.c.md#rtmp_free), [rtmp_packet](rtmp.c.md#rtmp_packet), [rtmp_replace](rtmp.c.md#rtmp_replace), [rtmp_request](rtmp.c.md#rtmp_request), [sendto_iface](main.c.md#sendto_iface), [setaddr](main.c.md#setaddr), [writeconf](config.c.md#writeconf), [zip_getnetinfo](zip.c.md#zip_getnetinfo), [zip_packet](zip.c.md#zip_packet)

Called by: [main](main.c.md#main)

Uses file-scope variables: `ciface`, `configfile`, `debug`, `interfaces`, `newrtmpdata`, `noparent`, `stable`, `stabletimer`, `ziptimeout`

### consistency

```c
static void consistency(void)
```

Defined at lines 692 to 724.

Calls: [atalkd_exit](main.c.md#atalkd_exit)

Called by: [as_timer](main.c.md#as_timer), [main](main.c.md#main)

### as_debug

```c
static void as_debug(int sig)
```

Defined at lines 727 to 852.

Calls: [tmpdir](../../libatalk/util/unix.c.md#tmpdir)

Called by: [main](main.c.md#main)

Uses file-scope variables: `interfaces`

### as_down

```c
static void as_down(int sig)
```

Defined at lines 858 to 890.

Calls: [atalkd_exit](main.c.md#atalkd_exit), [gateroute](rtmp.c.md#gateroute), [looproute](rtmp.c.md#looproute)

Called by: [main](main.c.md#main)

Uses file-scope variables: `interfaces`

### main

```c
int main(int ac, char **av)
```

Defined at lines 892 to 1279.

Calls: [as_debug](main.c.md#as_debug), [as_down](main.c.md#as_down), [as_timer](main.c.md#as_timer), [atalkd_exit](main.c.md#atalkd_exit), [bootaddr](main.c.md#bootaddr), [bprint](../../libatalk/util/bprint.c.md#bprint), [consistency](main.c.md#consistency), [dumpconfig](main.c.md#dumpconfig), [getifconf](config.c.md#getifconf), [ifconfig](main.c.md#ifconfig), [newiface](config.c.md#newiface), [newrt](rtmp.c.md#newrt), [readconf](config.c.md#readconf), [server_lock](../../libatalk/util/server_lock.c.md#server_lock), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name), [set_processname](../../libatalk/util/logger.c.md#set_processname), [syslog_setup](../../libatalk/util/logger.c.md#syslog_setup)

Calls through [`atport::ap_packet`](atserv.h.md#struct-atport): no table assigns this field

Uses file-scope variables: `Packet`, `ciface`, `configfile`, `debug`, `defphase`, `fds`, `interfaces`, `nfds`, `ninterfaces`, `pidfile`, `rtfd`, `transition`, `version`

### bootaddr

```c
void bootaddr(struct interface *iface)
```

Defined at lines 1286 to 1341.

Calls: [atalkd_exit](main.c.md#atalkd_exit), [rtmp_request](rtmp.c.md#rtmp_request), [setaddr](main.c.md#setaddr), [zip_getnetinfo](zip.c.md#zip_getnetinfo)

Called by: [as_timer](main.c.md#as_timer), [main](main.c.md#main), [rtmp_config](rtmp.c.md#rtmp_config), [zip_packet](zip.c.md#zip_packet)

Uses file-scope variables: `ciface`, `stabletimer`

### setaddr

```c
void setaddr(struct interface *iface, uint8_t phase, uint16_t net, uint8_t node, uint16_t first, uint16_t last)
```

Defined at lines 1348 to 1467.

Calls: [atalkd_exit](main.c.md#atalkd_exit), [ifconfig](main.c.md#ifconfig)

Called by: [as_timer](main.c.md#as_timer), [bootaddr](main.c.md#bootaddr), [rtmp_config](rtmp.c.md#rtmp_config), [zip_packet](zip.c.md#zip_packet)

Uses file-scope variables: `atservNATSERV`, `fds`, `interfaces`, `nfds`

### ifconfig

```c
int ifconfig(const char *iname, unsigned long cmd, struct sockaddr_at *sa)
```

Defined at lines 1469 to 1493.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [addmulti](multicast.c.md#addmulti), [atalkd_exit](main.c.md#atalkd_exit), [main](main.c.md#main), [setaddr](main.c.md#setaddr)

### dumpconfig

```c
void dumpconfig(struct interface *iface)
```

Defined at lines 1495 to 1535.

Called by: [main](main.c.md#main)

# Macros

* Undocumented: `MACCHARSET`, `PKTSZ`, `elements`

# File-scope variables

`Packet`, `atservNATSERV`, `ciface`, `configfile`, `debug`, `defphase`, `fds`, `interfaces`, `newrtmpdata`, `nfds`, `ninterfaces`, `noparent`, `pidfile`, `rtfd`, `stable`, `stabletimer`, `transition`, `version`, `ziptimeout`
