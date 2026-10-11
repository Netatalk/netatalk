---
type: C Source File
title: "etc/uams/uams_pam.c"
description: "10 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_pam.c"
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
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`

# Function tables

### PAM_conversation

Initialized at line 154. Dispatches to: [PAM_conv](uams_pam.c.md#pam_conv)

### uams_clrtxt

Initialized at line 808 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_pam.c.md#uam_setup)

### uams_pam

Initialized at line 814 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_pam.c.md#uam_setup)

# Functions

### PAM_conv

```c
static int PAM_conv(int num_msg, struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
```

Defined at lines 65 to 152.

Answer PAM prompts with the user name and the password of the login.

An echo-on prompt gets PAM_username and an echo-off prompt PAM_password. During a password change without root, the first echo-off prompt gets the old password instead. Info and binary prompts get no answer, and an error message or an unknown style fails the conversation.

Parameters:
* `num_msg`: number of messages
* `msg`: PAM messages
* `resp`: receives the responses
* `appdata_ptr`: unused

Returns: PAM_SUCCESS, or PAM_CONV_ERR

Uses file-scope variables: `PAM_chauthtok_count`, `PAM_chauthtok_mode`, `PAM_oldpassword`

Dispatched via: [PAM_conversation](uams_pam.c.md#pam_conversation)

### clrtxt_log_pam_error

```c
static void clrtxt_log_pam_error(enum loglevels level, pam_handle_t *ph, const char *user, const char *step, int pam_err)
```

Defined at lines 172 to 179.

Log a failed PAM step through [uam_log_pam_failure()](../../libatalk/util/unix.c.md#uam_log_pam_failure).

errno is read before pam_strerror(), whose message lookup can change it. A failed pam_start(), and a failed account or session step of a printer login, are errors; the other steps are informational.

Parameters:
* `level`: log level of the line
* `ph`: PAM handle, NULL when pam_start() failed
* `user`: user being authenticated
* `step`: PAM function that failed
* `pam_err`: its return code

Calls: [uam_log_pam_failure](../../libatalk/util/unix.c.md#uam_log_pam_failure)

Called by: [login](uams_pam.c.md#login), [pam_changepw](uams_pam.c.md#pam_changepw), [pam_printer](uams_pam.c.md#pam_printer)

### login

```c
static int login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 200 to 343.

Check a cleartext password of up to 8 bytes with PAM.

The user has to pass uam_checkuser. PAM then runs authenticate, acct_mgmt, setcred, and open_session, with SIGCHLD at its default action during the checks. A password that acct_mgmt reports as expired still opens the session and answers AFPERR_PWDEXPR, so the client can change it.

Parameters:
* `obj`: session object
* `username`: user name from the request, receives the account name of a match
* `ulen`: size of the username buffer
* `uam_pwd`: receives the user entry after a successful login
* `ibuf`: password field of the request, cut after 8 bytes
* `ibuflen`: length of the request data, which must exceed 8
* `rbuf`: reply block, unused
* `rbuflen`: unused

Returns: AFP_OK, AFPERR_PWDEXPR, AFPERR_NOTAUTH, or AFPERR_PARAM

Calls: [clrtxt_log_pam_error](uams_pam.c.md#clrtxt_log_pam_error), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [pam_login](uams_pam.c.md#pam_login), [pam_login_ext](uams_pam.c.md#pam_login_ext)

### pam_login

```c
static int pam_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 361 to 389.

Read the user name of a cleartext FPLogin request and check its password with PAM.

The request holds the name as a Pascal string, padded to an even address, and then the password.

Parameters:
* `obj`: session object
* `uam_pwd`: receives the user entry after a successful login
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the result of login, AFPERR_PARAM for a name longer than the buffer, or AFPERR_MISC

Calls: [login](uams_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_pam.c.md#uam_setup)

### pam_login_ext

```c
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 408 to 437.

Read the user name of a cleartext FPLoginExt request and check its password with PAM.

The name comes as an AFP name of type 3, a UTF-8 string with a 2-byte length.

Parameters:
* `obj`: session object
* `uname`: user name field of the request
* `uam_pwd`: receives the user entry after a successful login
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the result of login, AFPERR_PARAM for another name type or a bad length, or AFPERR_MISC

Calls: [login](uams_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_pam.c.md#uam_setup)

### pam_logout

```c
static void pam_logout(void)
```

Defined at lines 442 to 447.

Close the PAM session of the login and call pam_end.

Called by: [uam_setup](uams_pam.c.md#uam_setup)

### pam_changepw

```c
static int pam_changepw(void *obj, char *username, struct passwd *pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 468 to 588.

Change a cleartext password with PAM.

The request holds the old and the new password, 8 bytes each, and the call clears them from the request at once. Equal passwords fail with AFPERR_PWDSAME. PAM authenticates the old password and runs acct_mgmt and chauthtok, as root when seteuid allows it. Without root, the first echo-off prompt of chauthtok gets the old password.

Parameters:
* `obj`: session object, unused
* `username`: user name
* `pwd`: user entry, unused
* `ibuf`: UAM data of the request, cleared on return
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block, unused
* `rbuflen`: unused

Returns: AFP_OK, AFPERR_PWDSAME, AFPERR_NOTAUTH, AFPERR_ACCESS when chauthtok fails, or AFPERR_PARAM

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [clrtxt_log_pam_error](uams_pam.c.md#clrtxt_log_pam_error), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [uam_setup](uams_pam.c.md#uam_setup)

Uses file-scope variables: `PAM_chauthtok_count`, `PAM_chauthtok_mode`, `PAM_oldpassword`

### pam_printer

```c
static int pam_printer(char *start, char *stop, char *username, struct papfile *out)
```

Defined at lines 608 to 763.

Check a ClearTxtUAM printer login with PAM and close its session at once.

The login string has the form `(username) (password)`, and the password keeps up to 8 bytes. The user has to pass uam_checkuser, and PAM runs authenticate, acct_mgmt, and open_session. A success writes the "0" answer to out and closes the session right away.

Parameters:
* `start`: start of the login string
* `stop`: end of the login string
* `username`: receives the user name, up to UAM_USERNAMELEN bytes without a NUL
* `out`: PAP reply, receives the answer

Returns: 0, or -1 for a malformed string, a refused user, or a failed PAM step

Calls: [append](../afpd/uam.c.md#append), [clrtxt_log_pam_error](uams_pam.c.md#clrtxt_log_pam_error), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_pam.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 776 to 795.

Register the Cleartxt Passwrd login and password change and the ClearTxtUAM printer login.

A failed password change registration removes the login again.

Parameters:
* `obj`: AFP object, unused
* `path`: path of the module

Returns: 0, or -1 when a registration fails

Calls: [pam_changepw](uams_pam.c.md#pam_changepw), [pam_login](uams_pam.c.md#pam_login), [pam_login_ext](uams_pam.c.md#pam_login_ext), [pam_logout](uams_pam.c.md#pam_logout), [pam_printer](uams_pam.c.md#pam_printer), [uam_register](../afpd/uam.c.md#uam_register), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_clrtxt](uams_pam.c.md#uams_clrtxt), [uams_pam](uams_pam.c.md#uams_pam)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 801 to 806.

Unregister the Cleartxt Passwrd login and password change and the ClearTxtUAM printer login.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_clrtxt](uams_pam.c.md#uams_clrtxt), [uams_pam](uams_pam.c.md#uams_pam)

# Macros

* Undocumented: `COPY_STRING`, `PAM_CRED_ESTABLISH`, `PASSWDLEN`

# File-scope variables

`PAM_chauthtok_count`, `PAM_chauthtok_mode`, `PAM_oldpassword`, `PAM_password`, `PAM_username`, `hostname`, `pamh`
