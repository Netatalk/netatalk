---
type: C Source File
title: "etc/uams/uams_dhx_passwd.c"
description: "7 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_dhx_passwd.c"
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
* System headers: `arpa/inet.h`, `errno.h`, `gcrypt.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/time.h`, `time.h`, `unistd.h`

# Function tables

### uams_dhx

Initialized at line 477 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

### uams_dhx_passwd

Initialized at line 483 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

# Functions

### dhx_release_key

```c
static void dhx_release_key(void)
```

Defined at lines 48 to 54.

Called by: [passwd_logincont](uams_dhx_passwd.c.md#passwd_logincont), [pwd_login](uams_dhx_passwd.c.md#pwd_login), [uam_cleanup](uams_dhx_passwd.c.md#uam_cleanup)

Uses file-scope variables: `K`

### pwd_login

```c
static int pwd_login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 60 to 226.

dhx passwd

Calls: [dhx_release_key](uams_dhx_passwd.c.md#dhx_release_key), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [passwd_login](uams_dhx_passwd.c.md#passwd_login), [passwd_login_ext](uams_dhx_passwd.c.md#passwd_login_ext)

Uses file-scope variables: `K`, `dhxpwd`, `randbuf`

### passwd_login

```c
static int passwd_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 229 to 264.

cleartxt login

Calls: [pwd_login](uams_dhx_passwd.c.md#pwd_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

### passwd_login_ext

```c
static int passwd_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 276 to 305.

cleartxt login ext

uname format :

```
byte      3
2 bytes   len (network order)
len bytes utf8 name
```

Calls: [pwd_login](uams_dhx_passwd.c.md#pwd_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

### passwd_logincont

```c
static int passwd_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 307 to 452.

Calls: [dhx_release_key](uams_dhx_passwd.c.md#dhx_release_key), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

Uses file-scope variables: `K`, `dhxpwd`, `randbuf`

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 455 to 466.

Calls: [passwd_login](uams_dhx_passwd.c.md#passwd_login), [passwd_login_ext](uams_dhx_passwd.c.md#passwd_login_ext), [passwd_logincont](uams_dhx_passwd.c.md#passwd_logincont), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_dhx](uams_dhx_passwd.c.md#uams_dhx), [uams_dhx_passwd](uams_dhx_passwd.c.md#uams_dhx_passwd)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 468 to 475.

Calls: [dhx_release_key](uams_dhx_passwd.c.md#dhx_release_key), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_dhx](uams_dhx_passwd.c.md#uams_dhx), [uams_dhx_passwd](uams_dhx_passwd.c.md#uams_dhx_passwd)

# Macros

* Undocumented: `CRYPT2BUFLEN`, `CRYPTBUFLEN`, `KEYSIZE`, `PASSWDLEN`, `dhxhash`

# File-scope variables

`K`, `dhxpwd`, `randbuf`
