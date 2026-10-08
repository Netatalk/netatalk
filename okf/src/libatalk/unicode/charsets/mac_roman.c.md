---
type: C Source File
title: "libatalk/unicode/charsets/mac_roman.c"
description: "4 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_roman.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/unicode.h](../../../include/atalk/unicode.h.md)
* [generic_mb.h](generic_mb.h.md)
* [mac_roman.h](mac_roman.h.md)
* System headers: `arpa/inet.h`, `stdlib.h`

# Function tables

### charset_mac_roman

Initialized at line 85 as `struct charset_functions`. Assigns:

* `pull`: [mac_roman_pull](mac_roman.c.md#mac_roman_pull)
* `push`: [mac_roman_push](mac_roman.c.md#mac_roman_push)

# Functions

### char_ucs2_to_mac_roman

```c
static int char_ucs2_to_mac_roman(unsigned char *r, ucs2_t wc)
```

Defined at lines 55 to 90.

Called by: [mac_roman_push](mac_roman.c.md#mac_roman_push)

Uses file-scope variables: `mac_roman_page00` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page01` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page02` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page20` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page21` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page22` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_pagefb` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md)

### mac_roman_push

```c
static size_t mac_roman_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 92 to 98.

Calls: [char_ucs2_to_mac_roman](mac_roman.c.md#char_ucs2_to_mac_roman), [mb_generic_push](generic_mb.c.md#mb_generic_push)

Dispatched via: [charset_mac_roman](mac_roman.c.md#charset_mac_roman)

### char_mac_roman_to_ucs2

```c
static int char_mac_roman_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 102 to 114.

Called by: [mac_roman_pull](mac_roman.c.md#mac_roman_pull)

Uses file-scope variables: `mac_roman_2uni` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md)

### mac_roman_pull

```c
static size_t mac_roman_pull(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 116 to 121.

Calls: [char_mac_roman_to_ucs2](mac_roman.c.md#char_mac_roman_to_ucs2), [mb_generic_pull](generic_mb.c.md#mb_generic_pull)

Dispatched via: [charset_mac_roman](mac_roman.c.md#charset_mac_roman)
