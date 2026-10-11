---
type: C Source File
title: "etc/papd/uam.c"
description: "7 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/uam.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 43 to 94.

Load a UAM module for papd and run its setup.

The module has to export a server UAM under its file name without the extension, such as uams_pam for uams_pam.so. Its uam_setup runs without a server object, since papd has none. Each failure except a missing UAM symbol logs.

Parameters:
* `path`: module path
* `name`: module file name

Returns: the module record, or NULL when the module does not open, lacks the UAM, or fails its setup

Calls: [mod_close](../../include/atalk/util.h.md#mod_close), [mod_open](../../include/atalk/util.h.md#mod_open), [mod_symbol](../../include/atalk/util.h.md#mod_symbol), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [auth_load](auth.c.md#auth_load)

Calls through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export): [uam_setup](../uams/uams_dhx2_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx2_passwd.c.md#uam_setup), [uam_setup](../uams/uams_dhx_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx_passwd.c.md#uam_setup), [uam_setup](../uams/uams_gss.c.md#uam_setup), [uam_setup](../uams/uams_guest.c.md#uam_setup), [uam_setup](../uams/uams_pam.c.md#uam_setup), [uam_setup](../uams/uams_passwd.c.md#uam_setup), [uam_setup](../uams/uams_randnum.c.md#uam_setup), [uam_setup](../uams/uams_srp.c.md#uam_setup)

### uam_unload

```c
void uam_unload(struct uam_mod *mod)
```

Defined at lines 99 to 107.

Run the cleanup function of a UAM module, close it, and free its entry.

A module without a cleanup function still closes, but it may leak.

Parameters:
* `mod`: loaded module

Calls: [mod_close](../../include/atalk/util.h.md#mod_close)

Called by: [auth_unload](auth.c.md#auth_unload)

Calls through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export): [uam_cleanup](../uams/uams_dhx2_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx2_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_gss.c.md#uam_cleanup), [uam_cleanup](../uams/uams_guest.c.md#uam_cleanup), [uam_cleanup](../uams/uams_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_randnum.c.md#uam_cleanup), [uam_cleanup](../uams/uams_srp.c.md#uam_cleanup)

### uam_register

```c
int uam_register(const int type, const char *path, const char *name,...)
```

Defined at lines 117 to 182.

Register a UAM of a module under a name and type.

In papd UAM_SERVER_PRINTAUTH passes one handler, the uam_printer function, and a UAM_SERVER_LOGIN_EXT registration returns 1.

Calls: [auth_register](auth.c.md#auth_register), [auth_uamfind](auth.c.md#auth_uamfind)

### uam_unregister

```c
void uam_unregister(const int type, const char *name)
```

Defined at lines 187 to 204.

Drop a registration of a UAM and free the UAM after its last one.

Parameters:
* `type`: UAM type
* `name`: UAM name

Calls: [auth_uamfind](auth.c.md#auth_uamfind)

### uam_afpserver_option

```c
int uam_afpserver_option(void *private, const int what, void *option, size_t *len)
```

Defined at lines 213 to 217.

Answer a UAM request for a server or session value with nothing, for UAMs loaded by papd.

Answer a UAM request for a server or session value.

In papd the call is a stub that sets nothing and returns 0 for every code, since papd keeps no session values.

### uam_getname

```c
struct passwd * uam_getname(void *private, char *name, const int len)
```

Defined at lines 228 to 263.

Find the password entry of a login name for a UAM.

In papd the call skips the domain forms of afpd. After getpwnam fails, it lowers the case of name in place. It then compares the name with the login name and the first gecos field of every account.

### uam_checkuser

```c
int uam_checkuser(void *private, const struct passwd *pwd)
```

Defined at lines 271 to 294.

Check that a user may log in through a UAM.

In papd the shell check always runs, since papd.conf has no option to turn it off, and an empty shell fails as well.
