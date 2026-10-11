---
type: C Source File
title: "etc/uams/uams_dhx2_pam.c"
description: "19 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_dhx2_pam.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [etc/uams](../uams.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/constant_time.h](../../include/atalk/constant_time.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/uam.h](../../include/atalk/uam.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `gcrypt.h`, `signal.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`

# Function tables

### PAM_conversation

Initialized at line 305. Dispatches to: [PAM_conv](uams_dhx2_pam.c.md#pam_conv)

### uams_dhx2

Initialized at line 1384 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx2_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

### uams_dhx2_pam

Initialized at line 1391 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx2_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

# Functions

### dhx2_release_mpi

```c
static void dhx2_release_mpi(gcry_mpi_t *mpi)
```

Defined at lines 66 to 72.

Release a libgcrypt number and set its pointer to NULL.

Parameters:
* `mpi`: number to release, or a pointer to NULL

Called by: [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [logincont1](uams_dhx2_pam.c.md#logincont1)

### dhx2_clear_session

```c
static void dhx2_clear_session(void)
```

Defined at lines 81 to 90.

Drop the state of a DHX2 exchange, so the next step has to start over.

The call releases Ra and the server nonce, frees the session key, clears the session ID, and returns to DHX2_STATE_IDLE. The group p and g stay for the next login.

Calls: [dhx2_release_mpi](uams_dhx2_pam.c.md#dhx2_release_mpi)

Called by: [changepw_3](uams_dhx2_pam.c.md#changepw_3), [dhx2_setup](uams_dhx2_pam.c.md#dhx2_setup), [logincont1](uams_dhx2_pam.c.md#logincont1), [logincont2](uams_dhx2_pam.c.md#logincont2), [pam_logincont](uams_dhx2_pam.c.md#pam_logincont)

Uses file-scope variables: `ID`, `K_MD5hash`, `K_hash_len`, `Ra`, `serverNonce`

### dh_params_generate

```c
static int dh_params_generate(unsigned int bits)
```

Defined at lines 120 to 184.

Generate the prime p and the generator g of the Diffie-Hellman group.

The algorithm follows gnutls_dh_primes.c, since libgcrypt has no direct call for it. The call generates up to 10 primes until one passes gcry_prime_check. The bits value should be 768, 1024, 2048, 3072, or 4096.

Parameters:
* `bits`: size of p in bits

Returns: 0, or AFPERR_MISC for a libgcrypt version mismatch or a failed generation

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

Uses file-scope variables: `g`

### PAM_conv

```c
static int PAM_conv(int num_msg, struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
```

Defined at lines 200 to 303.

Answer PAM prompts with the user name and the password of the login.

An echo-on prompt gets PAM_username and an echo-off prompt PAM_password. Info and binary prompts get no answer, and an error message or an unknown style fails the conversation.

Parameters:
* `num_msg`: number of messages
* `msg`: PAM messages
* `resp`: receives the responses
* `appdata_ptr`: unused

Returns: PAM_SUCCESS, or PAM_CONV_ERR

Uses file-scope variables: `PAM_password`, `PAM_username`

Dispatched via: [PAM_conversation](uams_dhx2_pam.c.md#pam_conversation)

### dhx2_setup

```c
static int dhx2_setup(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 326 to 422.

Start a DHX2 exchange with the public key of the server.

The call picks a random Ra of PRIMEBITS bits and computes Ma = g^Ra mod p. The reply holds the session ID, g in 4 bytes, the length of p, p, and Ma, each number padded to its full length. The next step is the first login continuation.

Parameters:
* `obj`: session object, which also seeds the session ID
* `ibuf`: UAM data of the request, unused
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return, 0 on failure

Returns: AFPERR_AUTHCONT, or AFPERR_MISC

Calls: [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session)

Called by: [changepw_1](uams_dhx2_pam.c.md#changepw_1), [login](uams_dhx2_pam.c.md#login)

Uses file-scope variables: `ID`, `Ra`, `g`

### login

```c
static int login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 438 to 451.

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

Returns: AFPERR_AUTHCONT, AFPERR_NOTAUTH for an unknown user, or AFPERR_MISC

Calls: [dhx2_setup](uams_dhx2_pam.c.md#dhx2_setup), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [pam_login](uams_dhx2_pam.c.md#pam_login), [pam_login_ext](uams_dhx2_pam.c.md#pam_login_ext)

Uses file-scope variables: `PAM_username`, `dhxpwd`

### pam_login

```c
static int pam_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 468 to 503.

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

Calls: [login](uams_dhx2_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

### pam_login_ext

```c
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 521 to 556.

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

Calls: [login](uams_dhx2_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

### logincont1

```c
static int logincont1(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 576 to 736.

Derive the session key from the public key of the client and answer its nonce.

The request holds the session ID, Mb, and the client nonce under CAST5-CBC. The CAST5 secret is the MD5 hash of K = Mb^Ra mod p, with the fixed IV of Apple for each direction. The reply holds the session ID plus 1 and, encrypted, the client nonce plus 1 and a new server nonce. The next step is the second continuation.

Parameters:
* `obj`: session object, unused
* `ibuf`: UAM data of the request, decrypted in place
* `ibuflen`: length of ibuf, which must be 2 + PRIMEBITS / 8 + 16
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT, AFPERR_PARAM out of sequence or for a bad length, or AFPERR_MISC

Calls: [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [dhx2_release_mpi](uams_dhx2_pam.c.md#dhx2_release_mpi)

Called by: [changepw_2](uams_dhx2_pam.c.md#changepw_2), [pam_logincont](uams_dhx2_pam.c.md#pam_logincont)

Uses file-scope variables: `ID`, `K_MD5hash`, `K_hash_len`, `Ra`, `dhx_c2siv`, `dhx_s2civ`, `serverNonce`

### dhx2_log_pam_error

```c
static void dhx2_log_pam_error(pam_handle_t *ph, const char *step, int pam_err)
```

Defined at lines 748 to 753.

Log a failed PAM step through [uam_log_pam_failure()](../../libatalk/util/unix.c.md#uam_log_pam_failure).

errno is read before pam_strerror(), whose message lookup can change it.

Parameters:
* `ph`: PAM handle, NULL when pam_start() failed
* `step`: PAM function that failed
* `pam_err`: its return code

Calls: [uam_log_pam_failure](../../libatalk/util/unix.c.md#uam_log_pam_failure)

Called by: [changepw_3](uams_dhx2_pam.c.md#changepw_3), [logincont2](uams_dhx2_pam.c.md#logincont2)

Uses file-scope variables: `PAM_username`

### logincont2

```c
static int logincont2(void *obj_in, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 774 to 993.

Check the server nonce and the password of a DHX2 login with PAM.

The request holds the session ID and, under CAST5-CBC, the server nonce plus 1 and a 256-byte password buffer. Some clients add 10 more bytes. The password goes from the Mac charset to the Unix charset. PAM then runs authenticate, acct_mgmt, setcred, and open_session, with SIGCHLD at its default action during the checks. The call wipes the password, and a refused login calls pam_end.

Parameters:
* `obj_in`: session object
* `uam_pwd`: receives the user entry after a successful login
* `ibuf`: UAM data of the request, decrypted in place
* `ibuflen`: length of ibuf, which must be 274 or 284
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFP_OK, AFPERR_NOTAUTH, AFPERR_PWDEXPR, AFPERR_PWDCHNG, AFPERR_PARAM, or AFPERR_MISC

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [dhx2_log_pam_error](uams_dhx2_pam.c.md#dhx2_log_pam_error), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [pam_logincont](uams_dhx2_pam.c.md#pam_logincont)

Uses file-scope variables: `K_MD5hash`, `K_hash_len`, `PAM_conversation`, `PAM_password`, `PAM_username`, `dhx_c2siv`, `dhxpwd`, `pamh`, `serverNonce`

### pam_logincont

```c
static int pam_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1010 to 1032.

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

Calls: [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [logincont1](uams_dhx2_pam.c.md#logincont1), [logincont2](uams_dhx2_pam.c.md#logincont2)

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

Uses file-scope variables: `ID`

### pam_logout

```c
static void pam_logout(void)
```

Defined at lines 1038 to 1043.

Close the PAM session of the login and call pam_end.

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

Uses file-scope variables: `pamh`

### changepw_1

```c
static int changepw_1(void *obj, char *uname, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1060 to 1067.

Start the Diffie-Hellman exchange of a DHX2 password change and remember the user.

Parameters:
* `obj`: session object
* `uname`: user name, kept for changepw_3
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of dhx2_setup

Calls: [dhx2_setup](uams_dhx2_pam.c.md#dhx2_setup)

Called by: [dhx2_changepw](uams_dhx2_pam.c.md#dhx2_changepw)

Uses file-scope variables: `PAM_username`

### changepw_2

```c
static int changepw_2(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1080 to 1084.

Derive the session key of a DHX2 password change, as logincont1 does for a login.

Parameters:
* `obj`: session object
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of logincont1

Calls: [logincont1](uams_dhx2_pam.c.md#logincont1)

Called by: [dhx2_changepw](uams_dhx2_pam.c.md#dhx2_changepw)

### changepw_3

```c
static int changepw_3(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1104 to 1282.

Check the server nonce and the old password, then set the new password with PAM.

The request holds the session ID and, under CAST5-CBC, the server nonce plus 1, the new password, and the old password, 256 bytes each. Equal passwords fail with AFPERR_PWDSAME. As root PAM authenticates the old password and runs acct_mgmt and chauthtok with the new one. The call wipes both passwords after chauthtok.

Parameters:
* `obj`: session object, for the client host name
* `ibuf`: UAM data of the request, decrypted in place
* `ibuflen`: length of ibuf, which must be 2 + 16 + 2 * 256
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFP_OK, AFPERR_PWDSAME, AFPERR_NOTAUTH, AFPERR_ACCESS when chauthtok fails, AFPERR_PARAM, or AFPERR_MISC

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [dhx2_log_pam_error](uams_dhx2_pam.c.md#dhx2_log_pam_error), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [dhx2_changepw](uams_dhx2_pam.c.md#dhx2_changepw)

Uses file-scope variables: `K_MD5hash`, `K_hash_len`, `PAM_conversation`, `PAM_password`, `PAM_username`, `dhx_c2siv`, `serverNonce`

### dhx2_changepw

```c
static int dhx2_changepw(void *obj, char *uname, struct passwd *pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1300 to 1337.

Run the three FPChangePassword requests of a DHX2 password change in order.

A static step counter picks changepw_1, changepw_2, or changepw_3. A failed second step and every third step restart the sequence.

Parameters:
* `obj`: session object
* `uname`: user name
* `pwd`: user entry, unused
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of the step

Calls: [changepw_1](uams_dhx2_pam.c.md#changepw_1), [changepw_2](uams_dhx2_pam.c.md#changepw_2), [changepw_3](uams_dhx2_pam.c.md#changepw_3)

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 1349 to 1369.

Register the DHX2 login and password change, and generate the Diffie-Hellman group.

The group has a prime of PRIMEBITS bits, generated once per process.

Parameters:
* `obj`: AFP object, unused
* `path`: path of the module

Returns: 0, or -1 when a registration or the group generation fails

Calls: [dh_params_generate](uams_dhx2_pam.c.md#dh_params_generate), [dhx2_changepw](uams_dhx2_pam.c.md#dhx2_changepw), [pam_login](uams_dhx2_pam.c.md#pam_login), [pam_login_ext](uams_dhx2_pam.c.md#pam_login_ext), [pam_logincont](uams_dhx2_pam.c.md#pam_logincont), [pam_logout](uams_dhx2_pam.c.md#pam_logout), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_dhx2](uams_dhx2_pam.c.md#uams_dhx2), [uams_dhx2_pam](uams_dhx2_pam.c.md#uams_dhx2_pam)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 1375 to 1382.

Unregister the DHX2 login and password change, and release the Diffie-Hellman group.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Uses file-scope variables: `g`

Dispatched via: [uams_dhx2](uams_dhx2_pam.c.md#uams_dhx2), [uams_dhx2_pam](uams_dhx2_pam.c.md#uams_dhx2_pam)

# Typedefs and enums

* `enum dhx2_state`: `DHX2_STATE_IDLE`, `DHX2_STATE_EXPECT_CONT1`, `DHX2_STATE_EXPECT_CONT2`

# Macros

* `PRIMEBITS`: Number of bits for p which we generate.
* `dhxhash`: hash a number to a 16-bit quantity
* Undocumented: `COPY_STRING`, `PAM_CRED_ESTABLISH`, `PASSWDBUFLEN`

# File-scope variables

`ID`, `K_MD5hash`, `K_hash_len`, `PAM_password`, `PAM_username`, `Ra`, `dhx2_state`, `dhx_c2siv`, `dhx_s2civ`, `dhxpwd`, `g`, `p`, `pamh`, `serverNonce`
