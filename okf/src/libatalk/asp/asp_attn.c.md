---
type: C Source File
title: "libatalk/asp/asp_attn.c"
description: "1 function, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_attn.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `errno.h`, `stdio.h`, `string.h`, `sys/socket.h`, `sys/types.h`, `sys/uio.h`

# Functions

### asp_attention

```c
int asp_attention(ASP, AFPUserBytes)
```

Defined at lines 24 to 58. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Calls: [atp_rresp](../atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../atp/atp_sreq.c.md#atp_sreq)

Called by: [afp_asp_die_now](../../etc/afpd/afp_asp.c.md#afp_asp_die_now), [afp_asp_timedown_now](../../etc/afpd/afp_asp.c.md#afp_asp_timedown_now), [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp)
