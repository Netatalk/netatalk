---
type: Subsystem
title: "libatalk/dsi"
description: "12 files, 52 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/dsi"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T12:16:30+11:00 }
---

# Files

* [libatalk/dsi/dsi_attn.c](dsi/dsi_attn.c.md): DSIAttention requests.
* [libatalk/dsi/dsi_close.c](dsi/dsi_close.c.md): Closing a DSI session with a DSICloseSession request to the client.
* [libatalk/dsi/dsi_cmdreply.c](dsi/dsi_cmdreply.c.md): DSICommand replies.
* [libatalk/dsi/dsi_getsess.c](dsi/dsi_getsess.c.md): The DSI listener: accepting sessions, and refusing them at the session limit or out of descriptors.
* [libatalk/dsi/dsi_getstat.c](dsi/dsi_getstat.c.md): DSIGetStatus replies with the cached server status block.
* [libatalk/dsi/dsi_init.c](dsi/dsi_init.c.md): DSI listener creation.
* [libatalk/dsi/dsi_opensess.c](dsi/dsi_opensess.c.md): Answering DSIOpenSession and recording the client options.
* [libatalk/dsi/dsi_read.c](dsi/dsi_read.c.md): Streamed FPRead replies over DSI.
* [libatalk/dsi/dsi_stream.c](dsi/dsi_stream.c.md): DSI stream input and output on the session socket.
* [libatalk/dsi/dsi_tcp.c](dsi/dsi_tcp.c.md): DSI over TCP: listening, accepting, and session sockets.
* [libatalk/dsi/dsi_tickle.c](dsi/dsi_tickle.c.md): DSITickle requests.
* [libatalk/dsi/dsi_write.c](dsi/dsi_write.c.md): DSIWrite payload reception, with the recvfile pipe path.

# Includes headers from

* [include/atalk](../include/atalk.md): 34 includes

# Calls into

* [libatalk/util](util.md): 23 calls
* [libatalk/compat](compat.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 35 calls
* [libatalk/adouble](adouble.md): 1 calls

# Most called functions

* [dsi_attention](dsi/dsi_attn.c.md#dsi_attention): 5 callers
* [dsi_stream_read](dsi/dsi_stream.c.md#dsi_stream_read): 5 callers
* [dsi_stream_write](dsi/dsi_stream.c.md#dsi_stream_write): 5 callers
* [dsi_close_pipe](dsi/dsi_write.c.md#dsi_close_pipe): 4 callers
* [dsi_writedrain](dsi/dsi_write.c.md#dsi_writedrain): 4 callers
* [dsi_disconnect](dsi/dsi_stream.c.md#dsi_disconnect): 3 callers
* [dsi_peek](dsi/dsi_stream.c.md#dsi_peek): 3 callers
* [dsi_writeflush](dsi/dsi_write.c.md#dsi_writeflush): 3 callers
* [dsi_writeinit](dsi/dsi_write.c.md#dsi_writeinit): 3 callers
* [dsi_close_spare_fd](dsi/dsi_getsess.c.md#dsi_close_spare_fd): 2 callers
