---
type: C Source File
title: "etc/uams/uams_gss.c"
description: "16 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_gss.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/uams](../uams.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/uam.h](../../include/atalk/uam.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `stdbool.h`, `stdint.h`, `stdlib.h`, `string.h`

# Function tables

### uams_gss

Initialized at line 703 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_gss.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_gss.c.md#uam_setup)

# Functions

### log_status

```c
static void log_status(char *s, OM_uint32 major_status, OM_uint32 minor_status)
```

Defined at lines 47 to 80.

Called by: [accept_sec_context](uams_gss.c.md#accept_sec_context), [get_client_username](uams_gss.c.md#get_client_username), [log_service_name](uams_gss.c.md#log_service_name), [wrap_sessionkey](uams_gss.c.md#wrap_sessionkey)

### log_ctx_flags

```c
static void log_ctx_flags(OM_uint32 flags)
```

Defined at lines 82 to 107.

Called by: [accept_sec_context](uams_gss.c.md#accept_sec_context)

### log_service_name

```c
static void log_service_name(gss_ctx_id_t context)
```

Defined at lines 109 to 144.

Calls: [log_status](uams_gss.c.md#log_status)

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### get_client_username

```c
static int get_client_username(char *username, int ulen, gss_name_t *client_name)
```

Defined at lines 146 to 204.

Calls: [log_status](uams_gss.c.md#log_status), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### wrap_sessionkey

```c
static int wrap_sessionkey(gss_ctx_id_t context, struct session_info *sinfo)
```

Defined at lines 207 to 250.

wrap afpd's sessionkey

Calls: [log_status](uams_gss.c.md#log_status)

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### accept_sec_context

```c
static int accept_sec_context(gss_ctx_id_t *context, gss_buffer_desc *ticket_buffer, gss_name_t *client_name, gss_buffer_desc *authenticator_buff)
```

Defined at lines 252 to 288.

Calls: [log_ctx_flags](uams_gss.c.md#log_ctx_flags), [log_status](uams_gss.c.md#log_status)

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### build_gss_reply

```c
static int build_gss_reply(char *rbuf, size_t rbufsize, size_t *rbuflen, const gss_buffer_desc *authenticator)
```

Defined at lines 291 to 314.

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### do_gss_auth

```c
static int do_gss_auth(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t rbufsize, size_t *rbuflen, char *username, size_t ulen, struct session_info *sinfo)
```

Defined at lines 316 to 364.

Calls: [accept_sec_context](uams_gss.c.md#accept_sec_context), [build_gss_reply](uams_gss.c.md#build_gss_reply), [get_client_username](uams_gss.c.md#get_client_username), [log_service_name](uams_gss.c.md#log_service_name), [wrap_sessionkey](uams_gss.c.md#wrap_sessionkey)

Called by: [gss_logincont](uams_gss.c.md#gss_logincont)

### gss_login

```c
static int gss_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 373 to 386.

Called by: [gss_login_ext](uams_gss.c.md#gss_login_ext), [uam_setup](uams_gss.c.md#uam_setup)

### gss_logincont

```c
static int gss_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 388 to 525.

Calls: [do_gss_auth](uams_gss.c.md#do_gss_auth), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_gss.c.md#uam_setup)

### gss_logout

```c
static void gss_logout(void)
```

Defined at lines 528 to 530.

Called by: [uam_setup](uams_gss.c.md#uam_setup)

### gss_login_ext

```c
static int gss_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 532 to 539.

Calls: [gss_login](uams_gss.c.md#gss_login)

Called by: [uam_setup](uams_gss.c.md#uam_setup)

### set_principal

```c
static int set_principal(AFPObj *obj, char *principal)
```

Defined at lines 541 to 564.

Called by: [gss_create_principal](uams_gss.c.md#gss_create_principal)

### gss_create_principal

```c
static int gss_create_principal(AFPObj *obj)
```

Defined at lines 566 to 684.

Calls: [set_principal](uams_gss.c.md#set_principal)

Called by: [uam_setup](uams_gss.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *handle, const char *path)
```

Defined at lines 686 to 696.

Calls: [gss_create_principal](uams_gss.c.md#gss_create_principal), [gss_login](uams_gss.c.md#gss_login), [gss_login_ext](uams_gss.c.md#gss_login_ext), [gss_logincont](uams_gss.c.md#gss_logincont), [gss_logout](uams_gss.c.md#gss_logout), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_gss](uams_gss.c.md#uams_gss)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 698 to 701.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_gss](uams_gss.c.md#uams_gss)

# Macros

* Undocumented: `LOG_LOGINCONT`, `LOG_UAMS`
