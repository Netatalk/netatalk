---
type: C Source File
title: "libatalk/unicode/charsets/mac_korean.c"
description: "4 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_korean.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [generic_cjk.h](generic_cjk.h.md)
* [mac_korean.h](mac_korean.h.md)
* System headers: `stdlib.h`

# Function tables

### charset_mac_korean

Initialized at line 96 as `struct charset_functions`. Assigns:

* `pull`: [mac_korean_pull](mac_korean.c.md#mac_korean_pull)
* `push`: [mac_korean_push](mac_korean.c.md#mac_korean_push)

# Functions

### mac_korean_char_push

```c
static size_t mac_korean_char_push(uint8_t *out, const ucs2_t *in, size_t *size)
```

Defined at lines 51 to 100.

Calls: [cjk_char_push](generic_cjk.c.md#cjk_char_push), [cjk_compose](generic_cjk.c.md#cjk_compose), [cjk_compose_seq](generic_cjk.c.md#cjk_compose_seq), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_korean_push](mac_korean.c.md#mac_korean_push)

Uses file-scope variables: `mac_korean_compose` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md), `mac_korean_uni2_charset` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md), `mac_korean_uni2_index` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md)

### mac_korean_push

```c
static size_t mac_korean_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 102 to 107.

Calls: [cjk_generic_push](generic_cjk.c.md#cjk_generic_push), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)

Dispatched via: [charset_mac_korean](mac_korean.c.md#charset_mac_korean)

### mac_korean_char_pull

```c
static size_t mac_korean_char_pull(ucs2_t *out, const uint8_t *in, size_t *size)
```

Defined at lines 109 to 139.

Calls: [cjk_char_pull](generic_cjk.c.md#cjk_char_pull), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_korean_pull](mac_korean.c.md#mac_korean_pull)

Uses file-scope variables: `mac_korean_2uni_charset` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md), `mac_korean_2uni_index` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md), `mac_korean_compose` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md)

### mac_korean_pull

```c
static size_t mac_korean_pull(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 141 to 146.

Calls: [cjk_generic_pull](generic_cjk.c.md#cjk_generic_pull), [mac_korean_char_pull](mac_korean.c.md#mac_korean_char_pull)

Dispatched via: [charset_mac_korean](mac_korean.c.md#charset_mac_korean)
