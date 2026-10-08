---
type: C Header File
title: "libatalk/unicode/charsets/mac_cyrillic.h"
description: "2 functions."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_cyrillic.h"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Included by

* [libatalk/unicode/charsets/mac_cyrillic.c](mac_cyrillic.c.md)

# Functions

### char_mac_cyrillic_to_ucs2

```c
static int char_mac_cyrillic_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 54 to 65.

Called by: [mac_cyrillic_pull](mac_cyrillic.c.md#mac_cyrillic_pull)

Uses file-scope variables: `mac_cyrillic_2uni`

### char_ucs2_to_mac_cyrillic

```c
static int char_ucs2_to_mac_cyrillic(unsigned char *r, ucs2_t wc)
```

Defined at lines 114 to 149.

Called by: [mac_cyrillic_push](mac_cyrillic.c.md#mac_cyrillic_push)

Uses file-scope variables: `mac_cyrillic_page00`, `mac_cyrillic_page04`, `mac_cyrillic_page20`, `mac_cyrillic_page21`, `mac_cyrillic_page22`

# File-scope variables

`mac_cyrillic_2uni`, `mac_cyrillic_page00`, `mac_cyrillic_page04`, `mac_cyrillic_page20`, `mac_cyrillic_page21`, `mac_cyrillic_page22`
