---
type: C Source File
title: "libatalk/unicode/charsets/generic_mb.c"
description: "2 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/generic_mb.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/byteorder.h](../../../include/atalk/byteorder.h.md)
* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../../include/atalk/unicode.h.md)
* [generic_mb.h](generic_mb.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `stdlib.h`, `string.h`, `unistd.h`

# Functions

### mb_generic_push

```c
size_t mb_generic_push(int(*char_func)(unsigned char *, ucs2_t), void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 62 to 91.

Convert UCS2 to a single-byte charset with a per-character table function.

Each UCS2 character becomes one byte. A character that the table function does not map fails with EILSEQ, and input that does not fit the output fails with E2BIG. The call leaves `outbuf` at its start, so callers measure the output by `outbytesleft`.

Parameters:
* `char_func`: table function of the charset
* `cd`: iconv descriptor, unused
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position, which the call writes at but does not advance
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Called by: [mac_centraleurope_push](mac_centraleurope.c.md#mac_centraleurope_push), [mac_cyrillic_push](mac_cyrillic.c.md#mac_cyrillic_push), [mac_greek_push](mac_greek.c.md#mac_greek_push), [mac_roman_push](mac_roman.c.md#mac_roman_push), [mac_turkish_push](mac_turkish.c.md#mac_turkish_push)

### mb_generic_pull

```c
size_t mb_generic_pull(int(*char_func)(ucs2_t *, const unsigned char *), void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 109 to 139.

Convert a single-byte charset to UCS2 with a per-character table function.

Each byte becomes one UCS2 character. A byte that the table function does not map fails with EILSEQ, and input that does not fit the output fails with E2BIG.

Parameters:
* `char_func`: table function of the charset
* `cd`: iconv descriptor, unused
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Called by: [mac_centraleurope_pull](mac_centraleurope.c.md#mac_centraleurope_pull), [mac_cyrillic_pull](mac_cyrillic.c.md#mac_cyrillic_pull), [mac_greek_pull](mac_greek.c.md#mac_greek_pull), [mac_roman_pull](mac_roman.c.md#mac_roman_pull), [mac_turkish_pull](mac_turkish.c.md#mac_turkish_pull)
