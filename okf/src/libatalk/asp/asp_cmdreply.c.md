---
type: C Source File
title: "libatalk/asp/asp_cmdreply.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_cmdreply.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* System headers: `string.h`, `sys/socket.h`, `sys/types.h`, `sys/uio.h`

# Functions

### asp_cmdreply

```c
int asp_cmdreply(ASP, int)
```

Defined at lines 49 to 95. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Send the reply to an [ASP](../../include/atalk/asp.h.md#struct-asp) command or write, with its result code and the reply data.

The reply data in asp->data goes out in ATP packets of up to ASP_CMDSIZ bytes. Each packet gets a 4-byte header in front, and the first header carries the result. The call moves the data in place to make room for the headers, counts the bytes, and steps the sequence number after a send.

Parameters:
* `asp`: session, its data rewritten with the headers
* `result`: AFP result code

Returns: 0, or -1 when the send fails

Calls: [atp_sresp](../atp/atp_sresp.c.md#atp_sresp)

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp)
