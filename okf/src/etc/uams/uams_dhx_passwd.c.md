---
type: C Source File
title: "etc/uams/uams_dhx_passwd.c"
description: "DHCAST128 UAM with Unix password authentication."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_dhx_passwd.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Initialized at line 562 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

### uams_dhx_passwd

Initialized at line 568 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

# Functions

### dhx_release_key

```c
static void dhx_release_key(void)
```

Defined at lines 56 to 62.

Release the session key K of a DHCAST128 exchange.

Called by: [passwd_logincont](uams_dhx_passwd.c.md#passwd_logincont), [pwd_login](uams_dhx_passwd.c.md#pwd_login), [uam_cleanup](uams_dhx_passwd.c.md#uam_cleanup)

### pwd_login

```c
static int pwd_login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 89 to 255.

Look up the user of a DHCAST128 login and answer the public key of the client with Mb and a challenge.

The user has to pass uam_checkuser and needs a password hash, from the shadow file in a build that uses one. The group is the fixed 128-bit prime of the protocol with generator 7. The reply holds the session ID, Mb, and under CAST5-CBC a 16-byte random challenge and 16 zero bytes in the place of the server signature.

Parameters:
* `obj`: session object, which also seeds the session ID
* `username`: user name from the request, receives the account name of a match
* `ulen`: size of the username buffer
* `uam_pwd`: unused
* `ibuf`: Ma of the client, 16 bytes
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_NOTAUTH for an unknown or refused user, AFPERR_PARAM when a step fails, or AFPERR_MISC

Calls: [dhx_release_key](uams_dhx_passwd.c.md#dhx_release_key), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [passwd_login](uams_dhx_passwd.c.md#passwd_login), [passwd_login_ext](uams_dhx_passwd.c.md#passwd_login_ext)

### passwd_login

```c
static int passwd_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 271 to 306.

Read the user name of an FPLogin request and start a DHCAST128 login.

The request holds the name as a Pascal string, padded to an even address.

Parameters:
* `obj`: session object
* `uam_pwd`: unused until the continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_PARAM for an empty or oversized name, AFPERR_NOTAUTH, or AFPERR_MISC

Calls: [pwd_login](uams_dhx_passwd.c.md#pwd_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

### passwd_login_ext

```c
static int passwd_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 329 to 358.

Read the user name of an FPLoginExt request and start a DHCAST128 login.

The user name field has this form:

```
byte      3
2 bytes   len (network order)
len bytes utf8 name
```

Parameters:
* `obj`: session object
* `uname`: user name field of the request
* `uam_pwd`: unused until the continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_PARAM for another name type or a bad length, AFPERR_NOTAUTH, or AFPERR_MISC

Calls: [pwd_login](uams_dhx_passwd.c.md#pwd_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

### passwd_logincont

```c
static int passwd_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 379 to 524.

Check the challenge answer and the password of a DHCAST128 login against the password hash.

The request holds the session ID and, under CAST5-CBC, the challenge plus 1 and a 64-byte password field. The password goes to crypt_checkpass, or to crypt and a comparison. With shadow passwords an expired password gives AFPERR_PWDEXPR, and that check runs after a wrong password too. The call wipes the challenge and the password.

Parameters:
* `obj`: session object
* `uam_pwd`: receives the user entry after a matching password
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf, at least 2 + CRYPT2BUFLEN
* `rbuf`: reply block, which the call uses as a work buffer and clears
* `rbuflen`: 0 on return

Returns: AFP_OK, AFPERR_PWDEXPR, AFPERR_NOTAUTH, or AFPERR_PARAM

Calls: [dhx_release_key](uams_dhx_passwd.c.md#dhx_release_key), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [uam_setup](uams_dhx_passwd.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 536 to 547.

Register the DHCAST128 login of the passwd module.

The module has no logout and no password change.

Parameters:
* `obj`: AFP object, unused
* `path`: path of the module

Returns: 0, or -1 when the registration fails

Calls: [passwd_login](uams_dhx_passwd.c.md#passwd_login), [passwd_login_ext](uams_dhx_passwd.c.md#passwd_login_ext), [passwd_logincont](uams_dhx_passwd.c.md#passwd_logincont), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_dhx](uams_dhx_passwd.c.md#uams_dhx), [uams_dhx_passwd](uams_dhx_passwd.c.md#uams_dhx_passwd)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 553 to 560.

Release the session key and unregister the DHCAST128 login of the passwd module.

Calls: [dhx_release_key](uams_dhx_passwd.c.md#dhx_release_key), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_dhx](uams_dhx_passwd.c.md#uams_dhx), [uams_dhx_passwd](uams_dhx_passwd.c.md#uams_dhx_passwd)

# Macros

* `dhxhash`: hash a number to a 16-bit quantity
* Undocumented: `CRYPT2BUFLEN`, `CRYPTBUFLEN`, `KEYSIZE`, `PASSWDLEN`

# File-scope variables

`K`, `dhxpwd`, `randbuf`
