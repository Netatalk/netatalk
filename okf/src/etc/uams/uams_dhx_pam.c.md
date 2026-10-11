---
type: C Source File
title: "etc/uams/uams_dhx_pam.c"
description: "12 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_dhx_pam.c"
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
* System headers: `arpa/inet.h`, `errno.h`, `gcrypt.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`

# Function tables

### PAM_conversation

Initialized at line 200. Dispatches to: [PAM_conv](uams_dhx_pam.c.md#pam_conv)

### uams_dhx

Initialized at line 1066 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx_pam.c.md#uam_setup)

### uams_dhx_pam

Initialized at line 1072 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx_pam.c.md#uam_setup)

# Functions

### dhx_release_key

```c
static void dhx_release_key(void)
```

Defined at lines 50 to 56.

Release the session key K of a DHCAST128 exchange.

Called by: [dhx_setup](uams_dhx_pam.c.md#dhx_setup), [pam_changepw](uams_dhx_pam.c.md#pam_changepw), [pam_logincont](uams_dhx_pam.c.md#pam_logincont), [uam_cleanup](uams_dhx_pam.c.md#uam_cleanup)

Uses file-scope variables: `K`

### PAM_conv

```c
static int PAM_conv(int num_msg, struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
```

Defined at lines 90 to 198.

Answer PAM prompts with the user name and the password of the login.

An echo-on prompt gets PAM_username and an echo-off prompt PAM_password. Info and binary prompts get no answer, and an error message or an unknown style fails the conversation.

Parameters:
* `num_msg`: number of messages
* `msg`: PAM messages
* `resp`: receives the responses
* `appdata_ptr`: unused

Returns: PAM_SUCCESS, or PAM_CONV_ERR

Dispatched via: [PAM_conversation](uams_dhx_pam.c.md#pam_conversation)

### dhx_setup

```c
static int dhx_setup(void *obj, const unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 224 to 369.

Answer the public key of the client in a DHCAST128 exchange with Mb and a challenge.

The group is the fixed 128-bit prime of the protocol with generator 7. The call picks a random Rb, computes Mb = g^Rb mod p and the session key K = Ma^Rb mod p, and draws a 16-byte random challenge. The reply holds the session ID, Mb, and under CAST5-CBC the challenge and 16 zero bytes in the place of the server signature.

Parameters:
* `obj`: session object, which also seeds the session ID
* `ibuf`: Ma of the client, 16 bytes
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_MISC for a libgcrypt version mismatch, or AFPERR_PARAM when a step fails

Calls: [dhx_release_key](uams_dhx_pam.c.md#dhx_release_key), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [login](uams_dhx_pam.c.md#login), [pam_changepw](uams_dhx_pam.c.md#pam_changepw)

Uses file-scope variables: `K`, `g_binary`, `msg2_iv`, `p_binary`, `randbuf`

### login

```c
static int login(void *obj, unsigned char *username, int ulen, struct passwd **uam_pwd, const unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 387 to 399.

Look up the user of a DHCAST128 login and start the Diffie-Hellman exchange.

Parameters:
* `obj`: session object
* `username`: user name from the request, receives the account name of a match
* `ulen`: size of the username buffer
* `uam_pwd`: unused
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_NOTAUTH for an unknown user, AFPERR_PARAM, or AFPERR_MISC

Calls: [dhx_setup](uams_dhx_pam.c.md#dhx_setup), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [pam_login](uams_dhx_pam.c.md#pam_login), [pam_login_ext](uams_dhx_pam.c.md#pam_login_ext)

### pam_login

```c
static int pam_login(void *obj, struct passwd **uam_pwd, unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 416 to 452.

Read the user name of an FPLogin request and start a DHCAST128 login.

The request holds the name as a Pascal string, padded to an even address. The checks run before any state exists, so a refusal needs no cleanup.

Parameters:
* `obj`: session object
* `uam_pwd`: unused until the continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_PARAM for a name longer than the buffer, AFPERR_NOTAUTH, or AFPERR_MISC

Calls: [login](uams_dhx_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

### pam_login_ext

```c
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd, const unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 471 to 508.

Read the user name of an FPLoginExt request and start a DHCAST128 login.

The name comes as an AFP name of type 3, a UTF-8 string with a 2-byte length.

Parameters:
* `obj`: session object
* `uname`: user name field of the request
* `uam_pwd`: unused until the continuation
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_PARAM for another name type or a bad length, AFPERR_NOTAUTH, or AFPERR_MISC

Calls: [login](uams_dhx_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

### dhx_log_pam_error

```c
static void dhx_log_pam_error(pam_handle_t *ph, const char *user, const char *step, int pam_err)
```

Defined at lines 520 to 526.

Log a failed PAM step through [uam_log_pam_failure()](../../libatalk/util/unix.c.md#uam_log_pam_failure).

errno is read before pam_strerror(), whose message lookup can change it.

Parameters:
* `ph`: PAM handle, NULL when pam_start() failed
* `user`: user being authenticated
* `step`: PAM function that failed
* `pam_err`: its return code

Calls: [uam_log_pam_failure](../../libatalk/util/unix.c.md#uam_log_pam_failure)

Called by: [pam_changepw](uams_dhx_pam.c.md#pam_changepw), [pam_logincont](uams_dhx_pam.c.md#pam_logincont)

### pam_logincont

```c
static int pam_logincont(void *obj, struct passwd **uam_pwd, const unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 547 to 778.

Check the challenge answer and the password of a DHCAST128 login with PAM.

The request holds the session ID and, under CAST5-CBC, the challenge plus 1 and a 64-byte password field. PAM runs authenticate, acct_mgmt, setcred, and open_session. A password that acct_mgmt reports as expired still opens the session and answers AFPERR_PWDEXPR, so the client can change it. The call wipes the challenge and the password.

Parameters:
* `obj`: session object
* `uam_pwd`: receives the user entry after a successful login
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf, at least 2 + CRYPT2BUFLEN
* `rbuf`: reply block, which the call uses as a work buffer and clears
* `rbuflen`: 0 on return

Returns: AFP_OK, AFPERR_PWDEXPR, AFPERR_NOTAUTH, or AFPERR_PARAM

Calls: [dhx_log_pam_error](uams_dhx_pam.c.md#dhx_log_pam_error), [dhx_release_key](uams_dhx_pam.c.md#dhx_release_key), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

Uses file-scope variables: `K`, `msg3_iv`, `randbuf`

### pam_logout

```c
static void pam_logout(void)
```

Defined at lines 783 to 788.

Close the PAM session of the login and call pam_end.

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

### pam_changepw

```c
static int pam_changepw(void *obj, unsigned char *username, const struct passwd *pwd, unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 811 to 1021.

Change the password of a DHCAST128 user in two requests.

A request with session ID 0 starts the exchange through dhx_setup. The next request holds the session ID and, under CAST5-CBC, the challenge plus 1, the new password, and the old password, 64 bytes each. As root PAM authenticates the old password and runs acct_mgmt and chauthtok with the new one. The call wipes the old password before chauthtok and the new one after it.

Parameters:
* `obj`: session object
* `username`: user name
* `pwd`: user entry, unused
* `ibuf`: UAM data of the request, decrypted in place
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT after the first request, AFP_OK, AFPERR_NOTAUTH, AFPERR_ACCESS when chauthtok fails, AFPERR_PARAM, or AFPERR_MISC

Calls: [dhx_log_pam_error](uams_dhx_pam.c.md#dhx_log_pam_error), [dhx_release_key](uams_dhx_pam.c.md#dhx_release_key), [dhx_setup](uams_dhx_pam.c.md#dhx_setup), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

Uses file-scope variables: `K`, `msg3_iv`, `randbuf`

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 1033 to 1050.

Register the DHCAST128 login and password change of the PAM module.

A failed password change registration removes the login again.

Parameters:
* `obj`: AFP object, unused
* `path`: path of the module

Returns: 0, or -1 when a registration fails

Calls: [pam_changepw](uams_dhx_pam.c.md#pam_changepw), [pam_login](uams_dhx_pam.c.md#pam_login), [pam_login_ext](uams_dhx_pam.c.md#pam_login_ext), [pam_logincont](uams_dhx_pam.c.md#pam_logincont), [pam_logout](uams_dhx_pam.c.md#pam_logout), [uam_register](../afpd/uam.c.md#uam_register), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_dhx](uams_dhx_pam.c.md#uams_dhx), [uams_dhx_pam](uams_dhx_pam.c.md#uams_dhx_pam)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 1056 to 1064.

Release the session key and unregister the DHCAST128 login and password change.

Calls: [dhx_release_key](uams_dhx_pam.c.md#dhx_release_key), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_dhx](uams_dhx_pam.c.md#uams_dhx), [uams_dhx_pam](uams_dhx_pam.c.md#uams_dhx_pam)

# Macros

* `dhxhash`: hash a number to a 16-bit quantity
* Undocumented: `CHANGEPWBUFLEN`, `COPY_STRING`, `CRYPT2BUFLEN`, `CRYPTBUFLEN`, `KEYSIZE`, `PAM_CRED_ESTABLISH`, `PASSWDLEN`

# File-scope variables

`K`, `PAM_password`, `PAM_username`, `dhxpwd`, `g_binary`, `msg2_iv`, `msg3_iv`, `p_binary`, `pamh`, `randbuf`
