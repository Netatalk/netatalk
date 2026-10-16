---
type: C Header File
title: "etc/afpd/status.h"
description: "AFP server status construction and server information interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/status.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afp_config.h](afp_config.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)

# Included by

* [etc/afpd/afp_config.c](afp_config.c.md)
* [etc/afpd/afp_options.c](afp_options.c.md)
* [etc/afpd/auth.c](auth.c.md)
* [etc/afpd/main.c](main.c.md)
* [etc/afpd/status.c](status.c.md)
* [etc/afpd/switch.c](switch.c.md)

# Macros

* `AFPSTATUS_MACHLEN`: The number of characters for the MachineType.
* `AFPSTATUS_POSTLEN`: The number of bytes for offset records after the ServerName field.
* `AFPSTATUS_PRELEN`: The number of bytes for status data prior to the ServerName field.
* Undocumented: `AFPSTATUS_FLAGOFF`, `AFPSTATUS_ICONOFF`, `AFPSTATUS_LEN`, `AFPSTATUS_MACHOFF`, `AFPSTATUS_UAMSOFF`, `AFPSTATUS_VERSOFF`
