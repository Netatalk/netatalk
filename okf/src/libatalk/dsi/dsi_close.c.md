---
type: C Source File
title: "libatalk/dsi/dsi_close.c"
description: "Closing a DSI session with a DSICloseSession request to the client."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_close.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T12:16:30+11:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* System headers: `arpa/inet.h`, `stdio.h`, `stdlib.h`

# Functions

### dsi_close

```c
void dsi_close(DSI *)
```

Defined at lines 31 to 50. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Send a DSICloseSession request, close the transport, and free the [DSI](../../include/atalk/dsi.h.md#struct-dsi) session.

A sleeping or disconnected session gets neither the request nor the transport close, and a session whose reply is still under way gets the transport close alone. The call frees the structure in every case.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session to end, invalid afterwards

Called by: [afp_dsi_close](../../etc/afpd/afp_dsi.c.md#afp_dsi_close)

Calls through [`DSI::proto_close`](../../include/atalk/dsi.h.md#struct-dsi): no table assigns this field
