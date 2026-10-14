---
type: C Source File
title: "libatalk/dsi/dsi_write.c"
description: "DSIWrite payload reception, with the recvfile pipe path."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_write.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-14T11:59:27+11:00 }
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

Defined at lines 67 to 100. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Begin a DSIWrite transfer and hand back any payload the read-ahead buffer already holds.

datasize becomes the payload length from the request header, and the bytes already buffered leave it again. The buffer reference stays valid only until the next dsi call, so the caller consumes it at once. A NULL bufp discards the buffered payload, for callers that only drain the transfer.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session with the DSIWrite request header parsed
* `bufp`: receives a pointer to the buffered payload, or NULL

Returns: buffered payload bytes, 0 when none, or when the header length falls below the data offset

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [write_fork](../../etc/afpd/fork.c.md#write_fork)

### dsi_write

```c
size_t dsi_write(DSI *, void *, const size_t)
```

Defined at lines 114 to 129. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Receive the next part of a DSIWrite payload into buf.

Each call reads min(buflen, datasize) bytes. Callers repeat it until datasize reaches 0.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session after dsi_writeinit
* `buf`: receives the bytes
* `buflen`: capacity of buf

Returns: bytes received, or 0 when nothing remains or the read failed

Calls: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [write_fork](../../etc/afpd/fork.c.md#write_fork)

Mentioned in the documentation of: [ad_recvfile](../adouble/ad_recvfile.c.md#ad_recvfile), [dsi_write_file](dsi_write.c.md#dsi_write_file)

### dsi_writeflush

```c
void dsi_writeflush(DSI *)
```

Defined at lines 139 to 153. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Drain the unread remainder of a DSIWrite payload.

The call reads the bytes into the reply buffer and drops them. A handler that rejected the write thus leaves the stream at the next request boundary.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session with a DSIWrite in progress

Calls: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [write_fork](../../etc/afpd/fork.c.md#write_fork)

Mentioned in the documentation of: [dsi_write_file](dsi_write.c.md#dsi_write_file)

### dsi_close_pipe

```c
void dsi_close_pipe(DSI *)
```

Defined at lines 160 to 167. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Close the recvfile pipe of the session, the next write opens another.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Called by: [afp_zzzzz](../../etc/afpd/auth.c.md#afp_zzzzz), [dsi_disconnect](dsi_stream.c.md#dsi_disconnect), [dsi_write_file](dsi_write.c.md#dsi_write_file), [open_pipe](dsi_write.c.md#open_pipe)

### waitfordata

```c
static int waitfordata(int socket, int timeout)
```

Defined at lines 182 to 198.

Wait up to timeout milliseconds for socket data and report a TCP urgent mark.

The splice call stops at the urgent mark even before the urgent byte arrives, and POLLPRI waits for that byte. The recv call steps over the mark.

Parameters:
* `socket`: session socket
* `timeout`: poll timeout in milliseconds, -1 to wait without limit

Returns: 1 at a TCP urgent mark, 0 otherwise, or -1 with errno set when poll failed

Called by: [dsi_write_file](dsi_write.c.md#dsi_write_file)

### open_pipe

```c
static int open_pipe(DSI *dsi)
```

Defined at lines 213 to 272.

Open the recvfile pipe of the session at the largest size the kernel grants.

The goal is splice_size, and the session keeps the pipe across writes. When the kernel refuses that size, the request halves until it succeeds or reaches the current size, and splice_size shrinks to the result. A pipe that stays at 2 pages or less marks a spent pipe budget. Such a pipe is slower than recv, so the call closes it. Each warning logs once per process.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Returns: 0, or -1 when this write takes the userspace path

Calls: [dsi_close_pipe](dsi_write.c.md#dsi_close_pipe)

Called by: [dsi_write_file](dsi_write.c.md#dsi_write_file)

### pipe_to_file

```c
static ssize_t pipe_to_file(DSI *dsi, int tofd, loff_t *offset, size_t len)
```

Defined at lines 287 to 309.

Move bytes from the recvfile pipe into a file through the reply buffer.

It serves files on filesystems that cannot splice. One read takes up to len bytes, limited by the reply buffer, and the loop writes them all with pwrite. The contract matches splice with an output offset.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session whose pipe and reply buffer to use
* `tofd`: destination file
* `offset`: file offset, advanced by the bytes written
* `len`: limit for the bytes to move

Returns: bytes moved, 0 at end of input, or -1 with errno set

Called by: [dsi_write_file](dsi_write.c.md#dsi_write_file)

### dsi_write_file

```c
ssize_t dsi_write_file(DSI *, int tofd, off_t *offset, bool *nosplice)
```

Defined at lines 327 to 435. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

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
