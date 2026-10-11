---
type: C Source File
title: "libatalk/adouble/ad_sendfile.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_sendfile.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `errno.h`, `stdio.h`, `sys/socket.h`, `sys/uio.h`

# Functions

### sys_sendfile

```c
ssize_t sys_sendfile(int tofd, int fromfd, off_t *offset, size_t count)
```

Defined at lines 92 to 97.

Send file bytes to a socket through the sendfile call that the platform offers.

Linux and Solaris pass the arguments straight through. FreeBSD reorders them and advances offset in the wrapper. A platform without a sendfile flavor fails with ENOSYS.

Parameters:
* `tofd`: socket to write to
* `fromfd`: file to read from
* `offset`: file offset to start at, advanced by the bytes sent
* `count`: bytes to send

Returns: bytes sent, or -1 with errno set

Called by: [copy_fork](ad_write.c.md#copy_fork), [dsi_stream_read_file](../dsi/dsi_stream.c.md#dsi_stream_read_file)
