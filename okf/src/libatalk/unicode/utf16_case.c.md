---
type: C Source File
title: "libatalk/unicode/utf16_case.c"
description: "4 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/utf16_case.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../unicode.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [utf16_casetable.h](utf16_casetable.h.md)
* System headers: `stdint.h`

# Functions

### toupper_w

```c
ucs2_t toupper_w(ucs2_t)
```

Defined at lines 18 to 77. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Called by: [strupper_w](util_unistr.c.md#strupper_w)

Uses file-scope variables: `upper_table_1` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_10` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_11` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_12` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_13` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_14` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_2` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_3` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_4` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_5` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_6` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_7` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_8` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_9` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md)

### toupper_sp

```c
uint32_t toupper_sp(uint32_t)
```

Defined at lines 82 to 125. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Called by: [strupper_w](util_unistr.c.md#strupper_w)

Uses file-scope variables: `upper_table_sp_1` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_10` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_2` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_3` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_4` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_5` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_6` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_7` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_8` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_9` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md)

### tolower_w

```c
ucs2_t tolower_w(ucs2_t)
```

Defined at lines 130 to 185. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Called by: [strcasechr_w](util_unistr.c.md#strcasechr_w), [strcasecmp_w](util_unistr.c.md#strcasecmp_w), [strlower_w](util_unistr.c.md#strlower_w), [strncasecmp_w](util_unistr.c.md#strncasecmp_w)

Uses file-scope variables: `lower_table_1` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_10` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_11` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_12` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_13` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_2` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_3` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_4` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_5` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_6` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_7` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_8` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_9` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md)

### tolower_sp

```c
uint32_t tolower_sp(uint32_t)
```

Defined at lines 190 to 233. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Called by: [strcasechr_sp](util_unistr.c.md#strcasechr_sp), [strcasecmp_w](util_unistr.c.md#strcasecmp_w), [strlower_w](util_unistr.c.md#strlower_w), [strncasecmp_w](util_unistr.c.md#strncasecmp_w)

Uses file-scope variables: `lower_table_sp_1` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_10` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_2` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_3` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_4` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_5` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_6` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_7` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_8` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_9` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md)
