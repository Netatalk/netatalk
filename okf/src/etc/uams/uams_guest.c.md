---
type: C Source File
title: "etc/uams/uams_guest.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_guest.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Initialized at line 190 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_guest.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_guest.c.md#uam_setup)

# Functions

### noauth_login

```c
static int noauth_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 40 to 69.

Log in as the guest account of afp.conf without a password.

The call writes the guest name into the session user name, unbounded.

Parameters:
* `obj`: session object
* `uam_pwd`: receives the user entry of the guest account
* `ibuf`: UAM data of the request, unused
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFP_OK, AFPERR_BADUAM for a missing guest account, or AFPERR_MISC

Calls: [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [noauth_login_ext](uams_guest.c.md#noauth_login_ext), [uam_setup](uams_guest.c.md#uam_setup)

### noauth_login_ext

```c
static int noauth_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 84 to 89.

Log in an FPLoginExt request as the guest account, ignoring the user name.

Parameters:
* `obj`: session object
* `uname`: user name field of the request, unused
* `uam_pwd`: receives the user entry of the guest account
* `ibuf`: UAM data of the request, unused
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the result of noauth_login

Calls: [noauth_login](uams_guest.c.md#noauth_login)

Called by: [uam_setup](uams_guest.c.md#uam_setup)

### noauth_printer

```c
static int noauth_printer(char *start, char *stop, char *username, struct papfile *out)
```

Defined at lines 106 to 156.

Accept a NoAuthUAM printer login for any existing user name, without a password.

The login string carries the name in parentheses, and the name ends at the first closing parenthesis. The call stores up to UAM_USERNAMELEN bytes of it in username without adding a NUL. A known user gets the "0" answer in out.

Parameters:
* `start`: start of the login string
* `stop`: end of the login string
* `username`: receives the user name
* `out`: PAP reply, receives the answer

Returns: 0, or -1 for a malformed string, an unknown user, or a failed malloc

Calls: [append](../afpd/uam.c.md#append), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [uam_setup](uams_guest.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *handle, const char *path)
```

Defined at lines 166 to 179.

Register the No User Authent login and the NoAuthUAM printer login.

Parameters:
* `handle`: AFP object, unused
* `path`: path of the module

Returns: 0, or -1 when a registration fails

Calls: [noauth_login](uams_guest.c.md#noauth_login), [noauth_login_ext](uams_guest.c.md#noauth_login_ext), [noauth_printer](uams_guest.c.md#noauth_printer), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_guest](uams_guest.c.md#uams_guest)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 184 to 188.

Unregister the No User Authent login and the NoAuthUAM printer login.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_guest](uams_guest.c.md#uams_guest)
