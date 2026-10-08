---
type: C Source File
title: "libatalk/adouble/ad_recvfile.c"
description: "5 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_recvfile.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `stdio.h`, `stdlib.h`, `sys/select.h`, `sys/socket.h`, `sys/uio.h`

# Functions

### ad_recvfile_init

```c
static int ad_recvfile_init(const struct adouble *ad, int eid, off_t *off)
```

Defined at lines 37 to 49.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_recvfile](ad_recvfile.c.md#ad_recvfile)

### default_sys_recvfile

```c
static ssize_t default_sys_recvfile(int fromfd, int tofd, off_t offset, size_t count)
```

Defined at lines 63 to 133.

If tofd is -1, drain the incoming socket of count bytes without writing to the outgoing fd, if a write fails we do the same.

Returns -1 on short reads from fromfd (read error) and sets errno.

Returns number of bytes written to 'tofd' or thrown away if 'tofd == -1'. return != count then sets errno. Returns count if complete success.

### waitfordata

```c
static int waitfordata(int socket)
```

Defined at lines 136 to 170.

Called by: [sys_recvfile](ad_recvfile.c.md#sys_recvfile)

### sys_recvfile

```c
static ssize_t sys_recvfile(int fromfd, int tofd, off_t offset, size_t count, int splice_size)
```

Defined at lines 179 to 262.

Calls: [waitfordata](ad_recvfile.c.md#waitfordata)

Called by: [ad_recvfile](ad_recvfile.c.md#ad_recvfile)

### ad_recvfile

```c
ssize_t ad_recvfile(struct adouble *ad, int eid, int sock, off_t off, size_t len, int splice_size)
```

Defined at lines 276 to 293.

read from a socket and write to an adouble file

Calls: [ad_recvfile_init](ad_recvfile.c.md#ad_recvfile_init), [sys_recvfile](ad_recvfile.c.md#sys_recvfile)

Called by: [write_fork](../../etc/afpd/fork.c.md#write_fork)

# Macros

* Undocumented: `TRANSFER_BUF_SIZE`
