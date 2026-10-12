---
type: C Source File
title: "libatalk/dsi/dsi_attn.c"
description: "DSIAttention requests."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_attn.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `signal.h`, `stdio.h`, `string.h`, `sys/types.h`

# Functions

### dsi_attention

```c
int dsi_attention(DSI *, AFPUserBytes)
```

Defined at lines 39 to 70. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Send a DSIAttention request carrying flags to the client.

The attention is a separate 16-byte header plus the user bytes, limited by the attention quantum. The block lives on the stack, because a signal handler may trigger it while the [DSI](../../include/atalk/dsi.h.md#struct-dsi) buffers are in use. A sleeping session skips the send. A session in the middle of a reply refuses it, because the handler must not interleave with that write.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session
* `flags`: AFP attention user bytes, in host order

Returns: bytes sent, 1 for a sleeping session, or -1 when a reply is in progress or the send failed

Calls: [dsi_stream_write](dsi_stream.c.md#dsi_stream_write)

Called by: [afp_dsi_die](../../etc/afpd/afp_dsi.c.md#afp_dsi_die), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [handle_getmesg](../../etc/afpd/afp_dsi.c.md#handle_getmesg), [handle_timedown](../../etc/afpd/afp_dsi.c.md#handle_timedown), [pending_request](../../etc/afpd/afp_dsi.c.md#pending_request)
