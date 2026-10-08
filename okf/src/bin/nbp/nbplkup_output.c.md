---
type: C Source File
title: "bin/nbp/nbplkup_output.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nbp/nbplkup_output.c"
tags: ["bin/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 30 to 61.

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [main](nbplkup.c.md#main)
