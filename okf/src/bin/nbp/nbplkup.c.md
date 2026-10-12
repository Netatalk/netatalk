---
type: C Source File
title: "bin/nbp/nbplkup.c"
description: "The nbplkup command: look up NBP names."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nbp/nbplkup.c"
tags: ["bin/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [bin/nbp](../nbp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [nbplkup_output.h](nbplkup_output.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `limits.h`, `stdbool.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/types.h`

# Functions

### Usage

```c
static void Usage(char *av0)
```

Defined at lines 60 to 73.

Print the usage of nbplkup and exit with 1.

Parameters:
* `av0`: program path, whose base name the usage line shows

Called by: [main](nbplkup.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 91 to 298.

Look up NBP names and print each match with its address.

The pattern `obj:type@zone` defaults to `=:=@*`. The NBPLKUP environment variable and then the argument override its parts. The default operation is NBPOP_BRRQ, -f picks NBPOP_FWD, and -l picks NBPOP_LKUP.

The -A option sets the local address, -D the node to ask, -r the number of replies, 1000 by default, and -m the Mac charset. The -s option prints one line per match for scripts.

Parameters:
* `ac`: argument count
* `av`: argument vector

Returns: 0, while errors exit with 1 and a failed lookup exits with status 255

Calls: [Usage](nbplkup.c.md#usage), [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [nbp_do_lookup_op](../../libatalk/nbp/nbp_lkup.c.md#nbp_do_lookup_op), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [nbplkup_convert_field](nbplkup_output.c.md#nbplkup_convert_field), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name)

Uses file-scope variables: `Obj`, `Type`, `Zone`

# Macros

* Undocumented: `MACCHARSET`

# File-scope variables

`Obj`, `Type`, `Zone`
