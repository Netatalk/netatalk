---
type: C Source File
title: "libatalk/unicode/charsets/mac_turkish.c"
description: "2 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_turkish.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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
static size_t mac_turkish_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 63 to 68.

Convert UCS2 to MAC_TURKISH through mb_generic_push.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position, which mb_generic_push does not advance
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_ucs2_to_mac_turkish](mac_turkish.h.md#char_ucs2_to_mac_turkish), [mb_generic_push](generic_mb.c.md#mb_generic_push)

Dispatched via: [charset_mac_turkish](mac_turkish.c.md#charset_mac_turkish)

### mac_turkish_pull

```c
static size_t mac_turkish_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 80 to 85.

Convert MAC_TURKISH to UCS2 through mb_generic_pull.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_mac_turkish_to_ucs2](mac_turkish.h.md#char_mac_turkish_to_ucs2), [mb_generic_pull](generic_mb.c.md#mb_generic_pull)

Dispatched via: [charset_mac_turkish](mac_turkish.c.md#charset_mac_turkish)
