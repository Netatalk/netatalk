---
type: C Header File
title: "etc/afpd/directory.h"
description: "AFP directory command handlers and path resolution helpers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/directory.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-30T10:57:24+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `dirent.h`, `stdbool.h`, `sys/sysmacros.h`, `sys/types.h`, `time.h`

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

Declared at etc/afpd/directory.h line 164; no definition in the scanned sources.

# Types

### struct dir_modify_args

Parameters for [dir_modify()](directory.c.md#dir_modify) selective field updates.

Defined at line 69.
* `unsigned int flags`: DCMOD_* bitmask.
* `cnid_t new_pdid`: New parent DID (DCMOD_PATH).
* `const char * new_mname`: New mac name, NULL = use new_uname (DCMOD_PATH).
* `const char * new_uname`: New unix name (DCMOD_PATH).
* `bstring new_pdir_path`: New parent directory fullpath (DCMOD_PATH).
* `struct stat * st`: Fresh stat result (DCMOD_STAT).
* `struct adouble * adp`: AD data to store (DCMOD_AD).

### struct enumerate_stats

Defined at line 195.
* `unsigned long long reads`: directory reads
* `unsigned long long rereads`: reads by batches past index 1 whose listing no slot holds, not the root's
* `unsigned long long changed`: reads of folders that changed, or may have, since their listing's read
* `unsigned long long names`: names the reads saved

### struct maccess

Defined at line 109.
* `uint8_t ma_user`
* `uint8_t ma_world`
* `uint8_t ma_group`
* `uint8_t ma_owner`

### struct offcnt_stats

Defined at line 121.
* `unsigned long long requests`: DIRPBIT_OFFCNT answered.
* `unsigned long long cached`: served from the entry
* `unsigned long long first`: walked: never counted
* `unsigned long long older`: walked: stat ctime behind the count's
* `unsigned long long confirmed`: walked: the count held stood
* `unsigned long long changed`: walked: the count held was wrong
* `unsigned long long failed`: walk refused: unreadable folder
* `unsigned long long entries`: visible entries the walks counted

# Typedefs and enums

* `typedef int(* dir_loop`

# Macros

* `DCMOD_AD`: AD cache: finderinfo/filedatesi/afpfilei/rlen.
* `DCMOD_AD_INV`: AD invalidate: zero AD fields, set rlen = -1; with DCMOD_STAT drops d_rights_cache.
* `DCMOD_PATH`: Identity/navigation: d_pdid, names, d_fullpath.
* `DCMOD_STAT`: Stat cache: dcache_ctime/ino/mode/mtime/uid/gid/size; a change drops d_rights_cache.
* `DIRPBIT_PDINFO`: ProDOS Info.
* `FILDIRBIT_ISDIR`: is a directory
* `FILDIRBIT_ISFILE`: is a file
* Undocumented: `AR_UOWN`, `AR_UREAD`, `AR_USEARCH`, `AR_UWRITE`, `CNID`, `DCMOD_NO_PROMOTE`, `DIRPBITS_AD`, `DIRPBIT_ACCESS`, `DIRPBIT_ATTR`, `DIRPBIT_BDATE`, `DIRPBIT_CDATE`, `DIRPBIT_DID`, `DIRPBIT_FINFO`, `DIRPBIT_GID`, `DIRPBIT_LNAME`, `DIRPBIT_MDATE`, `DIRPBIT_OFFCNT`, `DIRPBIT_PDID`, `DIRPBIT_SNAME`, `DIRPBIT_UID`, `DIRPBIT_UNIXPR`
