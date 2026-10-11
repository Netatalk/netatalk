---
type: C Source File
title: "bin/nbp/nbplkup_output.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nbp/nbplkup_output.c"
tags: ["bin/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [bin/nbp](../nbp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [nbplkup_output.h](nbplkup_output.h.md)
* System headers: `errno.h`, `stdint.h`, `stdlib.h`, `string.h`

# Functions

### nbplkup_convert_field

```c
size_t nbplkup_convert_field(charset_t from, const char *src, size_t src_len, char **dest)
```

Defined at lines 43 to 74.

Convert an NBP name field from the Mac charset to the host charset for printing.

A field that does not convert keeps its raw bytes in a new string.

Parameters:
* `from`: Mac charset of the field
* `src`: field bytes, not NUL-terminated
* `src_len`: number of bytes
* `dest`: receives the new string, which the caller frees

Returns: the length of the new string, or `(size_t)-1` with errno for an error

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [main](nbplkup.c.md#main)
