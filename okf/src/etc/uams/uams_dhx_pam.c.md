---
type: C Source File
title: "etc/uams/uams_dhx_pam.c"
description: "12 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_dhx_pam.c"
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
* System headers: `arpa/inet.h`, `errno.h`, `gcrypt.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`

# Function tables

### PAM_conversation

Initialized at line 189. Dispatches to: [PAM_conv](uams_dhx_pam.c.md#pam_conv)

### uams_dhx

Initialized at line 950 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx_pam.c.md#uam_setup)

### uams_dhx_pam

Initialized at line 956 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx_pam.c.md#uam_setup)

# Functions

### dhx_release_key

```c
static void dhx_release_key(void)
```

Defined at lines 47 to 53.

Called by: [dhx_setup](uams_dhx_pam.c.md#dhx_setup), [pam_changepw](uams_dhx_pam.c.md#pam_changepw), [pam_logincont](uams_dhx_pam.c.md#pam_logincont), [uam_cleanup](uams_dhx_pam.c.md#uam_cleanup)

Uses file-scope variables: `K`

### PAM_conv

```c
static int PAM_conv(int num_msg, struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
```

Defined at lines 79 to 187.

PAM conversation function.

Note: Here we assume (for now, at least) that echo on means login name, and echo off means password.

Uses file-scope variables: `PAM_password`, `PAM_username`

Dispatched via: [PAM_conversation](uams_dhx_pam.c.md#pam_conversation)

### dhx_setup

```c
static int dhx_setup(void *obj, const unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 195 to 340.

Calls: [dhx_release_key](uams_dhx_pam.c.md#dhx_release_key), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [login](uams_dhx_pam.c.md#login), [pam_changepw](uams_dhx_pam.c.md#pam_changepw)

Uses file-scope variables: `K`, `g_binary`, `msg2_iv`, `p_binary`, `randbuf`

### login

```c
static int login(void *obj, unsigned char *username, int ulen, struct passwd **uam_pwd, const unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 343 to 355.

Calls: [dhx_setup](uams_dhx_pam.c.md#dhx_setup), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [pam_login](uams_dhx_pam.c.md#pam_login), [pam_login_ext](uams_dhx_pam.c.md#pam_login_ext)

Uses file-scope variables: `PAM_username`, `dhxpwd`

### pam_login

```c
static int pam_login(void *obj, struct passwd **uam_pwd, unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 363 to 399.

dhx login

Note: things are done in a slightly bizarre order to avoid having to clean things up if there's an error.

Calls: [login](uams_dhx_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

### pam_login_ext

```c
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd, const unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 402 to 439.

Calls: [login](uams_dhx_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

### dhx_log_pam_error

```c
static void dhx_log_pam_error(pam_handle_t *ph, const char *user, const char *step, int pam_err)
```

Defined at lines 451 to 457.

Log a failed PAM step through [uam_log_pam_failure()](../../libatalk/util/unix.c.md#uam_log_pam_failure)

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

Defined at lines 461 to 692.

Calls: [dhx_log_pam_error](uams_dhx_pam.c.md#dhx_log_pam_error), [dhx_release_key](uams_dhx_pam.c.md#dhx_release_key), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

Uses file-scope variables: `K`, `PAM_conversation`, `PAM_password`, `PAM_username`, `dhxpwd`, `msg3_iv`, `pamh`, `randbuf`

### pam_logout

```c
static void pam_logout(void)
```

Defined at lines 695 to 700.

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

Uses file-scope variables: `pamh`

### pam_changepw

```c
static int pam_changepw(void *obj, unsigned char *username, const struct passwd *pwd, unsigned char *ibuf, size_t ibuflen, unsigned char *rbuf, size_t *rbuflen)
```

Defined at lines 708 to 918.

change pw for dhx

Note: needs a couple passes to get everything all right. basically, it's like the login/logincont sequence

Calls: [dhx_log_pam_error](uams_dhx_pam.c.md#dhx_log_pam_error), [dhx_release_key](uams_dhx_pam.c.md#dhx_release_key), [dhx_setup](uams_dhx_pam.c.md#dhx_setup), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx_pam.c.md#uam_setup)

Uses file-scope variables: `K`, `PAM_conversation`, `PAM_password`, `PAM_username`, `msg3_iv`, `randbuf`

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 921 to 938.

Calls: [pam_changepw](uams_dhx_pam.c.md#pam_changepw), [pam_login](uams_dhx_pam.c.md#pam_login), [pam_login_ext](uams_dhx_pam.c.md#pam_login_ext), [pam_logincont](uams_dhx_pam.c.md#pam_logincont), [pam_logout](uams_dhx_pam.c.md#pam_logout), [uam_register](../afpd/uam.c.md#uam_register), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_dhx](uams_dhx_pam.c.md#uams_dhx), [uams_dhx_pam](uams_dhx_pam.c.md#uams_dhx_pam)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 940 to 948.

Calls: [dhx_release_key](uams_dhx_pam.c.md#dhx_release_key), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_dhx](uams_dhx_pam.c.md#uams_dhx), [uams_dhx_pam](uams_dhx_pam.c.md#uams_dhx_pam)

# Macros

* Undocumented: `CHANGEPWBUFLEN`, `COPY_STRING`, `CRYPT2BUFLEN`, `CRYPTBUFLEN`, `KEYSIZE`, `PAM_CRED_ESTABLISH`, `PASSWDLEN`, `dhxhash`

# File-scope variables

`K`, `PAM_password`, `PAM_username`, `dhxpwd`, `g_binary`, `msg2_iv`, `msg3_iv`, `p_binary`, `pamh`, `randbuf`
