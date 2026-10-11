---
type: C Source File
title: "bin/nbp/nbpunrgstr.c"
description: "2 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nbp/nbpunrgstr.c"
tags: ["bin/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 46 to 59.

Print the usage of nbpunrgstr to stderr and exit with 1.

Parameters:
* `av0`: program path, whose base name the usage line shows

Called by: [main](nbpunrgstr.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 71 to 132.

Remove an NBP name from atalkd.

The -A option names the node that holds the name, and -m the Mac charset of the name. The name needs an object and a type.

Parameters:
* `ac`: argument count
* `av`: argument vector

Returns: 0, while errors exit with 1

Calls: [Usage](nbpunrgstr.c.md#usage), [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [nbp_unrgstr](../../libatalk/nbp/nbp_unrgstr.c.md#nbp_unrgstr), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name)

# Macros

* Undocumented: `MACCHARSET`
