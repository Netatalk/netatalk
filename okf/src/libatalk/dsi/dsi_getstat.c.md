---
type: C Source File
title: "libatalk/dsi/dsi_getstat.c"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_getstat.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* System headers: `arpa/inet.h`, `stdio.h`, `string.h`

# Functions

### dsi_getstatus

```c
void dsi_getstatus(DSI *)
```

Defined at lines 24 to 34. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Reply to a DSIGetStatus request with the cached server status block.

The reply reuses the request header and sends the statuslen bytes of status through the command buffer.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session holding the status block

Called by: [dsi_getsession](dsi_getsess.c.md#dsi_getsession)
