---
type: C Source File
title: "etc/uams/uams_dhx2_passwd.c"
description: "12 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_dhx2_passwd.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Initialized at line 891 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx2_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

### uams_dhx2_passwd

Initialized at line 898 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx2_passwd.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

# Functions

### dhx2_release_mpi

```c
static void dhx2_release_mpi(gcry_mpi_t *mpi)
```

Defined at lines 65 to 71.

Release a libgcrypt number and set its pointer to NULL.

Parameters:
* `mpi`: number to release, or a pointer to NULL

Called by: [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session), [logincont1](uams_dhx2_passwd.c.md#logincont1)

### dhx2_clear_session

```c
static void dhx2_clear_session(void)
```

Defined at lines 79 to 89.

Drop the state of a DHX2 exchange, so the next step has to start over.

The call releases p, Ra, and the server nonce, frees the session key, clears the session ID, and returns to DHX2_STATE_IDLE.

Calls: [dhx2_release_mpi](uams_dhx2_passwd.c.md#dhx2_release_mpi)

Called by: [dhx2_setup](uams_dhx2_passwd.c.md#dhx2_setup), [logincont1](uams_dhx2_passwd.c.md#logincont1), [logincont2](uams_dhx2_passwd.c.md#logincont2), [passwd_logincont](uams_dhx2_passwd.c.md#passwd_logincont)

### dh_params_generate

```c
static int dh_params_generate(gcry_mpi_t *ret_p, gcry_mpi_t *ret_g, unsigned int bits)
```

Defined at lines 119 to 200.

Generate a prime p and a generator g for one Diffie-Hellman exchange.

The algorithm follows gnutls_dh_primes.c, since libgcrypt has no direct call for it. The call generates up to 10 primes until one passes gcry_prime_check. The bits value should be 768, 1024, 2048, 3072, or 4096.

Parameters:
* `ret_p`: receives p, or NULL to drop it
* `ret_g`: receives g, or NULL to drop it
* `bits`: size of p in bits

Returns: 0, or AFPERR_MISC for a libgcrypt version mismatch or a failed generation

Called by: [dhx2_setup](uams_dhx2_passwd.c.md#dhx2_setup)

### dhx2_setup

```c
static int dhx2_setup(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 219 to 346.

Start a DHX2 exchange with a new Diffie-Hellman group and the public key of the server.

The user needs a password hash, from the shadow file in a build that uses one. Each exchange generates a new group of PRIMEBITS bits, so every login attempt pays for a prime search. The reply holds the session ID, g in 4 bytes, the length of p, p, and Ma = g^Ra mod p.

Parameters:
* `obj`: session object, which also seeds the session ID
* `ibuf`: UAM data of the request, unused
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return, 0 on failure

Returns: AFPERR_AUTHCONT, AFPERR_NOTAUTH without a password hash, or AFPERR_MISC

Calls: [dh_params_generate](uams_dhx2_passwd.c.md#dh_params_generate), [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session)

Called by: [login](uams_dhx2_passwd.c.md#login)

### login

```c
static int login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 363 to 375.

Look up the user of a DHX2 login and start the Diffie-Hellman exchange.

Parameters:
* `obj`: session object
* `username`: user name from the request, receives the account name of a match
* `ulen`: size of the username buffer
* `uam_pwd`: unused
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_NOTAUTH for an unknown user or one without a password hash, or AFPERR_MISC

Calls: [dhx2_setup](uams_dhx2_passwd.c.md#dhx2_setup), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [passwd_login](uams_dhx2_passwd.c.md#passwd_login), [passwd_login_ext](uams_dhx2_passwd.c.md#passwd_login_ext)

### passwd_login

```c
static int passwd_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 392 to 427.

Read the user name of an FPLogin request and start a DHX2 login.

The request holds the name as a Pascal string, padded to an even address. The checks run before any state exists, so a refusal needs no cleanup.

Parameters:
* `obj`: session object
* `uam_pwd`: unused until the second continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_PARAM for a name longer than the buffer, AFPERR_NOTAUTH, or AFPERR_MISC

Calls: [login](uams_dhx2_passwd.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

### passwd_login_ext

```c
static int passwd_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 445 to 480.

Read the user name of an FPLoginExt request and start a DHX2 login.

The name comes as an AFP name of type 3, a UTF-8 string with a 2-byte length.

Parameters:
* `obj`: session object
* `uname`: user name field of the request
* `uam_pwd`: unused until the second continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_PARAM for another name type or a bad length, AFPERR_NOTAUTH, or AFPERR_MISC

Calls: [login](uams_dhx2_passwd.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

### logincont1

```c
static int logincont1(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 501 to 664.

Derive the session key from the public key of the client and answer its nonce.

The request holds the session ID, Mb, and the client nonce under CAST5-CBC. The CAST5 secret is the MD5 hash of K = Mb^Ra mod p, with the fixed IV of Apple for each direction. The reply holds the session ID plus 1 and, encrypted, the client nonce plus 1 and a new server nonce. A success releases p and Ra, and the next step is the second continuation.

Parameters:
* `obj`: session object, unused
* `uam_pwd`: unused
* `ibuf`: UAM data of the request, decrypted in place
* `ibuflen`: length of ibuf, which must be 2 + PRIMEBITS / 8 + 16
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_PARAM out of sequence or for a bad length, or AFPERR_MISC

Calls: [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session), [dhx2_release_mpi](uams_dhx2_passwd.c.md#dhx2_release_mpi)

Called by: [passwd_logincont](uams_dhx2_passwd.c.md#passwd_logincont)

### logincont2

```c
static int logincont2(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 684 to 822.

Check the server nonce and the password of a DHX2 login against the password hash.

The request holds the session ID and, under CAST5-CBC, the server nonce plus 1 and a 256-byte password buffer. Some clients add 10 more bytes. The password goes to crypt_checkpass, or to crypt and a comparison. With shadow passwords an expired password gives AFPERR_PWDEXPR, and that check runs after a wrong password too. The call wipes the password.

Parameters:
* `obj`: session object, unused
* `uam_pwd`: receives the user entry after a matching password
* `ibuf`: UAM data of the request, decrypted in place
* `ibuflen`: length of ibuf, which must be 274 or 284
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFP_OK, AFPERR_NOTAUTH, AFPERR_PWDEXPR, AFPERR_PARAM, or AFPERR_MISC

Calls: [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [passwd_logincont](uams_dhx2_passwd.c.md#passwd_logincont)

### passwd_logincont

```c
static int passwd_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 839 to 861.

Route an FPLoginCont request to the step that the session ID and the state expect.

The session ID goes to logincont1, and the ID plus 1 to logincont2. Any other ID or step clears the exchange.

Parameters:
* `obj`: session object
* `uam_pwd`: receives the user entry after a successful login
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of the step, or AFPERR_PARAM for an unexpected request

Calls: [dhx2_clear_session](uams_dhx2_passwd.c.md#dhx2_clear_session), [logincont1](uams_dhx2_passwd.c.md#logincont1), [logincont2](uams_dhx2_passwd.c.md#logincont2)

Called by: [uam_setup](uams_dhx2_passwd.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 872 to 880.

Register the DHX2 login of the passwd module.

The module has no logout and no password change.

Parameters:
* `obj`: AFP object, unused
* `path`: path of the module

Returns: 0, or -1 when the registration fails

Calls: [passwd_login](uams_dhx2_passwd.c.md#passwd_login), [passwd_login_ext](uams_dhx2_passwd.c.md#passwd_login_ext), [passwd_logincont](uams_dhx2_passwd.c.md#passwd_logincont), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_dhx2](uams_dhx2_passwd.c.md#uams_dhx2), [uams_dhx2_passwd](uams_dhx2_passwd.c.md#uams_dhx2_passwd)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 885 to 888.

Unregister the DHX2 login of the passwd module.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_dhx2](uams_dhx2_passwd.c.md#uams_dhx2), [uams_dhx2_passwd](uams_dhx2_passwd.c.md#uams_dhx2_passwd)

# Typedefs and enums

* `enum dhx2_state`: `DHX2_STATE_IDLE`, `DHX2_STATE_EXPECT_CONT1`, `DHX2_STATE_EXPECT_CONT2`

# Macros

* `PRIMEBITS`: Number of bits for p which we generate.
* `dhxhash`: hash a number to a 16-bit quantity
* Undocumented: `PASSWDBUFLEN`

# File-scope variables

`ID`, `K_MD5hash`, `K_hash_len`, `Ra`, `dhx2_state`, `dhx_c2siv`, `dhx_s2civ`, `dhxpwd`, `p`, `serverNonce`
