---
type: C Source File
title: "etc/afpd/messages.c"
description: "FPGetSrvrMsg: the login message, and the server messages from the message file."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/messages.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [misc.h](misc.h.md)
* System headers: `errno.h`, `inttypes.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`

# Functions

### setmessage

```c
int setmessage(const char *)
```

Defined at lines 44 to 52. Declared in [include/atalk/globals.h](../../include/atalk/globals.h.md).

Copy AFP message to message buffer.

Parameters:
* `message`: message to send

Returns: 0 if this message is being set the first time, return 1 if the preceeding message was the same

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afp_openvol](volume.c.md#afp_openvol), [cname](directory.c.md#cname), [handle_timedown](afp_dsi.c.md#handle_timedown)

Uses file-scope variables: `servermesg`

### readmessage

```c
void readmessage(AFPObj *)
```

Defined at lines 64 to 124. Declared in [include/atalk/globals.h](../../include/atalk/globals.h.md).

Load the server message for a session from its message file.

The file SERVERTEXT/message.pid of the session comes first, then the shared SERVERTEXT/message. The call turns line breaks into spaces, stops at the message size limit of the transport, and deletes the file as root. A build without SERVERTEXT does nothing.

Parameters:
* `obj`: session object

Calls: [become_root](../../libatalk/util/unix.c.md#become_root), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Called by: [afp_asp_getmesg_now](afp_asp.c.md#afp_asp_getmesg_now), [pending_request](afp_dsi.c.md#pending_request)

Uses file-scope variables: `servermesg`

### afp_getsrvrmesg

```c
int afp_getsrvrmesg(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 142 to 225.

FPGetSrvrMsg: return the login or the server message.

A login message request also appends the login message option to the server message. Mac OS X 10.4 drops a server message attention that comes before it asks for the login message, and the merged text covers that. The bitmap selects a UTF-8 reply with a 2-byte length or a Mac charset reply with a 1-byte length.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK, or AFPERR_BITMAP for an unknown message type

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat)

Uses file-scope variables: `localized_message`, `servermesg`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Macros

* Undocumented: `MAXMESGSIZE`

# File-scope variables

`localized_message`, `servermesg`
