---
type: C Source File
title: "libatalk/dsi/dsi_init.c"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_init.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* System headers: `signal.h`, `stdio.h`, `stdlib.h`

# Functions

### dsi_init

```c
DSI * dsi_init(AFPObj *obj, const char *hostname, const char *address, const char *port)
```

Defined at lines 17 to 37. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Calls: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

Called by: [configinit](../../etc/afpd/afp_config.c.md#configinit)
