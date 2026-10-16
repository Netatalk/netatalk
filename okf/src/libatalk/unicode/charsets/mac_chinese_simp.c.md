---
type: C Source File
title: "libatalk/unicode/charsets/mac_chinese_simp.c"
description: "Mac Chinese Simplified charset conversion."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_chinese_simp.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [generic_cjk.h](generic_cjk.h.md)
* [mac_chinese_simp.h](mac_chinese_simp.h.md)
* System headers: `stdlib.h`

# Function tables

### charset_mac_chinese_simp

Initialized at line 97 as `struct charset_functions`. Assigns:

* `pull`: [mac_chinese_simp_pull](mac_chinese_simp.c.md#mac_chinese_simp_pull)
* `push`: [mac_chinese_simp_push](mac_chinese_simp.c.md#mac_chinese_simp_push)

# Functions

### mac_chinese_simp_char_push

```c
static size_t mac_chinese_simp_char_push(uint8_t *out, const ucs2_t *in, size_t *size)
```

Defined at lines 68 to 96.

Map one UCS2 character, or a pair with an Apple hint character, to MAC_CHINESE_SIMP bytes.

ASCII passes as is. When a hint character in the range 0xf870 to 0xf87f follows, the composition table first combines the pair. Private-use characters and characters without a table entry return 0, so the caller converts them through EUC-CN.

Parameters:
* `out`: receives one or two bytes
* `in`: UCS2 input
* `size`: characters available, which receives the characters used

Returns: the number of bytes, 0 for the iconv fallback, or `(size_t)-1` with EILSEQ

Calls: [cjk_char_push](generic_cjk.c.md#cjk_char_push), [cjk_compose](generic_cjk.c.md#cjk_compose), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_chinese_simp_push](mac_chinese_simp.c.md#mac_chinese_simp_push)

Uses file-scope variables: `mac_chinese_simp_compose` in [libatalk/unicode/charsets/mac_chinese_simp.h](mac_chinese_simp.h.md), `mac_chinese_simp_uni2_charset` in [libatalk/unicode/charsets/mac_chinese_simp.h](mac_chinese_simp.h.md), `mac_chinese_simp_uni2_index` in [libatalk/unicode/charsets/mac_chinese_simp.h](mac_chinese_simp.h.md)

### mac_chinese_simp_push

```c
static size_t mac_chinese_simp_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 108 to 113.

Convert UCS2 to MAC_CHINESE_SIMP through cjk_generic_push.

Parameters:
* `cd`: EUC-CN iconv descriptor for the fallback
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with errno

Calls: [cjk_generic_push](generic_cjk.c.md#cjk_generic_push), [mac_chinese_simp_char_push](mac_chinese_simp.c.md#mac_chinese_simp_char_push)

Dispatched via: [charset_mac_chinese_simp](mac_chinese_simp.c.md#charset_mac_chinese_simp)

### mac_chinese_simp_char_pull

```c
static size_t mac_chinese_simp_char_pull(ucs2_t *out, const uint8_t *in, size_t *size)
```

Defined at lines 128 to 159.

Map one MAC_CHINESE_SIMP character of one or two bytes to UCS2.

ASCII passes as is. A lead byte from 0xa1 to 0xfc needs a trail byte from 0xa1 to 0xfe, and other bytes stand alone. The table value expands through the composition table into up to CJK_PULL_BUFFER characters.

Parameters:
* `out`: receives the UCS2 characters
* `in`: input bytes
* `size`: bytes available, which receives the bytes used

Returns: the number of characters, 0 for the iconv fallback, or `(size_t)-1` with EILSEQ or EINVAL

Calls: [cjk_char_pull](generic_cjk.c.md#cjk_char_pull), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_chinese_simp_pull](mac_chinese_simp.c.md#mac_chinese_simp_pull)

Uses file-scope variables: `mac_chinese_simp_2uni_charset` in [libatalk/unicode/charsets/mac_chinese_simp.h](mac_chinese_simp.h.md), `mac_chinese_simp_2uni_index` in [libatalk/unicode/charsets/mac_chinese_simp.h](mac_chinese_simp.h.md), `mac_chinese_simp_compose` in [libatalk/unicode/charsets/mac_chinese_simp.h](mac_chinese_simp.h.md)

### mac_chinese_simp_pull

```c
static size_t mac_chinese_simp_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 171 to 176.

Convert MAC_CHINESE_SIMP to UCS2 through cjk_generic_pull.

Parameters:
* `cd`: EUC-CN iconv descriptor for the fallback
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with errno

Calls: [cjk_generic_pull](generic_cjk.c.md#cjk_generic_pull), [mac_chinese_simp_char_pull](mac_chinese_simp.c.md#mac_chinese_simp_char_pull)

Dispatched via: [charset_mac_chinese_simp](mac_chinese_simp.c.md#charset_mac_chinese_simp)
