---
type: C Header File
title: "bin/nad/ftw.h"
description: "File tree traversal callbacks, flags and entry point for nad."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/ftw.h"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `sys/stat.h`, `sys/types.h`

# Included by

* [bin/nad/ftw.c](ftw.c.md)
* [bin/nad/nad_cp.c](nad_cp.c.md)
* [bin/nad/nad_rm.c](nad_rm.c.md)

# Types

### struct FTW

Structure used for fourth argument to callback function for 'nftw'.

Defined at line 83.
* `int base`
* `int level`

# Typedefs and enums

* `typedef void(* dir_notification_func_t`
* `typedef int(* nftw_func_t`
* `enum (anonymous)`: `FTW_CONTINUE`, `FTW_CONTINUE`, `FTW_STOP`, `FTW_STOP`, `FTW_SKIP_SUBTREE`, `FTW_SKIP_SUBTREE`, `FTW_SKIP_SIBLINGS`, `FTW_SKIP_SIBLINGS`
* `enum (anonymous)`: `FTW_F`, `FTW_F`, `FTW_D`, `FTW_D`, `FTW_DNR`, `FTW_DNR`, `FTW_NS`, `FTW_NS`, `FTW_SL`, `FTW_SL`, `FTW_DP`, `FTW_DP`
* `enum (anonymous)`: `FTW_PHYS`, `FTW_PHYS`, `FTW_MOUNT`, `FTW_MOUNT`, `FTW_CHDIR`, `FTW_CHDIR`, `FTW_DEPTH`, `FTW_DEPTH`, `FTW_ACTIONRETVAL`

# Macros

* `FTW_SLN`: Symbolic link naming non-existing file.
* Undocumented: `FTW_ACTIONRETVAL`, `FTW_CHDIR`, `FTW_CONTINUE`, `FTW_D`, `FTW_DEPTH`, `FTW_DNR`, `FTW_DP`, `FTW_F`, `FTW_MOUNT`, `FTW_NS`, `FTW_PHYS`, `FTW_SKIP_SIBLINGS`, `FTW_SKIP_SUBTREE`, `FTW_SL`, `FTW_STOP`
