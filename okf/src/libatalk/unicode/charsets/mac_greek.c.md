---
type: C Source File
title: "libatalk/unicode/charsets/mac_greek.c"
description: "4 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_greek.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 62 to 89.

Map one UCS2 character to a MAC_GREEK byte.

Parameters:
* `r`: receives the byte
* `wc`: UCS2 character

Returns: 1, or 0 when the charset has no byte for `wc`

Called by: [mac_greek_push](mac_greek.c.md#mac_greek_push)

Uses file-scope variables: `mac_greek_page00` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md), `mac_greek_page03` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md), `mac_greek_page20` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md), `mac_greek_page22` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md)

### mac_greek_push

```c
static size_t mac_greek_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 102 to 108.

Convert UCS2 to MAC_GREEK through mb_generic_push.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position, which mb_generic_push does not advance
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_ucs2_to_mac_greek](mac_greek.c.md#char_ucs2_to_mac_greek), [mb_generic_push](generic_mb.c.md#mb_generic_push)

Dispatched via: [charset_mac_greek](mac_greek.c.md#charset_mac_greek)

### char_mac_greek_to_ucs2

```c
static int char_mac_greek_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 120 to 137.

Map one MAC_GREEK byte to UCS2.

A byte whose table entry is 0xfffd has no character.

Parameters:
* `pwc`: receives the UCS2 character
* `s`: byte to map

Returns: 1, or 0 for a byte without a character

Called by: [mac_greek_pull](mac_greek.c.md#mac_greek_pull)

Uses file-scope variables: `mac_greek_2uni` in [libatalk/unicode/charsets/mac_greek.h](mac_greek.h.md)

### mac_greek_pull

```c
static size_t mac_greek_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 149 to 154.

Convert MAC_GREEK to UCS2 through mb_generic_pull.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_mac_greek_to_ucs2](mac_greek.c.md#char_mac_greek_to_ucs2), [mb_generic_pull](generic_mb.c.md#mb_generic_pull)

Dispatched via: [charset_mac_greek](mac_greek.c.md#charset_mac_greek)
