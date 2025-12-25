DSI over TCP Implementation Notes
=================================

Signals and Writes to Client
----------------------------

Because AFP/TCP uses a streaming protocol, we need to make sure that writes to the client are atomic.
Notably, signal handlers which write data can't interrupt data currently being written.
In addition, some functions get called from the signal handlers or can write partial packets.
To avoid corruption, we need to make sure that these functions DO NOT touch any common-use buffers.

The DSI struct maintains an in_write counter that is incremented when entering write operations
and decremented when leaving.
Signal handlers check this flag to determine if it's safe to write to the socket.

Signals that send data to the client (afp_dsi.c):

- SIGALRM (tickle handler)
- SIGHUP (attention)
- SIGTERM (attention)

Functions which need their own buffers: dsi_attention(), dsi_tickle()

Performance Tweaking
--------------------

Sending complete packets or the header and a partial packet to the client is handled by dsi_stream_send()
in dsi_stream.c.
dsi_stream_send() coalesces the header and data using writev().
In addition, AppleShare sessions often involve the sending and receiving of many small packets.
As a consequence, TCP_NODELAY is used to speed up the turnaround time.

Because dsi_stream_read() can send incomplete packets,
dsi_stream_send() does not use the length parameter to specify the dsi_len field in the header.
Instead, anything that uses dsi_stream_send() needs to specify dsi_len (in network byte order) before calling.
The dsi_send() macro already does this.

Functions that need to specify .dsi_len: dsi_readinit(), dsi_cmdreply()

To reduce the amount of tickles generated on a slow link,
SIGALRM is turned off for the duration of a "known" large file transfer
(i.e., dsi_read/write).

Read-Ahead Buffering
--------------------

DSI implements a read-ahead buffer to reduce the number of small reads.
The buffer size is controlled by the dsireadbuf option (default 12, meaning 12 × server quantum).
When a read would block due to EAGAIN/EWOULDBLOCK,
dsi_peek() attempts to read data from the client to break potential deadlock situations.
