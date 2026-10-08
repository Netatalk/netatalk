---
type: C Source File
title: "libatalk/unicode/charsets/mac_hebrew.c"
description: "4 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_hebrew.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/byteorder.h](../../../include/atalk/byteorder.h.md)
* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../../include/atalk/unicode.h.md)
* [mac_hebrew.h](mac_hebrew.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `stdlib.h`, `string.h`

# Function tables

### charset_mac_hebrew

Initialized at line 86 as `struct charset_functions`. Assigns:

* `pull`: [mac_hebrew_pull](mac_hebrew.c.md#mac_hebrew_pull)
* `push`: [mac_hebrew_push](mac_hebrew.c.md#mac_hebrew_push)

# Functions

### char_ucs2_to_mac_hebrew

```c
static int char_ucs2_to_mac_hebrew(unsigned char *r, ucs2_t wc)
```

Defined at lines 60 to 85.

from unicode to mac hebrew code page

Called by: [mac_hebrew_push](mac_hebrew.c.md#mac_hebrew_push)

Uses file-scope variables: `mac_hebrew_page00` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md), `mac_hebrew_page05` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md), `mac_hebrew_page20` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md), `mac_hebrew_pagefb` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md)

### mac_hebrew_push

```c
static size_t mac_hebrew_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 87 to 139.

Calls: [char_ucs2_to_mac_hebrew](mac_hebrew.c.md#char_ucs2_to_mac_hebrew)

Dispatched via: [charset_mac_hebrew](mac_hebrew.c.md#charset_mac_hebrew)

### char_mac_hebrew_to_ucs2

```c
static int char_mac_hebrew_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 143 to 160.

Called by: [mac_hebrew_pull](mac_hebrew.c.md#mac_hebrew_pull)

Uses file-scope variables: `mac_hebrew_2uni` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md)

### mac_hebrew_pull

```c
static size_t mac_hebrew_pull(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 162 to 228.

Calls: [char_mac_hebrew_to_ucs2](mac_hebrew.c.md#char_mac_hebrew_to_ucs2)

Dispatched via: [charset_mac_hebrew](mac_hebrew.c.md#charset_mac_hebrew)
