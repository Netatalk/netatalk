---
type: Subsystem
title: "etc/atalkd"
description: "18 files, 50 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/etc/atalkd"
tags: ["etc/atalkd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [etc/atalkd/aep.c](atalkd/aep.c.md): 1 function, includes 4 project headers.
* [etc/atalkd/atserv.h](atalkd/atserv.h.md): 2 types.
* [etc/atalkd/config.c](atalkd/config.c.md): 13 functions, 1 type, includes 11 project headers.
* [etc/atalkd/gate.h](atalkd/gate.h.md): 1 type.
* [etc/atalkd/interface.h](atalkd/interface.h.md): 1 type.
* [etc/atalkd/list.h](atalkd/list.h.md): 1 type.
* [etc/atalkd/main.c](atalkd/main.c.md): 11 functions, includes 16 project headers.
* [etc/atalkd/main.h](atalkd/main.h.md): No functions or types.
* [etc/atalkd/multicast.c](atalkd/multicast.c.md): 3 functions, includes 7 project headers.
* [etc/atalkd/multicast.h](atalkd/multicast.h.md): includes 1 project header.
* [etc/atalkd/nbp.c](atalkd/nbp.c.md): 2 functions, includes 13 project headers.
* [etc/atalkd/nbp.h](atalkd/nbp.h.md): 1 type.
* [etc/atalkd/route.c](atalkd/route.c.md): 1 function, includes 3 project headers.
* [etc/atalkd/route.h](atalkd/route.h.md): No functions or types.
* [etc/atalkd/rtmp.c](atalkd/rtmp.c.md): 13 functions, includes 12 project headers.
* [etc/atalkd/rtmp.h](atalkd/rtmp.h.md): 3 types.
* [etc/atalkd/zip.c](atalkd/zip.c.md): 6 functions, includes 13 project headers.
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
* [gateroute](atalkd/rtmp.c.md#gateroute): 5 callers
* [looproute](atalkd/rtmp.c.md#looproute): 5 callers
* [bootaddr](atalkd/main.c.md#bootaddr): 4 callers
* [ifconfig](atalkd/main.c.md#ifconfig): 4 callers
* [newrt](atalkd/rtmp.c.md#newrt): 4 callers
* [setaddr](atalkd/main.c.md#setaddr): 4 callers
* [addmulti](atalkd/multicast.c.md#addmulti): 3 callers
* [newiface](atalkd/config.c.md#newiface): 3 callers
* [rtmp_delinuse](atalkd/rtmp.c.md#rtmp_delinuse): 3 callers
