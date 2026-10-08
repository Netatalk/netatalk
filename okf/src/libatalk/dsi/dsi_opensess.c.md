---
type: C Source File
title: "libatalk/dsi/dsi_opensess.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_opensess.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 20 to 85. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

OpenSession. set up the connection

Calls: [setnonblock](../util/socket.c.md#setnonblock)

Called by: [dsi_getsession](dsi_getsess.c.md#dsi_getsession)
