---
type: C Source File
title: "etc/afpd/uam.c"
description: "9 functions, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/uam.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afp_config.h](afp_config.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [auth.h](auth.h.md)
* [uam_auth.h](uam_auth.h.md)
* System headers: `arpa/inet.h`, `bstrlib.h`, `ctype.h`, `errno.h`, `fcntl.h`, `netinet/in.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `unistd.h`

# Functions

### uam_load

```c
struct uam_mod * uam_load(AFPObj *obj, const char *path, const char *name)
```

Defined at lines 45 to 97.

Calls: [mod_close](../../include/atalk/util.h.md#mod_close), [mod_open](../../include/atalk/util.h.md#mod_open), [mod_symbol](../../include/atalk/util.h.md#mod_symbol), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [auth_load](auth.c.md#auth_load)

Calls through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export): [uam_setup](../uams/uams_dhx2_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx2_passwd.c.md#uam_setup), [uam_setup](../uams/uams_dhx_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx_passwd.c.md#uam_setup), [uam_setup](../uams/uams_gss.c.md#uam_setup), [uam_setup](../uams/uams_guest.c.md#uam_setup), [uam_setup](../uams/uams_pam.c.md#uam_setup), [uam_setup](../uams/uams_passwd.c.md#uam_setup), [uam_setup](../uams/uams_randnum.c.md#uam_setup), [uam_setup](../uams/uams_srp.c.md#uam_setup)

### uam_unload

```c
void uam_unload(struct uam_mod *mod)
```

Defined at lines 102 to 110.

Calls: [mod_close](../../include/atalk/util.h.md#mod_close)

Called by: [auth_unload](auth.c.md#auth_unload)

Calls through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export): [uam_cleanup](../uams/uams_dhx2_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx2_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_gss.c.md#uam_cleanup), [uam_cleanup](../uams/uams_guest.c.md#uam_cleanup), [uam_cleanup](../uams/uams_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_randnum.c.md#uam_cleanup), [uam_cleanup](../uams/uams_srp.c.md#uam_cleanup)

### uam_register

```c
int uam_register(const int type, const char *path, const char *name,...)
```

Defined at lines 114 to 192.

Calls: [auth_register](auth.c.md#auth_register), [auth_uamfind](auth.c.md#auth_uamfind)

Called by: [uam_setup](../uams/uams_dhx2_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx2_passwd.c.md#uam_setup), [uam_setup](../uams/uams_dhx_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx_passwd.c.md#uam_setup), [uam_setup](../uams/uams_gss.c.md#uam_setup), [uam_setup](../uams/uams_guest.c.md#uam_setup), [uam_setup](../uams/uams_pam.c.md#uam_setup), [uam_setup](../uams/uams_passwd.c.md#uam_setup), [uam_setup](../uams/uams_randnum.c.md#uam_setup), [uam_setup](../uams/uams_srp.c.md#uam_setup)

### uam_unregister

```c
void uam_unregister(const int type, const char *name)
```

Defined at lines 194 to 211.

Calls: [auth_uamfind](auth.c.md#auth_uamfind)

Called by: [uam_cleanup](../uams/uams_dhx2_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx2_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_gss.c.md#uam_cleanup), [uam_cleanup](../uams/uams_guest.c.md#uam_cleanup), [uam_cleanup](../uams/uams_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_randnum.c.md#uam_cleanup), [uam_cleanup](../uams/uams_srp.c.md#uam_cleanup), [uam_setup](../uams/uams_dhx_pam.c.md#uam_setup), [uam_setup](../uams/uams_pam.c.md#uam_setup), [uam_setup](../uams/uams_randnum.c.md#uam_setup)

### uam_getname

```c
struct passwd * uam_getname(void *private, char *name, const int len)
```

Defined at lines 219 to 314.

helper functions for plugin uams

Parameters:
* `private`: pointer to [AFPObj](../../include/atalk/globals.h.md#struct-afpobj)
* `name`: user name
* `len`: size of name buffer.

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strncasecmp_w](../../libatalk/unicode/util_unistr.c.md#strncasecmp_w)

Called by: [afp_changepw](auth.c.md#afp_changepw), [gss_logincont](../uams/uams_gss.c.md#gss_logincont), [login](../uams/uams_dhx2_pam.c.md#login), [login](../uams/uams_dhx2_passwd.c.md#login), [login](../uams/uams_dhx_pam.c.md#login), [login](../uams/uams_pam.c.md#login), [pam_printer](../uams/uams_pam.c.md#pam_printer), [passwd_printer](../uams/uams_passwd.c.md#passwd_printer), [pwd_login](../uams/uams_dhx_passwd.c.md#pwd_login), [pwd_login](../uams/uams_passwd.c.md#pwd_login), [rand_login](../uams/uams_randnum.c.md#rand_login), [srp_login](../uams/uams_srp.c.md#srp_login), [srp_login_ext](../uams/uams_srp.c.md#srp_login_ext)

### uam_checkuser

```c
int uam_checkuser(void *private, const struct passwd *pwd)
```

Defined at lines 316 to 348.

Called by: [gss_logincont](../uams/uams_gss.c.md#gss_logincont), [login](../uams/uams_pam.c.md#login), [pam_printer](../uams/uams_pam.c.md#pam_printer), [passwd_printer](../uams/uams_passwd.c.md#passwd_printer), [pwd_login](../uams/uams_dhx_passwd.c.md#pwd_login), [pwd_login](../uams/uams_passwd.c.md#pwd_login), [rand_login](../uams/uams_randnum.c.md#rand_login), [randnum_changepw](../uams/uams_randnum.c.md#randnum_changepw), [srp_login](../uams/uams_srp.c.md#srp_login), [srp_login_ext](../uams/uams_srp.c.md#srp_login_ext)

### uam_random_string

```c
int uam_random_string(AFPObj *obj, char *buf, int len)
```

Defined at lines 362 to 399.

Fill a buffer with cryptographically secure random bytes.

Reads from /dev/urandom, looping until all bytes are filled so the caller always receives a fully-populated nonce. Retries on EINTR; never falls back to a deterministic source.

Parameters:
* `obj`: AFP session object (unused)
* `buf`: destination buffer
* `len`: number of bytes to fill; must be a positive multiple of 4

Returns: 0 on success, -1 on error

Called by: [create_session_key](auth.c.md#create_session_key), [create_session_token](auth.c.md#create_session_token), [uam_afpserver_option](uam.c.md#uam_afpserver_option)

### uam_afpserver_option

```c
int uam_afpserver_option(void *private, const int what, void *option, size_t *len)
```

Defined at lines 402 to 591.

Calls: [getip_string](../../libatalk/util/socket.c.md#getip_string), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [uam_random_string](uam.c.md#uam_random_string)

Called by: [changepw_3](../uams/uams_dhx2_pam.c.md#changepw_3), [dhx_setup](../uams/uams_dhx_pam.c.md#dhx_setup), [gss_logincont](../uams/uams_gss.c.md#gss_logincont), [login](../uams/uams_pam.c.md#login), [logincont2](../uams/uams_dhx2_pam.c.md#logincont2), [noauth_login](../uams/uams_guest.c.md#noauth_login), [pam_changepw](../uams/uams_dhx_pam.c.md#pam_changepw), [pam_login](../uams/uams_dhx2_pam.c.md#pam_login), [pam_login](../uams/uams_dhx_pam.c.md#pam_login), [pam_login](../uams/uams_pam.c.md#pam_login), [pam_login_ext](../uams/uams_dhx2_pam.c.md#pam_login_ext), [pam_login_ext](../uams/uams_dhx_pam.c.md#pam_login_ext), [pam_login_ext](../uams/uams_pam.c.md#pam_login_ext), [pam_logincont](../uams/uams_dhx_pam.c.md#pam_logincont), [passwd_login](../uams/uams_dhx2_passwd.c.md#passwd_login), [passwd_login](../uams/uams_dhx_passwd.c.md#passwd_login), [passwd_login](../uams/uams_passwd.c.md#passwd_login), [passwd_login_ext](../uams/uams_dhx2_passwd.c.md#passwd_login_ext), [passwd_login_ext](../uams/uams_dhx_passwd.c.md#passwd_login_ext), [passwd_login_ext](../uams/uams_passwd.c.md#passwd_login_ext), [pwd_login](../uams/uams_dhx_passwd.c.md#pwd_login), [rand_login](../uams/uams_randnum.c.md#rand_login), [randnum_changepw](../uams/uams_randnum.c.md#randnum_changepw), [randnum_login](../uams/uams_randnum.c.md#randnum_login), [randnum_login_ext](../uams/uams_randnum.c.md#randnum_login_ext), [randnum_warn_passwdfile_key](../uams/uams_randnum.c.md#randnum_warn_passwdfile_key), [srp_login](../uams/uams_srp.c.md#srp_login), [srp_login_ext](../uams/uams_srp.c.md#srp_login_ext), [srp_setup](../uams/uams_srp.c.md#srp_setup)

### append

```c
UAM_MODULE_EXPORT void append(struct papfile *pf, const char *data, int len)
```

Defined at lines 596 to 600.

Called by: [noauth_printer](../uams/uams_guest.c.md#noauth_printer), [pam_printer](../uams/uams_pam.c.md#pam_printer), [passwd_printer](../uams/uams_passwd.c.md#passwd_printer)
