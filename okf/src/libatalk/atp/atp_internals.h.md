---
type: C Header File
title: "libatalk/atp/atp_internals.h"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/atp/atp_internals.h"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/atp](../atp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `sys/types.h`

# Included by

* [libatalk/atp/atp_bufs.c](atp_bufs.c.md)
* [libatalk/atp/atp_close.c](atp_close.c.md)
* [libatalk/atp/atp_open.c](atp_open.c.md)
* [libatalk/atp/atp_packet.c](atp_packet.c.md)
* [libatalk/atp/atp_rreq.c](atp_rreq.c.md)
* [libatalk/atp/atp_rresp.c](atp_rresp.c.md)
* [libatalk/atp/atp_rsel.c](atp_rsel.c.md)
* [libatalk/atp/atp_sreq.c](atp_sreq.c.md)
* [libatalk/atp/atp_sresp.c](atp_sresp.c.md)

# Functions

### atp_print_bufuse

```c
void atp_print_bufuse(ATP, char *)
```

Declared at libatalk/atp/atp_internals.h line 39; no definition in the scanned sources.

Called by: [atp_close](atp_close.c.md#atp_close), [atp_recv_atp](atp_packet.c.md#atp_recv_atp), [atp_rreq_try](atp_rreq.c.md#atp_rreq_try), [atp_rresp](atp_rresp.c.md#atp_rresp), [atp_rsel](atp_rsel.c.md#atp_rsel), [atp_sreq](atp_sreq.c.md#atp_sreq), [atp_sresp](atp_sresp.c.md#atp_sresp)

# Macros

* Undocumented: `ATP_FUNCANY`, `ATP_TIDANY`
