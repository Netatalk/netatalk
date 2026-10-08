---
type: C Source File
title: "libatalk/util/locking.c"
description: "Netatalk utility functions: locking."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/locking.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `fcntl.h`, `unistd.h`

# Functions

### lock_reg

```c
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
```

Defined at lines 86 to 94.

lock a file with fctnl

This function is called via the macros: read_lock, write_lock, un_lock

Parameters:
* `fd`: File descriptor
* `cmd`: cmd to fcntl, only F_SETLK is usable here
* `type`: F_RDLCK, F_WRLCK, F_UNLCK
* `offset`: byte offset relative to l_whence
* `whence`: SEEK_SET, SEEK_CUR, SEEK_END
* `len`: no. of bytes (0 means to EOF)

Returns: 0 on success, -1 on failure with fcntl return value and errno

See also: read_lock, write_lock, unlock
