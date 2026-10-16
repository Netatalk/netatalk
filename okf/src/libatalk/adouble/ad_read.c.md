---
type: C Source File
title: "libatalk/adouble/ad_read.c"
description: "Reading the forks of an AppleDouble."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_read.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T12:16:30+11:00 }
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
ssize_t adf_pread(struct ad_fd *adf, void *buf, size_t count, off_t offset)
```

Defined at lines 55 to 77.

Read count bytes that start at offset in an AppleDouble fork.

The read goes through pread and leaves the file position alone.

Parameters:
* `adf`: fork descriptor
* `buf`: receives the bytes
* `count`: bytes to read
* `offset`: file offset

Returns: bytes read, or -1 with errno set

Called by: [ad_convert_pread_full](ad_open.c.md#ad_convert_pread_full), [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx), [ad_read](ad_read.c.md#ad_read)

### ad_read

```c
ssize_t ad_read(struct adouble *ad, const uint32_t eid, off_t off, char *buf, const size_t buflen)
```

Defined at lines 97 to 153.

Read from the data or resource fork of an AppleDouble.

The data fork of a symlink yields the link target from off, and fails with ERANGE when buflen cannot hold the rest of it. Any other eid reads the resource fork, and a closed resource fork yields 0. The ea layout stores the fork at offset 0 of its EA, or behind the sidecar header without HAVE_EAFD. Version 2 adds the entry offset from the header. The function checks no byte-range locks.

Parameters:
* `ad`: AppleDouble with the fork open
* `eid`: ADEID_DFORK, or any other entry for the resource fork
* `off`: offset within the fork
* `buf`: receives the bytes
* `buflen`: bytes to read

Returns: bytes read, 0 at the end of the fork or for a closed resource fork, or -1 with errno set on failure

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [adf_pread](ad_read.c.md#adf_pread)

Called by: [nad_read](../../bin/nad/nad_adouble.c.md#nad_read), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [read_file](../../etc/afpd/fork.c.md#read_file), [rfork_cache_store_from_fd](../../etc/afpd/ad_cache.c.md#rfork_cache_store_from_fd)
