---
type: C Source File
title: "libatalk/unicode/charsets/mac_chinese_trad.c"
description: "4 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_chinese_trad.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [generic_cjk.h](generic_cjk.h.md)
* [mac_chinese_trad.h](mac_chinese_trad.h.md)
* System headers: `stdlib.h`

# Function tables

### charset_mac_chinese_trad

Initialized at line 95 as `struct charset_functions`. Assigns:

* `pull`: [mac_chinese_trad_pull](mac_chinese_trad.c.md#mac_chinese_trad_pull)
* `push`: [mac_chinese_trad_push](mac_chinese_trad.c.md#mac_chinese_trad_push)

# Functions

### mac_chinese_trad_char_push

```c
static size_t mac_chinese_trad_char_push(uint8_t *out, const ucs2_t *in, size_t *size)
```

Defined at lines 52 to 86.

Calls: [cjk_char_push](generic_cjk.c.md#cjk_char_push), [cjk_compose](generic_cjk.c.md#cjk_compose), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_chinese_trad_push](mac_chinese_trad.c.md#mac_chinese_trad_push)

Uses file-scope variables: `mac_chinese_trad_compose` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md), `mac_chinese_trad_uni2_charset` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md), `mac_chinese_trad_uni2_index` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md)

### mac_chinese_trad_push

```c
static size_t mac_chinese_trad_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 88 to 93.

Calls: [cjk_generic_push](generic_cjk.c.md#cjk_generic_push), [mac_chinese_trad_char_push](mac_chinese_trad.c.md#mac_chinese_trad_char_push)

Dispatched via: [charset_mac_chinese_trad](mac_chinese_trad.c.md#charset_mac_chinese_trad)

### mac_chinese_trad_char_pull

```c
static size_t mac_chinese_trad_char_pull(ucs2_t *out, const uint8_t *in, size_t *size)
```

Defined at lines 95 to 126.

Calls: [cjk_char_pull](generic_cjk.c.md#cjk_char_pull), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_chinese_trad_pull](mac_chinese_trad.c.md#mac_chinese_trad_pull)

Uses file-scope variables: `mac_chinese_trad_2uni_charset` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md), `mac_chinese_trad_2uni_index` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md), `mac_chinese_trad_compose` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md)

### mac_chinese_trad_pull

```c
static size_t mac_chinese_trad_pull(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 128 to 133.

Calls: [cjk_generic_pull](generic_cjk.c.md#cjk_generic_pull), [mac_chinese_trad_char_pull](mac_chinese_trad.c.md#mac_chinese_trad_char_pull)

Dispatched via: [charset_mac_chinese_trad](mac_chinese_trad.c.md#charset_mac_chinese_trad)
