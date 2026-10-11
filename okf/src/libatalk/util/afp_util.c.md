---
type: C Source File
title: "libatalk/util/afp_util.c"
description: "2 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/afp_util.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/afp_util.h](../../include/atalk/afp_util.h.md)

# Functions

### AfpNum2name

```c
const char * AfpNum2name(int num)
```

Defined at lines 22 to 247.

Return the name of an AFP command code for log messages and DTrace probes.

Parameters:
* `num`: AFP command code

Returns: the constant name, or "not yet defined" for an unlisted code

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

### AfpErr2name

```c
const char * AfpErr2name(int err)
```

Defined at lines 257 to 314.

Return the name of an AFP result code for log messages.

Parameters:
* `err`: AFP result code

Returns: the constant name, or "not yet defined" for an unlisted code

Called by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [dir_remove](../../etc/afpd/directory.c.md#dir_remove), [dirlookup_internal](../../etc/afpd/directory.c.md#dirlookup_internal), [getmetadata](../../etc/afpd/file.c.md#getmetadata)

# Macros

* Undocumented: `AFPERR2NAME`
