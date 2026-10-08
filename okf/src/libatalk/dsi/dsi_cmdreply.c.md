---
type: C Source File
title: "libatalk/dsi/dsi_cmdreply.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_cmdreply.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `arpa/inet.h`, `stdio.h`

# Functions

### dsi_cmdreply

```c
int dsi_cmdreply(DSI *, const int)
```

Defined at lines 19 to 31. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Calls: [dsi_stream_send](dsi_stream.c.md#dsi_stream_send)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [handle_transfer_session](../../etc/afpd/afp_dsi.c.md#handle_transfer_session)
