---
type: C Header File
title: "etc/papd/comment.h"
description: "1 function, 2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/comment.h"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `sys/types.h`

# Included by

* [etc/papd/comment.c](comment.c.md)
* [etc/papd/headers.c](headers.c.md)
* [etc/papd/magics.c](magics.c.md)
* [etc/papd/queries.c](queries.c.md)

# Functions

### comtoken

```c
char * comtoken(char *, char *, char *, char *)
```

Declared at etc/papd/comment.h line 65; no definition in the scanned sources.

# Types

### struct comstate

Defined at line 25.
* `struct papd_comment * cs_comment`
* `struct comstate * cs_prev`
* `int cs_flags`

### struct papd_comment

Defined at line 14.
* `char * c_begin`
* `char * c_end`
* `int(* c_handler`: Called through by [ps](magics.c.md#ps).
* `int c_flags`

# Macros

* Undocumented: `CH_DONE`, `CH_ERROR`, `CH_MORE`, `CM_NOPRINT`, `C_CONTINUE`, `C_FULL`, `comgetflags`, `compeek`, `comsetflags`
