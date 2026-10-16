---
type: C Header File
title: "libatalk/unicode/charsets/mac_turkish.h"
description: "Mac Turkish character mapping tables."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_turkish.h"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Included by

* [libatalk/unicode/charsets/mac_turkish.c](mac_turkish.c.md)

# Functions

### char_mac_turkish_to_ucs2

```c
static int char_mac_turkish_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 55 to 72.

Map a MAC_TURKISH byte to UCS2, or return 0 for a 0xfffd entry.

Called by: [mac_turkish_pull](mac_turkish.c.md#mac_turkish_pull)

Uses file-scope variables: `mac_turkish_2uni`

### char_ucs2_to_mac_turkish

```c
static int char_ucs2_to_mac_turkish(unsigned char *r, ucs2_t wc)
```

Defined at lines 140 to 175.

Map a UCS2 character to a MAC_TURKISH byte, or return 0.

Called by: [mac_turkish_push](mac_turkish.c.md#mac_turkish_push)

Uses file-scope variables: `mac_turkish_page00`, `mac_turkish_page01`, `mac_turkish_page02`, `mac_turkish_page20`, `mac_turkish_page21`, `mac_turkish_page22`

# File-scope variables

`mac_turkish_2uni`, `mac_turkish_page00`, `mac_turkish_page01`, `mac_turkish_page02`, `mac_turkish_page20`, `mac_turkish_page21`, `mac_turkish_page22`
