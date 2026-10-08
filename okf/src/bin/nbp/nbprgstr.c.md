---
type: C Source File
title: "bin/nbp/nbprgstr.c"
description: "2 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nbp/nbprgstr.c"
tags: ["bin/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nbp](../nbp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/types.h`

# Functions

### Usage

```c
static void Usage(char *av0)
```

Defined at lines 24 to 37.

Called by: [main](nbprgstr.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 39 to 117.

Calls: [Usage](nbprgstr.c.md#usage), [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [nbp_rgstr](../../libatalk/nbp/nbp_rgstr.c.md#nbp_rgstr), [netddp_close](../../include/atalk/netddp.h.md#netddp_close), [netddp_open](../../libatalk/netddp/netddp_open.c.md#netddp_open), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name)

# Macros

* Undocumented: `MACCHARSET`
