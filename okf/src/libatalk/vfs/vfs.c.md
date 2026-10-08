---
type: C Source File
title: "libatalk/vfs/vfs.c"
description: "54 functions, 1 type, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/vfs.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/vfs](../vfs.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/acl.h](../../include/atalk/acl.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/bstrlib_compat.h](../../include/atalk/bstrlib_compat.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `libgen.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Function tables

### netatalk_adouble_ea

Initialized at line 1190 as `struct vfs_ops`. Assigns:

* `vfs_chown`: [RF_chown_ea](vfs.c.md#rf_chown_ea)
* `vfs_copyfile`: [RF_copyfile_ea](vfs.c.md#rf_copyfile_ea)
* `vfs_deletecurdir`: [RF_deletecurdir_ea](vfs.c.md#rf_deletecurdir_ea)
* `vfs_deletefile`: [RF_deletefile_ea](vfs.c.md#rf_deletefile_ea)
* `vfs_renamedir`: [RF_renamedir_ea](vfs.c.md#rf_renamedir_ea)
* `vfs_renamefile`: [RF_renamefile_ea](vfs.c.md#rf_renamefile_ea)
* `vfs_setdirmode`: [RF_setdirmode_ea](vfs.c.md#rf_setdirmode_ea)
* `vfs_setdirowner`: [RF_setdirowner_ea](vfs.c.md#rf_setdirowner_ea)
* `vfs_setdirunixmode`: [RF_setdirunixmode_ea](vfs.c.md#rf_setdirunixmode_ea)
* `vfs_setfilmode`: [RF_setfilmode_ea](vfs.c.md#rf_setfilmode_ea)
* `vfs_validupath`: [validupath_ea](vfs.c.md#validupath_ea)

### netatalk_adouble_v2

Initialized at line 1162 as `struct vfs_ops`. Assigns:

* `vfs_chown`: [RF_chown_adouble](vfs.c.md#rf_chown_adouble)
* `vfs_copyfile`: [RF_copyfile_adouble](vfs.c.md#rf_copyfile_adouble)
* `vfs_deletecurdir`: [RF_deletecurdir_adouble](vfs.c.md#rf_deletecurdir_adouble)
* `vfs_deletefile`: [RF_deletefile_adouble](vfs.c.md#rf_deletefile_adouble)
* `vfs_renamedir`: [RF_renamedir_adouble](vfs.c.md#rf_renamedir_adouble)
* `vfs_renamefile`: [RF_renamefile_adouble](vfs.c.md#rf_renamefile_adouble)
* `vfs_setdirmode`: [RF_setdirmode_adouble](vfs.c.md#rf_setdirmode_adouble)
* `vfs_setdirowner`: [RF_setdirowner_adouble](vfs.c.md#rf_setdirowner_adouble)
* `vfs_setdirunixmode`: [RF_setdirunixmode_adouble](vfs.c.md#rf_setdirunixmode_adouble)
* `vfs_setfilmode`: [RF_setfilmode_adouble](vfs.c.md#rf_setfilmode_adouble)
* `vfs_validupath`: [validupath_adouble](vfs.c.md#validupath_adouble)

### netatalk_ea_adouble

Initialized at line 1222 as `struct vfs_ops`. Assigns:

* `vfs_chown`: [ea_chown](ea_ad.c.md#ea_chown)
* `vfs_copyfile`: [ea_copyfile](ea_ad.c.md#ea_copyfile)
* `vfs_deletefile`: [ea_deletefile](ea_ad.c.md#ea_deletefile)
* `vfs_ea_getcontent`: [get_eacontent](ea_ad.c.md#get_eacontent)
* `vfs_ea_getsize`: [get_easize](ea_ad.c.md#get_easize)
* `vfs_ea_list`: [list_eas](ea_ad.c.md#list_eas)
* `vfs_ea_remove`: [remove_ea](ea_ad.c.md#remove_ea)
* `vfs_ea_set`: [set_ea](ea_ad.c.md#set_ea)
* `vfs_renamefile`: [ea_renamefile](ea_ad.c.md#ea_renamefile)
* `vfs_setdirunixmode`: [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir)
* `vfs_setfilmode`: [ea_chmod_file](ea_ad.c.md#ea_chmod_file)

### netatalk_ea_sys

Initialized at line 1250 as `struct vfs_ops`. Assigns:

* `vfs_copyfile`: [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile)
* `vfs_ea_getcontent`: [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent)
* `vfs_ea_getsize`: [sys_get_easize](ea_sys.c.md#sys_get_easize)
* `vfs_ea_list`: [sys_list_eas](ea_sys.c.md#sys_list_eas)
* `vfs_ea_remove`: [sys_remove_ea](ea_sys.c.md#sys_remove_ea)
* `vfs_ea_set`: [sys_set_ea](ea_sys.c.md#sys_set_ea)

### netatalk_posix_acl_adouble

Initialized at line 1306 as `struct vfs_ops`. Assigns:

* `vfs_posix_acl`: [RF_posix_acl](vfs.c.md#rf_posix_acl)
* `vfs_remove_acl`: [RF_posix_remove_acl](vfs.c.md#rf_posix_remove_acl)

### netatalk_solaris_acl_adouble

Initialized at line 1283 as `struct vfs_ops`. Assigns:

* `vfs_remove_acl`: [RF_solaris_remove_acl](vfs.c.md#rf_solaris_remove_acl)
* `vfs_solaris_acl`: [RF_solaris_acl](vfs.c.md#rf_solaris_acl)

### vfs_master_funcs

Initialized at line 1130 as `struct vfs_ops`. Assigns:

* `vfs_chown`: [vfs_chown](vfs.c.md#vfs_chown)
* `vfs_copyfile`: [vfs_copyfile](vfs.c.md#vfs_copyfile)
* `vfs_deletecurdir`: [vfs_deletecurdir](vfs.c.md#vfs_deletecurdir)
* `vfs_deletefile`: [vfs_deletefile](vfs.c.md#vfs_deletefile)
* `vfs_ea_getcontent`: [vfs_ea_getcontent](vfs.c.md#vfs_ea_getcontent)
* `vfs_ea_getsize`: [vfs_ea_getsize](vfs.c.md#vfs_ea_getsize)
* `vfs_ea_list`: [vfs_ea_list](vfs.c.md#vfs_ea_list)
* `vfs_ea_remove`: [vfs_ea_remove](vfs.c.md#vfs_ea_remove)
* `vfs_ea_set`: [vfs_ea_set](vfs.c.md#vfs_ea_set)
* `vfs_posix_acl`: [vfs_posix_acl](vfs.c.md#vfs_posix_acl)
* `vfs_remove_acl`: [vfs_remove_acl](vfs.c.md#vfs_remove_acl)
* `vfs_renamedir`: [vfs_renamedir](vfs.c.md#vfs_renamedir)
* `vfs_renamefile`: [vfs_renamefile](vfs.c.md#vfs_renamefile)
* `vfs_setdirmode`: [vfs_setdirmode](vfs.c.md#vfs_setdirmode)
* `vfs_setdirowner`: [vfs_setdirowner](vfs.c.md#vfs_setdirowner)
* `vfs_setdirunixmode`: [vfs_setdirunixmode](vfs.c.md#vfs_setdirunixmode)
* `vfs_setfilmode`: [vfs_setfilmode](vfs.c.md#vfs_setfilmode)
* `vfs_solaris_acl`: [vfs_solaris_acl](vfs.c.md#vfs_solaris_acl)
* `vfs_validupath`: [vfs_validupath](vfs.c.md#vfs_validupath)

# Functions

### for_each_adouble

```c
static int for_each_adouble(const char *from, const char *name, rf_loop fn, const struct vol *vol, void *data, int flag)
```

Defined at lines 56 to 97.

Calls: [fullpathname](../util/unix.c.md#fullpathname), [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [RF_deletecurdir_adouble](vfs.c.md#rf_deletecurdir_adouble), [RF_deletecurdir_ea](vfs.c.md#rf_deletecurdir_ea), [RF_setdirmode_adouble](vfs.c.md#rf_setdirmode_adouble)

### netatalk_name

```c
static int netatalk_name(const char *name)
```

Defined at lines 99 to 102.

Called by: [validupath_adouble](vfs.c.md#validupath_adouble), [validupath_ea](vfs.c.md#validupath_ea)

### validupath_adouble

```c
static int validupath_adouble(const struct vol *vol, const char *name)
```

Defined at lines 108 to 116.

Calls: [netatalk_name](vfs.c.md#netatalk_name)

Called through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [check_dirent](../../etc/afpd/enumerate.c.md#check_dirent), [check_name](../../etc/afpd/filedir.c.md#check_name), [copy](../../bin/nad/nad_cp.c.md#copy), [dbd_readdir](../../bin/dbd/cmd_dbd_scanvol.c.md#dbd_readdir), [vfs_validupath](vfs.c.md#vfs_validupath)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_chown_adouble

```c
static int RF_chown_adouble(const struct vol *vol, const char *path, uid_t uid, gid_t gid)
```

Defined at lines 119 to 134.

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setfilowner](../../etc/afpd/unix.c.md#setfilowner), [vfs_chown](vfs.c.md#vfs_chown)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_renamedir_adouble

```c
static int RF_renamedir_adouble(const struct vol *vol, int dirfd, const char *oldpath, const char *newpath)
```

Defined at lines 137 to 142.

Called through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamedir](../../etc/afpd/directory.c.md#renamedir), [vfs_renamedir](vfs.c.md#vfs_renamedir)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### deletecurdir_adouble_loop

```c
static int deletecurdir_adouble_loop(const struct vol *vol, struct dirent *de, char *name, void *data, int flag)
```

Defined at lines 145 to 163.

Calls: [netatalk_unlink](unix.c.md#netatalk_unlink)

Called by: [RF_deletecurdir_adouble](vfs.c.md#rf_deletecurdir_adouble)

### RF_deletecurdir_adouble

```c
static int RF_deletecurdir_adouble(const struct vol *vol)
```

Defined at lines 165 to 177.

Calls: [deletecurdir_adouble_loop](vfs.c.md#deletecurdir_adouble_loop), [for_each_adouble](vfs.c.md#for_each_adouble), [netatalk_rmdir](unix.c.md#netatalk_rmdir)

Called through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [vfs_deletecurdir](vfs.c.md#vfs_deletecurdir)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### adouble_setfilmode

```c
static int adouble_setfilmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 180 to 184.

Calls: [ad_hf_mode](../adouble/ad_open.c.md#ad_hf_mode), [setfilmode](unix.c.md#setfilmode)

Called by: [RF_setdirunixmode_adouble](vfs.c.md#rf_setdirunixmode_adouble), [RF_setfilmode_adouble](vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](vfs.c.md#rf_setfilmode_ea)

### RF_setfilmode_adouble

```c
static int RF_setfilmode_adouble(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 186 to 190.

Calls: [adouble_setfilmode](vfs.c.md#adouble_setfilmode)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode), [vfs_setfilmode](vfs.c.md#vfs_setfilmode)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_setdirunixmode_adouble

```c
static int RF_setdirunixmode_adouble(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 193 to 223.

Calls: [ad_dir](../adouble/ad_open.c.md#ad_dir), [adouble_setfilmode](vfs.c.md#adouble_setfilmode), [dir_rx_set](unix.c.md#dir_rx_set), [ochmod](../util/unix.c.md#ochmod)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode), [vfs_setdirunixmode](vfs.c.md#vfs_setdirunixmode)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### setdirmode_adouble_loop

```c
static int setdirmode_adouble_loop(const struct vol *vol, struct dirent *de, char *name, void *data, int flag)
```

Defined at lines 226 to 245.

Calls: [ostat](../util/unix.c.md#ostat), [setfilmode](unix.c.md#setfilmode)

Called by: [RF_setdirmode_adouble](vfs.c.md#rf_setdirmode_adouble)

### RF_setdirmode_adouble

```c
static int RF_setdirmode_adouble(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 247 to 280.

Calls: [ad_dir](../adouble/ad_open.c.md#ad_dir), [ad_hf_mode](../adouble/ad_open.c.md#ad_hf_mode), [dir_rx_set](unix.c.md#dir_rx_set), [for_each_adouble](vfs.c.md#for_each_adouble), [ochmod](../util/unix.c.md#ochmod), [setdirmode_adouble_loop](vfs.c.md#setdirmode_adouble_loop)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_setdirmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [vfs_setdirmode](vfs.c.md#vfs_setdirmode)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_setdirowner_adouble

```c
static int RF_setdirowner_adouble(const struct vol *vol, const char *name, uid_t uid, gid_t gid)
```

Defined at lines 282 to 292.

Calls: [fullpathname](../util/unix.c.md#fullpathname)

Called through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirowner](../../etc/afpd/unix.c.md#setdirowner), [vfs_setdirowner](vfs.c.md#vfs_setdirowner)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_deletefile_adouble

```c
static int RF_deletefile_adouble(const struct vol *vol, int dirfd, const char *file)
```

Defined at lines 295 to 299.

Calls: [netatalk_unlinkat](unix.c.md#netatalk_unlinkat)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletefile](../../etc/afpd/file.c.md#deletefile), [ftw_copy_file](../../bin/nad/nad_cp.c.md#ftw_copy_file), [rm](../../bin/nad/nad_rm.c.md#rm), [vfs_deletefile](vfs.c.md#vfs_deletefile)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_renamefile_adouble

```c
static int RF_renamefile_adouble(const struct vol *vol, int dirfd, const char *src, const char *dst)
```

Defined at lines 302 to 351.

Calls: [ad_close](../adouble/ad_flush.c.md#ad_close), [ad_init](../adouble/ad_open.c.md#ad_init), [ad_open](../adouble/ad_open.c.md#ad_open), [ostatat](../util/unix.c.md#ostatat), [unix_rename](unix.c.md#unix_rename)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [do_move](../../bin/nad/nad_mv.c.md#do_move), [renamefile](../../etc/afpd/file.c.md#renamefile), [vfs_renamefile](vfs.c.md#vfs_renamefile)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_copyfile_adouble

```c
static int RF_copyfile_adouble(const struct vol *vol, int sfd, const char *src, const char *dst)
```

Defined at lines 353 to 405.

Calls: [copy_file](unix.c.md#copy_file), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [vfs_copyfile](vfs.c.md#vfs_copyfile)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_solaris_acl

```c
static int RF_solaris_acl(const struct vol *vol, const char *path, int cmd, int count, void *aces)
```

Defined at lines 408 to 431.

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_solaris_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [set_acl](../../etc/afpd/acls.c.md#set_acl), [vfs_solaris_acl](vfs.c.md#vfs_solaris_acl)

Dispatched via: [netatalk_solaris_acl_adouble](vfs.c.md#netatalk_solaris_acl_adouble)

### RF_solaris_remove_acl

```c
static int RF_solaris_remove_acl(const struct vol *vol, const char *path, int dir)
```

Defined at lines 433 to 454.

Calls: [remove_nfsv4_acl_vfs](acl.c.md#remove_nfsv4_acl_vfs)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_remove_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [remove_acl](../../etc/afpd/acls.c.md#remove_acl), [vfs_remove_acl](vfs.c.md#vfs_remove_acl)

Dispatched via: [netatalk_solaris_acl_adouble](vfs.c.md#netatalk_solaris_acl_adouble)

### RF_posix_acl

```c
static int RF_posix_acl(const struct vol *vol, const char *path, acl_type_t type, int count, acl_t acl)
```

Defined at lines 458 to 481.

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_posix_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [vfs_posix_acl](vfs.c.md#vfs_posix_acl)

Dispatched via: [netatalk_posix_acl_adouble](vfs.c.md#netatalk_posix_acl_adouble)

### RF_posix_remove_acl

```c
static int RF_posix_remove_acl(const struct vol *vol, const char *path, int dir)
```

Defined at lines 483 to 500.

Calls: [remove_posix_acl_vfs](acl.c.md#remove_posix_acl_vfs)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_remove_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [remove_acl](../../etc/afpd/acls.c.md#remove_acl), [vfs_remove_acl](vfs.c.md#vfs_remove_acl)

Dispatched via: [netatalk_posix_acl_adouble](vfs.c.md#netatalk_posix_acl_adouble)

### validupath_ea

```c
static int validupath_ea(const struct vol *vol, const char *name)
```

Defined at lines 506 to 520.

Calls: [ad_valid_header_osx](../adouble/ad_open.c.md#ad_valid_header_osx), [netatalk_name](vfs.c.md#netatalk_name)

Called through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [check_dirent](../../etc/afpd/enumerate.c.md#check_dirent), [check_name](../../etc/afpd/filedir.c.md#check_name), [copy](../../bin/nad/nad_cp.c.md#copy), [dbd_readdir](../../bin/dbd/cmd_dbd_scanvol.c.md#dbd_readdir), [vfs_validupath](vfs.c.md#vfs_validupath)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_chown_ea

```c
static int RF_chown_ea(const struct vol *vol, const char *path, uid_t uid, gid_t gid)
```

Defined at lines 523 to 535.

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setfilowner](../../etc/afpd/unix.c.md#setfilowner), [vfs_chown](vfs.c.md#vfs_chown)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_renamedir_ea

```c
static int RF_renamedir_ea(const struct vol *vol, int dirfd, const char *oldpath, const char *newpath)
```

Defined at lines 538 to 542.

Called through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamedir](../../etc/afpd/directory.c.md#renamedir), [vfs_renamedir](vfs.c.md#vfs_renamedir)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### deletecurdir_ea_osx_chkifempty_loop

```c
static int deletecurdir_ea_osx_chkifempty_loop(const struct vol *vol, struct dirent *de, char *name, void *data, int flag)
```

Defined at lines 545 to 553.

### deletecurdir_ea_osx_loop

```c
static int deletecurdir_ea_osx_loop(const struct vol *vol, struct dirent *de, char *name, void *data, int flag)
```

Defined at lines 555 to 573.

Calls: [netatalk_unlink](unix.c.md#netatalk_unlink)

Called by: [RF_deletecurdir_ea](vfs.c.md#rf_deletecurdir_ea)

### RF_deletecurdir_ea

```c
static int RF_deletecurdir_ea(const struct vol *vol)
```

Defined at lines 576 to 590.

Calls: [deletecurdir_ea_osx_loop](vfs.c.md#deletecurdir_ea_osx_loop), [for_each_adouble](vfs.c.md#for_each_adouble)

Called through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [vfs_deletecurdir](vfs.c.md#vfs_deletecurdir)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_setdirunixmode_ea

```c
static int RF_setdirunixmode_ea(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 593 to 600.

Called through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode), [vfs_setdirunixmode](vfs.c.md#vfs_setdirunixmode)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_setfilmode_ea

```c
static int RF_setfilmode_ea(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 602 to 614.

Calls: [adouble_setfilmode](vfs.c.md#adouble_setfilmode)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode), [vfs_setfilmode](vfs.c.md#vfs_setfilmode)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_setdirmode_ea

```c
static int RF_setdirmode_ea(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 617 to 623.

Called through [`vfs_ops::vfs_setdirmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [vfs_setdirmode](vfs.c.md#vfs_setdirmode)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_setdirowner_ea

```c
static int RF_setdirowner_ea(const struct vol *vol, const char *name, uid_t uid, gid_t gid)
```

Defined at lines 626 to 632.

Called through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirowner](../../etc/afpd/unix.c.md#setdirowner), [vfs_setdirowner](vfs.c.md#vfs_setdirowner)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_deletefile_ea

```c
static int RF_deletefile_ea(const struct vol *vol, int dirfd, const char *file)
```

Defined at lines 634 to 644.

Calls: [netatalk_unlinkat](unix.c.md#netatalk_unlinkat)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletefile](../../etc/afpd/file.c.md#deletefile), [ftw_copy_file](../../bin/nad/nad_cp.c.md#ftw_copy_file), [rm](../../bin/nad/nad_rm.c.md#rm), [vfs_deletefile](vfs.c.md#vfs_deletefile)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_copyfile_ea

```c
static int RF_copyfile_ea(const struct vol *vol, int sfd, const char *src, const char *dst)
```

Defined at lines 645 to 707.

Calls: [copy_file](unix.c.md#copy_file), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [vfs_copyfile](vfs.c.md#vfs_copyfile)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_renamefile_ea

```c
static int RF_renamefile_ea(const struct vol *vol, int dirfd, const char *src, const char *dst)
```

Defined at lines 710 to 739.

Calls: [ostatat](../util/unix.c.md#ostatat), [unix_rename](unix.c.md#unix_rename)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [do_move](../../bin/nad/nad_mv.c.md#do_move), [renamefile](../../etc/afpd/file.c.md#renamefile), [vfs_renamefile](vfs.c.md#vfs_renamefile)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### vfs_chown

```c
static int vfs_chown(const struct vol *vol, const char *path, uid_t uid, gid_t gid)
```

Defined at lines 760 to 776.

Calls through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_chown_adouble](vfs.c.md#rf_chown_adouble), [RF_chown_ea](vfs.c.md#rf_chown_ea), [ea_chown](ea_ad.c.md#ea_chown)

Called through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setfilowner](../../etc/afpd/unix.c.md#setfilowner)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_renamedir

```c
static int vfs_renamedir(const struct vol *vol, int dirfd, const char *oldpath, const char *newpath)
```

Defined at lines 778 to 794.

Calls through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_renamedir_adouble](vfs.c.md#rf_renamedir_adouble), [RF_renamedir_ea](vfs.c.md#rf_renamedir_ea)

Called through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamedir](../../etc/afpd/directory.c.md#renamedir)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_deletecurdir

```c
static int vfs_deletecurdir(const struct vol *vol)
```

Defined at lines 796 to 811.

Calls through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletecurdir_adouble](vfs.c.md#rf_deletecurdir_adouble), [RF_deletecurdir_ea](vfs.c.md#rf_deletecurdir_ea)

Called through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_setfilmode

```c
static int vfs_setfilmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 813 to 829.

Calls through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setfilmode_adouble](vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](vfs.c.md#rf_setfilmode_ea), [ea_chmod_file](ea_ad.c.md#ea_chmod_file)

Called through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_setdirmode

```c
static int vfs_setdirmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 831 to 847.

Calls through [`vfs_ops::vfs_setdirmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirmode_adouble](vfs.c.md#rf_setdirmode_adouble), [RF_setdirmode_ea](vfs.c.md#rf_setdirmode_ea)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_setdirunixmode

```c
static int vfs_setdirunixmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 849 to 865.

Calls through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirunixmode_adouble](vfs.c.md#rf_setdirunixmode_adouble), [RF_setdirunixmode_ea](vfs.c.md#rf_setdirunixmode_ea), [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir)

Called through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_setdirowner

```c
static int vfs_setdirowner(const struct vol *vol, const char *name, uid_t uid, gid_t gid)
```

Defined at lines 867 to 883.

Calls through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirowner_adouble](vfs.c.md#rf_setdirowner_adouble), [RF_setdirowner_ea](vfs.c.md#rf_setdirowner_ea)

Called through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirowner](../../etc/afpd/unix.c.md#setdirowner)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_deletefile

```c
static int vfs_deletefile(const struct vol *vol, int dirfd, const char *file)
```

Defined at lines 885 to 900.

Calls through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletefile_adouble](vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](vfs.c.md#rf_deletefile_ea), [ea_deletefile](ea_ad.c.md#ea_deletefile)

Called through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletefile](../../etc/afpd/file.c.md#deletefile), [ftw_copy_file](../../bin/nad/nad_cp.c.md#ftw_copy_file), [rm](../../bin/nad/nad_rm.c.md#rm)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_renamefile

```c
static int vfs_renamefile(const struct vol *vol, int dirfd, const char *src, const char *dst)
```

Defined at lines 902 to 918.

Calls through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_renamefile_adouble](vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](vfs.c.md#rf_renamefile_ea), [ea_renamefile](ea_ad.c.md#ea_renamefile)

Called through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [do_move](../../bin/nad/nad_mv.c.md#do_move), [renamefile](../../etc/afpd/file.c.md#renamefile)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_copyfile

```c
static int vfs_copyfile(const struct vol *vol, int sfd, const char *src, const char *dst)
```

Defined at lines 920 to 936.

Calls through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_copyfile_adouble](vfs.c.md#rf_copyfile_adouble), [RF_copyfile_ea](vfs.c.md#rf_copyfile_ea), [ea_copyfile](ea_ad.c.md#ea_copyfile), [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile)

Called through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_solaris_acl

```c
static int vfs_solaris_acl(const struct vol *vol, const char *path, int cmd, int count, void *aces)
```

Defined at lines 940 to 957.

Calls through [`vfs_ops::vfs_solaris_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_solaris_acl](vfs.c.md#rf_solaris_acl)

Called through [`vfs_ops::vfs_solaris_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [set_acl](../../etc/afpd/acls.c.md#set_acl)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_posix_acl

```c
static int vfs_posix_acl(const struct vol *vol, const char *path, acl_type_t type, int count, acl_t acl)
```

Defined at lines 960 to 976.

Calls through [`vfs_ops::vfs_posix_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_posix_acl](vfs.c.md#rf_posix_acl)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_remove_acl

```c
static int vfs_remove_acl(const struct vol *vol, const char *path, int dir)
```

Defined at lines 979 to 994.

Calls through [`vfs_ops::vfs_remove_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_posix_remove_acl](vfs.c.md#rf_posix_remove_acl), [RF_solaris_remove_acl](vfs.c.md#rf_solaris_remove_acl)

Called through [`vfs_ops::vfs_remove_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [remove_acl](../../etc/afpd/acls.c.md#remove_acl)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_attrname_invalid

```c
static int vfs_ea_attrname_invalid(const char *func, const char *attruname)
```

Defined at lines 997 to 1005.

Called by: [vfs_ea_getcontent](vfs.c.md#vfs_ea_getcontent), [vfs_ea_getsize](vfs.c.md#vfs_ea_getsize), [vfs_ea_remove](vfs.c.md#vfs_ea_remove), [vfs_ea_set](vfs.c.md#vfs_ea_set)

### vfs_ea_getsize

```c
static int vfs_ea_getsize(const struct vol *vol, char *rbuf, size_t *rbuflen, const char *uname, int oflag, const char *attruname, int fd)
```

Defined at lines 1007 to 1029.

Calls: [vfs_ea_attrname_invalid](vfs.c.md#vfs_ea_attrname_invalid)

Calls through [`vfs_ops::vfs_ea_getsize`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_easize](ea_ad.c.md#get_easize), [sys_get_easize](ea_sys.c.md#sys_get_easize)

Called through [`vfs_ops::vfs_ea_getsize`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_getcontent

```c
static int vfs_ea_getcontent(const struct vol *vol, char *rbuf, size_t *rbuflen, const char *uname, int oflag, const char *attruname, int maxreply, int fd)
```

Defined at lines 1031 to 1053.

Calls: [vfs_ea_attrname_invalid](vfs.c.md#vfs_ea_attrname_invalid)

Calls through [`vfs_ops::vfs_ea_getcontent`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_eacontent](ea_ad.c.md#get_eacontent), [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent)

Called through [`vfs_ops::vfs_ea_getcontent`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_list

```c
static int vfs_ea_list(const struct vol *vol, char *attrnamebuf, size_t *buflen, const char *uname, int oflag, int fd)
```

Defined at lines 1055 to 1073.

Calls through [`vfs_ops::vfs_ea_list`](../../include/atalk/vfs.h.md#struct-vfs_ops): [list_eas](ea_ad.c.md#list_eas), [sys_list_eas](ea_sys.c.md#sys_list_eas)

Called through [`vfs_ops::vfs_ea_list`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_set

```c
static int vfs_ea_set(const struct vol *vol, const char *uname, const char *attruname, const char *ibuf, size_t attrsize, int oflag, int fd)
```

Defined at lines 1075 to 1097.

Calls: [vfs_ea_attrname_invalid](vfs.c.md#vfs_ea_attrname_invalid)

Calls through [`vfs_ops::vfs_ea_set`](../../include/atalk/vfs.h.md#struct-vfs_ops): [set_ea](ea_ad.c.md#set_ea), [sys_set_ea](ea_sys.c.md#sys_set_ea)

Called through [`vfs_ops::vfs_ea_set`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_remove

```c
static int vfs_ea_remove(const struct vol *vol, const char *uname, const char *attruname, int oflag, int fd)
```

Defined at lines 1099 to 1120.

Calls: [vfs_ea_attrname_invalid](vfs.c.md#vfs_ea_attrname_invalid)

Calls through [`vfs_ops::vfs_ea_remove`](../../include/atalk/vfs.h.md#struct-vfs_ops): [remove_ea](ea_ad.c.md#remove_ea), [sys_remove_ea](ea_sys.c.md#sys_remove_ea)

Called through [`vfs_ops::vfs_ea_remove`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_validupath

```c
static int vfs_validupath(const struct vol *vol, const char *name)
```

Defined at lines 1122 to 1125.

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](vfs.c.md#validupath_adouble), [validupath_ea](vfs.c.md#validupath_ea)

Called through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [check_dirent](../../etc/afpd/enumerate.c.md#check_dirent), [check_name](../../etc/afpd/filedir.c.md#check_name), [copy](../../bin/nad/nad_cp.c.md#copy), [dbd_readdir](../../bin/dbd/cmd_dbd_scanvol.c.md#dbd_readdir)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### initvol_vfs

```c
void initvol_vfs(struct vol *vol)
```

Defined at lines 1329 to 1366.

Calls: [ad_path](../adouble/ad_open.c.md#ad_path), [ad_path_ea](../adouble/ad_open.c.md#ad_path_ea), [ad_path_osx](../adouble/ad_open.c.md#ad_path_osx)

Called by: [creatvol](../util/netatalk_conf.c.md#creatvol)

Uses file-scope variables: `netatalk_adouble_ea`, `netatalk_adouble_v2`, `netatalk_ea_adouble`, `netatalk_ea_sys`, `netatalk_posix_acl_adouble`, `netatalk_solaris_acl_adouble`, `vfs_master_funcs`

# Types

### struct perm

Defined at line 46.
* `uid_t uid`
* `gid_t gid`

# Typedefs and enums

* `typedef int(* rf_loop`
