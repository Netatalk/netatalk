---
type: C Header File
title: "etc/afpd/auth.h"
description: "1 type, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/auth.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/globals.h](../../include/atalk/globals.h.md)
* System headers: `limits.h`, `stdbool.h`

# Included by

* [etc/afpd/acls.c](acls.c.md)
* [etc/afpd/afp_asp.c](afp_asp.c.md)
* [etc/afpd/afp_dsi.c](afp_dsi.c.md)
* [etc/afpd/afp_options.c](afp_options.c.md)
* [etc/afpd/auth.c](auth.c.md)
* [etc/afpd/quota.c](quota.c.md)
* [etc/afpd/switch.c](switch.c.md)
* [etc/afpd/uam.c](uam.c.md)
* [etc/afpd/unix.c](unix.c.md)

# Types

### struct afp_versions

Defined at line 14.
* `char * av_name`
* `int av_number`

# Macros

* Undocumented: `USERIBIT_ALL`, `USERIBIT_GROUP`, `USERIBIT_USER`, `USERIBIT_UUID`

# File-scope variables

`afp_versions`, `groups`, `ngroups`, `uuid`
