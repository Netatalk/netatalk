---
type: Subsystem
title: "libatalk/adouble"
description: "11 files, 110 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/adouble"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
---

# Files

* [libatalk/adouble/ad_attr.c](adouble/ad_attr.c.md): 6 functions, includes 4 project headers.
* [libatalk/adouble/ad_conv.c](adouble/ad_conv.c.md): Part of Netatalk's AppleDouble implementatation.
* [libatalk/adouble/ad_date.c](adouble/ad_date.c.md): 3 functions, includes 2 project headers.
* [libatalk/adouble/ad_flush.c](adouble/ad_flush.c.md): 10 functions, includes 6 project headers.
* [libatalk/adouble/ad_lock.c](adouble/ad_lock.c.md): 21 functions, includes 5 project headers.
* [libatalk/adouble/ad_open.c](adouble/ad_open.c.md): Part of Netatalk's AppleDouble implementatation.
* [libatalk/adouble/ad_read.c](adouble/ad_read.c.md): 2 functions, includes 4 project headers.
* [libatalk/adouble/ad_recvfile.c](adouble/ad_recvfile.c.md): 1 function, includes 2 project headers.
* [libatalk/adouble/ad_sendfile.c](adouble/ad_sendfile.c.md): 1 function, includes 2 project headers.
* [libatalk/adouble/ad_size.c](adouble/ad_size.c.md): 1 function, includes 2 project headers.
* [libatalk/adouble/ad_write.c](adouble/ad_write.c.md): 7 functions, includes 5 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 49 includes

# Calls into

* [libatalk/util](util.md): 28 calls
* [libatalk/compat](compat.md): 8 calls
* [libatalk/vfs](vfs.md): 8 calls
* [include/atalk](../include/atalk.md): 7 calls
* [libatalk/dsi](dsi.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 188 calls
* [bin/nad](../bin/nad.md): 63 calls
* [bin/dbd](../bin/dbd.md): 19 calls
* [libatalk/vfs](vfs.md): 17 calls
* [libatalk/dsi](dsi.md): 1 calls

# Most called functions

* [ad_close](adouble/ad_flush.c.md#ad_close): 44 callers
* [ad_init](adouble/ad_open.c.md#ad_init): 41 callers
* [ad_open](adouble/ad_open.c.md#ad_open): 36 callers
* [ad_flush](adouble/ad_flush.c.md#ad_flush): 35 callers
* [ad_getentryoff](adouble/ad_open.c.md#ad_getentryoff): 26 callers
* [ad_setid](adouble/ad_attr.c.md#ad_setid): 15 callers
* [ad_setname](adouble/ad_attr.c.md#ad_setname): 13 callers
* [ad_getattr](adouble/ad_attr.c.md#ad_getattr): 11 callers
* [ad_setdate](adouble/ad_date.c.md#ad_setdate): 10 callers
* [adflags2logstr](adouble/ad_open.c.md#adflags2logstr): 8 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[ad_entry](adouble/ad_open.c.md#ad_entry), [ad_getfuid](adouble/ad_open.c.md#ad_getfuid)
