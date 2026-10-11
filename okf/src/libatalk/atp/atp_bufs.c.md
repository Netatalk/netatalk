---
type: C Source File
title: "libatalk/atp/atp_bufs.c"
description: "Our own memory maintenance for atp."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_bufs.c"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 73 to 112.

Fill the empty free list with N_MORE_BUFS new ATP buffers.

One malloc call provides all of them, and the chunk table records its base for the final free. The call replaces free_list, so it runs only while the list is empty.

Returns: 0, or -1 with ENOBUFS when an allocation fails

Called by: [atp_alloc_buf](atp_bufs.c.md#atp_alloc_buf)

Uses file-scope variables: `chunkcap`, `chunks`, `free_list`, `nchunks`

### atp_alloc_buf

```c
struct atpbuf * atp_alloc_buf(void)
```

Defined at lines 164 to 178.

Take an ATP buffer from the free list, refilling the list with more_bufs when it is empty.

Returns: the buffer, or NULL with ENOBUFS

Calls: [more_bufs](atp_bufs.c.md#more_bufs)

Called by: [atp_open](atp_open.c.md#atp_open), [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rreq_try](atp_rreq.c.md#atp_rreq_try), [atp_rsel](atp_rsel.c.md#atp_rsel), [atp_sreq](atp_sreq.c.md#atp_sreq), [atp_sresp](atp_sresp.c.md#atp_sresp)

Uses file-scope variables: `free_list`

### atp_bufs_release

```c
void atp_bufs_release(void)
```

Defined at lines 187 to 197.

Free every chunk of the ATP buffer pool and empty the free list.

The call frees the memory that handed-out buffers live in, so it runs only when no buffer is in use. A session process exits without it.

Uses file-scope variables: `chunkcap`, `chunks`, `free_list`, `nchunks`

### atp_free_buf

```c
int atp_free_buf(struct atpbuf *bp)
```

Defined at lines 205 to 217.

Return an ATP buffer to the free list.

Parameters:
* `bp`: buffer from atp_alloc_buf

Returns: 0, or -1 for NULL

Called by: [atp_close](atp_close.c.md#atp_close), [atp_queue_push](atp_packet.c.md#atp_queue_push), [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rreq_try](atp_rreq.c.md#atp_rreq_try), [atp_rresp](atp_rresp.c.md#atp_rresp), [atp_rsel](atp_rsel.c.md#atp_rsel), [atp_sreq](atp_sreq.c.md#atp_sreq), [atp_sresp](atp_sresp.c.md#atp_sresp)

Uses file-scope variables: `free_list`

# Macros

* Undocumented: `N_MORE_BUFS`

# File-scope variables

`chunkcap`, `chunks`, `free_list`, `nchunks`
