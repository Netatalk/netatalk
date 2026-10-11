---
type: C Source File
title: "libatalk/unicode/charsets/mac_roman.c"
description: "4 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_roman.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 63 to 98.

Map one UCS2 character to a MAC_ROMAN byte.

The Apple logo U+F8FF maps to 0xf0.

Parameters:
* `r`: receives the byte
* `wc`: UCS2 character

Returns: 1, or 0 when the charset has no byte for `wc`

Called by: [mac_roman_push](mac_roman.c.md#mac_roman_push)

Uses file-scope variables: `mac_roman_page00` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page01` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page02` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page20` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page21` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_page22` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md), `mac_roman_pagefb` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md)

### mac_roman_push

```c
static size_t mac_roman_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 111 to 117.

Convert UCS2 to MAC_ROMAN through mb_generic_push.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position, which mb_generic_push does not advance
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_ucs2_to_mac_roman](mac_roman.c.md#char_ucs2_to_mac_roman), [mb_generic_push](generic_mb.c.md#mb_generic_push)

Dispatched via: [charset_mac_roman](mac_roman.c.md#charset_mac_roman)

### char_mac_roman_to_ucs2

```c
static int char_mac_roman_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 127 to 139.

Map one MAC_ROMAN byte to UCS2, where every byte has a character.

Parameters:
* `pwc`: receives the UCS2 character
* `s`: byte to map

Returns: 1

Called by: [mac_roman_pull](mac_roman.c.md#mac_roman_pull)

Uses file-scope variables: `mac_roman_2uni` in [libatalk/unicode/charsets/mac_roman.h](mac_roman.h.md)

### mac_roman_pull

```c
static size_t mac_roman_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 151 to 156.

Convert MAC_ROMAN to UCS2 through mb_generic_pull.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_mac_roman_to_ucs2](mac_roman.c.md#char_mac_roman_to_ucs2), [mb_generic_pull](generic_mb.c.md#mb_generic_pull)

Dispatched via: [charset_mac_roman](mac_roman.c.md#charset_mac_roman)
