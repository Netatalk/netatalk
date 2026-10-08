---
type: C Header File
title: "libatalk/unicode/charsets/mac_turkish.h"
description: "2 functions."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_turkish.h"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Included by

* [libatalk/unicode/charsets/mac_turkish.c](mac_turkish.c.md)

# Functions

### char_mac_turkish_to_ucs2

```c
static int char_mac_turkish_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 54 to 71.

Called by: [mac_turkish_pull](mac_turkish.c.md#mac_turkish_pull)

Uses file-scope variables: `mac_turkish_2uni`

### char_ucs2_to_mac_turkish

```c
static int char_ucs2_to_mac_turkish(unsigned char *r, ucs2_t wc)
```

Defined at lines 138 to 173.

Called by: [mac_turkish_push](mac_turkish.c.md#mac_turkish_push)

Uses file-scope variables: `mac_turkish_page00`, `mac_turkish_page01`, `mac_turkish_page02`, `mac_turkish_page20`, `mac_turkish_page21`, `mac_turkish_page22`

# File-scope variables

`mac_turkish_2uni`, `mac_turkish_page00`, `mac_turkish_page01`, `mac_turkish_page02`, `mac_turkish_page20`, `mac_turkish_page21`, `mac_turkish_page22`
