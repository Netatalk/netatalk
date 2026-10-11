---
type: C Source File
title: "etc/uams/uams_randnum.c"
description: "15 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_randnum.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Initialized at line 891 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_randnum.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_randnum.c.md#uam_setup)

# Functions

### unhex

```c
static int unhex(unsigned char x)
```

Defined at lines 63 to 66.

Return the value of a hex digit, in either case.

The call does not check its input, so any other byte gives a meaningless value.

Parameters:
* `x`: hex digit

Returns: 0 to 15 for a hex digit

Called by: [afppasswd](uams_randnum.c.md#afppasswd), [afppasswd_read_keyfile](uams_randnum.c.md#afppasswd_read_keyfile)

### randnum_cipher_check

```c
static int randnum_cipher_check(const char *op, gcry_error_t err)
```

Defined at lines 75 to 84.

Log a failed libgcrypt call of the RandNum UAM.

Parameters:
* `op`: name of the call
* `err`: its error code

Returns: 0 for no error, -1 after logging one

Called by: [afppasswd](uams_randnum.c.md#afppasswd)

### afppasswd_open_keyfile

```c
static int afppasswd_open_keyfile(const char *path, const int pathlen)
```

Defined at lines 97 to 122.

Open the companion file of an afppasswd file, its path plus `.key`.

A missing `.key` file logs a warning, since the passwords of the file depend on it.

Parameters:
* `path`: path of the afppasswd file
* `pathlen`: length of path

Returns: the descriptor, or -1 for a path too long for the suffix or a failed open

Calls: [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afppasswd](uams_randnum.c.md#afppasswd), [randnum_warn_passwdfile_key](uams_randnum.c.md#randnum_warn_passwdfile_key)

### afppasswd_read_keyfile

```c
static int afppasswd_read_keyfile(int keyfd, uint8_t key[DES_KEY_SZ])
```

Defined at lines 134 to 175.

Read the DES secret from the `.key` file of an afppasswd file.

The file holds 16 hex digits, with an optional newline. The call wipes its read buffer.

Parameters:
* `keyfd`: descriptor of the `.key` file
* `key`: receives the 8-byte DES secret

Returns: 0, or -1 for a read error, another length, or a byte that is not hex

Calls: [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [unhex](uams_randnum.c.md#unhex)

Called by: [afppasswd](uams_randnum.c.md#afppasswd), [randnum_warn_passwdfile_key](uams_randnum.c.md#randnum_warn_passwdfile_key)

### randnum_warn_passwdfile_key

```c
static void randnum_warn_passwdfile_key(void *obj)
```

Defined at lines 186 to 228.

Warn at setup when the afppasswd file or its `.key` file cannot serve RandNum logins.

The UAM registers anyway, and its logins fail until the files are in place. A home password file cannot carry a `.key` file, so it always warns.

Parameters:
* `obj`: AFP object with the passwd file option

Calls: [afppasswd_open_keyfile](uams_randnum.c.md#afppasswd_open_keyfile), [afppasswd_read_keyfile](uams_randnum.c.md#afppasswd_read_keyfile), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

### afppasswd

```c
static int afppasswd(const struct passwd *pwd, const char *path, const int pathlen, unsigned char *passwd, int len, const int set)
```

Defined at lines 255 to 411.

Read or write the DES-encrypted password of a user in an afppasswd file.

The password field holds the hex of the encrypted password. The DES secret comes from the `.key` file, and a star in the field disables the entry. Reading decrypts the password into passwd. Writing encrypts passwd and writes its hex back under a write lock on the line. The call writes the entry itself, without the passwd functions of libc and their static state.

Formats:

```
password file:  username:password:last login date:failedcount
key file:       key (in hex)
```

Parameters:
* `pwd`: user entry
* `path`: afppasswd file
* `pathlen`: length of path
* `passwd`: receives the password, or holds the new one to write
* `len`: size of passwd
* `set`: nonzero to write, 0 to read

Returns: 0, AFPERR_PARAM for a user without an entry, AFPERR_ACCESS for a disabled entry or a missing or bad file, or AFPERR_MISC

Calls: [afppasswd_open_keyfile](uams_randnum.c.md#afppasswd_open_keyfile), [afppasswd_read_keyfile](uams_randnum.c.md#afppasswd_read_keyfile), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [randnum_cipher_check](uams_randnum.c.md#randnum_cipher_check), [unhex](uams_randnum.c.md#unhex)

Called by: [randpass](uams_randnum.c.md#randpass)

### randpass

```c
static int randpass(const struct passwd *pwd, const char *file, unsigned char *passwd, const int len, const int set)
```

Defined at lines 427 to 453.

Read or write an afppasswd entry as root.

The call raises the euid to root around afppasswd and drops it again.

Parameters:
* `pwd`: user entry
* `file`: afppasswd file
* `passwd`: password buffer for afppasswd
* `len`: size of passwd
* `set`: nonzero to write, 0 to read

Returns: the result of afppasswd, AFPERR_PARAM for a path over MAXPATHLEN, or AFPERR_MISC when seteuid fails

Calls: [afppasswd](uams_randnum.c.md#afppasswd)

Called by: [rand_login](uams_randnum.c.md#rand_login), [randnum_changepw](uams_randnum.c.md#randnum_changepw)

### rand_login

```c
static int rand_login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 474 to 524.

Start a RandNum login with an 8-byte challenge.

The user has to pass uam_checkuser, and the call reads the password of the user, the DES secret, from the afppasswd file. The reply holds the session ID and the challenge, which the client encrypts with its password.

Parameters:
* `obj`: session object
* `username`: user name from the request, receives the account name of a match
* `ulen`: size of the username buffer
* `uam_pwd`: unused
* `ibuf`: UAM data of the request, unused
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_NOTAUTH for an unknown or refused user, an afppasswd error, or AFPERR_PARAM

Calls: [randpass](uams_randnum.c.md#randpass), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [randnum_login](uams_randnum.c.md#randnum_login), [randnum_login_ext](uams_randnum.c.md#randnum_login_ext)

Uses file-scope variables: `randpwd`, `seskey`

### randnum_logincont

```c
static int randnum_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 544 to 576.

Check the encrypted challenge of a RandNum login.

The call encrypts the challenge with the password of the user as the DES secret and compares it with the client reply in constant time. The 2-way variant starts the same way.

Parameters:
* `obj`: session object
* `uam_pwd`: receives the user entry after a match
* `ibuf`: UAM data of the request, the session ID and the encrypted challenge
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFP_OK, AFPERR_NOTAUTH for a wrong answer, or AFPERR_PARAM for another session ID

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

Uses file-scope variables: `randpwd`, `seskey`

### rand2num_logincont

```c
static int rand2num_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 598 to 643.

Check the encrypted challenge of a 2-Way RandNum login and answer the challenge of the client.

Unlike RandNum, each byte of the DES secret shifts left 1 bit first. After a match the call encrypts the 8-byte challenge of the client and sends it back.

Parameters:
* `obj`: session object
* `uam_pwd`: receives the user entry after a match
* `ibuf`: UAM data of the request, the session ID, the encrypted challenge, and the challenge of the client
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block, receives the encrypted challenge of the client
* `rbuflen`: reply length on return

Returns: AFP_OK, AFPERR_NOTAUTH for a wrong answer, or AFPERR_PARAM for another session ID

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

Uses file-scope variables: `randpwd`, `seskey`

### randnum_changepw

```c
static int randnum_changepw(void *obj, const char *username, struct passwd *pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 664 to 739.

Change the RandNum password of a logged-in user in the afppasswd file.

The request holds the old password under DES with the new one as the secret, and the new password under DES with the old one. The call reads the old password from the file, checks both in constant time, and writes the new password. With cracklib a weak new password fails with AFPERR_PWDPOLCY.

Parameters:
* `obj`: session object
* `username`: user name, unused
* `pwd`: user entry
* `ibuf`: the 2 encrypted passwords, decrypted in place and wiped
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block, unused
* `rbuflen`: unused

Returns: AFP_OK, AFPERR_NOTAUTH for a wrong old password, AFPERR_PWDSAME, AFPERR_PWDPOLCY, AFPERR_ACCESS, or an afppasswd error

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [randpass](uams_randnum.c.md#randpass), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

Uses file-scope variables: `seskey`

### randnum_login

```c
static int randnum_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 755 to 790.

Read the user name of an FPLogin request and start a RandNum login.

The request holds the name as a Pascal string, padded to an even address.

Parameters:
* `obj`: session object
* `uam_pwd`: unused until the continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of rand_login, AFPERR_PARAM for an empty or oversized name, or AFPERR_MISC

Calls: [rand_login](uams_randnum.c.md#rand_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

### randnum_login_ext

```c
static int randnum_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 808 to 837.

Read the user name of an FPLoginExt request and start a RandNum login.

The name comes as an AFP name of type 3, a UTF-8 string with a 2-byte length.

Parameters:
* `obj`: session object
* `uname`: user name field of the request
* `uam_pwd`: unused until the continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of rand_login, AFPERR_PARAM for another name type or a bad length, or AFPERR_MISC

Calls: [rand_login](uams_randnum.c.md#rand_login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_randnum.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 850 to 876.

Register the 2 RandNum logins and the RandNum password change.

The call first checks the afppasswd file and its `.key` file through randnum_warn_passwdfile_key. A failed registration removes the earlier ones.

Parameters:
* `obj`: AFP object with the passwd file option
* `path`: path of the module

Returns: 0, or -1 when a registration fails

Calls: [rand2num_logincont](uams_randnum.c.md#rand2num_logincont), [randnum_changepw](uams_randnum.c.md#randnum_changepw), [randnum_login](uams_randnum.c.md#randnum_login), [randnum_login_ext](uams_randnum.c.md#randnum_login_ext), [randnum_logincont](uams_randnum.c.md#randnum_logincont), [randnum_warn_passwdfile_key](uams_randnum.c.md#randnum_warn_passwdfile_key), [uam_register](../afpd/uam.c.md#uam_register), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_randnum](uams_randnum.c.md#uams_randnum)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 881 to 889.

Unregister the 2 RandNum logins and the RandNum password change.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_randnum](uams_randnum.c.md#uams_randnum)

# Macros

* `randhash`: hash to a 16-bit number.
* Undocumented: `DES_KEY_SZ`, `HEXPASSWDLEN`, `PASSWDLEN`, `PASSWD_ILLEGAL`

# File-scope variables

`randbuf`, `randpwd`, `seskey`
