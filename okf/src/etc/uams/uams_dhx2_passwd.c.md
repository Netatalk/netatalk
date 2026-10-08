---
type: C Source File
title: "etc/uams/uams_dhx2_passwd.c"
description: "12 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_dhx2_passwd.c"
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

### uams_dhx2

Initialized at line 757 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx2_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

### uams_dhx2_passwd

Initialized at line 764 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx2_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

# Functions

### dhx2_release_mpi

```c
static void dhx2_release_mpi(gcry_mpi_t *mpi)
```

Defined at lines 60 to 66.

Called by: [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session), [logincont1](uams_dhx2_passwd.c.md#logincont1)

### dhx2_clear_session

```c
static void dhx2_clear_session(void)
```

Defined at lines 68 to 78.

Calls: [dhx2_release_mpi](uams_dhx2_passwd.c.md#dhx2_release_mpi)

Called by: [dhx2_setup](uams_dhx2_passwd.c.md#dhx2_setup), [logincont1](uams_dhx2_passwd.c.md#logincont1), [logincont2](uams_dhx2_passwd.c.md#logincont2), [passwd_logincont](uams_dhx2_passwd.c.md#passwd_logincont)

Uses file-scope variables: `ID`, `K_MD5hash`, `K_hash_len`, `Ra`, `p`, `serverNonce`

### dh_params_generate

```c
static int dh_params_generate(gcry_mpi_t *ret_p, gcry_mpi_t *ret_g, unsigned int bits)
```

Defined at lines 102 to 183.

Generate a new pair of prime and generator for use in the Diffie-Hellman key exchange.

The bits value should be one of 768, 1024, 2048, 3072 or 4096.

Called by: [dhx2_setup](uams_dhx2_passwd.c.md#dhx2_setup)

### dhx2_setup

```c
static int dhx2_setup(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 185 to 312.

Calls: [dh_params_generate](uams_dhx2_passwd.c.md#dh_params_generate), [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session)

Called by: [login](uams_dhx2_passwd.c.md#login)

Uses file-scope variables: `ID`, `Ra`, `dhxpwd`, `p`

### login

```c
static int login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 315 to 327.

Calls: [dhx2_setup](uams_dhx2_passwd.c.md#dhx2_setup), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [passwd_login](uams_dhx2_passwd.c.md#passwd_login), [passwd_login_ext](uams_dhx2_passwd.c.md#passwd_login_ext)

Uses file-scope variables: `dhxpwd`

### passwd_login

```c
static int passwd_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 335 to 370.

dhx login

Note: things are done in a slightly bizarre order to avoid having to clean things up if there's an error.

Calls: [login](uams_dhx2_passwd.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

### passwd_login_ext

```c
static int passwd_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 373 to 408.

Calls: [login](uams_dhx2_passwd.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

### logincont1

```c
static int logincont1(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 412 to 575.

Calls: [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session), [dhx2_release_mpi](uams_dhx2_passwd.c.md#dhx2_release_mpi)

Called by: [passwd_logincont](uams_dhx2_passwd.c.md#passwd_logincont)

Uses file-scope variables: `ID`, `K_MD5hash`, `K_hash_len`, `Ra`, `dhx_c2siv`, `dhx_s2civ`, `p`, `serverNonce`

### logincont2

```c
static int logincont2(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 577 to 715.

Calls: [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [passwd_logincont](uams_dhx2_passwd.c.md#passwd_logincont)

Uses file-scope variables: `K_MD5hash`, `K_hash_len`, `dhx_c2siv`, `dhxpwd`, `p`, `serverNonce`

### passwd_logincont

```c
static int passwd_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 717 to 739.

Calls: [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session), [logincont1](uams_dhx2_passwd.c.md#logincont1), [logincont2](uams_dhx2_passwd.c.md#logincont2)

Called by: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

Uses file-scope variables: `ID`

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 741 to 749.

Calls: [passwd_login](uams_dhx2_passwd.c.md#passwd_login), [passwd_login_ext](uams_dhx2_passwd.c.md#passwd_login_ext), [passwd_logincont](uams_dhx2_passwd.c.md#passwd_logincont), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_dhx2](uams_dhx2_passwd.c.md#uams_dhx2), [uams_dhx2_passwd](uams_dhx2_passwd.c.md#uams_dhx2_passwd)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 751 to 754.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_dhx2](uams_dhx2_passwd.c.md#uams_dhx2), [uams_dhx2_passwd](uams_dhx2_passwd.c.md#uams_dhx2_passwd)

# Typedefs and enums

* `enum dhx2_state`: `DHX2_STATE_IDLE`, `DHX2_STATE_EXPECT_CONT1`, `DHX2_STATE_EXPECT_CONT2`

# Macros

* Undocumented: `PASSWDBUFLEN`, `PRIMEBITS`, `dhxhash`

# File-scope variables

`ID`, `K_MD5hash`, `K_hash_len`, `Ra`, `dhx2_state`, `dhx_c2siv`, `dhx_s2civ`, `dhxpwd`, `p`, `serverNonce`
