---
type: C Source File
title: "libatalk/unicode/utf16_case.c"
description: "UTF-16 case mapping tables generated from the Unicode data."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/utf16_case.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 28 to 87. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Convert a UCS2 character to upper case.

A character without a mapping comes back unchanged.

Parameters:
* `val`: UCS2 character

Returns: the upper case character

Called by: [strupper_w](util_unistr.c.md#strupper_w)

Uses file-scope variables: `upper_table_1` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_10` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_11` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_12` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_13` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_14` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_2` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_3` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_4` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_5` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_6` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_7` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_8` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_9` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md)

### toupper_sp

```c
uint32_t toupper_sp(uint32_t)
```

Defined at lines 98 to 141. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Convert a surrogate pair to upper case.

The pair sits in one 32-bit value, with the high surrogate in bits 31 to

1. A pair without a mapping comes back unchanged.

Parameters:
* `val`: surrogate pair

Returns: the upper case surrogate pair

Called by: [strupper_w](util_unistr.c.md#strupper_w)

Uses file-scope variables: `upper_table_sp_1` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_10` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_2` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_3` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_4` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_5` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_6` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_7` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_8` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `upper_table_sp_9` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md)

### tolower_w

```c
ucs2_t tolower_w(ucs2_t)
```

Defined at lines 151 to 206. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Convert a UCS2 character to lower case.

A character without a mapping comes back unchanged.

Parameters:
* `val`: UCS2 character

Returns: the lower case character

Called by: [strcasechr_w](util_unistr.c.md#strcasechr_w), [strcasecmp_w](util_unistr.c.md#strcasecmp_w), [strlower_w](util_unistr.c.md#strlower_w), [strncasecmp_w](util_unistr.c.md#strncasecmp_w)

Uses file-scope variables: `lower_table_1` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_10` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_11` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_12` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_13` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_2` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_3` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_4` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_5` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_6` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_7` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_8` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_9` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md)

### tolower_sp

```c
uint32_t tolower_sp(uint32_t)
```

Defined at lines 217 to 260. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Convert a surrogate pair to lower case.

The pair sits in one 32-bit value, with the high surrogate in bits 31 to

1. A pair without a mapping comes back unchanged.

Parameters:
* `val`: surrogate pair

Returns: the lower case surrogate pair

Called by: [strcasechr_sp](util_unistr.c.md#strcasechr_sp), [strcasecmp_w](util_unistr.c.md#strcasecmp_w), [strlower_w](util_unistr.c.md#strlower_w), [strncasecmp_w](util_unistr.c.md#strncasecmp_w)

Uses file-scope variables: `lower_table_sp_1` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_10` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_2` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_3` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_4` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_5` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_6` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_7` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_8` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md), `lower_table_sp_9` in [libatalk/unicode/utf16_casetable.h](utf16_casetable.h.md)
