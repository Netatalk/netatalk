---
type: Subsystem
title: "bin/misc"
description: "3 files, 8 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/bin/misc"
tags: ["bin/misc"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T10:16:19+11:00 }
---

# Files

* [bin/misc/fce.c](misc/fce.c.md): 3 functions, includes 2 project headers.
* [bin/misc/netacnv.c](misc/netacnv.c.md): 2 functions, 1 type, includes 1 project header.
* [bin/misc/uuidtest.c](misc/uuidtest.c.md): 3 functions, includes 3 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 6 includes

# Calls into

* [libatalk/acl](../libatalk/acl.md): 5 calls
* [libatalk/unicode](../libatalk/unicode.md): 3 calls
* [libatalk/util](../libatalk/util.md): 1 calls

# Most called functions

* [parse_ldapconf](misc/uuidtest.c.md#parse_ldapconf): 1 callers
* [sig_handler](misc/fce.c.md#sig_handler): 1 callers
* [unpack_fce_packet](misc/fce.c.md#unpack_fce_packet): 1 callers
* [usage](misc/netacnv.c.md#usage): 1 callers
* [usage](misc/uuidtest.c.md#usage): 1 callers
