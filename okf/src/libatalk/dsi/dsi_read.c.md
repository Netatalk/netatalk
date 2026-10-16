---
type: C Source File
title: "libatalk/dsi/dsi_read.c"
description: "Streamed FPRead replies over DSI."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_read.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T12:16:30+11:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `signal.h`, `stdio.h`, `string.h`, `sys/time.h`, `sys/types.h`, `unistd.h`

# Functions

### dsi_readinit

```c
ssize_t dsi_readinit(DSI *, void *, const size_t, const size_t, const int)
```

Defined at lines 43 to 63. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Start a streamed FPRead reply of size bytes and send its first buflen bytes.

The reply header carries the full size and the result code err. DSI_NOREPLY tells the command loop that the handler sends the reply itself, and in_write blocks attentions until dsi_readdone. The remaining size minus buflen bytes follow through dsi_read.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `buf`: first part of the reply payload
* `buflen`: bytes in buf
* `size`: total reply payload length, at most UINT32_MAX, the [DSI](../../include/atalk/dsi.h.md#struct-dsi) header's 32-bit length
* `err`: AFP result code

Returns: bytes to send next, at most buflen, or -1 when the send failed

Calls: [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

Called by: [read_fork](../../etc/afpd/fork.c.md#read_fork), [rfork_cache_serve_from_buf](../../etc/afpd/fork.c.md#rfork_cache_serve_from_buf)

### dsi_readdone

```c
void dsi_readdone(DSI *)
```

Defined at lines 70 to 73. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

End a streamed FPRead reply that dsi_readinit started.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session whose in_write count drops

Called by: [read_fork](../../etc/afpd/fork.c.md#read_fork), [rfork_cache_serve_from_buf](../../etc/afpd/fork.c.md#rfork_cache_serve_from_buf)

### dsi_read

```c
ssize_t dsi_read(DSI *, void *, const size_t)
```

Defined at lines 83 to 94. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Send the next buflen bytes of a streamed FPRead reply.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session after dsi_readinit
* `buf`: payload bytes
* `buflen`: bytes in buf

Returns: bytes to send next, at most buflen, or -1 when the send failed

Calls: [dsi_stream_write](dsi_stream.c.md#dsi_stream_write)

Called by: [read_fork](../../etc/afpd/fork.c.md#read_fork), [rfork_cache_serve_from_buf](../../etc/afpd/fork.c.md#rfork_cache_serve_from_buf)
