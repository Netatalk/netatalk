---
type: C Source File
title: "etc/papd/uam.c"
description: "7 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/uam.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [uam_auth.h](uam_auth.h.md)
* System headers: `ctype.h`, `fcntl.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/time.h`, `unistd.h`

# Functions

### uam_load

```c
struct uam_mod * uam_load(const char *path, const char *name)
```

Defined at lines 31 to 82.

Calls: [mod_close](../../include/atalk/util.h.md#mod_close), [mod_open](../../include/atalk/util.h.md#mod_open), [mod_symbol](../../include/atalk/util.h.md#mod_symbol), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [auth_load](auth.c.md#auth_load)

Calls through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export): [uam_setup](../uams/uams_dhx2_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx2_passwd.c.md#uam_setup), [uam_setup](../uams/uams_dhx_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx_passwd.c.md#uam_setup), [uam_setup](../uams/uams_gss.c.md#uam_setup), [uam_setup](../uams/uams_guest.c.md#uam_setup), [uam_setup](../uams/uams_pam.c.md#uam_setup), [uam_setup](../uams/uams_passwd.c.md#uam_setup), [uam_setup](../uams/uams_randnum.c.md#uam_setup), [uam_setup](../uams/uams_srp.c.md#uam_setup)

### uam_unload

```c
void uam_unload(struct uam_mod *mod)
```

Defined at lines 90 to 98.

unload the module.

we check for a cleanup function, but we don't die if one doesn't exist. however, things are likely to leak without one.

Calls: [mod_close](../../include/atalk/util.h.md#mod_close)

Called by: [auth_unload](auth.c.md#auth_unload)

Calls through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export): [uam_cleanup](../uams/uams_dhx2_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx2_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_gss.c.md#uam_cleanup), [uam_cleanup](../uams/uams_guest.c.md#uam_cleanup), [uam_cleanup](../uams/uams_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_randnum.c.md#uam_cleanup), [uam_cleanup](../uams/uams_srp.c.md#uam_cleanup)

### uam_register

```c
int uam_register(const int type, const char *path, const char *name,...)
```

Defined at lines 103 to 168.

set up stuff for this uam.

Calls: [auth_register](auth.c.md#auth_register), [auth_uamfind](auth.c.md#auth_uamfind)

### uam_unregister

```c
void uam_unregister(const int type, const char *name)
```

Defined at lines 170 to 187.

Calls: [auth_uamfind](auth.c.md#auth_uamfind)

### uam_afpserver_option

```c
int uam_afpserver_option(void *private, const int what, void *option, size_t *len)
```

Defined at lines 190 to 194.

Crap to support uams which call this afpd function

### uam_getname

```c
struct passwd * uam_getname(void *private, char *name, const int len)
```

Defined at lines 198 to 233.

helper functions for plugin uams

Parameters:
* `private`: pointer to [AFPObj](../../include/atalk/globals.h.md#struct-afpobj)
* `name`: user name
* `len`: size of name buffer.

### uam_checkuser

```c
int uam_checkuser(void *private, const struct passwd *pwd)
```

Defined at lines 238 to 261.
