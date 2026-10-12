---
type: C Source File
title: "etc/afpd/status.c"
description: "The FPGetSrvrInfo status block."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/status.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 57 to 63.

Tell whether afpd loaded the Kerberos UAM, Client Krb v2.

Returns: nonzero when auth_uamfind finds the UAM

Calls: [auth_uamfind](auth.c.md#auth_uamfind)

Called by: [status_directorynames](status.c.md#status_directorynames)

### status_flags

```c
static void status_flags(char *data, const int notif, const int ipok, const unsigned char passwdbits, const int dirsrvcs, int flags)
```

Defined at lines 81 to 125.

Write the flags word of the FPGetSrvrInfo reply into a status block.

Every server sets AFPSRVRINFO_COPY, AFPSRVRINFO_SRVSIGNATURE, AFPSRVRINFO_SRVMSGS, AFPSRVRINFO_FASTBOZO, AFPSRVRINFO_SRVUTF8, and AFPSRVRINFO_EXTSLEEP. The arguments decide the password, TCP/IP, server notification, directory service, and UUID bits.

Parameters:
* `data`: status block
* `notif`: nonzero for server notifications
* `ipok`: nonzero when the server has a usable IP address or a DNS name
* `passwdbits`: PASSWD_SET and PASSWD_NOSAVE bits
* `dirsrvcs`: nonzero to announce directory services
* `flags`: server options, for OPTION_UUID

Called by: [status_init](status.c.md#status_init)

### status_server

```c
static int status_server(char *data, const char *server, const struct afp_options *options)
```

Defined at lines 140 to 188.

Write the server name of the FPGetSrvrInfo reply into a status block.

The name converts to the Mac charset in a 32-byte buffer, and its length rounds up to even. Room for the signature, network address, directory names, and UTF-8 name offsets follows. The call stores the machine type offset.

Parameters:
* `data`: status block
* `server`: server name
* `options`: server options, for the charsets

Returns: the offset of the signature offset field

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string)

Called by: [status_init](status.c.md#status_init)

### status_machine

```c
static void status_machine(char *data)
```

Defined at lines 198 to 227.

Write the machine type of the FPGetSrvrInfo reply into a status block.

The machine type is Netatalk and the version, cut to AFPSTATUS_MACHLEN bytes. The call stores the AFP version list offset after it.

Parameters:
* `data`: status block with the machine type offset set

Called by: [status_init](status.c.md#status_init)

### status_signature

```c
static uint16_t status_signature(char *data, int *servoffset, const struct afp_options *options)
```

Defined at lines 241 to 260.

Write the 16-byte server signature of the FPGetSrvrInfo reply into a status block.

The call stores the network address offset after the signature.

Parameters:
* `data`: status block
* `servoffset`: offset of the signature offset field, advanced to the network address offset field
* `options`: server options with the signature

Returns: the offset of the signature

Called by: [status_init](status.c.md#status_init)

### status_netaddress

```c
static size_t status_netaddress(char *data, int *servoffset, const ASP asp, const DSI *dsi, const struct afp_options *options)
```

Defined at lines 279 to 405.

Write the network address list of the FPGetSrvrInfo reply into a status block.

A [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener adds its IPv4 or IPv6 address, with the port when it is not

1. A DNS name adds an FQDN entry, and with OPTION_ANNOUNCESSH an SSH tunnel entry, while room remains. An [ASP](../../include/atalk/asp.h.md#struct-asp) listener adds its DDP address. The call stores the directory names offset after the list.

Parameters:
* `data`: status block
* `servoffset`: offset of the network address offset field, advanced to the next offset field
* `asp`: [ASP](../../include/atalk/asp.h.md#struct-asp) listener, or NULL
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener, or NULL
* `options`: server options with the DNS name

Returns: the length of the status block so far

Called by: [status_init](status.c.md#status_init)

Uses file-scope variables: `maxstatuslen`

### status_directorynames

```c
static size_t status_directorynames(char *data, int *diroffset, const DSI *dsi, const struct afp_options *options)
```

Defined at lines 428 to 453.

Build the DirectoryNames field of the status reply.

```
DirectoryNamesCount offset: uint16_t
...
DirectoryNamesCount: uint8_t
DirectoryNames: list of UTF-8 Pascal strings (uint8_t + char[1,255])
```

With the Kerberos UAM and a k5 principal, the list holds the principal. Otherwise the count is 0.

Parameters:
* `data`: status block
* `diroffset`: offset of the directory names offset field, advanced to the UTF-8 name offset field
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener, unused
* `options`: server options with the Kerberos principal

Returns: the length of the status block so far

Calls: [uam_gss_enabled](status.c.md#uam_gss_enabled)

Called by: [status_init](status.c.md#status_init)

### status_utf8servername

```c
static size_t status_utf8servername(char *data, int *nameoffset, const DSI *dsi, const struct afp_options *options)
```

Defined at lines 469 to 511.

Write the UTF-8 server name of the FPGetSrvrInfo reply into a status block.

The name, the server name option or the host name, converts to UTF8-MAC behind a 2-byte length. An [ASP](../../include/atalk/asp.h.md#struct-asp) status, or a failed conversion, sets the offset to 0.

Parameters:
* `data`: status block
* `nameoffset`: offset of the UTF-8 name offset field
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener, or NULL for the [ASP](../../include/atalk/asp.h.md#struct-asp) status
* `options`: server options with the names and charsets

Returns: the length of the status block

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string)

Called by: [status_init](status.c.md#status_init)

Uses file-scope variables: `maxstatuslen`

### status_icon

```c
static void status_icon(char *data, const unsigned char *icondata, const size_t iconlen, const int sigoffset)
```

Defined at lines 526 to 548.

Write the volume icon of the FPGetSrvrInfo reply into a status block and place the signature after it.

Without icon data the icon offset becomes 0, and the signature takes the place of the icon.

Parameters:
* `data`: status block
* `icondata`: icon and mask, or NULL
* `iconlen`: length of icondata
* `sigoffset`: offset of the signature offset field, or 0 to skip it

Called by: [status_init](status.c.md#status_init)

### status_init

```c
void status_init(AFPObj *dsi_obj, AFPObj *asp_obj, DSI *dsi)
```

Defined at lines 564 to 729.

Build the FPGetSrvrInfo replies of a [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener and of the [ASP](../../include/atalk/asp.h.md#struct-asp) server.

The parts go in a fixed order, because each one stores the offset of the next. The flags announce TCP/IP only with a DNS name or a usable listener address. The legacy icon option picks the volume icon. The [ASP](../../include/atalk/asp.h.md#struct-asp) reply skips the default icon to save room.

Parameters:
* `dsi_obj`: server object for AFP over TCP
* `asp_obj`: server object for AFP over AppleTalk, which receives the [ASP](../../include/atalk/asp.h.md#struct-asp) status
* `dsi`: listener that receives the [DSI](../../include/atalk/dsi.h.md#struct-dsi) status

Calls: [asp_setstatus](../../libatalk/asp/asp_init.c.md#asp_setstatus), [status_directorynames](status.c.md#status_directorynames), [status_flags](status.c.md#status_flags), [status_icon](status.c.md#status_icon), [status_machine](status.c.md#status_machine), [status_netaddress](status.c.md#status_netaddress), [status_server](status.c.md#status_server), [status_signature](status.c.md#status_signature), [status_uams](auth.c.md#status_uams), [status_utf8servername](status.c.md#status_utf8servername), [status_versions](auth.c.md#status_versions)

Called by: [configinit](afp_config.c.md#configinit)

Uses file-scope variables: `daemon_icon` in [etc/afpd/icon.c](icon.c.md), `declogo_icon` in [etc/afpd/icon.c](icon.c.md), `fileserver_icon` in [etc/afpd/icon.c](icon.c.md), `globe_icon` in [etc/afpd/icon.c](icon.c.md), `maxstatuslen`, `nas_icon` in [etc/afpd/icon.c](icon.c.md), `sdcard_icon` in [etc/afpd/icon.c](icon.c.md), `sunlogo_icon` in [etc/afpd/icon.c](icon.c.md), `viking_icon` in [etc/afpd/icon.c](icon.c.md)

### set_signature

```c
void set_signature(struct afp_options *options)
```

Defined at lines 743 to 937.

Set the 16-byte server signature.

The signature option gives up to 16 bytes directly. Otherwise afp_signature.conf holds a line per host name with 32 hex digits. A host without a line gets 16 random bytes, which the call appends. When afpd cannot read, write, or create the file, the random signature lasts for this run only.

Parameters:
* `options`: server options with the signature option, the host name, and the file path

Calls: [randombytes](../../libatalk/util/unix.c.md#randombytes)

Called by: [configinit](afp_config.c.md#configinit)

### afp_getsrvrinfo

```c
int afp_getsrvrinfo(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 951 to 968.

FPGetSrvrInfo: return the status block of the session transport.

The reply matches DSIGetStatus and ASPGetStatus.

Parameters:
* `obj`: session object
* `ibuf`: request block, unused
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# File-scope variables

`maxstatuslen`
