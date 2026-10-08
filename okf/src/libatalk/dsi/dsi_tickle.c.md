---
type: C Source File
title: "libatalk/dsi/dsi_tickle.c"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_tickle.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* System headers: `signal.h`, `stdio.h`, `string.h`, `sys/types.h`

# Functions

### dsi_tickle

```c
int dsi_tickle(DSI *)
```

Defined at lines 19 to 35. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

server generated tickles. as this is only called by the tickle handler, we don't need to block signals.

Calls: [dsi_stream_write](dsi_stream.c.md#dsi_stream_write)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [handle_alarm](../../etc/afpd/afp_dsi.c.md#handle_alarm)
