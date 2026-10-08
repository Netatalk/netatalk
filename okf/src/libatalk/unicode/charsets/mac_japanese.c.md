---
type: C Source File
title: "libatalk/unicode/charsets/mac_japanese.c"
description: "4 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_japanese.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [generic_cjk.h](generic_cjk.h.md)
* [mac_japanese.h](mac_japanese.h.md)
* System headers: `stdlib.h`

# Function tables

### charset_mac_japanese

Initialized at line 94 as `struct charset_functions`. Assigns:

* `pull`: [mac_japanese_pull](mac_japanese.c.md#mac_japanese_pull)
* `push`: [mac_japanese_push](mac_japanese.c.md#mac_japanese_push)

# Functions

### mac_japanese_char_push

```c
static size_t mac_japanese_char_push(uint8_t *out, const ucs2_t *in, size_t *size)
```

Defined at lines 51 to 99.

Calls: [cjk_char_push](generic_cjk.c.md#cjk_char_push), [cjk_compose](generic_cjk.c.md#cjk_compose), [cjk_compose_seq](generic_cjk.c.md#cjk_compose_seq), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_japanese_push](mac_japanese.c.md#mac_japanese_push)

Uses file-scope variables: `mac_japanese_compose` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md), `mac_japanese_uni2_charset` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md), `mac_japanese_uni2_index` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md)

### mac_japanese_push

```c
static size_t mac_japanese_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 101 to 106.

Calls: [cjk_generic_push](generic_cjk.c.md#cjk_generic_push), [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push)

Dispatched via: [charset_mac_japanese](mac_japanese.c.md#charset_mac_japanese)

### mac_japanese_char_pull

```c
static size_t mac_japanese_char_pull(ucs2_t *out, const uint8_t *in, size_t *size)
```

Defined at lines 108 to 145.

Calls: [cjk_char_pull](generic_cjk.c.md#cjk_char_pull), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_japanese_pull](mac_japanese.c.md#mac_japanese_pull)

Uses file-scope variables: `mac_japanese_2uni_charset` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md), `mac_japanese_2uni_index` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md), `mac_japanese_compose` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md)

### mac_japanese_pull

```c
static size_t mac_japanese_pull(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 147 to 152.

Calls: [cjk_generic_pull](generic_cjk.c.md#cjk_generic_pull), [mac_japanese_char_pull](mac_japanese.c.md#mac_japanese_char_pull)

Dispatched via: [charset_mac_japanese](mac_japanese.c.md#charset_mac_japanese)
