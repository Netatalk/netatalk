# DSI over TCP Transport Layer

DSI (Data Stream Interface) frames AFP traffic over TCP. It supplies session
setup, request/reply boundaries, keep-alive tickles, attention messages, and
streaming transfer support. AFP operation semantics belong to the handlers
described in [AFP Request Processing](request-flow.md).

## Key modules

| Location | Responsibility |
| --- | --- |
| `include/atalk/dsi.h` | Session structures, wire constants, and transport API |
| `libatalk/dsi/dsi_tcp.c`, `libatalk/dsi/dsi_getsess.c`, `libatalk/dsi/dsi_opensess.c` | TCP setup, accept/fork, and negotiation |
| `libatalk/dsi/dsi_stream.c` | Framing, buffered receive, send, and deadlock avoidance |
| `libatalk/dsi/dsi_read.c`, `libatalk/dsi/dsi_write.c` | Streaming read replies and write payload consumption |
| `etc/afpd/afp_dsi.c` | Session state, dispatch, deferred signals, and timers |

## Framing and negotiation

Every DSI message has a 16-byte wire header. Packing and decoding handle
multi-byte fields explicitly; a host-layout structure must not be sent
directly. Request IDs associate replies with requests. A DSIWrite frame
also distinguishes the AFP request block from the following write data.

dsi_stream_receive() checks frame bounds and command requirements. Write
request blocks and write data have different limits, and ordinary commands
must contain an AFP function byte. Oversized lengths must be rejected rather
than silently truncated into a different request.

OpenSession advertises the negotiated server quantum and replay-cache
capacity. The quantum limits incoming write data; it is not a filesystem
block size or a universal bound on streamed read replies. Session setup
can align the quantum to transport characteristics within configured bounds
and re-clamp buffering after alignment.

## Streaming and reply serialization

dsi_stream_send() packs a header and coalesces header/payload using writev().
The caller sets `dsi_len` in network byte order to the total payload length;
the payload supplied in one send can be only its first part. The `dsi_send`
macro sets the length for an ordinary complete reply. TCP_NODELAY improves
turnaround for the many small messages exchanged by AFP sessions.

A streamed read starts with dsi_readinit(), continues through dsi_read(),
and completes with dsi_readdone(). `DSI_NOREPLY` tells the command loop that
the handler is sending the reply itself. Incoming writes use dsi_writeinit()
and dsi_write(); dsi_writeflush() drains unread payload when required.

The session's `in_write` count prevents tickles, attentions, and close frames
from interleaving with an unfinished reply. dsi_tickle(), dsi_attention(),
and dsi_close() send only when that count permits it. An incomplete failed
read reply must not be followed by a new frame inside its payload.

Timer suppression around large transfers and the write count protect
different aspects of this sequencing. Signal ownership and deferred
tickle/attention processing are described in [AFP Daemon](afpd.md).

## Receive buffering and deadlock avoidance

DSI allocates a command buffer and incoming readahead buffer per session.
The `dsireadbuf` multiplier controls readahead relative to server quantum;
its current configured default is 32. This buffers client network input,
not outgoing file data or disk read-ahead.

If the socket cannot accept more output, dsi_peek() watches both read and
write readiness. It can consume client input into readahead while waiting
to send, avoiding a deadlock in which both endpoints have full send buffers.
A full readahead buffer is therefore a transport diagnostic, rather than
proof that the filesystem needs a larger cache.

The parser bounds the multiplier and the total allocation product, including
address-space checks. Increasing both quantum and readahead multiplies
per-worker memory. Explicit socket-buffer requests are also subject to the
kernel's actual grant. See [Performance Analysis](profiling.md).

## State, tickles, and replay

Session state, tickle limits, reconnect handling, and AFP replay-cache
eligibility belong to the worker command loop described in
[AFP Daemon](afpd.md).
