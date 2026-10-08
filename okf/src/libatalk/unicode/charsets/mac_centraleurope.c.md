---
type: C Source File
title: "libatalk/unicode/charsets/mac_centraleurope.c"
description: "2 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_centraleurope.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/unicode.h](../../../include/atalk/unicode.h.md)
* [generic_mb.h](generic_mb.h.md)
* [mac_centraleurope.h](mac_centraleurope.h.md)
* System headers: `arpa/inet.h`, `stdlib.h`

# Function tables

### charset_mac_centraleurope

Initialized at line 87 as `struct charset_functions`. Assigns:

* `pull`: [mac_centraleurope_pull](mac_centraleurope.c.md#mac_centraleurope_pull)
* `push`: [mac_centraleurope_push](mac_centraleurope.c.md#mac_centraleurope_push)

# Functions

### mac_centraleurope_push

```c
static size_t mac_centraleurope_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 57 to 63.

Calls: [char_ucs2_to_mac_centraleurope](mac_centraleurope.h.md#char_ucs2_to_mac_centraleurope), [mb_generic_push](generic_mb.c.md#mb_generic_push)

Dispatched via: [charset_mac_centraleurope](mac_centraleurope.c.md#charset_mac_centraleurope)

### mac_centraleurope_pull

```c
static size_t mac_centraleurope_pull(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 67 to 73.

Calls: [char_mac_centraleurope_to_ucs2](mac_centraleurope.h.md#char_mac_centraleurope_to_ucs2), [mb_generic_pull](generic_mb.c.md#mb_generic_pull)

Dispatched via: [charset_mac_centraleurope](mac_centraleurope.c.md#charset_mac_centraleurope)
