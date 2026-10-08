---
type: C Header File
title: "include/atalk/uam.h"
description: "1 function, 2 types, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/uam.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](logger.h.md)
* System headers: `pwd.h`, `stdarg.h`

# Included by

* [bin/afppasswd/afppasswd.c](../../bin/afppasswd/afppasswd.c.md)
* [etc/afpd/switch.c](../../etc/afpd/switch.c.md)
* [etc/afpd/uam_auth.h](../../etc/afpd/uam_auth.h.md)
* [etc/papd/uam_auth.h](../../etc/papd/uam_auth.h.md)
* [etc/uams/uams_dhx2_pam.c](../../etc/uams/uams_dhx2_pam.c.md)
* [etc/uams/uams_dhx2_passwd.c](../../etc/uams/uams_dhx2_passwd.c.md)
* [etc/uams/uams_dhx_pam.c](../../etc/uams/uams_dhx_pam.c.md)
* [etc/uams/uams_dhx_passwd.c](../../etc/uams/uams_dhx_passwd.c.md)
* [etc/uams/uams_gss.c](../../etc/uams/uams_gss.c.md)
* [etc/uams/uams_guest.c](../../etc/uams/uams_guest.c.md)
* [etc/uams/uams_pam.c](../../etc/uams/uams_pam.c.md)
* [etc/uams/uams_passwd.c](../../etc/uams/uams_passwd.c.md)
* [etc/uams/uams_randnum.c](../../etc/uams/uams_randnum.c.md)
* [etc/uams/uams_srp.c](../../etc/uams/uams_srp.c.md)
* [include/atalk/globals.h](globals.h.md)
* [libatalk/util/unix.c](../../libatalk/util/unix.c.md)

# Functions

### uam_afp_read

```c
UAM_MODULE_EXPORT int uam_afp_read(void *, char *, size_t *, int(*)(void *, void *, const int))
```

Declared at include/atalk/uam.h line 99; no definition in the scanned sources.

# Types

### struct session_info

Defined at line 72.
* `void * sessionkey`
* `size_t sessionkey_len`
* `void * cryptedkey`
* `size_t cryptedkey_len`
* `void * sessiontoken`
* `size_t sessiontoken_len`
* `void * clientid`
* `size_t clientid_len`

### struct uam_export

Defined at line 63.
* `int uam_type`
* `int uam_version`
* `int(* uam_setup`: Assigned in [uams_clrtxt](../../etc/uams/uams_pam.c.md#uams_clrtxt), [uams_clrtxt](../../etc/uams/uams_passwd.c.md#uams_clrtxt), [uams_dhx](../../etc/uams/uams_dhx_pam.c.md#uams_dhx), [uams_dhx](../../etc/uams/uams_dhx_passwd.c.md#uams_dhx), [uams_dhx2](../../etc/uams/uams_dhx2_pam.c.md#uams_dhx2), [uams_dhx2](../../etc/uams/uams_dhx2_passwd.c.md#uams_dhx2), [uams_dhx2_pam](../../etc/uams/uams_dhx2_pam.c.md#uams_dhx2_pam), [uams_dhx2_passwd](../../etc/uams/uams_dhx2_passwd.c.md#uams_dhx2_passwd), [uams_dhx_pam](../../etc/uams/uams_dhx_pam.c.md#uams_dhx_pam), [uams_dhx_passwd](../../etc/uams/uams_dhx_passwd.c.md#uams_dhx_passwd), [uams_gss](../../etc/uams/uams_gss.c.md#uams_gss), [uams_guest](../../etc/uams/uams_guest.c.md#uams_guest), [uams_pam](../../etc/uams/uams_pam.c.md#uams_pam), [uams_passwd](../../etc/uams/uams_passwd.c.md#uams_passwd), [uams_randnum](../../etc/uams/uams_randnum.c.md#uams_randnum), [uams_srp](../../etc/uams/uams_srp.c.md#uams_srp); called through by [uam_load](../../etc/afpd/uam.c.md#uam_load), [uam_load](../../etc/papd/uam.c.md#uam_load).
* `void(* uam_cleanup`: Assigned in [uams_clrtxt](../../etc/uams/uams_pam.c.md#uams_clrtxt), [uams_clrtxt](../../etc/uams/uams_passwd.c.md#uams_clrtxt), [uams_dhx](../../etc/uams/uams_dhx_pam.c.md#uams_dhx), [uams_dhx](../../etc/uams/uams_dhx_passwd.c.md#uams_dhx), [uams_dhx2](../../etc/uams/uams_dhx2_pam.c.md#uams_dhx2), [uams_dhx2](../../etc/uams/uams_dhx2_passwd.c.md#uams_dhx2), [uams_dhx2_pam](../../etc/uams/uams_dhx2_pam.c.md#uams_dhx2_pam), [uams_dhx2_passwd](../../etc/uams/uams_dhx2_passwd.c.md#uams_dhx2_passwd), [uams_dhx_pam](../../etc/uams/uams_dhx_pam.c.md#uams_dhx_pam), [uams_dhx_passwd](../../etc/uams/uams_dhx_passwd.c.md#uams_dhx_passwd), [uams_gss](../../etc/uams/uams_gss.c.md#uams_gss), [uams_guest](../../etc/uams/uams_guest.c.md#uams_guest), [uams_pam](../../etc/uams/uams_pam.c.md#uams_pam), [uams_passwd](../../etc/uams/uams_passwd.c.md#uams_passwd), [uams_randnum](../../etc/uams/uams_randnum.c.md#uams_randnum), [uams_srp](../../etc/uams/uams_srp.c.md#uams_srp); called through by [uam_unload](../../etc/afpd/uam.c.md#uam_unload), [uam_unload](../../etc/papd/uam.c.md#uam_unload).

# Macros

* Undocumented: `SESSIONKEY_LEN`, `SESSIONTOKEN_LEN`, `UAM_MODULE_CLIENT`, `UAM_MODULE_EXPORT`, `UAM_MODULE_SERVER`, `UAM_MODULE_VERSION`, `UAM_NEED_LIBGCRYPT_VERSION`, `UAM_OPTION_CLIENTNAME`, `UAM_OPTION_COOKIE`, `UAM_OPTION_FQDN`, `UAM_OPTION_GUEST`, `UAM_OPTION_HOSTNAME`, `UAM_OPTION_KRB5REALM`, `UAM_OPTION_KRB5SERVICE`, `UAM_OPTION_MACCHARSET`, `UAM_OPTION_PASSWDOPT`, `UAM_OPTION_PROTOCOL`, `UAM_OPTION_RANDNUM`, `UAM_OPTION_SESSIONINFO`, `UAM_OPTION_SIGNATURE`, `UAM_OPTION_UNIXCHARSET`, `UAM_OPTION_USERNAME`, `UAM_PASSWD_EXPIRETIME`, `UAM_PASSWD_FILENAME`, `UAM_PASSWD_MINLENGTH`, `UAM_PASSWD_SRP_VERIFIER_PATH`, `UAM_SERVER_CHANGEPW`, `UAM_SERVER_LOGIN`, `UAM_SERVER_LOGIN_EXT`, `UAM_SERVER_PRINTAUTH`, `UAM_USERNAMELEN`
