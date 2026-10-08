---
type: Subsystem
title: "libatalk/vfs"
description: "6 files, 114 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/vfs"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [libatalk/vfs/acl.c](vfs/acl.c.md): 2 functions, includes 5 project headers.
* [libatalk/vfs/ea_ad.c](vfs/ea_ad.c.md): 25 functions, includes 9 project headers.
* [libatalk/vfs/ea_sys.c](vfs/ea_sys.c.md): 7 functions, includes 9 project headers.
* [libatalk/vfs/extattr.c](vfs/extattr.c.md): 14 functions, includes 6 project headers.
* [libatalk/vfs/unix.c](vfs/unix.c.md): 12 functions, includes 10 project headers.
* [libatalk/vfs/vfs.c](vfs/vfs.c.md): 54 functions, 1 type, includes 13 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 52 includes

# Calls into

* [libatalk/adouble](adouble.md): 17 calls
* [libatalk/util](util.md): 12 calls
* [libatalk/compat](compat.md): 7 calls
* [libatalk/unicode](unicode.md): 3 calls
* [libatalk/acl](acl.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 18 calls
* [libatalk/adouble](adouble.md): 8 calls
* [bin/dbd](../bin/dbd.md): 3 calls
* [libatalk/util](util.md): 3 calls

# Most called functions

* [ea_close](vfs/ea_ad.c.md#ea_close): 12 callers
* [ea_open](vfs/ea_ad.c.md#ea_open): 12 callers
* [ea_path](vfs/ea_ad.c.md#ea_path): 11 callers
* [prefix](vfs/extattr.c.md#prefix): 9 callers
* [ea_attrname_invalid](vfs/ea_ad.c.md#ea_attrname_invalid): 8 callers
* [netatalk_unlinkat](vfs/unix.c.md#netatalk_unlinkat): 6 callers
* [setfilmode](vfs/unix.c.md#setfilmode): 6 callers
* [dir_rx_set](vfs/unix.c.md#dir_rx_set): 5 callers
* [sys_fgetxattr](vfs/extattr.c.md#sys_fgetxattr): 5 callers
* [unix_rename](vfs/unix.c.md#unix_rename): 5 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[copy_ea](vfs/unix.c.md#copy_ea), [deletecurdir_ea_osx_chkifempty_loop](vfs/vfs.c.md#deletecurdir_ea_osx_chkifempty_loop)
