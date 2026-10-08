---
type: C Source File
title: "bin/misc/netacnv.c"
description: "2 functions, 1 type, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/misc/netacnv.c"
tags: ["bin/misc"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/misc](../misc.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* System headers: `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`

# Functions

### usage

```c
static void usage(void)
```

Defined at lines 37 to 46.

Called by: [main](netacnv.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 48 to 154.

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name), [usage](netacnv.c.md#usage)

Uses file-scope variables: `buffer`

# Types

### struct flag_map

Defined at line 17.
* `int flag`
* `const char * flagname`

# Macros

* Undocumented: `MACCHARSET`, `flag`

# File-scope variables

`buffer`, `flag_map`
