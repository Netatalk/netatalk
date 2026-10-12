---
type: Subsystem
title: "libatalk/nbp"
description: "5 files, 7 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/nbp"
tags: ["libatalk/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

# Files

* [libatalk/nbp/nbp_conf.h](nbp/nbp_conf.h.md): includes 1 project header.
* [libatalk/nbp/nbp_lkup.c](nbp/nbp_lkup.c.md): NBP lookups through atalkd.
* [libatalk/nbp/nbp_rgstr.c](nbp/nbp_rgstr.c.md): NBP name registration through atalkd.
* [libatalk/nbp/nbp_unrgstr.c](nbp/nbp_unrgstr.c.md): NBP name removal through atalkd.
* [libatalk/nbp/nbp_util.c](nbp/nbp_util.c.md): NBP tuple parsing, matching, and name splitting.

# Includes headers from

* [include/atalk](../include/atalk.md): 10 includes
* [sys/netatalk](../sys/netatalk.md): 5 includes

# Calls into

* [include/atalk](../include/atalk.md): 9 calls
* [libatalk/netddp](netddp.md): 3 calls
* [libatalk/util](util.md): 1 calls

# Called from

* [bin/nbp](../bin/nbp.md): 6 calls
* [bin/pap](../bin/pap.md): 4 calls
* [etc/afpd](../etc/afpd.md): 3 calls
* [etc/papd](../etc/papd.md): 3 calls
* [bin/aecho](../bin/aecho.md): 1 calls

# Most called functions

* [nbp_name](nbp/nbp_util.c.md#nbp_name): 6 callers
* [nbp_lookup](nbp/nbp_lkup.c.md#nbp_lookup): 4 callers
* [nbp_unrgstr](nbp/nbp_unrgstr.c.md#nbp_unrgstr): 4 callers
* [nbp_rgstr](nbp/nbp_rgstr.c.md#nbp_rgstr): 3 callers
* [nbp_do_lookup_op](nbp/nbp_lkup.c.md#nbp_do_lookup_op): 2 callers
* [nbp_match](nbp/nbp_util.c.md#nbp_match): 1 callers
* [nbp_parse](nbp/nbp_util.c.md#nbp_parse): 1 callers
