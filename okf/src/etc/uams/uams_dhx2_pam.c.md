---
type: C Source File
title: "etc/uams/uams_dhx2_pam.c"
description: "19 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_dhx2_pam.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Initialized at line 280. Dispatches to: [PAM_conv](uams_dhx2_pam.c.md#pam_conv)

### uams_dhx2

Initialized at line 1183 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx2_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

### uams_dhx2_pam

Initialized at line 1190 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_dhx2_pam.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

# Functions

### dhx2_release_mpi

```c
static void dhx2_release_mpi(gcry_mpi_t *mpi)
```

Defined at lines 61 to 67.

Called by: [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [logincont1](uams_dhx2_pam.c.md#logincont1)

### dhx2_clear_session

```c
static void dhx2_clear_session(void)
```

Defined at lines 69 to 78.

Calls: [dhx2_release_mpi](uams_dhx2_pam.c.md#dhx2_release_mpi)

Called by: [changepw_3](uams_dhx2_pam.c.md#changepw_3), [dhx2_setup](uams_dhx2_pam.c.md#dhx2_setup), [logincont1](uams_dhx2_pam.c.md#logincont1), [logincont2](uams_dhx2_pam.c.md#logincont2), [pam_logincont](uams_dhx2_pam.c.md#pam_logincont)

Uses file-scope variables: `ID`, `K_MD5hash`, `K_hash_len`, `Ra`, `serverNonce`

### dh_params_generate

```c
static int dh_params_generate(unsigned int bits)
```

Defined at lines 103 to 167.

Generate a new pair of prime and generator for use in the Diffie-Hellman key exchange.

The bits value should be one of 768, 1024, 2048, 3072 or 4096.

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

Uses file-scope variables: `g`, `p`

### PAM_conv

```c
static int PAM_conv(int num_msg, struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
```

Defined at lines 175 to 278.

PAM conversation function.

Note: Here we assume (for now, at least) that echo on means login name, and echo off means password.

Uses file-scope variables: `PAM_password`, `PAM_username`

Dispatched via: [PAM_conversation](uams_dhx2_pam.c.md#pam_conversation)

### dhx2_setup

```c
static int dhx2_setup(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 286 to 382.

Calls: [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session)

Called by: [changepw_1](uams_dhx2_pam.c.md#changepw_1), [login](uams_dhx2_pam.c.md#login)

Uses file-scope variables: `ID`, `Ra`, `g`, `p`

### login

```c
static int login(void *obj, char *username, int ulen, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 385 to 398.

Calls: [dhx2_setup](uams_dhx2_pam.c.md#dhx2_setup), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [pam_login](uams_dhx2_pam.c.md#pam_login), [pam_login_ext](uams_dhx2_pam.c.md#pam_login_ext)

Uses file-scope variables: `PAM_username`, `dhxpwd`

### pam_login

```c
static int pam_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 406 to 441.

dhx login

Note: things are done in a slightly bizarre order to avoid having to clean things up if there's an error.

Calls: [login](uams_dhx2_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

### pam_login_ext

```c
static int pam_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 444 to 479.

Calls: [login](uams_dhx2_pam.c.md#login), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

### logincont1

```c
static int logincont1(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 482 to 642.

Calls: [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [dhx2_release_mpi](uams_dhx2_pam.c.md#dhx2_release_mpi)

Called by: [changepw_2](uams_dhx2_pam.c.md#changepw_2), [pam_logincont](uams_dhx2_pam.c.md#pam_logincont)

Uses file-scope variables: `ID`, `K_MD5hash`, `K_hash_len`, `Ra`, `dhx_c2siv`, `dhx_s2civ`, `p`, `serverNonce`

### dhx2_log_pam_error

```c
static void dhx2_log_pam_error(pam_handle_t *ph, const char *step, int pam_err)
```

Defined at lines 654 to 659.

Log a failed PAM step through [uam_log_pam_failure()](../../libatalk/util/unix.c.md#uam_log_pam_failure)

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

Defined at lines 661 to 880.

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [dhx2_log_pam_error](uams_dhx2_pam.c.md#dhx2_log_pam_error), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [pam_logincont](uams_dhx2_pam.c.md#pam_logincont)

Uses file-scope variables: `K_MD5hash`, `K_hash_len`, `PAM_conversation`, `PAM_password`, `PAM_username`, `dhx_c2siv`, `dhxpwd`, `pamh`, `serverNonce`

### pam_logincont

```c
static int pam_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 882 to 904.

Calls: [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [logincont1](uams_dhx2_pam.c.md#logincont1), [logincont2](uams_dhx2_pam.c.md#logincont2)

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

Uses file-scope variables: `ID`

### pam_logout

```c
static void pam_logout(void)
```

Defined at lines 908 to 913.

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

Uses file-scope variables: `pamh`

### changepw_1

```c
static int changepw_1(void *obj, char *uname, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 918 to 925.

Calls: [dhx2_setup](uams_dhx2_pam.c.md#dhx2_setup)

Called by: [dhx2_changepw](uams_dhx2_pam.c.md#dhx2_changepw)

Uses file-scope variables: `PAM_username`

### changepw_2

```c
static int changepw_2(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 927 to 931.

Calls: [logincont1](uams_dhx2_pam.c.md#logincont1)

Called by: [dhx2_changepw](uams_dhx2_pam.c.md#dhx2_changepw)

### changepw_3

```c
static int changepw_3(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 933 to 1111.

Calls: [atalk_ct_memcmp](../../libatalk/util/constant_time.c.md#atalk_ct_memcmp), [dhx2_clear_session](uams_dhx2_pam.c.md#dhx2_clear_session), [dhx2_log_pam_error](uams_dhx2_pam.c.md#dhx2_log_pam_error), [explicit_bzero](../../libatalk/compat/explicit_bzero.c.md#explicit_bzero), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option)

Called by: [dhx2_changepw](uams_dhx2_pam.c.md#dhx2_changepw)

Uses file-scope variables: `K_MD5hash`, `K_hash_len`, `PAM_conversation`, `PAM_password`, `PAM_username`, `dhx_c2siv`, `serverNonce`

### dhx2_changepw

```c
static int dhx2_changepw(void *obj, char *uname, struct passwd *pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1113 to 1150.

Calls: [changepw_1](uams_dhx2_pam.c.md#changepw_1), [changepw_2](uams_dhx2_pam.c.md#changepw_2), [changepw_3](uams_dhx2_pam.c.md#changepw_3)

Called by: [uam_setup](uams_dhx2_pam.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *obj, const char *path)
```

Defined at lines 1152 to 1172.

Calls: [dh_params_generate](uams_dhx2_pam.c.md#dh_params_generate), [dhx2_changepw](uams_dhx2_pam.c.md#dhx2_changepw), [pam_login](uams_dhx2_pam.c.md#pam_login), [pam_login_ext](uams_dhx2_pam.c.md#pam_login_ext), [pam_logincont](uams_dhx2_pam.c.md#pam_logincont), [pam_logout](uams_dhx2_pam.c.md#pam_logout), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_dhx2](uams_dhx2_pam.c.md#uams_dhx2), [uams_dhx2_pam](uams_dhx2_pam.c.md#uams_dhx2_pam)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 1174 to 1181.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Uses file-scope variables: `g`, `p`

Dispatched via: [uams_dhx2](uams_dhx2_pam.c.md#uams_dhx2), [uams_dhx2_pam](uams_dhx2_pam.c.md#uams_dhx2_pam)

# Typedefs and enums

* `enum dhx2_state`: `DHX2_STATE_IDLE`, `DHX2_STATE_EXPECT_CONT1`, `DHX2_STATE_EXPECT_CONT2`

# Macros

* Undocumented: `COPY_STRING`, `PAM_CRED_ESTABLISH`, `PASSWDBUFLEN`, `PRIMEBITS`, `dhxhash`

# File-scope variables

`ID`, `K_MD5hash`, `K_hash_len`, `PAM_password`, `PAM_username`, `Ra`, `dhx2_state`, `dhx_c2siv`, `dhx_s2civ`, `dhxpwd`, `g`, `p`, `pamh`, `serverNonce`
