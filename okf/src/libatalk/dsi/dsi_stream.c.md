---
type: C Source File
title: "libatalk/dsi/dsi_stream.c"
description: "DSI stream input and output on the session socket."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_stream.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/types.h`, `sys/uio.h`, `unistd.h`

# Functions

### dsi_header_pack_reply

```c
static void dsi_header_pack_reply(const DSI *dsi, char *buf)
```

Defined at lines 56 to 65.

Write the session [DSI](../../include/atalk/dsi.h.md#struct-dsi) header into buf in wire format.

The fields already hold network byte order, so the call only lays them out.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session whose header to write
* `buf`: receives DSI_BLOCKSIZ bytes

Called by: [dsi_stream_read_file](dsi_stream.c.md#dsi_stream_read_file), [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

### dsi_peek

```c
static int dsi_peek(DSI *dsi)
```

Defined at lines 80 to 169.

Wait until the socket accepts writes, reading pending input meanwhile.

A blocked send can mean the client itself blocks while it sends to afpd. The call therefore selects for both directions and reads into the read-ahead buffer. Reading may unblock the client. A full read-ahead buffer logs a note once and leaves only the write test. End of file, a select failure, a read error, or a socket at FD_SETSIZE or above ends the wait.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Returns: 0 when the socket accepts writes again, -1 on failure or a closed socket

Called by: [dsi_stream_read_file](dsi_stream.c.md#dsi_stream_read_file), [dsi_stream_send](dsi_stream.c.md#dsi_stream_send), [dsi_stream_write](dsi_stream.c.md#dsi_stream_write)

### from_buf

```c
static size_t from_buf(DSI *dsi, uint8_t *buf, size_t count)
```

Defined at lines 182 to 209.

Take up to count bytes from the read-ahead buffer.

An emptied buffer rewinds to its start. The master afpd has no read-ahead buffer, so it gets 0.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `buf`: receives the bytes
* `count`: capacity of buf

Returns: bytes taken, 0 when the buffer is empty

Called by: [buf_read](dsi_stream.c.md#buf_read), [dsi_buffered_stream_read](dsi_stream.c.md#dsi_buffered_stream_read)

### buf_read

```c
static ssize_t buf_read(DSI *dsi, uint8_t *buf, size_t count)
```

Defined at lines 223 to 241.

Read bytes from the read-ahead buffer, or from the socket when it is empty.

A non-empty buffer returns only the bytes it holds, up to count. Only an empty buffer leads to a socket read through readt.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `buf`: receives the bytes
* `count`: capacity of buf

Returns: bytes read, 0 for a zero count or end of file, or -1 with errno set

Calls: [from_buf](dsi_stream.c.md#from_buf), [readt](../util/socket.c.md#readt)

Called by: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

### dsi_buffered_stream_read

```c
static size_t dsi_buffered_stream_read(DSI *dsi, uint8_t *data, const size_t length)
```

Defined at lines 255 to 290.

Read length bytes and refill the read-ahead buffer in 8192-byte chunks.

The call first drains the read-ahead buffer. When that leaves a gap, one recv tops the buffer up by at most 8192 bytes, so later small requests come from memory. The dsi_stream_read call then supplies the rest.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `data`: receives the bytes
* `length`: bytes to read

Returns: length, or 0 when the read failed

Calls: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read), [from_buf](dsi_stream.c.md#from_buf)

Called by: [dsi_stream_receive](dsi_stream.c.md#dsi_stream_receive)

### block_sig

```c
static void block_sig(DSI *dsi)
```

Defined at lines 298 to 301.

Mark the start of a multi-packet write, so signal handlers leave the socket alone.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Called by: [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

### unblock_sig

```c
static void unblock_sig(DSI *dsi)
```

Defined at lines 308 to 311.

Mark the end of a multi-packet write that block_sig started.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Called by: [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

### dsi_disconnect

```c
int dsi_disconnect(DSI *dsi)
```

Defined at lines 330 to 343. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Enter the disconnected state after a communication error with the client.

The call closes the transport and the recvfile pipe, clears the sleep flags, and sets DSI_DISCONNECTED. A session that never logged in reports failure. login refuses uid 0 and stores the uid on success, so a zero uid means no login. The UAM logout hook is no usable mark, because a UAM may register none.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Returns: 0, or -1 when no user logged in to the session

Calls: [dsi_close_pipe](dsi_write.c.md#dsi_close_pipe)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [handle_alarm](../../etc/afpd/afp_dsi.c.md#handle_alarm)

Calls through [`DSI::proto_close`](../../include/atalk/dsi.h.md#struct-dsi): no table assigns this field

### dsi_stream_write

```c
ssize_t dsi_stream_write(DSI *, void *, const size_t, const int mode)
```

Defined at lines 361 to 423. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Send length raw bytes to the client socket.

The loop retries after EINTR. On EAGAIN it reads pending input through dsi_peek and then writes again. That keeps a client that itself blocks on sending from deadlocking the session. DSI_NOWAIT gives up at once when nothing went out yet, so an attention or a tickle never waits on a full socket. in_write rises for the duration of the call.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `data`: bytes to send
* `length`: bytes to send
* `mode`: 0, DSI_NOWAIT, or DSI_MSG_MORE to hint at a following send

Returns: bytes sent, or -1 for a disconnected session or a failed send

Calls: [dsi_peek](dsi_stream.c.md#dsi_peek)

Called by: [dsi_attention](dsi_attn.c.md#dsi_attention), [dsi_read](dsi_read.c.md#dsi_read), [dsi_stream_read_file](dsi_stream.c.md#dsi_stream_read_file), [dsi_stream_send](dsi_stream.c.md#dsi_stream_send), [dsi_tickle](dsi_tickle.c.md#dsi_tickle)

### dsi_stream_read_file

```c
ssize_t dsi_stream_read_file(DSI *, int, off_t off, const size_t len, const int err)
```

Defined at lines 445 to 596. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Send a streamed FPRead reply whose payload comes from a file through sendfile.

The reply header carries length and the result code err. Solaris sends the header and the file in one sendfilev call, and FreeBSD passes the header to sendfile. Other platforms write the header first with MSG_MORE. The loop retries after EINTR and EAGAIN, reads pending input through dsi_peek meanwhile, and treats a zero-length result as a failure. DSI_NOREPLY marks the reply as sent, and in_write rises for the duration of the call.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `fromfd`: file to send from
* `offset`: file offset to start at
* `length`: payload bytes to send
* `err`: AFP result code

Returns: payload bytes sent, or -1 for a disconnected session or a failed transfer

Calls: [dsi_header_pack_reply](dsi_stream.c.md#dsi_header_pack_reply), [dsi_peek](dsi_stream.c.md#dsi_peek), [dsi_stream_write](dsi_stream.c.md#dsi_stream_write), [sys_sendfile](../adouble/ad_sendfile.c.md#sys_sendfile)

Called by: [afp_geticon](../../etc/afpd/desktop.c.md#afp_geticon), [read_fork](../../etc/afpd/fork.c.md#read_fork)

### dsi_stream_read

```c
size_t dsi_stream_read(DSI *, void *, const size_t)
```

Defined at lines 612 to 651. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Read length bytes from the read-ahead buffer and the socket.

buf_read supplies the bytes, and the loop continues after EINTR and EAGAIN until length bytes arrived. End of file or a read error ends the call with

1. A disconnected session also yields 0.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `data`: receives the bytes
* `length`: bytes to read

Returns: length, or 0 on end of file or a read error

Calls: [buf_read](dsi_stream.c.md#buf_read)

Called by: [dsi_buffered_stream_read](dsi_stream.c.md#dsi_buffered_stream_read), [dsi_stream_receive](dsi_stream.c.md#dsi_stream_receive), [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open), [dsi_write](dsi_write.c.md#dsi_write), [dsi_writeflush](dsi_write.c.md#dsi_writeflush)

### dsi_stream_send

```c
int dsi_stream_send(DSI *, void *, size_t)
```

Defined at lines 666 to 737. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Send the reply header and length payload bytes as one [DSI](../../include/atalk/dsi.h.md#struct-dsi) message.

The header comes from the session header fields. A zero length sends the header alone. Otherwise writev sends header and payload together, retries after EINTR, and reads pending input through dsi_peek on EAGAIN. block_sig holds attentions off for the duration.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session with the header fields set
* `buf`: payload bytes
* `length`: payload bytes to send, matching dsi_len in the header

Returns: 1, or 0 for a disconnected session or a failed send

Calls: [block_sig](dsi_stream.c.md#block_sig), [dsi_header_pack_reply](dsi_stream.c.md#dsi_header_pack_reply), [dsi_peek](dsi_stream.c.md#dsi_peek), [dsi_stream_write](dsi_stream.c.md#dsi_stream_write), [unblock_sig](dsi_stream.c.md#unblock_sig)

Called by: [dsi_cmdreply](dsi_cmdreply.c.md#dsi_cmdreply), [dsi_readinit](dsi_read.c.md#dsi_readinit)

### dsi_stream_receive

```c
int dsi_stream_receive(DSI *)
```

Defined at lines 747 to 847. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Read [DSI](../../include/atalk/dsi.h.md#struct-dsi) command and data.

Parameters:
* `dsi`: (rw) [DSI](../../include/atalk/dsi.h.md#struct-dsi) handle

Returns: [DSI](../../include/atalk/dsi.h.md#struct-dsi) function on success, 0 on failure

Calls: [dsi_buffered_stream_read](dsi_stream.c.md#dsi_buffered_stream_read), [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

# Macros

* Undocumented: `MSG_DONTWAIT`, `MSG_MORE`
