---
type: Subsystem
title: "libatalk/adouble"
description: "10 files, 116 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/adouble"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-31T16:21:52+11:00 }
---

# Files

* [libatalk/adouble/ad_attr.c](adouble/ad_attr.c.md): AFP attributes, CNID, and name entries of an AppleDouble.
* [libatalk/adouble/ad_conv.c](adouble/ad_conv.c.md): Converting AppleDouble metadata and resource forks to the EA layout.
* [libatalk/adouble/ad_date.c](adouble/ad_date.c.md): Date entries of an AppleDouble.
* [libatalk/adouble/ad_flush.c](adouble/ad_flush.c.md): Writing AppleDouble headers to disk and closing the forks.
* [libatalk/adouble/ad_lock.c](adouble/ad_lock.c.md): Byte range and share mode locks on AppleDouble forks.
* [libatalk/adouble/ad_open.c](adouble/ad_open.c.md): Opening and creating AppleDouble forks and reading metadata headers.
* [libatalk/adouble/ad_read.c](adouble/ad_read.c.md): Reading the forks of an AppleDouble.
* [libatalk/adouble/ad_recvfile.c](adouble/ad_recvfile.c.md): Receiving a DSIWrite payload straight into an AppleDouble fork.
* [libatalk/adouble/ad_size.c](adouble/ad_size.c.md): Fork lengths of an AppleDouble.
* [libatalk/adouble/ad_write.c](adouble/ad_write.c.md): Writing and truncating the forks of an AppleDouble.

# Includes headers from

* [include/atalk](../include/atalk.md): 48 includes

# Calls into

* [libatalk/util](util.md): 29 calls
* [libatalk/vfs](vfs.md): 11 calls
* [include/atalk](../include/atalk.md): 9 calls
* [libatalk/compat](compat.md): 8 calls
* [libatalk/dsi](dsi.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 219 calls
* [bin/nad](../bin/nad.md): 71 calls
* [bin/dbd](../bin/dbd.md): 19 calls
* [libatalk/vfs](vfs.md): 17 calls

# Most called functions

* [ad_close](adouble/ad_flush.c.md#ad_close): 45 callers
* [ad_init](adouble/ad_open.c.md#ad_init): 42 callers
* [ad_entry](adouble/ad_open.c.md#ad_entry): 37 callers
* [ad_open](adouble/ad_open.c.md#ad_open): 37 callers
* [ad_flush](adouble/ad_flush.c.md#ad_flush): 36 callers
* [ad_getentryoff](adouble/ad_open.c.md#ad_getentryoff): 28 callers
* [ad_setid](adouble/ad_attr.c.md#ad_setid): 16 callers
* [ad_setname](adouble/ad_attr.c.md#ad_setname): 14 callers
* [ad_getattr](adouble/ad_attr.c.md#ad_getattr): 12 callers
* [ad_setdate](adouble/ad_date.c.md#ad_setdate): 11 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[ad_getfuid](adouble/ad_open.c.md#ad_getfuid)
