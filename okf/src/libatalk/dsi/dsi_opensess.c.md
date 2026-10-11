---
type: C Source File
title: "libatalk/dsi/dsi_opensess.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_opensess.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `stdio.h`, `stdlib.h`, `string.h`, `sys/types.h`

# Functions

### dsi_opensession

```c
void dsi_opensession(DSI *)
```

Defined at lines 31 to 96. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Answer a DSIOpenSession request and record the client options.

The socket becomes nonblocking first, and a failure there aborts the process. The option list in the command buffer may carry DSIOPT_ATTNQUANT, and that sets the attention quantum. Other options pass unread, and a malformed option length ends the process with EXITERR_CLNT. The reply announces the server quantum and the replay cache size as DSIOPT_SERVQUANT and DSIOPT_REPLCSIZE options.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session with the request in its command buffer

Calls: [setnonblock](../util/socket.c.md#setnonblock)

Called by: [dsi_getsession](dsi_getsess.c.md#dsi_getsession)
