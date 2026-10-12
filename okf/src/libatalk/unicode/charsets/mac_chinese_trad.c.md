---
type: C Source File
title: "libatalk/unicode/charsets/mac_chinese_trad.c"
description: "MAC_CHINESE_TRAD charset conversion."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_chinese_trad.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [generic_cjk.h](generic_cjk.h.md)
* [mac_chinese_trad.h](mac_chinese_trad.h.md)
* System headers: `stdlib.h`

# Function tables

### charset_mac_chinese_trad

Initialized at line 95 as `struct charset_functions`. Assigns:

* `pull`: [mac_chinese_trad_pull](mac_chinese_trad.c.md#mac_chinese_trad_pull)
* `push`: [mac_chinese_trad_push](mac_chinese_trad.c.md#mac_chinese_trad_push)

# Functions

### mac_chinese_trad_char_push

```c
static size_t mac_chinese_trad_char_push(uint8_t *out, const ucs2_t *in, size_t *size)
```

Defined at lines 69 to 103.

Map one UCS2 character, or a pair with an Apple hint character, to MAC_CHINESE_TRAD bytes.

ASCII passes as is, except that a backslash followed by 0xf87f becomes 0x80. When a hint character in the range 0xf870 to 0xf87f follows, the composition table first combines the pair. Private-use characters and characters without a table entry return 0, so the caller converts them through BIG-5.

Parameters:
* `out`: receives one or two bytes
* `in`: UCS2 input
* `size`: characters available, which receives the characters used

Returns: the number of bytes, 0 for the iconv fallback, or `(size_t)-1` with EILSEQ

Calls: [cjk_char_push](generic_cjk.c.md#cjk_char_push), [cjk_compose](generic_cjk.c.md#cjk_compose), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_chinese_trad_push](mac_chinese_trad.c.md#mac_chinese_trad_push)

Uses file-scope variables: `mac_chinese_trad_compose` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md), `mac_chinese_trad_uni2_charset` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md), `mac_chinese_trad_uni2_index` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md)

### mac_chinese_trad_push

```c
static size_t mac_chinese_trad_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 115 to 120.

Convert UCS2 to MAC_CHINESE_TRAD through cjk_generic_push.

Parameters:
* `cd`: BIG-5 iconv descriptor for the fallback
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with errno

Calls: [cjk_generic_push](generic_cjk.c.md#cjk_generic_push), [mac_chinese_trad_char_push](mac_chinese_trad.c.md#mac_chinese_trad_char_push)

Dispatched via: [charset_mac_chinese_trad](mac_chinese_trad.c.md#charset_mac_chinese_trad)

### mac_chinese_trad_char_pull

```c
static size_t mac_chinese_trad_char_pull(ucs2_t *out, const uint8_t *in, size_t *size)
```

Defined at lines 136 to 167.

Map one MAC_CHINESE_TRAD character of one or two bytes to UCS2.

ASCII passes as is, and bytes outside the lead byte range stand alone. A lead byte from 0xa1 to 0xfc needs a trail byte in 0x40 to 0x7e or in 0xa1 to 0xfe. The table value expands through the composition table into up to CJK_PULL_BUFFER characters.

Parameters:
* `out`: receives the UCS2 characters
* `in`: input bytes
* `size`: bytes available, which receives the bytes used

Returns: the number of characters, 0 for the iconv fallback, or `(size_t)-1` with EILSEQ or EINVAL

Calls: [cjk_char_pull](generic_cjk.c.md#cjk_char_pull), [cjk_lookup](generic_cjk.c.md#cjk_lookup)

Called by: [mac_chinese_trad_pull](mac_chinese_trad.c.md#mac_chinese_trad_pull)

Uses file-scope variables: `mac_chinese_trad_2uni_charset` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md), `mac_chinese_trad_2uni_index` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md), `mac_chinese_trad_compose` in [libatalk/unicode/charsets/mac_chinese_trad.h](mac_chinese_trad.h.md)

### mac_chinese_trad_pull

```c
static size_t mac_chinese_trad_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 179 to 184.

Convert MAC_CHINESE_TRAD to UCS2 through cjk_generic_pull.

Parameters:
* `cd`: BIG-5 iconv descriptor for the fallback
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with errno

Calls: [cjk_generic_pull](generic_cjk.c.md#cjk_generic_pull), [mac_chinese_trad_char_pull](mac_chinese_trad.c.md#mac_chinese_trad_char_pull)

Dispatched via: [charset_mac_chinese_trad](mac_chinese_trad.c.md#charset_mac_chinese_trad)
