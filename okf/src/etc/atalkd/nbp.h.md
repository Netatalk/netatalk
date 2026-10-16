---
type: C Header File
title: "etc/atalkd/nbp.h"
description: "NBP registration table and packet handler interface for atalkd."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/atalkd/nbp.h"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [etc/atalkd](../atalkd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [etc/atalkd/main.c](main.c.md)
* [etc/atalkd/nbp.c](nbp.c.md)

# Types

### struct nbptab

Defined at line 14.
* `struct nbptab * nt_prev`
* `struct nbptab * nt_next`
* `struct nbpnve nt_nve`
* `struct interface * nt_iface`
