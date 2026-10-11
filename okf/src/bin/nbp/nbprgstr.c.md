---
type: C Source File
title: "bin/nbp/nbprgstr.c"
description: "2 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nbp/nbprgstr.c"
tags: ["bin/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 29 to 42.

Print the usage of nbprgstr to stderr and exit with 1.

Parameters:
* `av0`: program path, whose base name the usage line shows

Called by: [main](nbprgstr.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 57 to 135.

Register an NBP name for a DDP socket with the local atalkd.

The tool opens a DDP socket to learn the node address, from the -A address when given. The name registers for that socket, or for the -p port, and the socket closes before the tool exits. The -m option names the Mac charset of the name, which needs an object and a type.

Parameters:
* `ac`: argument count
* `av`: argument vector

Returns: 0, or -1 for a socket error, while a failed registration exits with 1

Calls: [Usage](nbprgstr.c.md#usage), [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [nbp_rgstr](../../libatalk/nbp/nbp_rgstr.c.md#nbp_rgstr), [netddp_close](../../include/atalk/netddp.h.md#netddp_close), [netddp_open](../../libatalk/netddp/netddp_open.c.md#netddp_open), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name)

# Macros

* Undocumented: `MACCHARSET`
