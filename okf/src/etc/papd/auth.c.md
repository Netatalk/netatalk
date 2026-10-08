---
type: C Source File
title: "etc/papd/auth.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/auth.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [uam_auth.h](uam_auth.h.md)
* System headers: `ctype.h`, `grp.h`, `limits.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### getuamnames

```c
int getuamnames(const int type, char *uamnames)
```

Defined at lines 43 to 60.

Return a list of names for loaded uams

Called by: [gq_rbiuamlist](queries.c.md#gq_rbiuamlist)

### auth_uamfind

```c
struct uam_obj * auth_uamfind(const int type, const char *name, const int len)
```

Defined at lines 65 to 82.

just do a linked list search. this could be sped up with a hashed list, but i doubt anyone's going to have enough uams to matter.

Calls: [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp)

Called by: [cq_rbilogin](queries.c.md#cq_rbilogin), [uam_register](uam.c.md#uam_register), [uam_unregister](uam.c.md#uam_unregister)

### auth_register

```c
int auth_register(const int type, struct uam_obj *uam)
```

Defined at lines 84 to 98.

Called by: [uam_register](uam.c.md#uam_register)

### auth_load

```c
int auth_load(const char *path, const char *list)
```

Defined at lines 101 to 137.

load all of the modules

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_load](uam.c.md#uam_load)

Called by: [main](main.c.md#main)

### auth_unload

```c
void auth_unload(void)
```

Defined at lines 140 to 150.

get rid of all of the uams

Calls: [uam_unload](uam.c.md#uam_unload)

Called by: [papd_exit](main.c.md#papd_exit)

# File-scope variables

`uam_changepw`, `uam_login`, `uam_modules`, `uam_printer`
