---
type: C Source File
title: "bin/nbp/nbpunrgstr.c"
description: "2 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nbp/nbpunrgstr.c"
tags: ["bin/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nbp](../nbp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `stdio.h`, `stdlib.h`, `string.h`, `sys/types.h`, `unistd.h`

# Functions

### Usage

```c
static void Usage(char *av0)
```

Defined at lines 41 to 54.

Called by: [main](nbpunrgstr.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 56 to 117.

Calls: [Usage](nbpunrgstr.c.md#usage), [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [nbp_unrgstr](../../libatalk/nbp/nbp_unrgstr.c.md#nbp_unrgstr), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name)

# Macros

* Undocumented: `MACCHARSET`
