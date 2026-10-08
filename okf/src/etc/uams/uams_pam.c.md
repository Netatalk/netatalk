---
type: C Source File
title: "etc/uams/uams_pam.c"
description: "10 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_pam.c"
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
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`

# Function tables

### PAM_conversation

Initialized at line 145. Dispatches to: [PAM_conv](uams_pam.c.md#pam_conv)

### uams_clrtxt

Initialized at line 701 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_pam.c.md#uam_setup)

### uams_pam

Initialized at line 707 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_pam.c.md#uam_setup)

# Functions

### PAM_conv

```c
static int PAM_conv(int num_msg, struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
```

Defined at lines 56 to 143.

PAM conversation function.

Note: Here we assume (for now, at least) that echo on means login name, and echo off means password.

Uses file-scope variables: `PAM_chauthtok_count`, `PAM_chauthtok_mode`, `PAM_oldpassword`, `PAM_password`, `PAM_username`

Dispatched via: [PAM_conversation](uams_pam.c.md#pam_conversation)

### clrtxt_log_pam_error

```c
static void clrtxt_log_pam_error(enum loglevels level, pam_handle_t *ph, const char *user, const char *step, int pam_err)
```

Defined at lines 163 to 170.

Log a failed PAM step through [uam_log_pam_failure()](../../libatalk/util/unix.c.md#uam_log_pam_failure)

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

Defined at lines 172 to 315.

Calls: [clrtxt_log_pam_error](uams_pam.c.md#clrtxt_log_pam_error), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [pam_login](uams_pam.c.md#pam_login), [pam_login_ext](uams_pam.c.md#pam_login_ext)

Uses file-scope variables: `PAM_conversation`, `PAM_password`, `PAM_username`, `hostname`, `pamh`

### pam_login

```c
static int pam_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 320 to 348.

cleartxt login

Calls: [login](uams_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_pam.c.md#uam_setup)

### pam_login_ext

```c
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 351 to 380.

Calls: [login](uams_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_pam.c.md#uam_setup)

### pam_logout

```c
static void pam_logout(void)
```

Defined at lines 383 to 388.

Called by: [uam_setup](uams_pam.c.md#uam_setup)

Uses file-scope variables: `pamh`

### pam_changepw

```c
static int pam_changepw(void *obj, char *username, struct passwd *pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 391 to 511.

change passwd

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [clrtxt_log_pam_error](uams_pam.c.md#clrtxt_log_pam_error), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero)

Called by: [uam_setup](uams_pam.c.md#uam_setup)

Uses file-scope variables: `PAM_chauthtok_count`, `PAM_chauthtok_mode`, `PAM_conversation`, `PAM_oldpassword`, `PAM_password`, `PAM_username`, `hostname`

### pam_printer

```c
static int pam_printer(char *start, char *stop, char *username, struct papfile *out)
```

Defined at lines 515 to 670.

Printer ClearTxtUAM login

Calls: [append](../afpd/uam.c.md#append), [clrtxt_log_pam_error](uams_pam.c.md#clrtxt_log_pam_error), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_pam.c.md#uam_setup)

Uses file-scope variables: `PAM_conversation`, `PAM_password`, `PAM_username`, `hostname`, `pamh`

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 673 to 692.

Calls: [pam_changepw](uams_pam.c.md#pam_changepw), [pam_login](uams_pam.c.md#pam_login), [pam_login_ext](uams_pam.c.md#pam_login_ext), [pam_logout](uams_pam.c.md#pam_logout), [pam_printer](uams_pam.c.md#pam_printer), [uam_register](../afpd/uam.c.md#uam_register), [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_clrtxt](uams_pam.c.md#uams_clrtxt), [uams_pam](uams_pam.c.md#uams_pam)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 694 to 699.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_clrtxt](uams_pam.c.md#uams_clrtxt), [uams_pam](uams_pam.c.md#uams_pam)

# Macros

* Undocumented: `COPY_STRING`, `PAM_CRED_ESTABLISH`, `PASSWDLEN`

# File-scope variables

`PAM_chauthtok_count`, `PAM_chauthtok_mode`, `PAM_oldpassword`, `PAM_password`, `PAM_username`, `hostname`, `pamh`
