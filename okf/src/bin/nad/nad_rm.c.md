---
type: C Source File
title: "bin/nad/nad_rm.c"
description: "5 functions, includes 8 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_rm.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/queue.h](../../include/atalk/queue.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [ftw.h](ftw.h.md)
* [nad.h](nad.h.md)
* System headers: `bstrlib.h`, `errno.h`, `limits.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### check_netatalk_dirs

```c
static const char * check_netatalk_dirs(const char *name)
```

Defined at lines 66 to 77.

Called by: [rm](nad_rm.c.md#rm)

Uses file-scope variables: `netatalk_dirs`

### upfunc

```c
static void upfunc(void)
```

Defined at lines 79 to 82.

Called by: [nad_rm](nad_rm.c.md#nad_rm)

Uses file-scope variables: `did`, `pdid`

### usage_rm

```c
static void usage_rm(void)
```

Defined at lines 84 to 96.

Called by: [nad_rm](nad_rm.c.md#nad_rm)

### nad_rm

```c
int nad_rm(int argc, char *argv[], AFPObj *obj)
```

Defined at lines 98 to 184. Declared in [bin/nad/nad.h](nad.h.md).

Calls: [ad_valid_header_osx](../../libatalk/adouble/ad_open.c.md#ad_valid_header_osx), [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [nftw](ftw.h.md#nftw), [openvol_optional](nad_util.c.md#openvol_optional), [rm](nad_rm.c.md#rm), [set_signal](nad_util.c.md#set_signal), [upfunc](nad_rm.c.md#upfunc), [usage_rm](nad_rm.c.md#usage_rm)

Called by: [copy](nad_mv.c.md#copy), [main](nad.c.md#main)

Uses file-scope variables: `Rflag`, `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `badrm`, `did`, `pdid`, `rval`, `vflag` in [bin/nad/nad.h](nad.h.md), `volume`

### rm

```c
static int rm(const char *fpath, const struct stat *sb, int tflag, struct FTW *ftwbuf)
```

Defined at lines 186 to 379.

Calls: [ad_valid_header_osx](../../libatalk/adouble/ad_open.c.md#ad_valid_header_osx), [check_netatalk_dirs](nad_rm.c.md#check_netatalk_dirs), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset)

Called by: [nad_rm](nad_rm.c.md#nad_rm)

Calls through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletefile_adouble](../../libatalk/vfs/vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](../../libatalk/vfs/vfs.c.md#rf_deletefile_ea), [ea_deletefile](../../libatalk/vfs/ea_ad.c.md#ea_deletefile), [vfs_deletefile](../../libatalk/vfs/vfs.c.md#vfs_deletefile)

Uses file-scope variables: `Rflag`, `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `badrm`, `did`, `pdid`, `rval`, `vflag` in [bin/nad/nad.h](nad.h.md), `volume`

# Macros

* Undocumented: `STRIP_TRAILING_SLASH`

# File-scope variables

`Rflag`, `badrm`, `did`, `netatalk_dirs`, `pdid`, `rval`, `volume`
