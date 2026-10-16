---
type: Subsystem
title: "bin/nbp"
description: "5 files, 7 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/bin/nbp"
tags: ["bin/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

# Files

* [bin/nbp/nbplkup.c](nbp/nbplkup.c.md): The nbplkup command: look up NBP names.
* [bin/nbp/nbplkup_output.c](nbp/nbplkup_output.c.md): Output formatting of nbplkup: NBP name fields in the host charset.
* [bin/nbp/nbplkup_output.h](nbp/nbplkup_output.h.md): Character set conversion interface for nbplkup output fields.
* [bin/nbp/nbprgstr.c](nbp/nbprgstr.c.md): The nbprgstr command: register an NBP name with atalkd.
* [bin/nbp/nbpunrgstr.c](nbp/nbpunrgstr.c.md): The nbpunrgstr command: unregister an NBP name with atalkd.

# Includes headers from

* [include/atalk](../include/atalk.md): 12 includes
* [sys/netatalk](../sys/netatalk.md): 3 includes

# Calls into

* [libatalk/unicode](../libatalk/unicode.md): 10 calls
* [libatalk/nbp](../libatalk/nbp.md): 6 calls
* [libatalk/util](../libatalk/util.md): 3 calls
* [include/atalk](../include/atalk.md): 1 calls
* [libatalk/netddp](../libatalk/netddp.md): 1 calls

# Most called functions

* [Usage](nbp/nbplkup.c.md#usage): 1 callers
* [Usage](nbp/nbprgstr.c.md#usage): 1 callers
* [Usage](nbp/nbpunrgstr.c.md#usage): 1 callers
* [nbplkup_convert_field](nbp/nbplkup_output.c.md#nbplkup_convert_field): 1 callers
