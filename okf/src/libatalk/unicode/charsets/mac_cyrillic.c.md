---
type: C Source File
title: "libatalk/unicode/charsets/mac_cyrillic.c"
description: "MAC_CYRILLIC charset conversion."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_cyrillic.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/unicode.h](../../../include/atalk/unicode.h.md)
* [generic_mb.h](generic_mb.h.md)
* [mac_cyrillic.h](mac_cyrillic.h.md)
* System headers: `arpa/inet.h`, `stdlib.h`

# Function tables

### charset_mac_cyrillic

Initialized at line 88 as `struct charset_functions`. Assigns:

* `pull`: [mac_cyrillic_pull](mac_cyrillic.c.md#mac_cyrillic_pull)
* `push`: [mac_cyrillic_push](mac_cyrillic.c.md#mac_cyrillic_push)

# Functions

### mac_cyrillic_push

```c
static size_t mac_cyrillic_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 69 to 74.

Convert UCS2 to MAC_CYRILLIC through mb_generic_push.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position, which mb_generic_push does not advance
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_ucs2_to_mac_cyrillic](mac_cyrillic.h.md#char_ucs2_to_mac_cyrillic), [mb_generic_push](generic_mb.c.md#mb_generic_push)

Dispatched via: [charset_mac_cyrillic](mac_cyrillic.c.md#charset_mac_cyrillic)

### mac_cyrillic_pull

```c
static size_t mac_cyrillic_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 86 to 91.

Convert MAC_CYRILLIC to UCS2 through mb_generic_pull.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_mac_cyrillic_to_ucs2](mac_cyrillic.h.md#char_mac_cyrillic_to_ucs2), [mb_generic_pull](generic_mb.c.md#mb_generic_pull)

Dispatched via: [charset_mac_cyrillic](mac_cyrillic.c.md#charset_mac_cyrillic)
