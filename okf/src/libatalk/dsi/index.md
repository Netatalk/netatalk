# src/libatalk/dsi

* [dsi_attn.c](dsi_attn.c.md) - DSIAttention requests.
* [dsi_close.c](dsi_close.c.md) - Closing a DSI session with a DSICloseSession request to the client.
* [dsi_cmdreply.c](dsi_cmdreply.c.md) - DSICommand replies.
* [dsi_getsess.c](dsi_getsess.c.md) - The DSI listener: accepting sessions, and refusing them at the session limit or out of descriptors.
* [dsi_getstat.c](dsi_getstat.c.md) - DSIGetStatus replies with the cached server status block.
* [dsi_init.c](dsi_init.c.md) - DSI listener creation.
* [dsi_opensess.c](dsi_opensess.c.md) - Answering DSIOpenSession and recording the client options.
* [dsi_read.c](dsi_read.c.md) - Streamed FPRead replies over DSI.
* [dsi_stream.c](dsi_stream.c.md) - DSI stream input and output on the session socket.
* [dsi_tcp.c](dsi_tcp.c.md) - DSI over TCP: listening, accepting, and session sockets.
* [dsi_tickle.c](dsi_tickle.c.md) - DSITickle requests.
* [dsi_write.c](dsi_write.c.md) - DSIWrite payload reception, with the recvfile pipe path.
