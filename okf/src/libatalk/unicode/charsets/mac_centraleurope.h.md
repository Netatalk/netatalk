---
type: C Header File
title: "libatalk/unicode/charsets/mac_centraleurope.h"
description: "2 functions."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/mac_centraleurope.h"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Included by

* [libatalk/unicode/charsets/mac_centraleurope.c](mac_centraleurope.c.md)

# Functions

### char_mac_centraleurope_to_ucs2

```c
static int char_mac_centraleurope_to_ucs2(ucs2_t *pwc, const unsigned char *s)
```

Defined at lines 53 to 64.

Called by: [mac_centraleurope_pull](mac_centraleurope.c.md#mac_centraleurope_pull)

Uses file-scope variables: `mac_centraleurope_2uni`

### char_ucs2_to_mac_centraleurope

```c
static int char_ucs2_to_mac_centraleurope(unsigned char *r, ucs2_t wc)
```

Defined at lines 116 to 145.

Called by: [mac_centraleurope_push](mac_centraleurope.c.md#mac_centraleurope_push)

Uses file-scope variables: `mac_centraleurope_page00`, `mac_centraleurope_page20`, `mac_centraleurope_page22`, `mac_centraleurope_page22_1`

# File-scope variables

`mac_centraleurope_2uni`, `mac_centraleurope_page00`, `mac_centraleurope_page20`, `mac_centraleurope_page22`, `mac_centraleurope_page22_1`
