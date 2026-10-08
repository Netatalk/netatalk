---
type: C Source File
title: "etc/uams/uams_passwd.c"
description: "6 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_passwd.c"
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
* System headers: `arpa/inet.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/time.h`, `sys/types.h`, `time.h`, `unistd.h`

# Function tables

### uams_clrtxt

Initialized at line 331 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_passwd.c.md#uam_setup)

### uams_passwd

Initialized at line 337 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_passwd.c.md#uam_setup)

# Functions

### pwd_login

```c
static int pwd_login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 41 to 108.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [passwd_login](uams_passwd.c.md#passwd_login), [passwd_login_ext](uams_passwd.c.md#passwd_login_ext)

### passwd_login

```c
static int passwd_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 111 to 146.

cleartxt login

Calls: [pwd_login](uams_passwd.c.md#pwd_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_passwd.c.md#uam_setup)

### passwd_login_ext

```c
static int passwd_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 158 to 187.

cleartxt login ext

uname format :

```
byte      3
2 bytes   len (network order)
len bytes unicode name
```

Calls: [pwd_login](uams_passwd.c.md#pwd_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_passwd.c.md#uam_setup)

### passwd_printer

```c
static int passwd_printer(char *start, char *stop, char *username, struct papfile *out)
```

Defined at lines 190 to 308.

Printer ClearTxtUAM login

Calls: [append](../afpd/uam.c.md#append), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_passwd.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 310 to 323.

Calls: [passwd_login](uams_passwd.c.md#passwd_login), [passwd_login_ext](uams_passwd.c.md#passwd_login_ext), [passwd_printer](uams_passwd.c.md#passwd_printer), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_clrtxt](uams_passwd.c.md#uams_clrtxt), [uams_passwd](uams_passwd.c.md#uams_passwd)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 325 to 329.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_clrtxt](uams_passwd.c.md#uams_clrtxt), [uams_passwd](uams_passwd.c.md#uams_passwd)

# Macros

* Undocumented: `PASSWDLEN`
