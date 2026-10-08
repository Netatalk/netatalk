---
type: C Header File
title: "sys/netatalk/ddp_var.h"
description: "2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp_var.h"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [sys/netatalk/aarp.c](aarp.c.md)
* [sys/netatalk/ddp_input.c](ddp_input.c.md)
* [sys/netatalk/ddp_output.c](ddp_output.c.md)
* [sys/netatalk/ddp_usrreq.c](ddp_usrreq.c.md)

# Types

### struct ddpcb

Defined at line 9.
* `struct sockaddr_at ddp_fsat ddp_lsat`
* `struct route ddp_route`
* `struct socket * ddp_socket`
* `struct ddpcb * ddp_prev`
* `struct ddpcb * ddp_next`
* `struct ddpcb * ddp_pprev`
* `struct ddpcb * ddp_pnext`

### struct ddpstat

Defined at line 19.
* `uint32_t ddps_short`
* `uint32_t ddps_long`
* `uint32_t ddps_nosum`
* `uint32_t ddps_badsum`
* `uint32_t ddps_tooshort`
* `uint32_t ddps_toosmall`
* `uint32_t ddps_forward`
* `uint32_t ddps_encap`
* `uint32_t ddps_cantforward`
* `uint32_t ddps_nosockspace`

# Macros

* Undocumented: `sotoddpcb`
