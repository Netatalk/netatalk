---
type: C Source File
title: "etc/uams/uams_passwd.c"
description: "Cleartext UAM with Unix password authentication and the ClearTxtUAM printer login."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_passwd.c"
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
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/time.h`, `sys/types.h`, `time.h`, `unistd.h`

# Function tables

### uams_clrtxt

Initialized at line 409 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_passwd.c.md#uam_setup)

### uams_passwd

Initialized at line 415 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_passwd.c.md#uam_setup)

# Functions

### pwd_login

```c
static int pwd_login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 66 to 133.

Check a cleartext password of up to 8 bytes against the password hash.

The user has to pass uam_checkuser. With shadow passwords an expired password answers AFPERR_PWDEXPR before the password check. The password goes to crypt_checkpass, or to crypt and a comparison, and a match wipes it from the request.

Parameters:
* `obj`: session object
* `username`: user name from the request, receives the account name of a match
* `ulen`: size of the username buffer
* `uam_pwd`: receives the user entry before the password check
* `ibuf`: password field of the request, cut after 8 bytes
* `ibuflen`: length of ibuf, at least 8
* `rbuf`: reply block, unused
* `rbuflen`: unused

Returns: AFP_OK, AFPERR_PWDEXPR, AFPERR_NOTAUTH, or AFPERR_PARAM

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [passwd_login](uams_passwd.c.md#passwd_login), [passwd_login_ext](uams_passwd.c.md#passwd_login_ext)

### passwd_login

```c
static int passwd_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 151 to 186.

Read the user name of a cleartext FPLogin request and check its password against the hash.

The request holds the name as a Pascal string, padded to an even address, and then the password.

Parameters:
* `obj`: session object
* `uam_pwd`: receives the user entry
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the result of pwd_login, AFPERR_PARAM for an empty or oversized name, or AFPERR_MISC

Calls: [pwd_login](uams_passwd.c.md#pwd_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_passwd.c.md#uam_setup)

### passwd_login_ext

```c
static int passwd_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 209 to 238.

Read the user name of a cleartext FPLoginExt request and check its password against the hash.

The user name field has this form:

```
byte      3
2 bytes   len (network order)
len bytes unicode name
```

Parameters:
* `obj`: session object
* `uname`: user name field of the request
* `uam_pwd`: receives the user entry
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf, at least 2
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the result of pwd_login, AFPERR_PARAM for another name type or a bad length, or AFPERR_MISC

Calls: [pwd_login](uams_passwd.c.md#pwd_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_passwd.c.md#uam_setup)

### passwd_printer

```c
static int passwd_printer(char *start, char *stop, char *username, struct papfile *out)
```

Defined at lines 254 to 372.

Check a ClearTxtUAM printer login against the password hash.

The login string has the form `(username) (password)`, and the password keeps up to 8 bytes. The user has to pass uam_checkuser, and an expired shadow password fails. A match writes the "0" answer to out.

Parameters:
* `start`: start of the login string
* `stop`: end of the login string
* `username`: receives the user name, up to UAM_USERNAMELEN bytes without a NUL
* `out`: PAP reply, receives the answer

Returns: 0, or -1 for a malformed string, a refused user, or a wrong password

Calls: [append](../afpd/uam.c.md#append), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_passwd.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 384 to 397.

Register the Cleartxt Passwrd login and the ClearTxtUAM printer login of the passwd module.

The module has no logout and no password change.

Parameters:
* `obj`: AFP object, unused
* `path`: path of the module

Returns: 0, or -1 when a registration fails

Calls: [passwd_login](uams_passwd.c.md#passwd_login), [passwd_login_ext](uams_passwd.c.md#passwd_login_ext), [passwd_printer](uams_passwd.c.md#passwd_printer), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_clrtxt](uams_passwd.c.md#uams_clrtxt), [uams_passwd](uams_passwd.c.md#uams_passwd)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 403 to 407.

Unregister the Cleartxt Passwrd login and the ClearTxtUAM printer login of the passwd module.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_clrtxt](uams_passwd.c.md#uams_clrtxt), [uams_passwd](uams_passwd.c.md#uams_passwd)

# Macros

* Undocumented: `PASSWDLEN`
