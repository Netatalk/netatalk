---
type: Subsystem
title: "etc/atalkd"
description: "18 files, 50 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/etc/atalkd"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

# Files

* [etc/atalkd/aep.c](atalkd/aep.c.md): AppleTalk Echo Protocol replies of atalkd.
* [etc/atalkd/atserv.h](atalkd/atserv.h.md): AppleTalk service sockets and packet dispatch callback types.
* [etc/atalkd/config.c](atalkd/config.c.md): Reading and writing atalkd.conf, with the kernel interface list as the fallback.
* [etc/atalkd/gate.h](atalkd/gate.h.md): AppleTalk gateway and route list types for atalkd.
* [etc/atalkd/interface.h](atalkd/interface.h.md): AppleTalk interface state, flags and allocation interface.
* [etc/atalkd/list.h](atalkd/list.h.md): Doubly linked list node type for atalkd routing tables.
* [etc/atalkd/main.c](atalkd/main.c.md): The atalkd daemon: startup, interface setup, timers, and signals.
* [etc/atalkd/main.h](atalkd/main.h.md): Interface configuration helpers and daemon state for atalkd.
* [etc/atalkd/multicast.c](atalkd/multicast.c.md): Zone multicast addresses of AppleTalk interfaces.
* [etc/atalkd/multicast.h](atalkd/multicast.h.md): AppleTalk multicast membership and zone broadcast interface.
* [etc/atalkd/nbp.c](atalkd/nbp.c.md): NBP packet processing of atalkd.
* [etc/atalkd/nbp.h](atalkd/nbp.h.md): NBP registration table and packet handler interface for atalkd.
* [etc/atalkd/route.c](atalkd/route.c.md): Kernel route changes of atalkd through the route socket.
* [etc/atalkd/route.h](atalkd/route.h.md): Kernel AppleTalk route update interface for atalkd.
* [etc/atalkd/rtmp.c](atalkd/rtmp.c.md): RTMP routing table maintenance of atalkd.
* [etc/atalkd/rtmp.h](atalkd/rtmp.h.md): RTMP routing table types, flags and packet handling interface.
* [etc/atalkd/zip.c](atalkd/zip.c.md): ZIP zone table maintenance of atalkd.
* [etc/atalkd/zip.h](atalkd/zip.h.md): ZIP zone table and zone discovery interface for atalkd.

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
