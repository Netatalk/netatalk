---
type: C Header File
title: "libatalk/unicode/charsets/generic_cjk.h"
description: "CJK mapping table types and character conversion helpers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/generic_cjk.h"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/byteorder.h](../../../include/atalk/byteorder.h.md)
* [atalk/unicode.h](../../../include/atalk/unicode.h.md)
* System headers: `iconv.h`

# Included by

* [libatalk/unicode/charsets/generic_cjk.c](generic_cjk.c.md)
* [libatalk/unicode/charsets/mac_chinese_simp.c](mac_chinese_simp.c.md)
* [libatalk/unicode/charsets/mac_chinese_trad.c](mac_chinese_trad.c.md)
* [libatalk/unicode/charsets/mac_japanese.c](mac_japanese.c.md)
* [libatalk/unicode/charsets/mac_korean.c](mac_korean.c.md)

# Types

### struct cjk_index_t

Defined at line 32.
* `uint16_t range`
* `const uint16_t(* summary`

# Macros

* Undocumented: `CJK_PULL_BUFFER`, `CJK_PUSH_BUFFER`
