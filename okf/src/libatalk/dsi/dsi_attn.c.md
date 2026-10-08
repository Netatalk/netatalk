---
type: C Source File
title: "libatalk/dsi/dsi_attn.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_attn.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 25 to 56. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

send an attention.

Note: this may get called at any time, so we can't use [DSI](../../include/atalk/dsi.h.md#struct-dsi) buffers to send one.

Returns: 0 on error

Calls: [dsi_stream_write](dsi_stream.c.md#dsi_stream_write)

Called by: [afp_dsi_die](../../etc/afpd/afp_dsi.c.md#afp_dsi_die), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [handle_getmesg](../../etc/afpd/afp_dsi.c.md#handle_getmesg), [handle_timedown](../../etc/afpd/afp_dsi.c.md#handle_timedown), [pending_request](../../etc/afpd/afp_dsi.c.md#pending_request)
