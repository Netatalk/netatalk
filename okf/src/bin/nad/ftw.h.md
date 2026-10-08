---
type: C Header File
title: "bin/nad/ftw.h"
description: "1 function, 1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/ftw.h"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `sys/stat.h`, `sys/types.h`

# Included by

* [bin/nad/ftw.c](ftw.c.md)
* [bin/nad/nad_cp.c](nad_cp.c.md)
* [bin/nad/nad_rm.c](nad_rm.c.md)

# Functions

### nftw

```c
int nftw(const char *dir, nftw_func_t func, dir_notification_func_t up, int descriptors, int flag)
```

Declared at bin/nad/ftw.h line 95; no definition in the scanned sources.

Called by: [nad_cp](nad_cp.c.md#nad_cp), [nad_rm](nad_rm.c.md#nad_rm)

Mentioned in the documentation of: [copy](nad_cp.c.md#copy), [nad_cp](nad_cp.c.md#nad_cp)

# Types

### struct FTW

Defined at line 82.
* `int base`
* `int level`

# Typedefs and enums

* `typedef void(* dir_notification_func_t`
* `typedef int(* nftw_func_t`
* `enum (anonymous)`: `FTW_CONTINUE`, `FTW_CONTINUE`, `FTW_STOP`, `FTW_STOP`, `FTW_SKIP_SUBTREE`, `FTW_SKIP_SUBTREE`, `FTW_SKIP_SIBLINGS`, `FTW_SKIP_SIBLINGS`
* `enum (anonymous)`: `FTW_F`, `FTW_F`, `FTW_D`, `FTW_D`, `FTW_DNR`, `FTW_DNR`, `FTW_NS`, `FTW_NS`, `FTW_SL`, `FTW_SL`, `FTW_DP`, `FTW_DP`
* `enum (anonymous)`: `FTW_PHYS`, `FTW_PHYS`, `FTW_MOUNT`, `FTW_MOUNT`, `FTW_CHDIR`, `FTW_CHDIR`, `FTW_DEPTH`, `FTW_DEPTH`, `FTW_ACTIONRETVAL`

# Macros

* Undocumented: `FTW_ACTIONRETVAL`, `FTW_CHDIR`, `FTW_CONTINUE`, `FTW_D`, `FTW_DEPTH`, `FTW_DNR`, `FTW_DP`, `FTW_F`, `FTW_MOUNT`, `FTW_NS`, `FTW_PHYS`, `FTW_SKIP_SIBLINGS`, `FTW_SKIP_SUBTREE`, `FTW_SL`, `FTW_SLN`, `FTW_STOP`
