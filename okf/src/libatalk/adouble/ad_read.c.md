---
type: C Source File
title: "libatalk/adouble/ad_read.c"
description: "2 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_read.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### adf_pread

```c
ssize_t adf_pread(struct ad_fd *ad_fd, void *buf, size_t count, off_t offset)
```

Defined at lines 39 to 61.

Called by: [ad_convert_pread_full](ad_open.c.md#ad_convert_pread_full), [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx), [ad_read](ad_read.c.md#ad_read)

### ad_read

```c
ssize_t ad_read(struct adouble *ad, const uint32_t eid, off_t off, char *buf, const size_t buflen)
```

Defined at lines 66 to 112.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [adf_pread](ad_read.c.md#adf_pread)

Called by: [nad_read](../../bin/nad/nad_adouble.c.md#nad_read), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [read_file](../../etc/afpd/fork.c.md#read_file), [rfork_cache_store_from_fd](../../etc/afpd/ad_cache.c.md#rfork_cache_store_from_fd)

Mentioned in the documentation of: [rfork_cache_store_from_fd](../../etc/afpd/ad_cache.c.md#rfork_cache_store_from_fd)
