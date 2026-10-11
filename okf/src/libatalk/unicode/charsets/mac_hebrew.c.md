---
type: C Source File
title: "libatalk/unicode/charsets/mac_hebrew.c"
description: "4 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_hebrew.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 64 to 89.

Map one UCS2 character to a MAC_HEBREW byte.

Parameters:
* `r`: receives the byte
* `wc`: UCS2 character

Returns: 1, or 0 when the charset has no byte for `wc`

Called by: [mac_hebrew_push](mac_hebrew.c.md#mac_hebrew_push)

Uses file-scope variables: `mac_hebrew_page00` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md), `mac_hebrew_page05` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md), `mac_hebrew_page20` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md), `mac_hebrew_pagefb` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md)

### mac_hebrew_push

```c
static size_t mac_hebrew_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 108 to 160.

Convert UCS2 to MAC_HEBREW, with the three Apple sequences that map to one byte.

U+05B8 gives 0xcb, or 0xde with a following U+F87F. U+05F2 U+05B7 gives 0x81, and U+F86A U+05DC U+05B9 gives 0xc0. Other characters go through the table. The call leaves `outbuf` at its start, as mb_generic_push does.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position, which the call writes at but does not advance
* `outbytesleft`: output space

Returns: the number of bytes written, or `(size_t)-1` with EILSEQ or E2BIG

Calls: [char_ucs2_to_mac_hebrew](mac_hebrew.c.md#char_ucs2_to_mac_hebrew)

Dispatched via: [charset_mac_hebrew](mac_hebrew.c.md#charset_mac_hebrew)

### char_mac_hebrew_to_ucs2

```c
static int char_mac_hebrew_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 173 to 190.

Map one MAC_HEBREW byte to UCS2 or to a sequence marker.

The table values 1, 2, and 3 mark the bytes that stand for a sequence of UCS2 characters. The value 0xfffd marks a byte without a character.

Parameters:
* `pwc`: receives the UCS2 character or the marker
* `s`: byte to map

Returns: 1, or 0 for a byte without a character

Called by: [mac_hebrew_pull](mac_hebrew.c.md#mac_hebrew_pull)

Uses file-scope variables: `mac_hebrew_2uni` in [libatalk/unicode/charsets/mac_hebrew.h](mac_hebrew.h.md)

### mac_hebrew_pull

```c
static size_t mac_hebrew_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 207 to 273.

Convert MAC_HEBREW to UCS2 and expand the three bytes that stand for sequences.

The byte 0x81 becomes U+05F2 U+05B7, 0xc0 becomes U+F86A U+05DC U+05B9, and 0xde becomes U+05B8 U+F87F. Other bytes map to one character each.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: the number of characters written, or `(size_t)-1` with EILSEQ or E2BIG

Calls: [char_mac_hebrew_to_ucs2](mac_hebrew.c.md#char_mac_hebrew_to_ucs2)

Dispatched via: [charset_mac_hebrew](mac_hebrew.c.md#charset_mac_hebrew)
