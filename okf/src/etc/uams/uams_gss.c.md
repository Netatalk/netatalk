---
type: C Source File
title: "etc/uams/uams_gss.c"
description: "Kerberos UAM, Client Krb v2, through the GSS-API."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/uams/uams_gss.c"
tags: ["etc/uams"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Initialized at line 862 as `struct uam_export`. Assigns:

* `uam_cleanup`: [uam_cleanup](uams_gss.c.md#uam_cleanup)
* `uam_setup`: [uam_setup](uams_gss.c.md#uam_setup)

# Functions

### log_status

```c
static void log_status(char *s, OM_uint32 major_status, OM_uint32 minor_status)
```

Defined at lines 59 to 92.

Log the GSS-API major and minor status messages of a failed call.

Parameters:
* `s`: name of the failed call, as the prefix of each line
* `major_status`: GSS major status
* `minor_status`: mechanism minor status

Called by: [accept_sec_context](uams_gss.c.md#accept_sec_context), [get_client_username](uams_gss.c.md#get_client_username), [log_service_name](uams_gss.c.md#log_service_name), [wrap_sessionkey](uams_gss.c.md#wrap_sessionkey)

### log_ctx_flags

```c
static void log_ctx_flags(OM_uint32 flags)
```

Defined at lines 99 to 124.

Log each flag of an accepted GSS context at debug level.

Parameters:
* `flags`: context flags from gss_accept_sec_context

Called by: [accept_sec_context](uams_gss.c.md#accept_sec_context)

### log_service_name

```c
static void log_service_name(gss_ctx_id_t context)
```

Defined at lines 131 to 166.

Log the service principal of an accepted GSS context at debug level.

Parameters:
* `context`: accepted context

Calls: [log_status](uams_gss.c.md#log_status)

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### get_client_username

```c
static int get_client_username(char *username, int ulen, gss_name_t *client_name)
```

Defined at lines 180 to 238.

Write the user part of the client principal into the username buffer of afpd.

The call cuts the principal at its first @ and its first /, so neither the realm nor an instance reaches the local name.

Parameters:
* `username`: receives the user name
* `ulen`: size of username
* `client_name`: client principal of the context

Returns: 0, or 1 when the name does not display or does not fit

Calls: [log_status](uams_gss.c.md#log_status), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### wrap_sessionkey

```c
static int wrap_sessionkey(gss_ctx_id_t context, struct session_info *sinfo)
```

Defined at lines 253 to 296.

Encrypt the sessionkey of afpd with gss_wrap into the cryptedkey field of the session info.

Mac OS X 10.3 clients fetch the result with FPGetSessionToken type 8, kGetKerberosSessionKey. The call asks gss_wrap for confidentiality and integrity.

Parameters:
* `context`: established GSS context
* `sinfo`: session info with the sessionkey, receives the cryptedkey

Returns: 0, or 1 when gss_wrap or malloc fails

Calls: [log_status](uams_gss.c.md#log_status)

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### accept_sec_context

```c
static int accept_sec_context(gss_ctx_id_t *context, gss_buffer_desc *ticket_buffer, gss_name_t *client_name, gss_buffer_desc *authenticator_buff)
```

Defined at lines 311 to 347.

Accept the GSS context that the ticket of the client starts, in one step.

The call uses the default acceptor credential and needs GSS_S_COMPLETE, so a mechanism that asks for another round fails.

Parameters:
* `context`: receives the context
* `ticket_buffer`: ticket from the request
* `client_name`: receives the client principal
* `authenticator_buff`: receives the token for the client

Returns: 0, or 1 when the context does not complete

Calls: [log_ctx_flags](uams_gss.c.md#log_ctx_flags), [log_status](uams_gss.c.md#log_status)

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### build_gss_reply

```c
static int build_gss_reply(char *rbuf, size_t rbufsize, size_t *rbuflen, const gss_buffer_desc *authenticator)
```

Defined at lines 359 to 382.

Encode the authenticator with its 2-byte length, when the reply fits in rbuf.

Parameters:
* `rbuf`: reply block
* `rbufsize`: capacity of rbuf
* `rbuflen`: reply length on return, 0 on failure
* `authenticator`: token for the client

Returns: 0, or 1 when the authenticator does not fit

Called by: [do_gss_auth](uams_gss.c.md#do_gss_auth)

### do_gss_auth

```c
static int do_gss_auth(void *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t rbufsize, size_t *rbuflen, char *username, size_t ulen, struct session_info *sinfo)
```

Defined at lines 399 to 447.

Accept the ticket of the client, take its user name, encrypt the sessionkey, and build the reply.

Parameters:
* `obj`: session object, unused
* `ibuf`: ticket
* `ibuflen`: length of the ticket
* `rbuf`: reply block
* `rbufsize`: capacity of rbuf
* `rbuflen`: reply length on return
* `username`: receives the user part of the client principal
* `ulen`: size of username
* `sinfo`: session info, receives the cryptedkey

Returns: 0, or nonzero when a step fails

Calls: [accept_sec_context](uams_gss.c.md#accept_sec_context), [build_gss_reply](uams_gss.c.md#build_gss_reply), [get_client_username](uams_gss.c.md#get_client_username), [log_service_name](uams_gss.c.md#log_service_name), [wrap_sessionkey](uams_gss.c.md#wrap_sessionkey)

Called by: [gss_logincont](uams_gss.c.md#gss_logincont)

### gss_login

```c
static int gss_login(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 465 to 478.

Answer a Kerberos login with the 2-byte login ID 1.

Nothing that the client sent so far is trustworthy, since the signed ticket comes with FPLoginCont. Apple returns 1 as well.

Parameters:
* `obj`: session object, unused
* `uam_pwd`: unused
* `ibuf`: UAM data of the request, unused
* `ibuflen`: length of ibuf, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT

Called by: [gss_login_ext](uams_gss.c.md#gss_login_ext), [uam_setup](uams_gss.c.md#uam_setup)

### gss_logincont

```c
static int gss_logincont(void *obj, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 500 to 637.

Authenticate a Kerberos login with the ticket of FPLoginCont.

The request holds a byte that is always 0, the login ID, and a NUL-terminated user name that the call ignores. Padding to an even length and the ticket with its 2-byte length follow. The user of the session comes from the client principal, without its realm and instance, through uam_getname. That user has to pass uam_checkuser. The reply holds the authenticator for the client.

Parameters:
* `obj`: session object
* `uam_pwd`: receives the user entry after a successful login
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_PARAM for a malformed request, AFPERR_NOTAUTH for an unknown or refused user, or AFPERR_MISC when the ticket fails

Calls: [do_gss_auth](uams_gss.c.md#do_gss_auth), [uam_afpserver_option](../afpd/uam.c.md#uam_afpserver_option), [uam_checkuser](../afpd/uam.c.md#uam_checkuser), [uam_getname](../afpd/uam.c.md#uam_getname)

Called by: [uam_setup](uams_gss.c.md#uam_setup)

### gss_logout

```c
static void gss_logout(void)
```

Defined at lines 642 to 644.

End a Kerberos login, which keeps no state.

Called by: [uam_setup](uams_gss.c.md#uam_setup)

### gss_login_ext

```c
static int gss_login_ext(void *obj, char *uname, struct passwd **uam_pwd, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 658 to 665.

Answer a Kerberos FPLoginExt as gss_login does, ignoring the user name.

Parameters:
* `obj`: session object
* `uname`: user name field of the request, unused
* `uam_pwd`: unused
* `ibuf`: UAM data of the request
* `ibuflen`: length of ibuf
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFPERR_AUTHCONT

Calls: [gss_login](uams_gss.c.md#gss_login)

Called by: [uam_setup](uams_gss.c.md#uam_setup)

### set_principal

```c
static int set_principal(AFPObj *obj, char *principal)
```

Defined at lines 678 to 701.

Store the service principal for the directory names of FPGetSrvrInfo.

The buffer holds 1 principal: a count byte of 1, a length byte, and the name without a NUL.

Parameters:
* `obj`: AFP object, receives k5principal and k5principal_buflen
* `principal`: principal name, up to 255 bytes

Returns: 0, or -1 for a longer name or a failed malloc

Called by: [gss_create_principal](uams_gss.c.md#gss_create_principal)

### gss_create_principal

```c
static int gss_create_principal(AFPObj *obj)
```

Defined at lines 715 to 833.

Pick the service principal of the Kerberos UAM and store it in the AFP options.

With Kerberos and all 3 of k5 service, fqdn, and k5 realm in afp.conf, the principal comes from them. It has to be in the default keytab. Without them the first keytab entry serves. A build without the Kerberos library composes `service/fqdn@realm` from the 3 options, which it then needs.

Parameters:
* `obj`: AFP object with the options

Returns: 0, or -1 without a usable principal

Calls: [set_principal](uams_gss.c.md#set_principal)

Called by: [uam_setup](uams_gss.c.md#uam_setup)

### uam_setup

```c
static int uam_setup(void *handle, const char *path)
```

Defined at lines 842 to 852.

Pick the service principal and register the Client Krb v2 login.

Parameters:
* `handle`: AFP object
* `path`: path of the module

Returns: 0, or -1 without a principal or when the registration fails

Calls: [gss_create_principal](uams_gss.c.md#gss_create_principal), [gss_login](uams_gss.c.md#gss_login), [gss_login_ext](uams_gss.c.md#gss_login_ext), [gss_logincont](uams_gss.c.md#gss_logincont), [gss_logout](uams_gss.c.md#gss_logout), [uam_register](../afpd/uam.c.md#uam_register)

Called through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_load](../afpd/uam.c.md#uam_load), [uam_load](../papd/uam.c.md#uam_load)

Dispatched via: [uams_gss](uams_gss.c.md#uams_gss)

### uam_cleanup

```c
static void uam_cleanup(void)
```

Defined at lines 857 to 860.

Unregister the Client Krb v2 login.

Calls: [uam_unregister](../afpd/uam.c.md#uam_unregister)

Called through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export) by: [uam_unload](../afpd/uam.c.md#uam_unload), [uam_unload](../papd/uam.c.md#uam_unload)

Dispatched via: [uams_gss](uams_gss.c.md#uams_gss)

# Macros

* Undocumented: `LOG_LOGINCONT`, `LOG_UAMS`
