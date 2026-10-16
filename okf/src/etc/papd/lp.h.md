---
type: C Header File
title: "etc/papd/lp.h"
description: "Print job submission, queue control and spool I/O interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/lp.h"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [file.h](file.h.md)
* System headers: `sys/socket.h`, `sys/types.h`

# Included by

* [etc/papd/headers.c](headers.c.md)
* [etc/papd/lp.c](lp.c.md)
* [etc/papd/magics.c](magics.c.md)
* [etc/papd/queries.c](queries.c.md)
* [etc/papd/session.c](session.c.md)

# Functions

### lp_pagecost

```c
int lp_pagecost(void)
```

Declared at etc/papd/lp.h line 14; no definition in the scanned sources.

Called by: [gq_pagecost](queries.c.md#gq_pagecost)

### lp_rmjob

```c
int lp_rmjob(int)
```

Declared at etc/papd/lp.h line 19; no definition in the scanned sources.

### lp_queue

```c
int lp_queue(struct papfile *)
```

Declared at etc/papd/lp.h line 20; no definition in the scanned sources.
