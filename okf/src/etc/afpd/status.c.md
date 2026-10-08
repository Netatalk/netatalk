---
type: C Source File
title: "etc/afpd/status.c"
description: "12 functions, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/status.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afp_config.h](afp_config.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [icon.h](icon.h.md)
* [status.h](status.h.md)
* [uam_auth.h](uam_auth.h.md)
* System headers: `arpa/inet.h`, `ctype.h`, `fcntl.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/stat.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### uam_gss_enabled

```c
static int uam_gss_enabled(void)
```

Defined at lines 47 to 53.

Calls: [auth_uamfind](auth.c.md#auth_uamfind)

Called by: [status_directorynames](status.c.md#status_directorynames)

### status_flags

```c
static void status_flags(char *data, const int notif, const int ipok, const unsigned char passwdbits, const int dirsrvcs, int flags)
```

Defined at lines 55 to 99.

Called by: [status_init](status.c.md#status_init)

### status_server

```c
static int status_server(char *data, const char *server, const struct afp_options *options)
```

Defined at lines 101 to 149.

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string)

Called by: [status_init](status.c.md#status_init)

### status_machine

```c
static void status_machine(char *data)
```

Defined at lines 151 to 180.

Called by: [status_init](status.c.md#status_init)

### status_signature

```c
static uint16_t status_signature(char *data, int *servoffset, const struct afp_options *options)
```

Defined at lines 183 to 202.

Called by: [status_init](status.c.md#status_init)

### status_netaddress

```c
static size_t status_netaddress(char *data, int *servoffset, const ASP asp, const DSI *dsi, const struct afp_options *options)
```

Defined at lines 204 to 330.

Called by: [status_init](status.c.md#status_init)

Uses file-scope variables: `maxstatuslen`

### status_directorynames

```c
static size_t status_directorynames(char *data, int *diroffset, const DSI *dsi, const struct afp_options *options)
```

Defined at lines 343 to 368.

Build the DirectoryNames field of the status reply.

```
DirectoryNamesCount offset: uint16_t
...
DirectoryNamesCount: uint8_t
DirectoryNames: list of UTF-8 Pascal strings (uint8_t + char[1,255])
```

Calls: [uam_gss_enabled](status.c.md#uam_gss_enabled)

Called by: [status_init](status.c.md#status_init)

### status_utf8servername

```c
static size_t status_utf8servername(char *data, int *nameoffset, const DSI *dsi, const struct afp_options *options)
```

Defined at lines 370 to 412.

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string)

Called by: [status_init](status.c.md#status_init)

Uses file-scope variables: `maxstatuslen`

### status_icon

```c
static void status_icon(char *data, const unsigned char *icondata, const size_t iconlen, const int sigoffset)
```

Defined at lines 415 to 437.

Called by: [status_init](status.c.md#status_init)

### status_init

```c
void status_init(AFPObj *dsi_obj, AFPObj *asp_obj, DSI *dsi)
```

Defined at lines 441 to 606.

Calls: [asp_setstatus](../../libatalk/asp/asp_init.c.md#asp_setstatus), [status_directorynames](status.c.md#status_directorynames), [status_flags](status.c.md#status_flags), [status_icon](status.c.md#status_icon), [status_machine](status.c.md#status_machine), [status_netaddress](status.c.md#status_netaddress), [status_server](status.c.md#status_server), [status_signature](status.c.md#status_signature), [status_uams](auth.c.md#status_uams), [status_utf8servername](status.c.md#status_utf8servername), [status_versions](auth.c.md#status_versions)

Called by: [configinit](afp_config.c.md#configinit)

Uses file-scope variables: `daemon_icon` in [etc/afpd/icon.c](icon.c.md), `declogo_icon` in [etc/afpd/icon.c](icon.c.md), `fileserver_icon` in [etc/afpd/icon.c](icon.c.md), `globe_icon` in [etc/afpd/icon.c](icon.c.md), `maxstatuslen`, `nas_icon` in [etc/afpd/icon.c](icon.c.md), `sdcard_icon` in [etc/afpd/icon.c](icon.c.md), `sunlogo_icon` in [etc/afpd/icon.c](icon.c.md), `viking_icon` in [etc/afpd/icon.c](icon.c.md)

### set_signature

```c
void set_signature(struct afp_options *options)
```

Defined at lines 618 to 812.

Set the server signature.

* If found in conf file, use it.
* If not found in conf file, genarate and append in conf file.
* If conf file don't exist, create and genarate.
* If cannot open conf file, use one-time signature.
* If signature = xxxxx, use it.

Calls: [randombytes](../../libatalk/util/unix.c.md#randombytes)

Called by: [configinit](afp_config.c.md#configinit)

### afp_getsrvrinfo

```c
int afp_getsrvrinfo(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 815 to 832.

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# File-scope variables

`maxstatuslen`
