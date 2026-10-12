---
type: Subsystem
title: "etc/papd"
description: "22 files, 119 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/etc/papd"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

# Files

* [etc/papd/auth.c](papd/auth.c.md): UAM registration and loading for papd printer authentication.
* [etc/papd/comment.c](papd/comment.c.md): DSC comment state stack of the papd PostScript parser.
* [etc/papd/comment.h](papd/comment.h.md): 1 function, 2 types.
* [etc/papd/file.c](papd/file.c.md): Print job buffers of papd: lines, appended data, and status replies.
* [etc/papd/file.h](papd/file.h.md): 1 type.
* [etc/papd/headers.c](papd/headers.c.md): DSC header comment handlers of papd.
* [etc/papd/lp.c](papd/lp.c.md): Interface to lpr system.
* [etc/papd/lp.h](papd/lp.h.md): 3 functions, includes 1 project header.
* [etc/papd/magics.c](papd/magics.c.md): PostScript job type detection and the papd job loop.
* [etc/papd/main.c](papd/main.c.md): The papd daemon: printers, NBP registration, and the accept loop.
* [etc/papd/ppd.c](papd/ppd.c.md): PPD file reading for the printer of a papd job.
* [etc/papd/ppd.h](papd/ppd.h.md): 2 types.
* [etc/papd/print_cups.c](papd/print_cups.c.md): CUPS printing backend of papd.
* [etc/papd/print_cups.h](papd/print_cups.h.md): 1 type.
* [etc/papd/printcap.c](papd/printcap.c.md): routines for dealing with the terminal capability data base based on termcap
* [etc/papd/printcap.h](papd/printcap.h.md): No functions or types.
* [etc/papd/printer.h](papd/printer.h.md): 1 type.
* [etc/papd/queries.c](papd/queries.c.md): PostScript query answers of papd.
* [etc/papd/session.c](papd/session.c.md): One PAP connection of papd: reading jobs and sending replies.
* [etc/papd/session.h](papd/session.h.md): includes 1 project header.
* [etc/papd/uam.c](papd/uam.c.md): UAM module loading and the server services that papd offers UAMs.
* [etc/papd/uam_auth.h](papd/uam_auth.h.md): interface between uam.c and auth.c

# Includes headers from

* [include/atalk](../include/atalk.md): 40 includes
* [sys/netatalk](../sys/netatalk.md): 7 includes

# Calls into

* [libatalk/atp](../libatalk/atp.md): 10 calls
* [libatalk/unicode](../libatalk/unicode.md): 8 calls
* [libatalk/compat](../libatalk/compat.md): 7 calls
* [libatalk/util](../libatalk/util.md): 6 calls
* [include/atalk](../include/atalk.md): 4 calls
* [libatalk/nbp](../libatalk/nbp.md): 3 calls

# Most called functions

* [markline](papd/file.c.md#markline): 20 callers
* [compop](papd/comment.c.md#compop): 19 callers
* [append](papd/file.c.md#append): 16 callers
* [lp_write](papd/lp.c.md#lp_write): 9 callers
* [comcmp](papd/comment.c.md#comcmp): 8 callers
* [compush](papd/comment.c.md#compush): 4 callers
* [comswitch](papd/comment.c.md#comswitch): 4 callers
* [cups_passwd_cb](papd/print_cups.c.md#cups_passwd_cb): 4 callers
* [lp_close](papd/lp.c.md#lp_close): 4 callers
* [spoolerror](papd/file.c.md#spoolerror): 4 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[lp_host](papd/lp.c.md#lp_host), [pdecode](papd/printcap.c.md#pdecode), [pnamatch](papd/printcap.c.md#pnamatch), [pskip](papd/printcap.c.md#pskip), [rresvport](papd/lp.c.md#rresvport), [uam_afpserver_option](papd/uam.c.md#uam_afpserver_option), [uam_checkuser](papd/uam.c.md#uam_checkuser), [uam_getname](papd/uam.c.md#uam_getname), [uam_register](papd/uam.c.md#uam_register), [uam_unregister](papd/uam.c.md#uam_unregister)
