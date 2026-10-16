---
type: C Source File
title: "libatalk/dsi/dsi_stream.c"
description: "DSI stream input and output on the session socket."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_stream.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `limits.h`, `poll.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/stat.h`, `sys/types.h`, `sys/uio.h`, `time.h`, `unistd.h`

# Functions

### dsi_header_pack_reply

```c
static void dsi_header_pack_reply(const DSI *dsi, char *buf)
```

Defined at lines 77 to 86.

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

Defined at lines 101 to 190.

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

Defined at lines 203 to 230.

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

Defined at lines 245 to 269.

Read bytes from the read-ahead buffer, or from the socket when it is empty.

A non-empty buffer returns only the bytes it holds, up to count. Only an empty buffer leads to a socket read, which waits for input through [dsi_wait_input()](dsi_stream.c.md#dsi_wait_input).

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `buf`: receives the bytes
* `count`: capacity of buf

Returns: bytes read, 0 for a zero count or end of file, or -1 with errno set

Calls: [dsi_wait_input](dsi_stream.c.md#dsi_wait_input), [from_buf](dsi_stream.c.md#from_buf)

Called by: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

### dsi_buffered_stream_read

```c
static size_t dsi_buffered_stream_read(DSI *dsi, uint8_t *data, const size_t length)
```

Defined at lines 283 to 318.

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

Defined at lines 326 to 329.

Mark the start of a multi-packet write, which no tickle, attention or CloseSession frame may interleave.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Called by: [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

### unblock_sig

```c
static void unblock_sig(DSI *dsi)
```

Defined at lines 336 to 339.

Mark the end of a multi-packet write that block_sig started.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Called by: [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

### monotonic_ms

```c
static long long monotonic_ms(void)
```

Defined at lines 346 to 351.

Read the monotonic clock.

Returns: milliseconds since an arbitrary start

Called by: [dsi_wait_input](dsi_stream.c.md#dsi_wait_input)

### dsi_disconnect

```c
int dsi_disconnect(DSI *dsi)
```

Defined at lines 370 to 386. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Enter the disconnected state after a communication error with the client.

The call closes the transport and the recvfile pipe, clears the sleep flags, and sets DSI_DISCONNECTED. A session that never logged in reports failure. login refuses uid 0 and stores the uid on success, so a zero uid means no login. The UAM logout hook is no usable mark, because a UAM may register none. A call on a session already disconnected logs nothing.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Returns: 0, or -1 when no user logged in to the session

Calls: [dsi_close_pipe](dsi_write.c.md#dsi_close_pipe)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [dsi_wait_input](dsi_stream.c.md#dsi_wait_input), [handle_alarm](../../etc/afpd/afp_dsi.c.md#handle_alarm)

Calls through [`DSI::proto_close`](../../include/atalk/dsi.h.md#struct-dsi): no table assigns this field

### dsi_wait_input

```c
int dsi_wait_input(DSI *dsi)
```

Defined at lines 406 to 455. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Wait until the client sends more of the request being read.

The tickle timer that disconnects a client silent for the timeout option's count of tickleval intervals runs in the main loop, which a request being read never reaches, so the wait applies the same bound from the last bytes received; past it the session enters the disconnected state, from which the client can reconnect. The flag stop_wait points to, set for a reconnect's hand-over, ends the wait at once the same way, as the client has left the connection. Other signals leave the wait waiting, so a termination lets a request whose client still sends finish. A session without its options waits without a bound.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Returns: 0 once the socket has input; -1 with errno ETIMEDOUT at the bound or ECONNABORTED for a hand-over, both after the session disconnected, or with the errno of poll

Calls: [atalk_sigpipe_drain](../util/sigpipe.c.md#atalk_sigpipe_drain), [atalk_sigpipe_readfd](../util/sigpipe.c.md#atalk_sigpipe_readfd), [dsi_disconnect](dsi_stream.c.md#dsi_disconnect), [monotonic_ms](dsi_stream.c.md#monotonic_ms)

Called by: [buf_read](dsi_stream.c.md#buf_read), [dsi_write_file](dsi_write.c.md#dsi_write_file)

Mentioned in the documentation of: [buf_read](dsi_stream.c.md#buf_read)

### dsi_stream_write

```c
ssize_t dsi_stream_write(DSI *, void *, const size_t, const int mode)
```

Defined at lines 472 to 528. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Send length raw bytes to the client socket.

The loop retries after EINTR. On EAGAIN it reads pending input through dsi_peek and then writes again. That keeps a client that itself blocks on sending from deadlocking the session. DSI_NOWAIT gives up at once when nothing went out yet, so an attention or a tickle never waits on a full socket. in_write rises for the duration of the call.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `data`: bytes to send
* `length`: bytes to send
* `mode`: 0 or DSI_NOWAIT

Returns: bytes sent, or -1 for a disconnected session or a failed send

Calls: [dsi_peek](dsi_stream.c.md#dsi_peek)

Called by: [dsi_attention](dsi_attn.c.md#dsi_attention), [dsi_read](dsi_read.c.md#dsi_read), [dsi_stream_copy_file](dsi_stream.c.md#dsi_stream_copy_file), [dsi_stream_send](dsi_stream.c.md#dsi_stream_send), [dsi_tickle](dsi_tickle.c.md#dsi_tickle)

### sendfile_call

```c
static int sendfile_call(const DSI *dsi, int fromfd, off_t pos, size_t count, char *hdr, size_t hdrlen, size_t *sent)
```

Defined at lines 547 to 595.

Make one sendfile call: the unsent header bytes, then file bytes.

Linux sendfile takes no header, so the header goes out in a send of its own, held back by MSG_MORE until the file data follows. sent counts header bytes first, and is set after EAGAIN and EINTR too.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session whose socket to send on
* `fromfd`: file to send from
* `pos`: file offset of the first file byte
* `count`: file bytes to send at most
* `hdr`: unsent header bytes
* `hdrlen`: bytes at hdr, 0 once the header is out
* `sent`: bytes the call put on the socket

Returns: 0, or -1 with errno set

Called by: [dsi_stream_read_file](dsi_stream.c.md#dsi_stream_read_file)

### dsi_stream_copy_file

```c
static int dsi_stream_copy_file(DSI *dsi, int fromfd, off_t pos, size_t *left, char *hdr, size_t hdrlen)
```

Defined at lines 612 to 647.

Send the rest of a read reply through the request buffer.

The unsent header bytes go first, then the file bytes from pos a session quantum at a time, until left reaches 0 or the file ends.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session whose socket and request buffer to use
* `fromfd`: file to send from
* `pos`: file offset of the next file byte
* `left`: file bytes the reply still owes, reduced by those sent
* `hdr`: unsent header bytes
* `hdrlen`: bytes at hdr

Returns: 0, or -1 with errno set, which stays EAGAIN when the client ends the stream during a wait

Calls: [dsi_stream_write](dsi_stream.c.md#dsi_stream_write)

Called by: [dsi_stream_read_file](dsi_stream.c.md#dsi_stream_read_file)

### dsi_stream_read_file

```c
int dsi_stream_read_file(DSI *, int, off_t off, const size_t len, const int err)
```

Defined at lines 672 to 771. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Send a streamed FPRead reply whose payload comes from a file through sendfile.

The reply header carries length and the result code err. Each sendfile_call sends header bytes before file bytes; the loop retries after EINTR and EAGAIN, reading pending input through dsi_peek, and stops at a call that sends nothing or an EINVAL at the end of the file. A file the kernel will not send from (EINVAL, ENOSYS, EOPNOTSUPP) goes through the request buffer instead. DSI_NOREPLY marks the reply as sent. A failure leaves in_write raised, as dsi_readinit does, so no tickle, attention or CloseSession frame follows the broken reply.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `fromfd`: file to send from
* `offset`: file offset to start at
* `length`: payload bytes to send, at least one and at most UINT32_MAX, the [DSI](../../include/atalk/dsi.h.md#struct-dsi) header's 32-bit length
* `err`: AFP result code

Returns: 0, or -1 with errno set: ENOTCONN for a disconnected session, EIO when the file ends first, or the error of the failed call, which stays EAGAIN or EINTR when the client ends the stream during a wait

Calls: [dsi_header_pack_reply](dsi_stream.c.md#dsi_header_pack_reply), [dsi_peek](dsi_stream.c.md#dsi_peek), [dsi_stream_copy_file](dsi_stream.c.md#dsi_stream_copy_file), [log_backoff](../util/unix.c.md#log_backoff), [sendfile_call](dsi_stream.c.md#sendfile_call)

Called by: [read_fork](../../etc/afpd/fork.c.md#read_fork)

### dsi_stream_read

```c
size_t dsi_stream_read(DSI *, void *, const size_t)
```

Defined at lines 787 to 826. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

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

Defined at lines 841 to 912. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

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

Defined at lines 922 to 1022. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Read [DSI](../../include/atalk/dsi.h.md#struct-dsi) command and data.

Parameters:
* `dsi`: (rw) [DSI](../../include/atalk/dsi.h.md#struct-dsi) handle

Returns: [DSI](../../include/atalk/dsi.h.md#struct-dsi) function on success, 0 on failure

Calls: [dsi_buffered_stream_read](dsi_stream.c.md#dsi_buffered_stream_read), [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)
