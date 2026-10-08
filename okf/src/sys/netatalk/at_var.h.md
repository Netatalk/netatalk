---
type: C Header File
title: "sys/netatalk/at_var.h"
description: "1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/at_var.h"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [sys/netatalk/aarp.c](aarp.c.md)
* [sys/netatalk/at_control.c](at_control.c.md)
* [sys/netatalk/ddp_input.c](ddp_input.c.md)
* [sys/netatalk/ddp_output.c](ddp_output.c.md)
* [sys/netatalk/ddp_usrreq.c](ddp_usrreq.c.md)

# Types

### struct at_ifaddr

Defined at line 14.
* `struct ifaddr aa_ifa`
* `int aa_flags`
* `unsigned short aa_firstnet`
* `unsigned short aa_lastnet`
* `int aa_probcnt`
* `struct at_ifaddr * aa_next`

# Macros

* Undocumented: `AA_SAT`, `AFA_PHASE2`, `AFA_PROBING`, `AFA_ROUTE`, `aa_addr`, `aa_broadaddr`, `aa_dstaddr`, `aa_ifp`, `satosat`
