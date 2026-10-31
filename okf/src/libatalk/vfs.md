---
type: Subsystem
title: "libatalk/vfs"
description: "6 files, 118 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/vfs"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-31T16:21:52+11:00 }
---

# Files

* [libatalk/vfs/acl.c](vfs/acl.c.md): Reducing ACLs to the entries that mirror the mode bits, in the VFS layer.
* [libatalk/vfs/ea_ad.c](vfs/ea_ad.c.md): Store Extended Attributes inside .AppleDouble folders as follows:
* [libatalk/vfs/ea_sys.c](vfs/ea_sys.c.md): The sys EA backend: AFP extended attributes in native filesystem EAs.
* [libatalk/vfs/extattr.c](vfs/extattr.c.md): Platform wrappers for extended attribute system calls, from Samba.
* [libatalk/vfs/unix.c](vfs/unix.c.md): Unix file operations of the VFS layer: modes, duplication, deletion, and renaming.
* [libatalk/vfs/vfs.c](vfs/vfs.c.md): VFS operation tables for the AppleDouble layouts and the EA and ACL layers.

# Includes headers from

* [include/atalk](../include/atalk.md): 52 includes

# Calls into

* [libatalk/adouble](adouble.md): 17 calls
* [libatalk/util](util.md): 13 calls
* [libatalk/compat](compat.md): 7 calls
* [libatalk/unicode](unicode.md): 3 calls
* [libatalk/acl](acl.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 21 calls
* [libatalk/adouble](adouble.md): 11 calls
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
* [sys_fgetxattr](vfs/extattr.c.md#sys_fgetxattr): 6 callers
* [dir_rx_set](vfs/unix.c.md#dir_rx_set): 5 callers
* [ea_absent](vfs/ea_ad.c.md#ea_absent): 5 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[copy_ea](vfs/unix.c.md#copy_ea), [deletecurdir_ea_osx_chkifempty_loop](vfs/vfs.c.md#deletecurdir_ea_osx_chkifempty_loop)
