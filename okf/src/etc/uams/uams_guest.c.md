---
type: C Source File
title: "etc/uams/uams_guest.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_guest.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/uams](../uams.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/uam.h](../../include/atalk/uam.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`

# Function tables

### uams_guest

Initialized at line 142 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_guest.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_guest.c.md#uam_setup)

# Functions

### noauth_login

```c
static int noauth_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 28 to 57.

login and login_ext are almost the same

Calls: [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [noauth_login_ext](uams_guest.c.md#noauth_login_ext), [uam_setup](uams_guest.c.md#uam_setup)

### noauth_login_ext

```c
static int noauth_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 59 to 64.

Calls: [noauth_login](uams_guest.c.md#noauth_login)

Called by: [uam_setup](uams_guest.c.md#uam_setup)

### noauth_printer

```c
static int noauth_printer(char *start, char *stop, char *username, struct papfile *out)
```

Defined at lines 68 to 118.

Printer NoAuthUAM Login

Calls: [append](../afpd/uam.c.md#append), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [uam_setup](uams_guest.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *handle, const char *path)
```

Defined at lines 121 to 134.

Calls: [noauth_login](uams_guest.c.md#noauth_login), [noauth_login_ext](uams_guest.c.md#noauth_login_ext), [noauth_printer](uams_guest.c.md#noauth_printer), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_guest](uams_guest.c.md#uams_guest)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 136 to 140.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_guest](uams_guest.c.md#uams_guest)
