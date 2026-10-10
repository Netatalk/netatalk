---
type: C Source File
title: "libatalk/adouble/ad_sendfile.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_sendfile.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
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
