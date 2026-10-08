---
type: C Source File
title: "etc/uams/uams_randnum.c"
description: "15 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_randnum.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/uams](../uams.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/constant_time.h](../../include/atalk/constant_time.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/uam.h](../../include/atalk/uam.h.md)
* System headers: `arpa/inet.h`, `crack.h`, `ctype.h`, `errno.h`, `fcntl.h`, `gcrypt.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `unistd.h`

# Function tables

### uams_randnum

Initialized at line 737 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_randnum.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_randnum.c.md#uam_setup)

# Functions

### unhex

```c
static int unhex(unsigned char x)
```

Defined at lines 54 to 57.

Called by: [afppasswd](uams_randnum.c.md#afppasswd), [afppasswd_read_keyfile](uams_randnum.c.md#afppasswd_read_keyfile)

### randnum_cipher_check

```c
static int randnum_cipher_check(const char *op, gcry_error_t err)
```

Defined at lines 59 to 68.

Called by: [afppasswd](uams_randnum.c.md#afppasswd)

### afppasswd_open_keyfile

```c
static int afppasswd_open_keyfile(const char *path, const int pathlen)
```

Defined at lines 70 to 95.

Calls: [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afppasswd](uams_randnum.c.md#afppasswd), [randnum_warn_passwdfile_key](uams_randnum.c.md#randnum_warn_passwdfile_key)

### afppasswd_read_keyfile

```c
static int afppasswd_read_keyfile(int keyfd, uint8_t key[DES_KEY_SZ])
```

Defined at lines 97 to 138.

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [unhex](uams_randnum.c.md#unhex)

Called by: [afppasswd](uams_randnum.c.md#afppasswd), [randnum_warn_passwdfile_key](uams_randnum.c.md#randnum_warn_passwdfile_key)

### randnum_warn_passwdfile_key

```c
static void randnum_warn_passwdfile_key(void *obj)
```

Defined at lines 140 to 182.

Calls: [afppasswd_open_keyfile](uams_randnum.c.md#afppasswd_open_keyfile), [afppasswd_read_keyfile](uams_randnum.c.md#afppasswd_read_keyfile), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

### afppasswd

```c
static int afppasswd(const struct passwd *pwd, const char *path, const int pathlen, unsigned char *passwd, int len, const int set)
```

Defined at lines 207 to 363.

handle /path/afppasswd with a required key file. we're a lot more trusting of this file.

here are the formats:

Note: we use our own password entry writing bits as we want to avoid tromping over global variables. in addition, we require a key file and fail if it is not available.

Calls: [afppasswd_open_keyfile](uams_randnum.c.md#afppasswd_open_keyfile), [afppasswd_read_keyfile](uams_randnum.c.md#afppasswd_read_keyfile), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [randnum_cipher_check](uams_randnum.c.md#randnum_cipher_check), [unhex](uams_randnum.c.md#unhex)

Called by: [randpass](uams_randnum.c.md#randpass)

### randpass

```c
static int randpass(const struct passwd *pwd, const char *file, unsigned char *passwd, const int len, const int set)
```

Defined at lines 370 to 396.

this sets the uid.

Note: the afppasswd file must be read and updated as root.

Calls: [afppasswd](uams_randnum.c.md#afppasswd)

Called by: [rand_login](uams_randnum.c.md#rand_login), [randnum_changepw](uams_randnum.c.md#randnum_changepw)

### rand_login

```c
static int rand_login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 402 to 452.

randnum sends an 8-byte number and uses the user's password to check against the encrypted reply.

Calls: [randpass](uams_randnum.c.md#randpass), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [randnum_login](uams_randnum.c.md#randnum_login), [randnum_login_ext](uams_randnum.c.md#randnum_login_ext)

Uses file-scope variables: `randbuf`, `randpwd`, `seskey`

### randnum_logincont

```c
static int randnum_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 460 to 492.

check encrypted reply.

Note: we actually setup the encryption stuff here as the first part of randnum and rand2num are identical.

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

Uses file-scope variables: `randbuf`, `randpwd`, `seskey`

### rand2num_logincont

```c
static int rand2num_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 500 to 545.

differences from randnum:

1. each byte of the key is shifted left one bit
1. client sends the server a 64-bit number. the server encrypts it and sends it back as part of the reply.

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

Uses file-scope variables: `randbuf`, `randpwd`, `seskey`

### randnum_changepw

```c
static int randnum_changepw(void *obj, const char *username, struct passwd *pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 552 to 627.

change password

Note: an FPLogin must already have completed successfully for this to work.

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [randpass](uams_randnum.c.md#randpass), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

Uses file-scope variables: `seskey`

### randnum_login

```c
static int randnum_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 630 to 665.

randnum login

Calls: [rand_login](uams_randnum.c.md#rand_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

### randnum_login_ext

```c
static int randnum_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 668 to 697.

randnum login ext

Calls: [rand_login](uams_randnum.c.md#rand_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 699 to 725.

Calls: [rand2num_logincont](uams_randnum.c.md#rand2num_logincont), [randnum_changepw](uams_randnum.c.md#randnum_changepw), [randnum_login](uams_randnum.c.md#randnum_login), [randnum_login_ext](uams_randnum.c.md#randnum_login_ext), [randnum_logincont](uams_randnum.c.md#randnum_logincont), [randnum_warn_passwdfile_key](uams_randnum.c.md#randnum_warn_passwdfile_key), [uam_register](../afpd/uam.c.md#uam_register), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_randnum](uams_randnum.c.md#uams_randnum)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 727 to 735.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_randnum](uams_randnum.c.md#uams_randnum)

# Macros

* Undocumented: `DES_KEY_SZ`, `HEXPASSWDLEN`, `PASSWDLEN`, `PASSWD_ILLEGAL`, `randhash`

# File-scope variables

`randbuf`, `randpwd`, `seskey`
