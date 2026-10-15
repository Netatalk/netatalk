# DSI over TCP Transport Layer

## Signals and Writes to Client

Because AFP/TCP uses a streaming protocol, writes to the client must not interleave:
a frame goes out only between whole replies.

The DSI struct maintains an in_write counter that is incremented when entering write operations
and decremented when leaving.
dsi_tickle(), dsi_attention() and dsi_close() send their frames only while it reads 0,
and a read reply that fails leaves it raised, since nothing may follow an unfinished reply on the stream.

The signal handlers in afp_dsi.c record the signal, and the main loop sends the tickle and attentions.
dsi_attention() and dsi_tickle() build their frames in buffers of their own.

## Performance Tweaking

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

## Read-Ahead Buffering

DSI implements a read-ahead buffer to reduce the number of small reads.
The buffer size is controlled by the dsireadbuf option (default 12, meaning 12 × server quantum).
When a read would block due to EAGAIN/EWOULDBLOCK,
dsi_peek() attempts to read data from the client to break potential deadlock situations.
