---
type: C Header File
title: "etc/afpd/directory.h"
description: "1 function, 2 types, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/directory.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `dirent.h`, `stdbool.h`, `sys/sysmacros.h`, `sys/types.h`

# Included by

* [etc/afpd/acls.c](acls.c.md)
* [etc/afpd/acls.h](acls.h.md)
* [etc/afpd/ad_cache.c](ad_cache.c.md)
* [etc/afpd/afp_asp.c](afp_asp.c.md)
* [etc/afpd/afp_dsi.c](afp_dsi.c.md)
* [etc/afpd/appl.c](appl.c.md)
* [etc/afpd/catsearch.c](catsearch.c.md)
* [etc/afpd/desktop.c](desktop.c.md)
* [etc/afpd/dircache.c](dircache.c.md)
* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/enumerate.c](enumerate.c.md)
* [etc/afpd/extattrs.c](extattrs.c.md)
* [etc/afpd/fce_api.c](fce_api.c.md)
* [etc/afpd/fce_util.c](fce_util.c.md)
* [etc/afpd/file.c](file.c.md)
* [etc/afpd/file.h](file.h.md)
* [etc/afpd/filedir.c](filedir.c.md)
* [etc/afpd/fork.c](fork.c.md)
* [etc/afpd/fork.h](fork.h.md)
* [etc/afpd/idle_worker.c](idle_worker.c.md)
* [etc/afpd/mangle.h](mangle.h.md)
* [etc/afpd/ofork.c](ofork.c.md)
* [etc/afpd/pfd_cache.c](pfd_cache.c.md)
* [etc/afpd/quota.c](quota.c.md)
* [etc/afpd/spotlight.c](spotlight.c.md)
* [etc/afpd/switch.c](switch.c.md)
* [etc/afpd/unix.c](unix.c.md)
* [etc/afpd/virtual_icon.c](virtual_icon.c.md)
* [etc/afpd/volume.c](volume.c.md)

# Functions

### dirlookup_bypath

```c
struct dir * dirlookup_bypath(const struct vol *vol, const char *path)
```

Declared at etc/afpd/directory.h line 132; no definition in the scanned sources.

# Types

### struct dir_modify_args

Parameters for [dir_modify()](directory.c.md#dir_modify) selective field updates.

Defined at line 61.
* `unsigned int flags`
* `cnid_t new_pdid`
* `const char * new_mname`
* `const char * new_uname`
* `bstring new_pdir_path`
* `struct stat * st`
* `struct adouble * adp`

### struct maccess

Defined at line 98.
* `uint8_t ma_user`
* `uint8_t ma_world`
* `uint8_t ma_group`
* `uint8_t ma_owner`

# Typedefs and enums

* `typedef int(* dir_loop`

# Macros

* Undocumented: `AR_UOWN`, `AR_UREAD`, `AR_USEARCH`, `AR_UWRITE`, `CNID`, `DCMOD_AD`, `DCMOD_AD_INV`, `DCMOD_NO_PROMOTE`, `DCMOD_PATH`, `DCMOD_STAT`, `DIRPBIT_ACCESS`, `DIRPBIT_ATTR`, `DIRPBIT_BDATE`, `DIRPBIT_CDATE`, `DIRPBIT_DID`, `DIRPBIT_FINFO`, `DIRPBIT_GID`, `DIRPBIT_LNAME`, `DIRPBIT_MDATE`, `DIRPBIT_OFFCNT`, `DIRPBIT_PDID`, `DIRPBIT_PDINFO`, `DIRPBIT_SNAME`, `DIRPBIT_UID`, `DIRPBIT_UNIXPR`, `FILDIRBIT_ISDIR`, `FILDIRBIT_ISFILE`
