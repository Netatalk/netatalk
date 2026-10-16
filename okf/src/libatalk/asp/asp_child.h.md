---
type: C Header File
title: "libatalk/asp/asp_child.h"
description: "Private ASP child session state and status flags."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_child.h"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [libatalk/asp/asp_getsess.c](asp_getsess.c.md)

# Types

### struct asp_child

Defined at line 32.
* `int ac_pid`
* `int ac_state`
* `struct sockaddr_at ac_sat`

# Macros

* Undocumented: `ACSTATE_BAD`, `ACSTATE_DEAD`, `ACSTATE_OK`
