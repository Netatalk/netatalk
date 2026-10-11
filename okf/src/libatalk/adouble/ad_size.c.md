---
type: C Source File
title: "libatalk/adouble/ad_size.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_size.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `string.h`

# Functions

### ad_size

```c
off_t ad_size(const struct adouble *ad, const uint32_t eid)
```

Defined at lines 28 to 48.

Length of a fork of an AppleDouble.

The data fork length comes from fstat, or from the link target of a symlink. Any other eid yields ad_rlen, the resource fork length that the header read and ad_write maintain.

Parameters:
* `ad`: AppleDouble with the fork open
* `eid`: ADEID_DFORK, or any other entry for the resource fork

Returns: the length in bytes, 0 when the data fork stat failed

Called by: [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams), [byte_lock](../../etc/afpd/fork.c.md#byte_lock), [nad_header_read](../../bin/nad/nad_adouble.c.md#nad_header_read), [read_fork](../../etc/afpd/fork.c.md#read_fork), [write_fork](../../etc/afpd/fork.c.md#write_fork)
