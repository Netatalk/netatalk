---
type: C Source File
title: "libatalk/unicode/charsets/mac_korean.c"
description: "Mac Korean charset conversion."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_korean.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [generic_cjk.h](generic_cjk.h.md)
* [mac_korean.h](mac_korean.h.md)
* System headers: `stdlib.h`

# Function tables

### charset_mac_korean

Initialized at line 96 as `struct charset_functions`. Assigns:

* `pull`: [mac_korean_pull](mac_korean.c.md#mac_korean_pull)
* `push`: [mac_korean_push](mac_korean.c.md#mac_korean_push)

# Functions

### mac_korean_char_push

```c
static size_t mac_korean_char_push(uint8_t *out, const ucs2_t *in, size_t *size)
```

Defined at lines 68 to 117.

Map one UCS2 character, with the combining characters after it, to MAC_KOREAN bytes.

An Apple sequence that starts with U+F860 to U+F867 composes first. Otherwise the call keeps composing while a hint from U+F870 to U+F87F or a combining mark follows and the composition table has the pair. Private-use characters and characters without a table entry return 0 for the EUC-KR fallback.

Parameters:
* `out`: receives one or two bytes
* `in`: UCS2 input
* `size`: characters available, which receives the characters used

Returns: the number of bytes, 0 for the iconv fallback, or `(size_t)-1` with errno

Calls: [cjk_char_push](generic_cjk.c.md#cjk_char_push), [cjk_compose](generic_cjk.c.md#cjk_compose), [cjk_compose_seq](generic_cjk.c.md#cjk_compose_seq), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_korean_push](mac_korean.c.md#mac_korean_push)

Uses file-scope variables: `mac_korean_compose` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md), `mac_korean_uni2_charset` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md), `mac_korean_uni2_index` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md)

### mac_korean_push

```c
static size_t mac_korean_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 129 to 134.

Convert UCS2 to MAC_KOREAN through cjk_generic_push.

Parameters:
* `cd`: EUC-KR iconv descriptor for the fallback
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with errno

Calls: [cjk_generic_push](generic_cjk.c.md#cjk_generic_push), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)

Dispatched via: [charset_mac_korean](mac_korean.c.md#charset_mac_korean)

### mac_korean_char_pull

```c
static size_t mac_korean_char_pull(ucs2_t *out, const uint8_t *in, size_t *size)
```

Defined at lines 150 to 180.

Map one MAC_KOREAN character of one or two bytes to UCS2.

ASCII passes as is, and bytes outside the lead byte range stand alone. A lead byte from 0xa1 to 0xfe needs a trail byte in 0x41 to 0x7d or in 0x81 to 0xfe. The table value expands through the composition table into up to CJK_PULL_BUFFER characters.

Parameters:
* `out`: receives the UCS2 characters
* `in`: input bytes
* `size`: bytes available, which receives the bytes used

Returns: the number of characters, 0 for the iconv fallback, or `(size_t)-1` with EILSEQ or EINVAL

Calls: [cjk_char_pull](generic_cjk.c.md#cjk_char_pull), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_korean_pull](mac_korean.c.md#mac_korean_pull)

Uses file-scope variables: `mac_korean_2uni_charset` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md), `mac_korean_2uni_index` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md), `mac_korean_compose` in [libatalk/unicode/charsets/mac_korean.h](mac_korean.h.md)

### mac_korean_pull

```c
static size_t mac_korean_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 192 to 197.

Convert MAC_KOREAN to UCS2 through cjk_generic_pull.

Parameters:
* `cd`: EUC-KR iconv descriptor for the fallback
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with errno

Calls: [cjk_generic_pull](generic_cjk.c.md#cjk_generic_pull), [mac_korean_char_pull](mac_korean.c.md#mac_korean_char_pull)

Dispatched via: [charset_mac_korean](mac_korean.c.md#charset_mac_korean)
