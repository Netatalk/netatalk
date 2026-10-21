---
type: Subsystem
title: "libatalk/compat"
description: "4 files, 6 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/compat"
tags: ["libatalk/compat"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T15:29:27+11:00 }
---

# Files

* [libatalk/compat/explicit_bzero.c](compat/explicit_bzero.c.md): explicit_bzero for systems without it
* [libatalk/compat/misc.c](compat/misc.c.md): strnlen, vasprintf, and asprintf for systems without them
* [libatalk/compat/rquota_xdr.c](compat/rquota_xdr.c.md): XDR routines of the rquota protocol, from the Linux quota tools.
* [libatalk/compat/strlcpy.c](compat/strlcpy.c.md): strlcpy and strlcat for systems without them

# Includes headers from

* [include/atalk](../include/atalk.md): 2 includes

# Called from

* [etc/afpd](../etc/afpd.md): 55 calls
* [bin/afppasswd](../bin/afppasswd.md): 30 calls
* [bin/nad](../bin/nad.md): 30 calls
* [etc/uams](../etc/uams.md): 30 calls
* [libatalk/cnid](cnid.md): 22 calls
* [libatalk/util](util.md): 19 calls
* [etc/netatalk](../etc/netatalk.md): 8 calls
* [libatalk/adouble](adouble.md): 8 calls
* [etc/papd](../etc/papd.md): 7 calls
* [libatalk/vfs](vfs.md): 7 calls
* [bin/dbd](../bin/dbd.md): 5 calls
* [etc/atalkd](../etc/atalkd.md): 4 calls
* [etc/spotlight](../etc/spotlight.md): 4 calls
* [bin/getzones](../bin/getzones.md): 1 calls
* [include/atalk](../include/atalk.md): 1 calls
* [libatalk/dsi](dsi.md): 1 calls

# Most called functions

* [strlcpy](compat/strlcpy.c.md#strlcpy): 87 callers
* [strnlen](compat/misc.c.md#strnlen): 60 callers
* [explicit_bzero](compat/explicit_bzero.c.md#explicit_bzero): 35 callers
* [strlcat](compat/strlcpy.c.md#strlcat): 25 callers
* [asprintf](compat/misc.c.md#asprintf): 22 callers
* [vasprintf](compat/misc.c.md#vasprintf): 4 callers
