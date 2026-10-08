---
type: C Source File
title: "libatalk/unicode/charsets/mac_greek.c"
description: "4 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_greek.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/unicode.h](../../../include/atalk/unicode.h.md)
* [generic_mb.h](generic_mb.h.md)
* [mac_greek.h](mac_greek.h.md)
* System headers: `arpa/inet.h`, `stdlib.h`

# Function tables

### charset_mac_greek

Initialized at line 89 as `struct charset_functions`. Assigns:

* `pull`: [mac_greek_pull](mac_greek.c.md#mac_greek_pull)
* `push`: [mac_greek_push](mac_greek.c.md#mac_greek_push)

# Functions

### char_ucs2_to_mac_greek

```c
static int char_ucs2_to_mac_greek(unsigned char *r, ucs2_t wc)
```

Defined at lines 56 to 83.

Called by: [mac_greek_push](mac_greek.c.md#mac_greek_push)

Uses file-scope variables: `mac_greek_page00` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md), `mac_greek_page03` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md), `mac_greek_page20` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md), `mac_greek_page22` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md)

### mac_greek_push

```c
static size_t mac_greek_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 85 to 91.

Calls: [char_ucs2_to_mac_greek](mac_greek.c.md#char_ucs2_to_mac_greek), [mb_generic_push](generic_mb.c.md#mb_generic_push)

Dispatched via: [charset_mac_greek](mac_greek.c.md#charset_mac_greek)

### char_mac_greek_to_ucs2

```c
static int char_mac_greek_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 95 to 112.

Called by: [mac_greek_pull](mac_greek.c.md#mac_greek_pull)

Uses file-scope variables: `mac_greek_2uni` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md)

### mac_greek_pull

```c
static size_t mac_greek_pull(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 114 to 119.

Calls: [char_mac_greek_to_ucs2](mac_greek.c.md#char_mac_greek_to_ucs2), [mb_generic_pull](generic_mb.c.md#mb_generic_pull)

Dispatched via: [charset_mac_greek](mac_greek.c.md#charset_mac_greek)
