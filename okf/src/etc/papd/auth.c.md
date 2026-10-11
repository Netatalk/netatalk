---
type: C Source File
title: "etc/papd/auth.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/auth.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 52 to 69.

Write the names of the UAMs of a type into a buffer, one per line.

The names come in reverse order of registration, and a final line with `*` ends the list. The call appends to uamnames with strcat and has no bound, so the buffer has to hold every name.

Parameters:
* `type`: UAM_SERVER_LOGIN, UAM_SERVER_CHANGEPW, or UAM_SERVER_PRINTAUTH
* `uamnames`: NUL-terminated buffer that receives the names

Returns: 0, or -1 for an unknown type

Called by: [gq_rbiuamlist](queries.c.md#gq_rbiuamlist)

### auth_uamfind

```c
struct uam_obj * auth_uamfind(const int type, const char *name, const int len)
```

Defined at lines 77 to 94.

Find a registered UAM by name.

In papd each UAM type, UAM_SERVER_PRINTAUTH included, has a separate list.

Calls: [strndiacasecmp](../../libatalk/util/strdicasecmp.c.md#strndiacasecmp)

Called by: [cq_rbilogin](queries.c.md#cq_rbilogin), [uam_register](uam.c.md#uam_register), [uam_unregister](uam.c.md#uam_unregister)

### auth_register

```c
int auth_register(const int type, struct uam_obj *uam)
```

Defined at lines 101 to 115.

Add a UAM to the list of its type.

In papd UAM_SERVER_PRINTAUTH has a list as well.

Called by: [uam_register](uam.c.md#uam_register)

### auth_load

```c
int auth_load(const char *path, const char *list)
```

Defined at lines 131 to 167.

Load the UAM modules that the am option of papd.conf names.

Each name in list, separated by commas, loads from the UAM directory. The call skips a missing module without a log line. A module that fails to open, is no server module, or fails its setup gets a log line from uam_load.

Parameters:
* `path`: UAM directory
* `list`: comma-separated module names, or NULL when no printer sets am

Returns: 0, or -1 for a NULL path or list, a path that is too long, or a list without names

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [uam_load](uam.c.md#uam_load)

Called by: [main](main.c.md#main)

### auth_unload

```c
void auth_unload(void)
```

Defined at lines 170 to 180.

get rid of all of the uams

Calls: [uam_unload](uam.c.md#uam_unload)

Called by: [papd_exit](main.c.md#papd_exit)

# File-scope variables

`uam_changepw`, `uam_login`, `uam_modules`, `uam_printer`
