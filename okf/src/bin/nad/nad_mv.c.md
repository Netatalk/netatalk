---
type: C Source File
title: "bin/nad/nad_mv.c"
description: "5 functions, includes 8 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_mv.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/bstrlib_compat.h](../../include/atalk/bstrlib_compat.h.md)
* [atalk/queue.h](../../include/atalk/queue.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [nad.h](nad.h.md)
* System headers: `dirent.h`, `errno.h`, `libgen.h`, `limits.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### usage_mv

```c
static void usage_mv(void)
```

Defined at lines 54 to 77.

Called by: [nad_mv](nad_mv.c.md#nad_mv)

### nad_mv

```c
int nad_mv(int argc, char *argv[], AFPObj *obj)
```

Defined at lines 79 to 220. Declared in [bin/nad/nad.h](nad.h.md).

Calls: [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [do_move](nad_mv.c.md#do_move), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [usage_mv](nad_mv.c.md#usage_mv)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `afp_obj`, `did`, `dvolume`, `fflg`, `iflg`, `nflg`, `pdid`, `svolume`, `vflg`

### do_move

```c
static int do_move(const char *, const char *)
```

Defined at lines 222 to 455.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [cnid_for_paths_parent](nad_util.c.md#cnid_for_paths_parent), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [copy](nad_mv.c.md#copy), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset)

Called by: [nad_mv](nad_mv.c.md#nad_mv)

Calls through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_renamefile_adouble](../../libatalk/vfs/vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](../../libatalk/vfs/vfs.c.md#rf_renamefile_ea), [ea_renamefile](../../libatalk/vfs/ea_ad.c.md#ea_renamefile), [vfs_renamefile](../../libatalk/vfs/vfs.c.md#vfs_renamefile)

Uses file-scope variables: `did`, `dvolume`, `fflg`, `iflg`, `nflg`, `svolume`, `vflg`

### remove_path

```c
static int remove_path(const char *path)
```

Defined at lines 457 to 494.

Called by: [copy](nad_mv.c.md#copy)

### copy

```c
static int copy(const char *, const char *)
```

Defined at lines 496 to 548.

Calls: [nad_cp](nad_cp.c.md#nad_cp), [nad_rm](nad_rm.c.md#nad_rm), [remove_path](nad_mv.c.md#remove_path)

Called by: [do_move](nad_mv.c.md#do_move)

Uses file-scope variables: `afp_obj`, `svolume`, `vflg`

# Macros

* Undocumented: `STRIP_TRAILING_SLASH`

# File-scope variables

`afp_obj`, `did`, `dvolume`, `fflg`, `iflg`, `nflg`, `pdid`, `svolume`, `vflg`
