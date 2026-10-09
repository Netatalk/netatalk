---
type: C Source File
title: "etc/afpd/messages.c"
description: "3 functions, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/messages.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-09T22:45:51+11:00 }
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

Defined at lines 38 to 46. Declared in [include/atalk/globals.h](../../include/atalk/globals.h.md).

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

Defined at lines 48 to 108. Declared in [include/atalk/globals.h](../../include/atalk/globals.h.md).

Calls: [become_root](../../libatalk/util/unix.c.md#become_root), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Called by: [pending_request](afp_dsi.c.md#pending_request)

Uses file-scope variables: `servermesg`

### afp_getsrvrmesg

```c
int afp_getsrvrmesg(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 110 to 193.

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat)

Uses file-scope variables: `localized_message`, `servermesg`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Macros

* Undocumented: `MAXMESGSIZE`

# File-scope variables

`localized_message`, `servermesg`
