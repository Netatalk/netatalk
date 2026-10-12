---
type: C Source File
title: "libatalk/unicode/charsets/mac_centraleurope.c"
description: "MAC_CENTRALEUROPE charset conversion."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_centraleurope.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/unicode.h](../../../include/atalk/unicode.h.md)
* [generic_mb.h](generic_mb.h.md)
* [mac_centraleurope.h](mac_centraleurope.h.md)
* System headers: `arpa/inet.h`, `stdlib.h`

# Function tables

### charset_mac_centraleurope

Initialized at line 87 as `struct charset_functions`. Assigns:

* `pull`: [mac_centraleurope_pull](mac_centraleurope.c.md#mac_centraleurope_pull)
* `push`: [mac_centraleurope_push](mac_centraleurope.c.md#mac_centraleurope_push)

# Functions

### mac_centraleurope_push

```c
static size_t mac_centraleurope_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 71 to 77.

Convert UCS2 to MAC_CENTRALEUROPE through mb_generic_push.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position, which mb_generic_push does not advance
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_ucs2_to_mac_centraleurope](mac_centraleurope.h.md#char_ucs2_to_mac_centraleurope), [mb_generic_push](generic_mb.c.md#mb_generic_push)

Dispatched via: [charset_mac_centraleurope](mac_centraleurope.c.md#charset_mac_centraleurope)

### mac_centraleurope_pull

```c
static size_t mac_centraleurope_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 89 to 95.

Convert MAC_CENTRALEUROPE to UCS2 through mb_generic_pull.

Parameters:
* `cd`: iconv descriptor, unused
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: the number of characters converted, or `(size_t)-1` with errno

Calls: [char_mac_centraleurope_to_ucs2](mac_centraleurope.h.md#char_mac_centraleurope_to_ucs2), [mb_generic_pull](generic_mb.c.md#mb_generic_pull)

Dispatched via: [charset_mac_centraleurope](mac_centraleurope.c.md#charset_mac_centraleurope)
