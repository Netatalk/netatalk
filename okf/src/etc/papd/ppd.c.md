---
type: C Source File
title: "etc/papd/ppd.c"
description: "6 functions, 1 type, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/ppd.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [ppd.h](ppd.h.md)
* [printer.h](printer.h.md)
* System headers: `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/time.h`, `sys/types.h`

# Functions

### ppd_init

```c
static void ppd_init(void)
```

Defined at lines 55 to 66.

Calls: [read_ppd](ppd.c.md#read_ppd)

Called by: [ppd_feature](ppd.c.md#ppd_feature), [ppd_font](ppd.c.md#ppd_font)

Uses file-scope variables: `ppd_inited`

### my_fgets

```c
static char * my_fgets(char *buf, size_t bufsize, FILE *stream)
```

Defined at lines 72 to 97.

Called by: [getppdent](ppd.c.md#getppdent)

### getppdent

```c
static struct ppdent * getppdent(FILE *stream)
```

Defined at lines 99 to 198.

Calls: [my_fgets](ppd.c.md#my_fgets)

Called by: [read_ppd](ppd.c.md#read_ppd)

### read_ppd

```c
int read_ppd(char *file, int fcnt)
```

Defined at lines 200 to 275.

Calls: [getppdent](ppd.c.md#getppdent)

Called by: [ppd_init](ppd.c.md#ppd_init)

Uses file-scope variables: `ppd_features`, `ppd_fonts`

### ppd_font

```c
struct ppd_font * ppd_font(char *font)
```

Defined at lines 277 to 295.

Calls: [ppd_init](ppd.c.md#ppd_init)

Uses file-scope variables: `ppd_fonts`, `ppd_inited`

### ppd_feature

```c
struct ppd_feature * ppd_feature(const char *feature, int len)
```

Defined at lines 297 to 333.

Calls: [ppd_init](ppd.c.md#ppd_init)

Uses file-scope variables: `ppd_features`, `ppd_inited`

# Types

### struct ppdent

Defined at line 45.
* `char * pe_main`
* `char * pe_option`
* `char * pe_translation`
* `char * pe_value`

# File-scope variables

`ppd_features`, `ppd_fonts`, `ppd_inited`
