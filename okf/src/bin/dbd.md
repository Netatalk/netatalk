---
type: Subsystem
title: "bin/dbd"
description: "3 files, 19 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/bin/dbd"
tags: ["bin/dbd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [bin/dbd/cmd_dbd.c](dbd/cmd_dbd.c.md): 5 functions, includes 6 project headers.
* [bin/dbd/cmd_dbd.h](dbd/cmd_dbd.h.md): includes 1 project header.
* [bin/dbd/cmd_dbd_scanvol.c](dbd/cmd_dbd_scanvol.c.md): 14 functions, includes 11 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 16 includes

# Calls into

* [libatalk/adouble](../libatalk/adouble.md): 19 calls
* [libatalk/cnid](../libatalk/cnid.md): 9 calls
* [libatalk/util](../libatalk/util.md): 8 calls
* [libatalk/compat](../libatalk/compat.md): 5 calls
* [libatalk/vfs](../libatalk/vfs.md): 3 calls

# Most called functions

* [dbd_log](dbd/cmd_dbd.c.md#dbd_log): 12 callers
* [adouble_dir_unusable](dbd/cmd_dbd_scanvol.c.md#adouble_dir_unusable): 1 callers
* [check_addir](dbd/cmd_dbd_scanvol.c.md#check_addir): 1 callers
* [check_adfile](dbd/cmd_dbd_scanvol.c.md#check_adfile): 1 callers
* [check_cnid](dbd/cmd_dbd_scanvol.c.md#check_cnid): 1 callers
* [check_eafile_in_adouble](dbd/cmd_dbd_scanvol.c.md#check_eafile_in_adouble): 1 callers
* [check_eafiles](dbd/cmd_dbd_scanvol.c.md#check_eafiles): 1 callers
* [check_netatalk_dirs](dbd/cmd_dbd_scanvol.c.md#check_netatalk_dirs): 1 callers
* [check_orphaned](dbd/cmd_dbd_scanvol.c.md#check_orphaned): 1 callers
* [check_special_dirs](dbd/cmd_dbd_scanvol.c.md#check_special_dirs): 1 callers
