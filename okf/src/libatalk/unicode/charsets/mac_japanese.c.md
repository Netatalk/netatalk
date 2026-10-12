---
type: C Source File
title: "libatalk/unicode/charsets/mac_japanese.c"
description: "MAC_JAPANESE charset conversion."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_japanese.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [generic_cjk.h](generic_cjk.h.md)
* [mac_japanese.h](mac_japanese.h.md)
* System headers: `stdlib.h`

# Function tables

### charset_mac_japanese

Initialized at line 94 as `struct charset_functions`. Assigns:

* `pull`: [mac_japanese_pull](mac_japanese.c.md#mac_japanese_pull)
* `push`: [mac_japanese_push](mac_japanese.c.md#mac_japanese_push)

# Functions

### mac_japanese_char_push

```c
static size_t mac_japanese_char_push(uint8_t *out, const ucs2_t *in, size_t *size)
```

Defined at lines 67 to 115.

Map one UCS2 character, or an Apple sequence, to MAC_JAPANESE bytes.

ASCII passes as is, except that a backslash becomes 0x80. Private-use characters up to U+E98B map to the user-defined lead bytes from 0xf0. An Apple sequence that starts with U+F860 to U+F867 composes first. So does a pair with a hint from U+F870 to U+F87F or with U+20DD. Characters without a table entry return 0 for the SHIFT_JIS fallback.

Parameters:
* `out`: receives one or two bytes
* `in`: UCS2 input
* `size`: characters available, which receives the characters used

Returns: the number of bytes, 0 for the iconv fallback, or `(size_t)-1` with errno

Calls: [cjk_char_push](generic_cjk.c.md#cjk_char_push), [cjk_compose](generic_cjk.c.md#cjk_compose), [cjk_compose_seq](generic_cjk.c.md#cjk_compose_seq), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_japanese_push](mac_japanese.c.md#mac_japanese_push)

Uses file-scope variables: `mac_japanese_compose` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md), `mac_japanese_uni2_charset` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md), `mac_japanese_uni2_index` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md)

### mac_japanese_push

```c
static size_t mac_japanese_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 127 to 132.

Convert UCS2 to MAC_JAPANESE through cjk_generic_push.

Parameters:
* `cd`: SHIFT_JIS iconv descriptor for the fallback
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with errno

Calls: [cjk_generic_push](generic_cjk.c.md#cjk_generic_push), [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push)

Dispatched via: [charset_mac_japanese](mac_japanese.c.md#charset_mac_japanese)

### mac_japanese_char_pull

```c
static size_t mac_japanese_char_pull(ucs2_t *out, const uint8_t *in, size_t *size)
```

Defined at lines 148 to 185.

Map one MAC_JAPANESE character of one or two bytes to UCS2.

ASCII passes as is, except that 0x5c becomes U+00A5. A lead byte in 0x81 to 0x9f or 0xe0 to 0xfc needs a trail byte in 0x40 to 0x7e or 0x80 to 0xfc. Lead bytes from 0xf0 map to the private-use range from U+E000. Other values expand through the table and the composition table.

Parameters:
* `out`: receives the UCS2 characters
* `in`: input bytes
* `size`: bytes available, which receives the bytes used

Returns: the number of characters, 0 for the iconv fallback, or `(size_t)-1` with EILSEQ or EINVAL

Calls: [cjk_char_pull](generic_cjk.c.md#cjk_char_pull), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_japanese_pull](mac_japanese.c.md#mac_japanese_pull)

Uses file-scope variables: `mac_japanese_2uni_charset` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md), `mac_japanese_2uni_index` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md), `mac_japanese_compose` in [libatalk/unicode/charsets/mac_japanese.h](mac_japanese.h.md)

### mac_japanese_pull

```c
static size_t mac_japanese_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 197 to 202.

Convert MAC_JAPANESE to UCS2 through cjk_generic_pull.

Parameters:
* `cd`: SHIFT_JIS iconv descriptor for the fallback
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with errno

Calls: [cjk_generic_pull](generic_cjk.c.md#cjk_generic_pull), [mac_japanese_char_pull](mac_japanese.c.md#mac_japanese_char_pull)

Dispatched via: [charset_mac_japanese](mac_japanese.c.md#charset_mac_japanese)
