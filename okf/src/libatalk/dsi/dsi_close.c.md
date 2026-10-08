---
type: C Source File
title: "libatalk/dsi/dsi_close.c"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_close.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 16 to 30. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Called by: [afp_dsi_close](../../etc/afpd/afp_dsi.c.md#afp_dsi_close)

Calls through [`DSI::proto_close`](../../include/atalk/dsi.h.md#struct-dsi): no table assigns this field
