---
type: Subsystem
title: "etc/atalkd"
description: "18 files, 50 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/etc/atalkd"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

# Files

* [etc/atalkd/aep.c](atalkd/aep.c.md): AppleTalk Echo Protocol replies of atalkd.
* [etc/atalkd/atserv.h](atalkd/atserv.h.md): 2 types.
* [etc/atalkd/config.c](atalkd/config.c.md): Reading and writing atalkd.conf, with the kernel interface list as the fallback.
* [etc/atalkd/gate.h](atalkd/gate.h.md): 1 type.
* [etc/atalkd/interface.h](atalkd/interface.h.md): 1 type.
* [etc/atalkd/list.h](atalkd/list.h.md): 1 type.
* [etc/atalkd/main.c](atalkd/main.c.md): The atalkd daemon: startup, interface setup, timers, and signals.
* [etc/atalkd/main.h](atalkd/main.h.md): No functions or types.
* [etc/atalkd/multicast.c](atalkd/multicast.c.md): Zone multicast addresses of AppleTalk interfaces.
* [etc/atalkd/multicast.h](atalkd/multicast.h.md): includes 1 project header.
* [etc/atalkd/nbp.c](atalkd/nbp.c.md): NBP packet processing of atalkd.
* [etc/atalkd/nbp.h](atalkd/nbp.h.md): 1 type.
* [etc/atalkd/route.c](atalkd/route.c.md): Kernel route changes of atalkd through the route socket.
* [etc/atalkd/route.h](atalkd/route.h.md): No functions or types.
* [etc/atalkd/rtmp.c](atalkd/rtmp.c.md): RTMP routing table maintenance of atalkd.
* [etc/atalkd/rtmp.h](atalkd/rtmp.h.md): 3 types.
* [etc/atalkd/zip.c](atalkd/zip.c.md): ZIP zone table maintenance of atalkd.
* [etc/atalkd/zip.h](atalkd/zip.h.md): 1 type.

# Includes headers from

* [include/atalk](../include/atalk.md): 27 includes
* [sys/netatalk](../sys/netatalk.md): 7 includes

# Calls into

* [libatalk/util](../libatalk/util.md): 12 calls
* [libatalk/compat](../libatalk/compat.md): 4 calls
* [libatalk/unicode](../libatalk/unicode.md): 3 calls

# Most called functions

* [atalkd_exit](atalkd/main.c.md#atalkd_exit): 6 callers
* [bootaddr](atalkd/main.c.md#bootaddr): 4 callers
* [gateroute](atalkd/rtmp.c.md#gateroute): 4 callers
* [ifconfig](atalkd/main.c.md#ifconfig): 4 callers
* [looproute](atalkd/rtmp.c.md#looproute): 4 callers
* [setaddr](atalkd/main.c.md#setaddr): 4 callers
* [addmulti](atalkd/multicast.c.md#addmulti): 3 callers
* [newiface](atalkd/config.c.md#newiface): 3 callers
* [newrt](atalkd/rtmp.c.md#newrt): 3 callers
* [rtmp_delinuse](atalkd/rtmp.c.md#rtmp_delinuse): 3 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[rtmp_config](atalkd/rtmp.c.md#rtmp_config), [rtmp_new](atalkd/rtmp.c.md#rtmp_new), [writeconf](atalkd/config.c.md#writeconf)
