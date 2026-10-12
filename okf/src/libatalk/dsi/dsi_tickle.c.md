---
type: C Source File
title: "libatalk/dsi/dsi_tickle.c"
description: "DSITickle requests."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_tickle.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 31 to 47. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Send a DSITickle request to the client.

Its callers run in the main loop, so it blocks no signals. A sleeping session or one in the middle of a reply skips the tickle and yields 1. Otherwise the result is that of dsi_stream_write, the bytes sent or -1.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Calls: [dsi_stream_write](dsi_stream.c.md#dsi_stream_write)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [handle_alarm](../../etc/afpd/afp_dsi.c.md#handle_alarm)
