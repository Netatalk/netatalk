---
type: C Source File
title: "libatalk/unicode/utf8.c"
description: "UTF-8 charset conversion."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/utf8.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/unicode](../unicode.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/byteorder.h](../../include/atalk/byteorder.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `stdlib.h`

# Function tables

### charset_utf8

Initialized at line 91 as `struct charset_functions`. Assigns:

* `pull`: [utf8_pull](utf8.c.md#utf8_pull)
* `push`: [utf8_push](utf8.c.md#utf8_push)

### charset_utf8_mac

Initialized at line 92 as `struct charset_functions`. Assigns:

* `pull`: [utf8_pull](utf8.c.md#utf8_pull)
* `push`: [utf8_push](utf8.c.md#utf8_push)

# Functions

### utf8_pull

```c
static size_t utf8_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 115 to 236.

Convert UTF-8 to UTF-16 and reject sequences outside the valid byte ranges.

Characters above U+FFFF become surrogate pairs. The valid ranges follow.

```
Code Points         First   Second  Third   Fourth
U+0000..U+007F      00..7F
U+0080..U+07FF      C2..DF  80..BF
U+0800..U+0FFF      E0      A0..BF  80..BF
U+1000..U+CFFF      E1..EC  80..BF  80..BF
U+D000..U+D7FF      ED      80..9F  80..BF
U+E000..U+FFFF      EE..EF  80..BF  80..BF
U+10000..U+3FFFF    F0      90..BF  80..BF  80..BF
U+40000..U+FFFFF    F1..F3  80..BF  80..BF  80..BF
U+100000..U+10FFFF  F4      80..8F  80..BF  80..BF
```

Parameters:
* `cd`: descriptor, unused
* `inbuf`: UTF-8 input position
* `inbytesleft`: input bytes left
* `outbuf`: UTF-16 output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with EILSEQ for a malformed sequence, EINVAL for a cut sequence, or E2BIG

Dispatched via: [charset_utf8](utf8.c.md#charset_utf8), [charset_utf8_mac](utf8.c.md#charset_utf8_mac)

### utf8_push

```c
static size_t utf8_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 254 to 355.

Convert UTF-16 to UTF-8 and drop the bidirectional formatting characters.

A surrogate pair becomes one 4-byte sequence, and a lone or malformed surrogate fails with EINVAL. The characters U+202A to U+202E produce no output.

Parameters:
* `cd`: descriptor, unused
* `inbuf`: UTF-16 input position
* `inbytesleft`: input bytes left
* `outbuf`: UTF-8 output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with EINVAL for a malformed surrogate or a trailing odd byte, or E2BIG

Dispatched via: [charset_utf8](utf8.c.md#charset_utf8), [charset_utf8_mac](utf8.c.md#charset_utf8_mac)

# Macros

* `GETUCVAL`: Given a trailing UTF-8 byte, get the contribution from it to the Unicode scalar value for a particular bit shift amount.
* `GETUTF8TRAILBYTE`: Given a unicode scalar, get a trail UTF-8 byte for a particular bit shift amount.
