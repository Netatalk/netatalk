---
type: C Header File
title: "bin/nad/nad.h"
description: "2 types, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad.h"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `signal.h`, `stdio.h`, `sys/stat.h`, `sys/types.h`

# Included by

* [bin/nad/hqx.c](hqx.c.md)
* [bin/nad/macbin.c](macbin.c.md)
* [bin/nad/megatron.c](megatron.c.md)
* [bin/nad/nad.c](nad.c.md)
* [bin/nad/nad_adouble.c](nad_adouble.c.md)
* [bin/nad/nad_cp.c](nad_cp.c.md)
* [bin/nad/nad_find.c](nad_find.c.md)
* [bin/nad/nad_ls.c](nad_ls.c.md)
* [bin/nad/nad_mkdir.c](nad_mkdir.c.md)
* [bin/nad/nad_mv.c](nad_mv.c.md)
* [bin/nad/nad_rm.c](nad_rm.c.md)
* [bin/nad/nad_rmdir.c](nad_rmdir.c.md)
* [bin/nad/nad_set.c](nad_set.c.md)
* [bin/nad/nad_stuffit.c](nad_stuffit.c.md)
* [bin/nad/nad_util.c](nad_util.c.md)

# Types

### struct PATH_T

Defined at line 89.
* `char * p_end`
* `char * target_end`
* `char p_path`

### struct afpvol_t

Defined at line 53.
* `struct vol * vol`
* `char db_stamp`
* `bool owns_cdb`

# Macros

* Undocumented: `ADVOL_V2_OR_EA`, `DIR_DOT_OR_DOTDOT`, `NAD_DEBUG`, `NAD_FATAL`, `NAD_INFO`

# File-scope variables

`iflag`, `lflag`, `nflag`, `pflag`, `vflag`
