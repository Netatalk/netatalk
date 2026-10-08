---
type: Subsystem
title: "bin/misc"
description: "4 files, 9 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/bin/misc"
tags: ["bin/misc"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [bin/misc/fce.c](misc/fce.c.md): 3 functions, includes 2 project headers.
* [bin/misc/logger_test.c](misc/logger_test.c.md): 1 function, includes 1 project header.
* [bin/misc/netacnv.c](misc/netacnv.c.md): 2 functions, 1 type, includes 1 project header.
* [bin/misc/uuidtest.c](misc/uuidtest.c.md): 3 functions, includes 3 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 7 includes

# Calls into

* [libatalk/acl](../libatalk/acl.md): 5 calls
* [libatalk/unicode](../libatalk/unicode.md): 3 calls
* [libatalk/util](../libatalk/util.md): 3 calls

# Most called functions

* [parse_ldapconf](misc/uuidtest.c.md#parse_ldapconf): 1 callers
* [sig_handler](misc/fce.c.md#sig_handler): 1 callers
* [unpack_fce_packet](misc/fce.c.md#unpack_fce_packet): 1 callers
* [usage](misc/netacnv.c.md#usage): 1 callers
* [usage](misc/uuidtest.c.md#usage): 1 callers
