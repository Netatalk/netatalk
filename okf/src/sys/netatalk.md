---
type: Subsystem
title: "sys/netatalk"
description: "13 files, 35 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/sys/netatalk"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

# Files

* [sys/netatalk/aarp.c](netatalk/aarp.c.md): AARP: AppleTalk address resolution in the BSD kernel stack.
* [sys/netatalk/aarp.h](netatalk/aarp.h.md): 4 types.
* [sys/netatalk/at.h](netatalk/at.h.md): 3 types.
* [sys/netatalk/at_control.c](netatalk/at_control.c.md): AppleTalk interface ioctls and address setup in the BSD kernel stack.
* [sys/netatalk/at_proto.c](netatalk/at_proto.c.md): AppleTalk protocol switch and domain in the BSD kernel stack.
* [sys/netatalk/at_var.h](netatalk/at_var.h.md): 1 type.
* [sys/netatalk/ddp.h](netatalk/ddp.h.md): 3 types, includes 1 project header.
* [sys/netatalk/ddp_input.c](netatalk/ddp_input.c.md): DDP input: packet delivery and forwarding in the BSD kernel stack.
* [sys/netatalk/ddp_output.c](netatalk/ddp_output.c.md): DDP output and routing in the BSD kernel stack.
* [sys/netatalk/ddp_usrreq.c](netatalk/ddp_usrreq.c.md): DDP socket requests and control blocks in the BSD kernel stack.
* [sys/netatalk/ddp_var.h](netatalk/ddp_var.h.md): 2 types.
* [sys/netatalk/endian.h](netatalk/endian.h.md): No functions or types.
* [sys/netatalk/phase2.h](netatalk/phase2.h.md): IEEE 802.2 Link Level Control headers, for use in conjunction with 802.

# Includes headers from

* [include/atalk](../include/atalk.md): 2 includes

# Headers included by

* [libatalk/atp](../libatalk/atp.md): 10 includes
* [etc/atalkd](../etc/atalkd.md): 7 includes
* [etc/papd](../etc/papd.md): 7 includes
* [include/atalk](../include/atalk.md): 7 includes
* [libatalk/asp](../libatalk/asp.md): 6 includes
* [libatalk/nbp](../libatalk/nbp.md): 5 includes
* [bin/nad](../bin/nad.md): 4 includes
* [bin/nbp](../bin/nbp.md): 3 includes
* [bin/pap](../bin/pap.md): 2 includes
* [bin/aecho](../bin/aecho.md): 1 includes
* [bin/getzones](../bin/getzones.md): 1 includes
* [bin/rtmpqry](../bin/rtmpqry.md): 1 includes
* [etc/afpd](../etc/afpd.md): 1 includes
* [libatalk/netddp](../libatalk/netddp.md): 1 includes
* [libatalk/util](../libatalk/util.md): 1 includes

# Most called functions

* [aarptfree](netatalk/aarp.c.md#aarptfree): 3 callers
* [at_ifawithnet](netatalk/aarp.c.md#at_ifawithnet): 3 callers
* [aarptimer](netatalk/aarp.c.md#aarptimer): 2 callers
* [aarptnew](netatalk/aarp.c.md#aarptnew): 2 callers
* [at_pcbdetach](netatalk/ddp_usrreq.c.md#at_pcbdetach): 2 callers
* [at_pcbsetaddr](netatalk/ddp_usrreq.c.md#at_pcbsetaddr): 2 callers
* [at_scrub](netatalk/at_control.c.md#at_scrub): 2 callers
* [ddp_route](netatalk/ddp_output.c.md#ddp_route): 2 callers
* [aarpprobe](netatalk/aarp.c.md#aarpprobe): 1 callers
* [aarpwhohas](netatalk/aarp.c.md#aarpwhohas): 1 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[aa_clean](netatalk/at_control.c.md#aa_clean), [aarp_clean](netatalk/aarp.c.md#aarp_clean), [aarpinput](netatalk/aarp.c.md#aarpinput), [aarpresolve](netatalk/aarp.c.md#aarpresolve), [atalk_hash](netatalk/at_control.c.md#atalk_hash), [atalk_netmatch](netatalk/at_control.c.md#atalk_netmatch), [atintr](netatalk/ddp_input.c.md#atintr), [ddp_clean](netatalk/ddp_usrreq.c.md#ddp_clean), [m_printm](netatalk/ddp_input.c.md#m_printm)
