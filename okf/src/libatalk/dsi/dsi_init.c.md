---
type: C Source File
title: "libatalk/dsi/dsi_init.c"
description: "DSI listener creation."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_init.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 36 to 58. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Allocate a [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener and bind it to the TCP transport.

The attention quantum starts at DSI_DEFQUANT, and the server quantum, read-ahead buffer size, and recvfile pipe size come from the [AFPObj](../../include/atalk/globals.h.md#struct-afpobj) options. The recvfile pipe stays closed until first use. dsi_tcp_init creates the listening socket, and its failure frees the structure again.

Parameters:
* `obj`: server object supplying the options
* `hostname`: server host name
* `address`: address to listen on, or NULL for the wildcard address
* `port`: TCP port to listen on

Returns: the listener, or NULL when allocation or the transport setup failed

Calls: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

Called by: [configinit](../../etc/afpd/afp_config.c.md#configinit)
