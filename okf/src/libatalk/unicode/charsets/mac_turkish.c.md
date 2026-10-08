---
type: C Source File
title: "libatalk/unicode/charsets/mac_turkish.c"
description: "2 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_turkish.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/unicode.h](../../../include/atalk/unicode.h.md)
* [generic_mb.h](generic_mb.h.md)
* [mac_turkish.h](mac_turkish.h.md)
* System headers: `arpa/inet.h`, `stdlib.h`

# Function tables

### charset_mac_turkish

Initialized at line 90 as `struct charset_functions`. Assigns:

* `pull`: [mac_turkish_pull](mac_turkish.c.md#mac_turkish_pull)
* `push`: [mac_turkish_push](mac_turkish.c.md#mac_turkish_push)

# Functions

### mac_turkish_push

```c
static size_t mac_turkish_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 52 to 57.

Calls: [char_ucs2_to_mac_turkish](mac_turkish.h.md#char_ucs2_to_mac_turkish), [mb_generic_push](generic_mb.c.md#mb_generic_push)

Dispatched via: [charset_mac_turkish](mac_turkish.c.md#charset_mac_turkish)

### mac_turkish_pull

```c
static size_t mac_turkish_pull(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 61 to 66.

Calls: [char_mac_turkish_to_ucs2](mac_turkish.h.md#char_mac_turkish_to_ucs2), [mb_generic_pull](generic_mb.c.md#mb_generic_pull)

Dispatched via: [charset_mac_turkish](mac_turkish.c.md#charset_mac_turkish)
