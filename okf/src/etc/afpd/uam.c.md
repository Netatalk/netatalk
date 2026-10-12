---
type: C Source File
title: "etc/afpd/uam.c"
description: "UAM module loading and the server services that UAMs call."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/uam.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 60 to 112.

Load a UAM module and run its setup function.

The module must export a server module structure named after the file name without its extension, and that structure must carry a uam_setup function.

Parameters:
* `obj`: server object passed to uam_setup
* `path`: path of the module file
* `name`: file name of the module

Returns: the loaded module, or NULL when loading or setup fails

Calls: [mod_close](../../include/atalk/util.h.md#mod_close), [mod_open](../../include/atalk/util.h.md#mod_open), [mod_symbol](../../include/atalk/util.h.md#mod_symbol), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [auth_load](auth.c.md#auth_load)

Calls through [`uam_export::uam_setup`](../../include/atalk/uam.h.md#struct-uam_export): [uam_setup](../uams/uams_dhx2_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx2_passwd.c.md#uam_setup), [uam_setup](../uams/uams_dhx_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx_passwd.c.md#uam_setup), [uam_setup](../uams/uams_gss.c.md#uam_setup), [uam_setup](../uams/uams_guest.c.md#uam_setup), [uam_setup](../uams/uams_pam.c.md#uam_setup), [uam_setup](../uams/uams_passwd.c.md#uam_setup), [uam_setup](../uams/uams_randnum.c.md#uam_setup), [uam_setup](../uams/uams_srp.c.md#uam_setup)

### uam_unload

```c
void uam_unload(struct uam_mod *mod)
```

Defined at lines 121 to 129.

Run the cleanup function of a UAM module, close it, and free its entry.

A module without a cleanup function still closes, but it may leak.

Parameters:
* `mod`: loaded module

Calls: [mod_close](../../include/atalk/util.h.md#mod_close)

Called by: [auth_unload](auth.c.md#auth_unload)

Calls through [`uam_export::uam_cleanup`](../../include/atalk/uam.h.md#struct-uam_export): [uam_cleanup](../uams/uams_dhx2_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx2_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_gss.c.md#uam_cleanup), [uam_cleanup](../uams/uams_guest.c.md#uam_cleanup), [uam_cleanup](../uams/uams_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_randnum.c.md#uam_cleanup), [uam_cleanup](../uams/uams_srp.c.md#uam_cleanup)

### uam_register

```c
int uam_register(const int type, const char *path, const char *name,...)
```

Defined at lines 147 to 225.

Register a UAM of a module under a name and type.

The variadic arguments carry the handlers of the type: login, logincont, and logout for UAM_SERVER_LOGIN, plus login_ext for UAM_SERVER_LOGIN_EXT, or the change function for UAM_SERVER_CHANGEPW. A second registration from the same module only raises its count.

Parameters:
* `type`: UAM type
* `path`: path of the module that registers
* `name`: UAM name, kept by pointer
* `...`: handlers of the type

Returns: 0, -1 when another module holds the name or an allocation fails, or 1 for an unknown type

Calls: [auth_register](auth.c.md#auth_register), [auth_uamfind](auth.c.md#auth_uamfind)

Called by: [uam_setup](../uams/uams_dhx2_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx2_passwd.c.md#uam_setup), [uam_setup](../uams/uams_dhx_pam.c.md#uam_setup), [uam_setup](../uams/uams_dhx_passwd.c.md#uam_setup), [uam_setup](../uams/uams_gss.c.md#uam_setup), [uam_setup](../uams/uams_guest.c.md#uam_setup), [uam_setup](../uams/uams_pam.c.md#uam_setup), [uam_setup](../uams/uams_passwd.c.md#uam_setup), [uam_setup](../uams/uams_randnum.c.md#uam_setup), [uam_setup](../uams/uams_srp.c.md#uam_setup)

### uam_unregister

```c
void uam_unregister(const int type, const char *name)
```

Defined at lines 233 to 250.

Drop a registration of a UAM and free the UAM after its last one.

Parameters:
* `type`: UAM type
* `name`: UAM name

Calls: [auth_uamfind](auth.c.md#auth_uamfind)

Called by: [uam_cleanup](../uams/uams_dhx2_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx2_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_dhx_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_gss.c.md#uam_cleanup), [uam_cleanup](../uams/uams_guest.c.md#uam_cleanup), [uam_cleanup](../uams/uams_pam.c.md#uam_cleanup), [uam_cleanup](../uams/uams_passwd.c.md#uam_cleanup), [uam_cleanup](../uams/uams_randnum.c.md#uam_cleanup), [uam_cleanup](../uams/uams_srp.c.md#uam_cleanup), [uam_setup](../uams/uams_dhx_pam.c.md#uam_setup), [uam_setup](../uams/uams_pam.c.md#uam_setup), [uam_setup](../uams/uams_randnum.c.md#uam_setup)

### uam_getname

```c
struct passwd * uam_getname(void *private, char *name, const int len)
```

Defined at lines 266 to 361.

Find the password entry of a login name for a UAM.

The call tries getpwnam first, then the name with the AD domain or the NT domain prefix. A last pass compares the name, without case, against the login name and the first gecos field of every account. A match writes the login name back into name.

Parameters:
* `private`: session object
* `name`: login name from the client, receives the account name of a match
* `len`: size of the name buffer

Returns: the password entry, or NULL without a match

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strncasecmp_w](../../libatalk/unicode/util_unistr.c.md#strncasecmp_w)

Called by: [afp_changepw](auth.c.md#afp_changepw), [gss_logincont](../uams/uams_gss.c.md#gss_logincont), [login](../uams/uams_dhx2_pam.c.md#login), [login](../uams/uams_dhx2_passwd.c.md#login), [login](../uams/uams_dhx_pam.c.md#login), [login](../uams/uams_pam.c.md#login), [pam_printer](../uams/uams_pam.c.md#pam_printer), [passwd_printer](../uams/uams_passwd.c.md#passwd_printer), [pwd_login](../uams/uams_dhx_passwd.c.md#pwd_login), [pwd_login](../uams/uams_passwd.c.md#pwd_login), [rand_login](../uams/uams_randnum.c.md#rand_login), [srp_login](../uams/uams_srp.c.md#srp_login), [srp_login_ext](../uams/uams_srp.c.md#srp_login_ext)

### uam_checkuser

```c
int uam_checkuser(void *private, const struct passwd *pwd)
```

Defined at lines 372 to 404.

Check that a user may log in through a UAM.

With OPTION_VALID_SHELLCHECK the user needs a shell that /etc/shells lists.

Parameters:
* `private`: session object, or NULL
* `pwd`: password entry of the user

Returns: 0, or -1 for a NULL entry or a shell that is missing or not listed

Called by: [gss_logincont](../uams/uams_gss.c.md#gss_logincont), [login](../uams/uams_pam.c.md#login), [pam_printer](../uams/uams_pam.c.md#pam_printer), [passwd_printer](../uams/uams_passwd.c.md#passwd_printer), [pwd_login](../uams/uams_dhx_passwd.c.md#pwd_login), [pwd_login](../uams/uams_passwd.c.md#pwd_login), [rand_login](../uams/uams_randnum.c.md#rand_login), [randnum_changepw](../uams/uams_randnum.c.md#randnum_changepw), [srp_login](../uams/uams_srp.c.md#srp_login), [srp_login_ext](../uams/uams_srp.c.md#srp_login_ext)

### uam_random_string

```c
int uam_random_string(AFPObj *obj, char *buf, int len)
```

Defined at lines 418 to 455.

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

Defined at lines 481 to 670.

Answer a UAM request for a server or session value.

The what code selects the value:

* UAM_OPTION_USERNAME and UAM_OPTION_GUEST name the session user and the guest account.
* UAM_OPTION_PASSWDOPT returns the password option that len selects.
* UAM_OPTION_SIGNATURE, UAM_OPTION_HOSTNAME, UAM_OPTION_FQDN, and the Kerberos codes return server values.
* UAM_OPTION_RANDNUM fills option with random bytes.
* The other codes return the protocol, the client name, the cookie, the charsets, or the session information.

String values come back as pointers in option, with their length in len when the caller passes one.

Parameters:
* `private`: session object
* `what`: UAM_OPTION_* code
* `option`: receives the value or a pointer to it
* `len`: length on return, or the password option code for UAM_OPTION_PASSWDOPT

Returns: 0, or in afpd -1 for an unknown code or a missing argument

Calls: [getip_string](../../libatalk/util/socket.c.md#getip_string), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [uam_random_string](uam.c.md#uam_random_string)

Called by: [changepw_3](../uams/uams_dhx2_pam.c.md#changepw_3), [dhx_setup](../uams/uams_dhx_pam.c.md#dhx_setup), [gss_logincont](../uams/uams_gss.c.md#gss_logincont), [login](../uams/uams_pam.c.md#login), [logincont2](../uams/uams_dhx2_pam.c.md#logincont2), [noauth_login](../uams/uams_guest.c.md#noauth_login), [pam_changepw](../uams/uams_dhx_pam.c.md#pam_changepw), [pam_login](../uams/uams_dhx2_pam.c.md#pam_login), [pam_login](../uams/uams_dhx_pam.c.md#pam_login), [pam_login](../uams/uams_pam.c.md#pam_login), [pam_login_ext](../uams/uams_dhx2_pam.c.md#pam_login_ext), [pam_login_ext](../uams/uams_dhx_pam.c.md#pam_login_ext), [pam_login_ext](../uams/uams_pam.c.md#pam_login_ext), [pam_logincont](../uams/uams_dhx_pam.c.md#pam_logincont), [passwd_login](../uams/uams_dhx2_passwd.c.md#passwd_login), [passwd_login](../uams/uams_dhx_passwd.c.md#passwd_login), [passwd_login](../uams/uams_passwd.c.md#passwd_login), [passwd_login_ext](../uams/uams_dhx2_passwd.c.md#passwd_login_ext), [passwd_login_ext](../uams/uams_dhx_passwd.c.md#passwd_login_ext), [passwd_login_ext](../uams/uams_passwd.c.md#passwd_login_ext), [pwd_login](../uams/uams_dhx_passwd.c.md#pwd_login), [rand_login](../uams/uams_randnum.c.md#rand_login), [randnum_changepw](../uams/uams_randnum.c.md#randnum_changepw), [randnum_login](../uams/uams_randnum.c.md#randnum_login), [randnum_login_ext](../uams/uams_randnum.c.md#randnum_login_ext), [randnum_warn_passwdfile_key](../uams/uams_randnum.c.md#randnum_warn_passwdfile_key), [srp_login](../uams/uams_srp.c.md#srp_login), [srp_login_ext](../uams/uams_srp.c.md#srp_login_ext), [srp_setup](../uams/uams_srp.c.md#srp_setup)

### append

```c
UAM_MODULE_EXPORT void append(struct papfile *pf, const char *data, int len)
```

Defined at lines 683 to 687.

Stub of the papd append call for the UAM modules that afpd loads, which ignores its arguments.

Parameters:
* `pf`: papfile buffer in papd, unused in afpd
* `data`: bytes to append in papd, unused in afpd
* `len`: number of bytes in papd, unused in afpd

Called by: [noauth_printer](../uams/uams_guest.c.md#noauth_printer), [pam_printer](../uams/uams_pam.c.md#pam_printer), [passwd_printer](../uams/uams_passwd.c.md#passwd_printer)
