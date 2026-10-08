---
type: C Source File
title: "libatalk/adouble/ad_sendfile.c"
description: "2 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_sendfile.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `errno.h`, `stdio.h`, `sys/socket.h`, `sys/uio.h`

# Functions

### sys_sendfile

```c
ssize_t sys_sendfile(int out_fd, int in_fd, off_t *_offset, size_t count)
```

Defined at lines 78 to 83.

Called by: [copy_fork](ad_write.c.md#copy_fork), [dsi_stream_read_file](../dsi/dsi_stream.c.md#dsi_stream_read_file)

### ad_readfile_init

```c
int ad_readfile_init(const struct adouble *ad, const int eid, off_t *off, const int end)
```

Defined at lines 87 to 105.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_size](ad_size.c.md#ad_size)

Called by: [read_fork](../../etc/afpd/fork.c.md#read_fork)
