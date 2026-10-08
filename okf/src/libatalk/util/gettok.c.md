---
type: C Source File
title: "libatalk/util/gettok.c"
description: "2 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/gettok.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/globals.h](../../include/atalk/globals.h.md)
* System headers: `ctype.h`, `pwd.h`, `string.h`, `sys/param.h`

# Functions

### initline

```c
void initline(int len, char *line)
```

Defined at lines 20 to 24.

Called by: [readextmap](netatalk_conf.c.md#readextmap)

Uses file-scope variables: `l_curr`, `l_end`

### parseline

```c
int parseline(int len, char *token)
```

Defined at lines 31 to 86.

Called by: [readextmap](netatalk_conf.c.md#readextmap)

Uses file-scope variables: `l_curr`, `l_end`

# Macros

* Undocumented: `ST_BEGIN`, `ST_QUOTE`, `ST_WORD`

# File-scope variables

`l_curr`, `l_end`
