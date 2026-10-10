---
type: C Source File
title: "libatalk/dsi/dsi_write.c"
description: "8 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_write.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `fcntl.h`, `poll.h`, `stdio.h`, `string.h`, `sys/ioctl.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `unistd.h`

# Functions

### dsi_writeinit

```c
size_t dsi_writeinit(DSI *, char **)
```

Defined at lines 53 to 86. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Begin a [DSI](../../include/atalk/dsi.h.md#struct-dsi) write transfer: set datasize and hand back, by reference, any payload already in the read-ahead buffer. *bufp is valid only until the next dsi_* call; consume it immediately. A NULL bufp discards the buffered payload, for callers that only drain the transfer.

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [write_fork](../../etc/afpd/fork.c.md#write_fork)

### dsi_write

```c
size_t dsi_write(DSI *, void *, const size_t)
```

Defined at lines 92 to 107. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

fill up buf and then return. this should be called repeatedly until all the data has been read. i block alarm processing during the transfer to avoid sending unnecessary tickles.

Calls: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [write_fork](../../etc/afpd/fork.c.md#write_fork)

Mentioned in the documentation of: [ad_recvfile](../adouble/ad_recvfile.c.md#ad_recvfile), [dsi_write_file](dsi_write.c.md#dsi_write_file)

### dsi_writeflush

```c
void dsi_writeflush(DSI *)
```

Defined at lines 110 to 124. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

flush any unread buffers.

Calls: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [write_fork](../../etc/afpd/fork.c.md#write_fork)

Mentioned in the documentation of: [dsi_write_file](dsi_write.c.md#dsi_write_file)

### dsi_close_pipe

```c
void dsi_close_pipe(DSI *)
```

Defined at lines 127 to 134. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Close the session's recvfile pipe; the next write opens another.

Called by: [afp_zzzzz](../../etc/afpd/auth.c.md#afp_zzzzz), [dsi_disconnect](dsi_stream.c.md#dsi_disconnect), [dsi_write_file](dsi_write.c.md#dsi_write_file), [open_pipe](dsi_write.c.md#open_pipe)

### waitfordata

```c
static int waitfordata(int socket, int timeout)
```

Defined at lines 142 to 158.

Wait up to timeout ms for socket data.

Returns: 1 at a TCP urgent mark, 0 when not, -1 with errno set

Called by: [dsi_write_file](dsi_write.c.md#dsi_write_file)

### open_pipe

```c
static int open_pipe(DSI *dsi)
```

Defined at lines 166 to 225.

Open the session's recvfile pipe, grown to the largest size up to dsi->splice_size the kernel grants, which the session keeps.

Returns: 0, or -1 when this write takes the userspace path

Calls: [dsi_close_pipe](dsi_write.c.md#dsi_close_pipe)

Called by: [dsi_write_file](dsi_write.c.md#dsi_write_file)

### pipe_to_file

```c
static ssize_t pipe_to_file(DSI *dsi, int tofd, loff_t *offset, size_t len)
```

Defined at lines 233 to 255.

splice() from the pipe into a file through dsi->data, for a file whose filesystem cannot splice

Same contract as splice() with an output offset.

Called by: [dsi_write_file](dsi_write.c.md#dsi_write_file)

### dsi_write_file

```c
ssize_t dsi_write_file(DSI *, int tofd, off_t *offset, bool *nosplice)
```

Defined at lines 273 to 381. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Receive the rest of a DSIWrite payload straight into a file.

dsi->datasize falls as the socket is read, so [dsi_write()](dsi_write.c.md#dsi_write) and [dsi_writeflush()](dsi_write.c.md#dsi_writeflush) continue where this stops.

Parameters:
* `dsi`: session; its datasize is the payload left to read
* `tofd`: destination file
* `offset`: file offset of the first byte, advanced past every byte written, also when the call fails
* `nosplice`: set when tofd's filesystem cannot splice, once the round in the pipe is written through dsi->data

Returns: the bytes written: all of datasize, fewer when the caller is to finish from the socket, 0 when nothing was read; -1 with errno set

Calls: [dsi_close_pipe](dsi_write.c.md#dsi_close_pipe), [open_pipe](dsi_write.c.md#open_pipe), [pipe_to_file](dsi_write.c.md#pipe_to_file), [waitfordata](dsi_write.c.md#waitfordata)

Called by: [ad_recvfile](../adouble/ad_recvfile.c.md#ad_recvfile)
