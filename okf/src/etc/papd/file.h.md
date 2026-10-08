---
type: C Header File
title: "etc/papd/file.h"
description: "1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/file.h"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `sys/types.h`

# Included by

* [etc/papd/file.c](file.c.md)
* [etc/papd/headers.c](headers.c.md)
* [etc/papd/lp.c](lp.c.md)
* [etc/papd/lp.h](lp.h.md)
* [etc/papd/magics.c](magics.c.md)
* [etc/papd/queries.c](queries.c.md)
* [etc/papd/session.c](session.c.md)
* [etc/papd/uam_auth.h](uam_auth.h.md)

# Types

### struct papfile

Defined at line 11.
* `int pf_state`
* `struct state * pf_xstate`
* `int pf_bufsize`
* `int pf_datalen`
* `char * pf_buf`
* `char * pf_data`
* `int origin`

# Macros

* Undocumented: `CONSUME`, `PF_BOT`, `PF_EOF`, `PF_FONT_QUERY`, `PF_MORESPACE`, `PF_QUERY`, `PF_STW`, `PF_TRANSLATE`
