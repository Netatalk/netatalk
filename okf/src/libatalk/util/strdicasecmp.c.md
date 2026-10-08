---
type: C Source File
title: "libatalk/util/strdicasecmp.c"
description: "2 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/strdicasecmp.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/util.h](../../include/atalk/util.h.md)

# Functions

### strdiacasecmp

```c
int strdiacasecmp(const char *s1, const char *s2)
```

Defined at lines 527 to 538.

Called by: [ext_cmp_key](netatalk_conf.c.md#ext_cmp_key), [extmap_cmp](netatalk_conf.c.md#extmap_cmp), [setextmap](netatalk_conf.c.md#setextmap)

Uses file-scope variables: `_diacasemap`

### strndiacasecmp

```c
int strndiacasecmp(const char *s1, const char *s2, size_t n)
```

Defined at lines 540 to 558.

Called by: [addzone](../../etc/atalkd/zip.c.md#addzone), [auth_uamfind](../../etc/afpd/auth.c.md#auth_uamfind), [auth_uamfind](../../etc/papd/auth.c.md#auth_uamfind), [nbp_match](../nbp/nbp_util.c.md#nbp_match), [nbp_packet](../../etc/atalkd/nbp.c.md#nbp_packet), [zip_packet](../../etc/atalkd/zip.c.md#zip_packet), [zonecheck](../../etc/atalkd/zip.c.md#zonecheck)

Uses file-scope variables: `_diacasemap`

# File-scope variables

`_diacasemap`, `_dialowermap`
