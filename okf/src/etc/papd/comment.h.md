---
type: C Header File
title: "etc/papd/comment.h"
description: "PostScript comment parser callbacks and parser stack interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/comment.h"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
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

Declared at etc/papd/comment.h line 70; no definition in the scanned sources.

# Types

### struct comstate

Defined at line 30.
* `struct papd_comment * cs_comment`
* `struct comstate * cs_prev`
* `int cs_flags`

### struct papd_comment

Defined at line 19.
* `char * c_begin`
* `char * c_end`
* `int(* c_handler`: Called through by [ps](magics.c.md#ps).
* `int c_flags`

# Macros

* `CM_NOPRINT`: or print
* `C_FULL`: or prefix
* Undocumented: `CH_DONE`, `CH_ERROR`, `CH_MORE`, `C_CONTINUE`, `comgetflags`, `compeek`, `comsetflags`
