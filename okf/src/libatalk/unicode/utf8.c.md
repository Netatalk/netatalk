---
type: C Source File
title: "libatalk/unicode/utf8.c"
description: "2 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/utf8.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 103 to 224.

Convert from UTF-8 to UTF-16.

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

Dispatched via: [charset_utf8](utf8.c.md#charset_utf8), [charset_utf8_mac](utf8.c.md#charset_utf8_mac)

### utf8_push

```c
static size_t utf8_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 227 to 328.

Convert from UTF-16 to UTF-8

Dispatched via: [charset_utf8](utf8.c.md#charset_utf8), [charset_utf8_mac](utf8.c.md#charset_utf8_mac)

# Macros

* Undocumented: `GETUCVAL`, `GETUTF8TRAILBYTE`
