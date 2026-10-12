---
type: C Source File
title: "libatalk/util/strdicasecmp.c"
description: "Case-insensitive string comparison through the Mac Roman uppercase map."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/strdicasecmp.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/util.h](../../include/atalk/util.h.md)

# Functions

### strdiacasecmp

```c
int strdiacasecmp(const char *s1, const char *s2)
```

Defined at lines 545 to 556.

Compare two strings without regard to case, through the Mac Roman upper case map.

Each byte maps through _diacasemap, which folds accented Mac Roman letters to upper case and keeps their diacritics. A byte above 127 maps as the Mac Roman character with that code, whatever the encoding of the string.

Parameters:
* `s1`: first string
* `s2`: second string

Returns: 0 for equal strings, else the difference of the first mapped bytes that differ

Called by: [ext_cmp_key](netatalk_conf.c.md#ext_cmp_key), [extmap_cmp](netatalk_conf.c.md#extmap_cmp), [setextmap](netatalk_conf.c.md#setextmap)

Uses file-scope variables: `_diacasemap`

### strndiacasecmp

```c
int strndiacasecmp(const char *s1, const char *s2, size_t n)
```

Defined at lines 570 to 588.

Compare up to n bytes of two strings without regard to case, through the Mac Roman upper case map.

The map is the one of strdiacasecmp.

Parameters:
* `s1`: first string
* `s2`: second string
* `n`: byte limit

Returns: 0 when the first n bytes match, else the difference of the first mapped bytes that differ

Called by: [addzone](../../etc/atalkd/zip.c.md#addzone), [auth_uamfind](../../etc/afpd/auth.c.md#auth_uamfind), [auth_uamfind](../../etc/papd/auth.c.md#auth_uamfind), [nbp_match](../nbp/nbp_util.c.md#nbp_match), [nbp_packet](../../etc/atalkd/nbp.c.md#nbp_packet), [zip_packet](../../etc/atalkd/zip.c.md#zip_packet), [zonecheck](../../etc/atalkd/zip.c.md#zonecheck)

Uses file-scope variables: `_diacasemap`

# File-scope variables

`_diacasemap`, `_dialowermap`
