---
type: C Source File
title: "libatalk/atp/atp_bufs.c"
description: "4 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_bufs.c"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/atp](../atp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atp_internals.h](atp_internals.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `unistd.h`

# Functions

### more_bufs

```c
static int more_bufs(void)
```

Defined at lines 67 to 106.

Called by: [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf)

Uses file-scope variables: `chunkcap`, `chunks`, `free_list`, `nchunks`

### atp_alloc_buf

```c
struct atpbuf * atp_alloc_buf(void)
```

Defined at lines 152 to 166.

Calls: [more_bufs](atp_bufs.c.md#more_bufs)

Called by: [atp_open](atp_open.c.md#atp_open), [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rreq_try](atp_rreq.c.md#atp_rreq_try), [atp_rsel](atp_rsel.c.md#atp_rsel), [atp_sreq](atp_sreq.c.md#atp_sreq), [atp_sresp](atp_sresp.c.md#atp_sresp)

Uses file-scope variables: `free_list`

### atp_bufs_release

```c
void atp_bufs_release(void)
```

Defined at lines 174 to 184.

Uses file-scope variables: `chunkcap`, `chunks`, `free_list`, `nchunks`

### atp_free_buf

```c
int atp_free_buf(struct atpbuf *bp)
```

Defined at lines 186 to 198.

Called by: [atp_close](atp_close.c.md#atp_close), [atp_queue_push](atp_packet.c.md#atp_queue_push), [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rreq_try](atp_rreq.c.md#atp_rreq_try), [atp_rresp](atp_rresp.c.md#atp_rresp), [atp_rsel](atp_rsel.c.md#atp_rsel), [atp_sreq](atp_sreq.c.md#atp_sreq), [atp_sresp](atp_sresp.c.md#atp_sresp)

Uses file-scope variables: `free_list`

# Macros

* Undocumented: `N_MORE_BUFS`

# File-scope variables

`chunkcap`, `chunks`, `free_list`, `nchunks`
