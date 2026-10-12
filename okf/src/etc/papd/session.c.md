---
type: C Source File
title: "etc/papd/session.c"
description: "One PAP connection of papd: reading jobs and sending replies."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/session.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/pap.h](../../include/atalk/pap.h.md)
* [file.h](file.h.md)
* [lp.h](lp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [session.h](session.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `stdlib.h`, `string.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### session

```c
int session(ATP atp, struct sockaddr_at *sat)
```

Defined at lines 67 to 426.

Serve one PAP connection: read the print or query jobs and send the replies.

The call asks the client for data with PAP_READ requests and feeds each response to ps, the job loop. Replies go out while the client has a read pending, in up to quantum packets of PAP_MAXDATA bytes. A font list reply goes out a line per packet. Each silent minute brings a tickle, and the fourth cancels the job.

PAP_CLOSE prints the job when its data ended, or cancels it otherwise, and answers with PAP_CLOSEREPLY.

Parameters:
* `atp`: ATP socket of the connection
* `sat`: address of the client

Returns: 0 after a close, or -1 after a timeout, a parse error, or a transport error

Calls: [append](file.c.md#append), [atp_rreq](../../libatalk/atp/atp_rreq.c.md#atp_rreq), [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_rsel](../../libatalk/atp/atp_rsel.c.md#atp_rsel), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [atp_sresp](../../libatalk/atp/atp_sresp.c.md#atp_sresp), [lp_cancel](lp.c.md#lp_cancel), [lp_print](lp.c.md#lp_print), [markline](file.c.md#markline), [ps](magics.c.md#ps)

Called by: [main](main.c.md#main)

Uses file-scope variables: `niov`

# File-scope variables

`buf`, `niov`, `oquantum`, `quantum`
