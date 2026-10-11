---
type: C Header File
title: "sys/netatalk/ddp_var.h"
description: "2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp_var.h"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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
* `uint32_t ddps_short`: short header packets received
* `uint32_t ddps_long`: long header packets received
* `uint32_t ddps_nosum`: no checksum
* `uint32_t ddps_badsum`: bad checksum
* `uint32_t ddps_tooshort`: packet too short
* `uint32_t ddps_toosmall`: not enough data
* `uint32_t ddps_forward`: packets forwarded
* `uint32_t ddps_encap`: packets encapsulated
* `uint32_t ddps_cantforward`: packets rcvd for unreachable dest
* `uint32_t ddps_nosockspace`: no space in sockbuf for packet

# Macros

* Undocumented: `sotoddpcb`
