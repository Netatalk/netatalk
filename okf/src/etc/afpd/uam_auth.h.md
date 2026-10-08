---
type: C Header File
title: "etc/afpd/uam_auth.h"
description: "2 types, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/uam_auth.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/uam.h](../../include/atalk/uam.h.md)
* System headers: `pwd.h`

# Included by

* [etc/afpd/afp_config.c](afp_config.c.md)
* [etc/afpd/auth.c](auth.c.md)
* [etc/afpd/main.c](main.c.md)
* [etc/afpd/status.c](status.c.md)
* [etc/afpd/uam.c](uam.c.md)

# Types

### struct uam_mod

Defined at line 19.
* `void * uam_module`
* `struct uam_export * uam_fcn`
* `struct uam_mod * uam_prev`
* `struct uam_mod * uam_next`

### struct uam_obj

Defined at line 25.
* `const char * uam_name`
* `char * uam_path`
* `int uam_count`
* `int(* login`: Called through by [afp_login](auth.c.md#afp_login).
* `int(* logincont`: Called through by [afp_logincont](auth.c.md#afp_logincont).
* `void(* logout`
* `int(* login_ext`: Called through by [afp_login_ext](auth.c.md#afp_login_ext).
* `struct uam_obj uam_login`
* `int(* uam_changepw`: Called through by [afp_changepw](auth.c.md#afp_changepw).
* `union uam_obj u`
* `struct uam_obj * uam_prev`
* `struct uam_obj * uam_next`
* `struct uam_obj uam_login`
* `int(* uam_printer`: Called through by [cq_rbilogin](../papd/queries.c.md#cq_rbilogin).
* `union uam_obj u`

# Macros

* Undocumented: `auth_unregister`, `uam_attach`, `uam_detach`
