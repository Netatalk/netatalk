---
type: C Source File
title: "libatalk/dsi/dsi_stream.c"
description: "13 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_stream.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 44 to 53.

Pack a [DSI](../../include/atalk/dsi.h.md#struct-dsi) header in wire format

Called by: [dsi_stream_read_file](dsi_stream.c.md#dsi_stream_read_file), [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

### dsi_peek

```c
static int dsi_peek(DSI *dsi)
```

Defined at lines 65 to 154.

Check if we can write to the [DSI](../../include/atalk/dsi.h.md#struct-dsi) socket.

afpd is sleeping too much while trying to send something. May be there's no reader or the reader is also sleeping in write, look if there's some data for us to read, hopefully it will wake up the reader so we can write again.

Returns: 0 when is possible to send again, -1 on error

Called by: [dsi_stream_read_file](dsi_stream.c.md#dsi_stream_read_file), [dsi_stream_send](dsi_stream.c.md#dsi_stream_send), [dsi_stream_write](dsi_stream.c.md#dsi_stream_write)

### from_buf

```c
static size_t from_buf(DSI *dsi, uint8_t *buf, size_t count)
```

Defined at lines 160 to 187.

Return all bytes up to count from dsi->buffer if there are any buffered there

Called by: [buf_read](dsi_stream.c.md#buf_read), [dsi_buffered_stream_read](dsi_stream.c.md#dsi_buffered_stream_read)

### buf_read

```c
static ssize_t buf_read(DSI *dsi, uint8_t *buf, size_t count)
```

Defined at lines 197 to 215.

Get bytes from buffer dsi->buffer or read from socket.

1. Check if there are bytes in the the dsi->buffer buffer.
1. Return bytes from (1) if yes. Note: this may return fewer bytes then requested in count !!
1. If the buffer was empty, read from the socket.

Calls: [from_buf](dsi_stream.c.md#from_buf), [readt](../util/socket.c.md#readt)

Called by: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Mentioned in the documentation of: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

### dsi_buffered_stream_read

```c
static size_t dsi_buffered_stream_read(DSI *dsi, uint8_t *data, const size_t length)
```

Defined at lines 223 to 258.

Get "length" bytes from buffer and/or socket.

In order to avoid frequent small reads this tries to read larger chunks (8192 bytes) into a buffer.

Calls: [dsi_stream_read](dsi_stream.c.md#dsi_stream_read), [from_buf](dsi_stream.c.md#from_buf)

Called by: [dsi_stream_receive](dsi_stream.c.md#dsi_stream_receive)

### block_sig

```c
static void block_sig(DSI *dsi)
```

Defined at lines 262 to 265.

Called by: [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

### unblock_sig

```c
static void unblock_sig(DSI *dsi)
```

Defined at lines 269 to 272.

Called by: [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

### dsi_disconnect

```c
int dsi_disconnect(DSI *dsi)
```

Defined at lines 290 to 302. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Communication error with the client, enter disconnected state.

1. close the socket
1. set the DSI_DISCONNECTED flag, remove possible sleep flags

Returns: 0 if successfully entered disconnected state

Returns: -1 if ppid is 1 which means afpd master died, or the session has not logged in: [login()](../../etc/afpd/auth.c.md#login) refuses uid 0 and sets AFPobj->uid on success, so a zero uid is no login (the UAM's logout hook is no mark, most UAMs register none)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [handle_alarm](../../etc/afpd/afp_dsi.c.md#handle_alarm)

Calls through [`DSI::proto_close`](../../include/atalk/dsi.h.md#struct-dsi): no table assigns this field

### dsi_stream_write

```c
ssize_t dsi_stream_write(DSI *, void *, const size_t, const int mode)
```

Defined at lines 312 to 374. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

write raw [DSI](../../include/atalk/dsi.h.md#struct-dsi) data

Checks against EINTR aren't necessary if all of the signals have SA_RESTART specified.

Returns: actual bytes written, -1 on error

Calls: [dsi_peek](dsi_stream.c.md#dsi_peek)

Called by: [dsi_attention](dsi_attn.c.md#dsi_attention), [dsi_read](dsi_read.c.md#dsi_read), [dsi_stream_read_file](dsi_stream.c.md#dsi_stream_read_file), [dsi_stream_send](dsi_stream.c.md#dsi_stream_send), [dsi_tickle](dsi_tickle.c.md#dsi_tickle)

### dsi_stream_read_file

```c
ssize_t dsi_stream_read_file(DSI *, int, off_t off, const size_t len, const int err)
```

Defined at lines 379 to 530. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Calls: [dsi_header_pack_reply](dsi_stream.c.md#dsi_header_pack_reply), [dsi_peek](dsi_stream.c.md#dsi_peek), [dsi_stream_write](dsi_stream.c.md#dsi_stream_write), [sys_sendfile](../adouble/ad_sendfile.c.md#sys_sendfile)

Called by: [afp_geticon](../../etc/afpd/desktop.c.md#afp_geticon), [read_fork](../../etc/afpd/fork.c.md#read_fork)

### dsi_stream_read

```c
size_t dsi_stream_read(DSI *, void *, const size_t)
```

Defined at lines 542 to 581. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Read data from [DSI](../../include/atalk/dsi.h.md#struct-dsi) buffer.

Essentially a loop around [buf_read()](dsi_stream.c.md#buf_read) to ensure "length" bytes are read from dsi->buffer and/or the socket.

Returns: length on success, some value smaller than length indicates an error

Calls: [buf_read](dsi_stream.c.md#buf_read)

Called by: [dsi_buffered_stream_read](dsi_stream.c.md#dsi_buffered_stream_read), [dsi_stream_receive](dsi_stream.c.md#dsi_stream_receive), [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open), [dsi_write](dsi_write.c.md#dsi_write), [dsi_writeflush](dsi_write.c.md#dsi_writeflush)

### dsi_stream_send

```c
int dsi_stream_send(DSI *, void *, size_t)
```

Defined at lines 589 to 660. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

write data.

Returns: 0 on failure.

Note: this assumes that dsi_len will never cause an overflow in the data buffer.

Calls: [block_sig](dsi_stream.c.md#block_sig), [dsi_header_pack_reply](dsi_stream.c.md#dsi_header_pack_reply), [dsi_peek](dsi_stream.c.md#dsi_peek), [dsi_stream_write](dsi_stream.c.md#dsi_stream_write), [unblock_sig](dsi_stream.c.md#unblock_sig)

Called by: [dsi_cmdreply](dsi_cmdreply.c.md#dsi_cmdreply), [dsi_readinit](dsi_read.c.md#dsi_readinit)

### dsi_stream_receive

```c
int dsi_stream_receive(DSI *)
```

Defined at lines 670 to 770. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Read [DSI](../../include/atalk/dsi.h.md#struct-dsi) command and data.

Parameters:
* `dsi`: (rw) [DSI](../../include/atalk/dsi.h.md#struct-dsi) handle

Returns: [DSI](../../include/atalk/dsi.h.md#struct-dsi) function on success, 0 on failure

Calls: [dsi_buffered_stream_read](dsi_stream.c.md#dsi_buffered_stream_read), [dsi_stream_read](dsi_stream.c.md#dsi_stream_read)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

# Macros

* Undocumented: `MSG_DONTWAIT`, `MSG_MORE`
