---
type: C Source File
title: "etc/papd/session.c"
description: "1 function, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/session.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 49 to 408.

Accept files until the client closes the connection. Read lines of a file, until the client sends eof, after which we'll send eof also.

Calls: [append](file.c.md#append), [atp_rreq](../../libatalk/atp/atp_rreq.c.md#atp_rreq), [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_rsel](../../libatalk/atp/atp_rsel.c.md#atp_rsel), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [atp_sresp](../../libatalk/atp/atp_sresp.c.md#atp_sresp), [lp_cancel](lp.c.md#lp_cancel), [lp_print](lp.c.md#lp_print), [markline](file.c.md#markline), [ps](magics.c.md#ps)

Called by: [main](main.c.md#main)

Uses file-scope variables: `connid` in [etc/papd/main.c](main.c.md), `niov`, `oquantum`, `quantum`

# File-scope variables

`buf`, `niov`, `oquantum`, `quantum`
