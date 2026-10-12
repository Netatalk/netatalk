---
type: C Source File
title: "libatalk/asp/asp_getreq.c"
description: "Reading an ASP request from the session socket."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_getreq.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `string.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### asp_getrequest

```c
int asp_getrequest(ASP)
```

Defined at lines 60 to 112. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Read one [ASP](../../include/atalk/asp.h.md#struct-asp) request from the session socket without blocking.

The call reads no more than one datagram. ASP_NOREQUEST means that a datagram went by without a usable request, and the caller waits for the socket to turn readable again. A request shorter than the [ASP](../../include/atalk/asp.h.md#struct-asp) header, or a command or write without a payload, counts as no request. Every function except ASPFUNC_CLOSE has to carry the next sequence number.

The session ID and the function byte compare as unsigned bytes. A session ID of 0x80 or above therefore matches, and a function byte from 0xFD to 0xFF never reads as an error code.

Parameters:
* `asp`: session, receives the request in cmdbuf and its payload length in cmdlen

Returns: the [ASP](../../include/atalk/asp.h.md#struct-asp) function byte, ASP_NOREQUEST, ASP_ERR_READ, ASP_ERR_SEQ for a wrong sequence number, or ASP_ERR_SID for another session

Calls: [atp_rreq_try](../atp/atp_rreq.c.md#atp_rreq_try)

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp)
