---
type: C Source File
title: "libatalk/vfs/vfs.c"
description: "54 functions, 1 type, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/vfs.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Initialized at line 1815 as `struct vfs_ops`. Assigns:

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

Initialized at line 1787 as `struct vfs_ops`. Assigns:

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

Initialized at line 1847 as `struct vfs_ops`. Assigns:

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

Initialized at line 1875 as `struct vfs_ops`. Assigns:

* `vfs_copyfile`: [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile)
* `vfs_ea_getcontent`: [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent)
* `vfs_ea_getsize`: [sys_get_easize](ea_sys.c.md#sys_get_easize)
* `vfs_ea_list`: [sys_list_eas](ea_sys.c.md#sys_list_eas)
* `vfs_ea_remove`: [sys_remove_ea](ea_sys.c.md#sys_remove_ea)
* `vfs_ea_set`: [sys_set_ea](ea_sys.c.md#sys_set_ea)

### netatalk_posix_acl_adouble

Initialized at line 1931 as `struct vfs_ops`. Assigns:

* `vfs_posix_acl`: [RF_posix_acl](vfs.c.md#rf_posix_acl)
* `vfs_remove_acl`: [RF_posix_remove_acl](vfs.c.md#rf_posix_remove_acl)

### netatalk_solaris_acl_adouble

Initialized at line 1908 as `struct vfs_ops`. Assigns:

* `vfs_remove_acl`: [RF_solaris_remove_acl](vfs.c.md#rf_solaris_remove_acl)
* `vfs_solaris_acl`: [RF_solaris_acl](vfs.c.md#rf_solaris_acl)

### vfs_master_funcs

These function pointers get called from the lib users via vol->vfs->func.

Initialized at line 1755 as `struct vfs_ops`. Assigns:

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

Defined at lines 71 to 112.

Call fn for every entry of directory name except .

and ..

fn receives the entry and its path below name, and the walk stops at the first nonzero result. A directory that fails to open yields 0 under a nonzero flag. Otherwise the call logs an error that names from and fails.

Parameters:
* `from`: caller name, for the log line
* `name`: directory to walk
* `fn`: callback, or NULL to walk without one
* `vol`: volume, passed to fn
* `data`: callback data, passed to fn
* `flag`: nonzero to tolerate an opendir failure, passed to fn

Returns: 0, the first nonzero fn result, or -1 when opendir failed and flag is 0

Calls: [fullpathname](../util/unix.c.md#fullpathname), [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [RF_deletecurdir_adouble](vfs.c.md#rf_deletecurdir_adouble), [RF_deletecurdir_ea](vfs.c.md#rf_deletecurdir_ea), [RF_setdirmode_adouble](vfs.c.md#rf_setdirmode_adouble)

### netatalk_name

```c
static int netatalk_name(const char *name)
```

Defined at lines 120 to 123.

Whether name differs from the netatalk database directory names.

Parameters:
* `name`: directory entry name

Returns: 1 for any name except .AppleDB and .AppleDesktop, 0 for those two

Called by: [validupath_adouble](vfs.c.md#validupath_adouble), [validupath_ea](vfs.c.md#validupath_ea)

### validupath_adouble

```c
static int validupath_adouble(const struct vol *vol, const char *name)
```

Defined at lines 140 to 148.

Whether clients may see name on a version 2 AppleDouble volume.

Names without a leading dot always pass. The netatalk directories .AppleDB, .AppleDesktop, and .AppleDouble stay hidden, and so does .Parent in any letter case.

Parameters:
* `vol`: unused
* `name`: directory entry name

Returns: 1 when clients may see name, 0 otherwise

Calls: [netatalk_name](vfs.c.md#netatalk_name)

Called through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [check_dirent](../../etc/afpd/enumerate.c.md#check_dirent), [check_name](../../etc/afpd/filedir.c.md#check_name), [dbd_readdir](../../bin/dbd/cmd_dbd_scanvol.c.md#dbd_readdir), [vfs_validupath](vfs.c.md#vfs_validupath)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_chown_adouble

```c
static int RF_chown_adouble(const struct vol *vol, const char *path, uid_t uid, gid_t gid)
```

Defined at lines 161 to 176.

Give the version 2 metadata file of path a new owner.

chown follows symlinks, and a missing metadata file counts as success.

Parameters:
* `vol`: volume holding the file
* `path`: file whose metadata file to change
* `uid`: new owner
* `gid`: new group

Returns: 0, or -1 with errno set

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setfilowner](../../etc/afpd/unix.c.md#setfilowner), [vfs_chown](vfs.c.md#vfs_chown)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_renamedir_adouble

```c
static int RF_renamedir_adouble(const struct vol *vol, int dirfd, const char *oldpath, const char *newpath)
```

Defined at lines 188 to 193.

Rename nothing, because the .AppleDouble directory moves with the directory.

Parameters:
* `vol`: unused
* `dirfd`: unused
* `oldpath`: unused
* `newpath`: unused

Returns: 0

Called through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamedir](../../etc/afpd/directory.c.md#renamedir), [vfs_renamedir](vfs.c.md#vfs_renamedir)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### deletecurdir_adouble_loop

```c
static int deletecurdir_adouble_loop(const struct vol *vol, struct dirent *de, char *name, void *data, int flag)
```

Defined at lines 211 to 229.

Delete one .AppleDouble entry whose file no longer exists in the working directory.

A file of the same name in the working directory means the directory still has content, and the walk stops with AFPERR_DIRNEMPT. lstat also sees a dangling symlink. Any other entry, the .Parent file among them, goes through netatalk_unlink.

Parameters:
* `vol`: unused
* `de`: .AppleDouble entry
* `name`: path of the entry
* `data`: unused
* `flag`: unused

Returns: AFP_OK, AFPERR_DIRNEMPT, or the netatalk_unlink error

Calls: [netatalk_unlink](unix.c.md#netatalk_unlink)

Called by: [RF_deletecurdir_adouble](vfs.c.md#rf_deletecurdir_adouble)

### RF_deletecurdir_adouble

```c
static int RF_deletecurdir_adouble(const struct vol *vol)
```

Defined at lines 242 to 254.

Delete the .AppleDouble directory of the working directory.

The walk first deletes every metadata file whose file no longer exists, the .Parent file among them. A metadata file that still has its file stops the call with AFPERR_DIRNEMPT. A missing .AppleDouble directory counts as success.

Parameters:
* `vol`: volume holding the directory

Returns: AFP_OK, or the AFP error of the first failure

Calls: [deletecurdir_adouble_loop](vfs.c.md#deletecurdir_adouble_loop), [for_each_adouble](vfs.c.md#for_each_adouble), [netatalk_rmdir](unix.c.md#netatalk_rmdir)

Called through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [vfs_deletecurdir](vfs.c.md#vfs_deletecurdir)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### adouble_setfilmode

```c
static int adouble_setfilmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 265 to 269.

Derive a metadata file mode with ad_hf_mode and apply it to name.

Parameters:
* `vol`: volume holding the file
* `name`: metadata file
* `mode`: data file mode
* `st`: stat whose non-permission bits to keep, or NULL

Returns: 0, or -1 with errno set

Calls: [ad_hf_mode](../adouble/ad_open.c.md#ad_hf_mode), [setfilmode](unix.c.md#setfilmode)

Called by: [RF_setdirunixmode_adouble](vfs.c.md#rf_setdirunixmode_adouble), [RF_setfilmode_adouble](vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](vfs.c.md#rf_setfilmode_ea)

### RF_setfilmode_adouble

```c
static int RF_setfilmode_adouble(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 283 to 287.

Apply the mode of a data file to its version 2 metadata file.

ad_hf_mode derives the metadata mode, so read access also grants write access for fcntl locks.

Parameters:
* `vol`: volume holding the file
* `name`: data file
* `mode`: new data file mode
* `st`: stat of the data file, or NULL

Returns: 0, or -1 with errno set

Calls: [adouble_setfilmode](vfs.c.md#adouble_setfilmode)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode), [vfs_setfilmode](vfs.c.md#vfs_setfilmode)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_setdirunixmode_adouble

```c
static int RF_setdirunixmode_adouble(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 303 to 333.

Apply a directory mode to its .AppleDouble directory and .Parent file.

The .AppleDouble directory gets DIRBITS plus mode, minus the volume umask, and .Parent gets the ad_hf_mode result. A mode that grants the owner read and search access changes the directory first, otherwise last. That ordering keeps .Parent reachable while it changes.

Parameters:
* `vol`: volume holding the directory
* `name`: directory
* `mode`: new directory mode
* `st`: stat of name, or NULL

Returns: 0, or -1 with errno set

Calls: [ad_dir](../adouble/ad_open.c.md#ad_dir), [adouble_setfilmode](vfs.c.md#adouble_setfilmode), [dir_rx_set](unix.c.md#dir_rx_set), [ochmod](../util/unix.c.md#ochmod)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode), [vfs_setdirunixmode](vfs.c.md#vfs_setdirunixmode)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### setdirmode_adouble_loop

```c
static int setdirmode_adouble_loop(const struct vol *vol, struct dirent *de, char *name, void *data, int flag)
```

Defined at lines 349 to 368.

Apply the metadata mode in data to one .AppleDouble entry.

Subdirectories keep their mode. A failed stat logs an error unless flag is nonzero, and a failed mode change passes without notice, so the walk always continues.

Parameters:
* `vol`: volume holding the directory
* `de`: unused
* `name`: path of the entry
* `data`: pointer to the metadata mode
* `flag`: nonzero to stay silent about a failed stat

Returns: 0

Calls: [ostat](../util/unix.c.md#ostat), [setfilmode](unix.c.md#setfilmode)

Called by: [RF_setdirmode_adouble](vfs.c.md#rf_setdirmode_adouble)

### RF_setdirmode_adouble

```c
static int RF_setdirmode_adouble(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 385 to 418.

Apply a directory mode to its .AppleDouble directory and the metadata files there.

The .AppleDouble directory gets DIRBITS plus mode, minus the volume umask. Each metadata file gets the ad_hf_mode result. A mode that grants the owner read and search access changes the directory first, otherwise last, so the metadata files stay reachable while they change.

Parameters:
* `vol`: volume holding the directory
* `name`: directory
* `mode`: new directory mode
* `st`: stat of name, or NULL

Returns: 0, or -1 when a chmod or the directory walk failed

Calls: [ad_dir](../adouble/ad_open.c.md#ad_dir), [ad_hf_mode](../adouble/ad_open.c.md#ad_hf_mode), [dir_rx_set](unix.c.md#dir_rx_set), [for_each_adouble](vfs.c.md#for_each_adouble), [ochmod](../util/unix.c.md#ochmod), [setdirmode_adouble_loop](vfs.c.md#setdirmode_adouble_loop)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_setdirmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [vfs_setdirmode](vfs.c.md#vfs_setdirmode)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_setdirowner_adouble

```c
static int RF_setdirowner_adouble(const struct vol *vol, const char *name, uid_t uid, gid_t gid)
```

Defined at lines 432 to 442.

Give the .AppleDouble directory of the working directory a new owner.

The call ignores name and acts on the working directory. EPERM passes without notice, and other failures only log at debug level.

Parameters:
* `vol`: unused
* `name`: unused
* `uid`: new owner
* `gid`: new group

Returns: 0

Calls: [fullpathname](../util/unix.c.md#fullpathname)

Called through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirowner](../../etc/afpd/unix.c.md#setdirowner), [vfs_setdirowner](vfs.c.md#vfs_setdirowner)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_deletefile_adouble

```c
static int RF_deletefile_adouble(const struct vol *vol, int dirfd, const char *file)
```

Defined at lines 456 to 460.

Delete the version 2 metadata file of file.

A missing metadata file counts as success.

Parameters:
* `vol`: volume holding the file
* `dirfd`: directory that file is relative to, or -1 for the working directory
* `file`: data file

Returns: AFP_OK, AFPERR_VLOCK on a read-only file system, AFPERR_ACCESS, or AFPERR_PARAM

Calls: [netatalk_unlinkat](unix.c.md#netatalk_unlinkat)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletefile](../../etc/afpd/file.c.md#deletefile), [vfs_deletefile](vfs.c.md#vfs_deletefile)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_renamefile_adouble

```c
static int RF_renamefile_adouble(const struct vol *vol, int dirfd, const char *src, const char *dst)
```

Defined at lines 478 to 527.

Rename the version 2 metadata file of src to match dst.

src is relative to dirfd, and dst is relative to the working directory. A source without a metadata file needs nothing and succeeds. When the rename fails with ENOENT for an existing source, the destination .AppleDouble directory is missing. ad_open then creates it through a separate AppleDouble, and the rename runs again.

Parameters:
* `vol`: volume holding the file
* `dirfd`: directory that src is relative to, or -1 for the working directory
* `src`: current file name
* `dst`: new file name

Returns: 0, or -1 with errno set

Calls: [ad_close](../adouble/ad_flush.c.md#ad_close), [ad_init](../adouble/ad_open.c.md#ad_init), [ad_open](../adouble/ad_open.c.md#ad_open), [ostatat](../util/unix.c.md#ostatat), [unix_rename](unix.c.md#unix_rename)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamefile](../../etc/afpd/file.c.md#renamefile), [vfs_renamefile](vfs.c.md#vfs_renamefile)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_copyfile_adouble

```c
static int RF_copyfile_adouble(const struct vol *vol, int sfd, const char *src, const char *dst)
```

Defined at lines 544 to 596.

Clone the version 2 metadata file of src for dst.

The call stats dst first and fails when it does not exist. A directory pair uses the .Parent files, and a file pair uses the files in the .AppleDouble directory beside each. A source without a metadata file needs nothing and succeeds.

Parameters:
* `vol`: unused
* `sfd`: directory that src is relative to, or -1 for the working directory
* `src`: source file or directory
* `dst`: existing destination file or directory

Returns: 0, or -1 with errno set

Calls: [copy_file](unix.c.md#copy_file), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [copyfile](../../etc/afpd/file.c.md#copyfile), [vfs_copyfile](vfs.c.md#vfs_copyfile)

Dispatched via: [netatalk_adouble_v2](vfs.c.md#netatalk_adouble_v2)

### RF_solaris_acl

```c
static int RF_solaris_acl(const struct vol *vol, const char *path, int cmd, int count, void *aces)
```

Defined at lines 612 to 635.

Apply an NFSv4 ACL change to the AppleDouble file of path.

A missing path and a directory need nothing and succeed. For a file, acl runs cmd on the metadata path that vol->ad_path returns.

Parameters:
* `vol`: volume holding the file
* `path`: file or directory
* `cmd`: acl command, ACE_SETACL from the callers
* `count`: number of entries in aces
* `aces`: NFSv4 access control entries

Returns: AFP_OK, or AFPERR_MISC when the stat or the acl call failed

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_solaris_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [set_acl](../../etc/afpd/acls.c.md#set_acl), [vfs_solaris_acl](vfs.c.md#vfs_solaris_acl)

Dispatched via: [netatalk_solaris_acl_adouble](vfs.c.md#netatalk_solaris_acl_adouble)

### RF_solaris_remove_acl

```c
static int RF_solaris_remove_acl(const struct vol *vol, const char *path, int dir)
```

Defined at lines 649 to 670.

Keep only the owner@, group@, and everyone@ entries in the NFSv4 ACL of an AppleDouble file.

The AppleDouble file is the one that vol->ad_path returns for path. Directories need nothing, and a missing AppleDouble file counts as success.

Parameters:
* `vol`: volume holding the file
* `path`: file or directory
* `dir`: nonzero when path is a directory

Returns: AFP_OK, or the AFP error from remove_nfsv4_acl_vfs

Calls: [remove_nfsv4_acl_vfs](acl.c.md#remove_nfsv4_acl_vfs)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_remove_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [remove_acl](../../etc/afpd/acls.c.md#remove_acl), [vfs_remove_acl](vfs.c.md#vfs_remove_acl)

Dispatched via: [netatalk_solaris_acl_adouble](vfs.c.md#netatalk_solaris_acl_adouble)

### RF_posix_acl

```c
static int RF_posix_acl(const struct vol *vol, const char *path, acl_type_t type, int count, acl_t acl)
```

Defined at lines 689 to 712.

Apply a POSIX ACL to the AppleDouble file of path.

A directory needs nothing. For a file, acl_set_file sets acl of type on the metadata path that vol->ad_path returns. ENOENT from either the stat or the ACL call counts as success.

Parameters:
* `vol`: volume holding the file
* `path`: file or directory
* `type`: ACL_TYPE_ACCESS or ACL_TYPE_DEFAULT
* `count`: unused
* `acl`: POSIX ACL to set

Returns: AFP_OK, AFPERR_MISC when acl_set_file failed, or -1 when the stat failed

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_posix_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [vfs_posix_acl](vfs.c.md#vfs_posix_acl)

Dispatched via: [netatalk_posix_acl_adouble](vfs.c.md#netatalk_posix_acl_adouble)

### RF_posix_remove_acl

```c
static int RF_posix_remove_acl(const struct vol *vol, const char *path, int dir)
```

Defined at lines 726 to 743.

Drop the ACL_USER, ACL_GROUP, ACL_MASK, and default entries from an AppleDouble file.

The AppleDouble file is the one that vol->ad_path returns for path. Directories need nothing, and a missing AppleDouble file counts as success.

Parameters:
* `vol`: volume holding the file
* `path`: file or directory
* `dir`: nonzero when path is a directory

Returns: AFP_OK, or AFPERR_MISC when remove_posix_acl_vfs failed

Calls: [remove_posix_acl_vfs](acl.c.md#remove_posix_acl_vfs)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_remove_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [remove_acl](../../etc/afpd/acls.c.md#remove_acl), [vfs_remove_acl](vfs.c.md#vfs_remove_acl)

Dispatched via: [netatalk_posix_acl_adouble](vfs.c.md#netatalk_posix_acl_adouble)

### validupath_ea

```c
static int validupath_ea(const struct vol *vol, const char *name)
```

Defined at lines 760 to 774.

Whether clients may see name on an ea AppleDouble volume.

Names without a leading dot always pass. Of the dot names, .AppleDB and .AppleDesktop stay hidden. Without HAVE_EAFD a ._ file stays hidden when it carries a netatalk AppleDouble header, because it is a resource fork sidecar.

Parameters:
* `vol`: unused
* `name`: directory entry name

Returns: 1 when clients may see name, 0 otherwise

Calls: [ad_valid_header_osx](../adouble/ad_open.c.md#ad_valid_header_osx), [netatalk_name](vfs.c.md#netatalk_name)

Called through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [check_dirent](../../etc/afpd/enumerate.c.md#check_dirent), [check_name](../../etc/afpd/filedir.c.md#check_name), [dbd_readdir](../../bin/dbd/cmd_dbd_scanvol.c.md#dbd_readdir), [vfs_validupath](vfs.c.md#vfs_validupath)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_chown_ea

```c
static int RF_chown_ea(const struct vol *vol, const char *path, uid_t uid, gid_t gid)
```

Defined at lines 788 to 800.

Give the ._ resource fork sidecar of path a new owner.

Without HAVE_EAFD chown follows symlinks, and a missing sidecar fails with ENOENT. With HAVE_EAFD the resource fork lives in an EA, so nothing changes.

Parameters:
* `vol`: volume holding the file
* `path`: file whose sidecar to change
* `uid`: new owner
* `gid`: new group

Returns: 0, or -1 with errno set

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setfilowner](../../etc/afpd/unix.c.md#setfilowner), [vfs_chown](vfs.c.md#vfs_chown)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_renamedir_ea

```c
static int RF_renamedir_ea(const struct vol *vol, int dirfd, const char *oldpath, const char *newpath)
```

Defined at lines 812 to 816.

Rename nothing, because the ._ sidecars inside the directory move with it.

Parameters:
* `vol`: unused
* `dirfd`: unused
* `oldpath`: unused
* `newpath`: unused

Returns: 0

Called through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamedir](../../etc/afpd/directory.c.md#renamedir), [vfs_renamedir](vfs.c.md#vfs_renamedir)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### deletecurdir_ea_osx_chkifempty_loop

```c
static int deletecurdir_ea_osx_chkifempty_loop(const struct vol *vol, struct dirent *de, char *name, void *data, int flag)
```

Defined at lines 828 to 836.

Report a directory entry whose name has no leading dot.

Parameters:
* `vol`: unused
* `de`: directory entry
* `name`: unused
* `data`: unused
* `flag`: unused

Returns: 1 when the name lacks a leading dot, 0 for every dot name

### deletecurdir_ea_osx_loop

```c
static int deletecurdir_ea_osx_loop(const struct vol *vol, struct dirent *de, char *name, void *data, int flag)
```

Defined at lines 849 to 867.

Delete name when the path starts with ._ and names a regular file.

Parameters:
* `vol`: unused
* `de`: unused
* `name`: path of the entry, including the prefix of the walked directory
* `data`: unused
* `flag`: unused

Returns: 0, -1 when lstat failed, or the netatalk_unlink error

Calls: [netatalk_unlink](unix.c.md#netatalk_unlink)

Called by: [RF_deletecurdir_ea](vfs.c.md#rf_deletecurdir_ea)

### RF_deletecurdir_ea

```c
static int RF_deletecurdir_ea(const struct vol *vol)
```

Defined at lines 880 to 894.

Run deletecurdir_ea_osx_loop over the working directory before afpd deletes it.

Without HAVE_EAFD the walk hands every entry path to deletecurdir_ea_osx_loop and stops at the first error. With HAVE_EAFD the resource forks live in EAs, so the call does nothing.

Parameters:
* `vol`: volume holding the directory

Returns: 0, or the first error of the walk

Calls: [deletecurdir_ea_osx_loop](vfs.c.md#deletecurdir_ea_osx_loop), [for_each_adouble](vfs.c.md#for_each_adouble)

Called through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [vfs_deletecurdir](vfs.c.md#vfs_deletecurdir)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_setdirunixmode_ea

```c
static int RF_setdirunixmode_ea(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 906 to 913.

Change nothing, because the ea layout keeps directory metadata in an EA of the directory.

Parameters:
* `vol`: unused
* `name`: unused
* `mode`: unused
* `st`: unused

Returns: 0

Called through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode), [vfs_setdirunixmode](vfs.c.md#vfs_setdirunixmode)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_setfilmode_ea

```c
static int RF_setfilmode_ea(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 927 to 939.

Apply the mode of a data file to its ._ resource fork sidecar.

ad_hf_mode derives the sidecar mode. With HAVE_EAFD the resource fork lives in an EA, so nothing changes.

Parameters:
* `vol`: volume holding the file
* `name`: data file
* `mode`: new data file mode
* `st`: stat of the data file, or NULL

Returns: 0, or -1 with errno set

Calls: [adouble_setfilmode](vfs.c.md#adouble_setfilmode)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode), [vfs_setfilmode](vfs.c.md#vfs_setfilmode)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_setdirmode_ea

```c
static int RF_setdirmode_ea(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 951 to 957.

Change nothing, because the ea layout keeps directory metadata in an EA of the directory.

Parameters:
* `vol`: unused
* `name`: unused
* `mode`: unused
* `st`: unused

Returns: 0

Called through [`vfs_ops::vfs_setdirmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [vfs_setdirmode](vfs.c.md#vfs_setdirmode)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_setdirowner_ea

```c
static int RF_setdirowner_ea(const struct vol *vol, const char *name, uid_t uid, gid_t gid)
```

Defined at lines 969 to 975.

Change nothing, because the ea layout keeps directory metadata in an EA of the directory.

Parameters:
* `vol`: unused
* `name`: unused
* `uid`: unused
* `gid`: unused

Returns: 0

Called through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirowner](../../etc/afpd/unix.c.md#setdirowner), [vfs_setdirowner](vfs.c.md#vfs_setdirowner)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_deletefile_ea

```c
static int RF_deletefile_ea(const struct vol *vol, int dirfd, const char *file)
```

Defined at lines 990 to 1000.

Delete the ._ resource fork sidecar of file.

A missing sidecar counts as success. With HAVE_EAFD the resource fork lives in an EA, so nothing needs deleting.

Parameters:
* `vol`: volume holding the file
* `dirfd`: directory that file is relative to, or -1 for the working directory
* `file`: data file

Returns: AFP_OK, AFPERR_VLOCK on a read-only file system, AFPERR_ACCESS, or AFPERR_PARAM

Calls: [netatalk_unlinkat](unix.c.md#netatalk_unlinkat)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletefile](../../etc/afpd/file.c.md#deletefile), [vfs_deletefile](vfs.c.md#vfs_deletefile)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_copyfile_ea

```c
static int RF_copyfile_ea(const struct vol *vol, int sfd, const char *src, const char *dst)
```

Defined at lines 1015 to 1077.

Clone the resource fork sidecar of src for dst.

The sidecar is ._name beside the file, or name/..namedfork/rsrc on macOS. A source without a sidecar needs nothing and succeeds. On Solaris with HAVE_EAFD the EA module transfers the resource fork, so nothing happens here.

Parameters:
* `vol`: unused
* `sfd`: directory that src is relative to, or -1 for the working directory
* `src`: source file
* `dst`: destination file

Returns: 0, or -1 with errno set

Calls: [copy_file](unix.c.md#copy_file), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [copyfile](../../etc/afpd/file.c.md#copyfile), [vfs_copyfile](vfs.c.md#vfs_copyfile)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### RF_renamefile_ea

```c
static int RF_renamefile_ea(const struct vol *vol, int dirfd, const char *src, const char *dst)
```

Defined at lines 1093 to 1122.

Rename the ._ resource fork sidecar of src to match dst.

src is relative to dirfd, and dst is relative to the working directory. A source without a sidecar needs nothing and succeeds. With HAVE_EAFD the resource fork lives in an EA and moves with the file.

Parameters:
* `vol`: volume holding the file
* `dirfd`: directory that src is relative to, or -1 for the working directory
* `src`: current file name
* `dst`: new file name

Returns: 0, or -1 with errno set

Calls: [ostatat](../util/unix.c.md#ostatat), [unix_rename](unix.c.md#unix_rename)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Called through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamefile](../../etc/afpd/file.c.md#renamefile), [vfs_renamefile](vfs.c.md#vfs_renamefile)

Dispatched via: [netatalk_adouble_ea](vfs.c.md#netatalk_adouble_ea)

### vfs_chown

```c
static int vfs_chown(const struct vol *vol, const char *path, uid_t uid, gid_t gid)
```

Defined at lines 1155 to 1171.

Run the vfs_chown hook of every VFS module of vol.

The modules run in order: AppleDouble layout, EA backend, and ACL support. Every module runs even after a failure.

Parameters:
* `vol`: volume
* `path`: file whose metadata to change
* `uid`: new owner
* `gid`: new group

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_chown_adouble](vfs.c.md#rf_chown_adouble), [RF_chown_ea](vfs.c.md#rf_chown_ea), [ea_chown](ea_ad.c.md#ea_chown)

Called through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setfilowner](../../etc/afpd/unix.c.md#setfilowner)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_renamedir

```c
static int vfs_renamedir(const struct vol *vol, int dirfd, const char *oldpath, const char *newpath)
```

Defined at lines 1185 to 1201.

Run the vfs_renamedir hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `dirfd`: directory that oldpath is relative to, or -1 for the working directory
* `oldpath`: current directory name
* `newpath`: new directory name

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_renamedir_adouble](vfs.c.md#rf_renamedir_adouble), [RF_renamedir_ea](vfs.c.md#rf_renamedir_ea)

Called through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamedir](../../etc/afpd/directory.c.md#renamedir)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_deletecurdir

```c
static int vfs_deletecurdir(const struct vol *vol)
```

Defined at lines 1211 to 1226.

Run the vfs_deletecurdir hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume whose working directory afpd is about to delete

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletecurdir_adouble](vfs.c.md#rf_deletecurdir_adouble), [RF_deletecurdir_ea](vfs.c.md#rf_deletecurdir_ea)

Called through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_setfilmode

```c
static int vfs_setfilmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 1239 to 1255.

Run the vfs_setfilmode hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `name`: data file
* `mode`: new data file mode
* `st`: stat of the data file, or NULL

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setfilmode_adouble](vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](vfs.c.md#rf_setfilmode_ea), [ea_chmod_file](ea_ad.c.md#ea_chmod_file)

Called through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_setdirmode

```c
static int vfs_setdirmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 1268 to 1284.

Run the vfs_setdirmode hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `name`: directory
* `mode`: new directory mode
* `st`: stat of the directory, or NULL

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_setdirmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirmode_adouble](vfs.c.md#rf_setdirmode_adouble), [RF_setdirmode_ea](vfs.c.md#rf_setdirmode_ea)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_setdirunixmode

```c
static int vfs_setdirunixmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 1297 to 1313.

Run the vfs_setdirunixmode hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `name`: directory
* `mode`: new directory mode
* `st`: stat of the directory, or NULL

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirunixmode_adouble](vfs.c.md#rf_setdirunixmode_adouble), [RF_setdirunixmode_ea](vfs.c.md#rf_setdirunixmode_ea), [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir)

Called through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_setdirowner

```c
static int vfs_setdirowner(const struct vol *vol, const char *name, uid_t uid, gid_t gid)
```

Defined at lines 1326 to 1342.

Run the vfs_setdirowner hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `name`: directory
* `uid`: new owner
* `gid`: new group

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirowner_adouble](vfs.c.md#rf_setdirowner_adouble), [RF_setdirowner_ea](vfs.c.md#rf_setdirowner_ea)

Called through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirowner](../../etc/afpd/unix.c.md#setdirowner)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_deletefile

```c
static int vfs_deletefile(const struct vol *vol, int dirfd, const char *file)
```

Defined at lines 1355 to 1370.

Run the vfs_deletefile hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `dirfd`: directory that file is relative to, or -1 for the working directory
* `file`: data file whose metadata to delete

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletefile_adouble](vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](vfs.c.md#rf_deletefile_ea), [ea_deletefile](ea_ad.c.md#ea_deletefile)

Called through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletefile](../../etc/afpd/file.c.md#deletefile)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_renamefile

```c
static int vfs_renamefile(const struct vol *vol, int dirfd, const char *src, const char *dst)
```

Defined at lines 1384 to 1400.

Run the vfs_renamefile hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `dirfd`: directory that src is relative to, or -1 for the working directory
* `src`: current file name
* `dst`: new file name, relative to the working directory

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_renamefile_adouble](vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](vfs.c.md#rf_renamefile_ea), [ea_renamefile](ea_ad.c.md#ea_renamefile)

Called through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamefile](../../etc/afpd/file.c.md#renamefile)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_copyfile

```c
static int vfs_copyfile(const struct vol *vol, int sfd, const char *src, const char *dst)
```

Defined at lines 1414 to 1430.

Run the vfs_copyfile hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `sfd`: directory that src is relative to, or -1 for the working directory
* `src`: source file
* `dst`: destination file

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_copyfile_adouble](vfs.c.md#rf_copyfile_adouble), [RF_copyfile_ea](vfs.c.md#rf_copyfile_ea), [ea_copyfile](ea_ad.c.md#ea_copyfile), [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile)

Called through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [copyfile](../../etc/afpd/file.c.md#copyfile)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_solaris_acl

```c
static int vfs_solaris_acl(const struct vol *vol, const char *path, int cmd, int count, void *aces)
```

Defined at lines 1446 to 1463.

Run the vfs_solaris_acl hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `path`: file or directory
* `cmd`: acl command
* `count`: number of entries in aces
* `aces`: NFSv4 access control entries

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_solaris_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_solaris_acl](vfs.c.md#rf_solaris_acl)

Called through [`vfs_ops::vfs_solaris_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [set_acl](../../etc/afpd/acls.c.md#set_acl)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_posix_acl

```c
static int vfs_posix_acl(const struct vol *vol, const char *path, acl_type_t type, int count, acl_t acl)
```

Defined at lines 1478 to 1494.

Run the vfs_posix_acl hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `path`: file or directory
* `type`: ACL_TYPE_ACCESS or ACL_TYPE_DEFAULT
* `count`: number of entries in acl
* `acl`: POSIX ACL to set

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_posix_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_posix_acl](vfs.c.md#rf_posix_acl)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_remove_acl

```c
static int vfs_remove_acl(const struct vol *vol, const char *path, int dir)
```

Defined at lines 1507 to 1522.

Run the vfs_remove_acl hook of every VFS module of vol.

Every module runs even after a failure.

Parameters:
* `vol`: volume
* `path`: file or directory
* `dir`: nonzero when path is a directory

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_remove_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_posix_remove_acl](vfs.c.md#rf_posix_remove_acl), [RF_solaris_remove_acl](vfs.c.md#rf_solaris_remove_acl)

Called through [`vfs_ops::vfs_remove_acl`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [remove_acl](../../etc/afpd/acls.c.md#remove_acl)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_attrname_invalid

```c
static int vfs_ea_attrname_invalid(const char *func, const char *attruname)
```

Defined at lines 1533 to 1541.

Reject a missing EA name with a log line that names the calling function.

Parameters:
* `func`: name of the calling function, for the log line
* `attruname`: EA name to test

Returns: 1 when attruname is NULL, 0 otherwise

Called by: [vfs_ea_getcontent](vfs.c.md#vfs_ea_getcontent), [vfs_ea_getsize](vfs.c.md#vfs_ea_getsize), [vfs_ea_remove](vfs.c.md#vfs_ea_remove), [vfs_ea_set](vfs.c.md#vfs_ea_set)

### vfs_ea_getsize

```c
static int vfs_ea_getsize(const struct vol *vol, char *rbuf, size_t *rbuflen, const char *uname, int oflag, const char *attruname, int fd)
```

Defined at lines 1559 to 1581.

Run the vfs_ea_getsize hook of every VFS module of vol.

A missing EA name fails before any module runs. Only the EA backend module implements the hook, and it appends the EA size to rbuf.

Parameters:
* `vol`: volume
* `rbuf`: reply buffer receiving the size
* `rbuflen`: reply length, advanced by the bytes appended
* `uname`: file name
* `oflag`: O_NOFOLLOW to act on a symlink itself
* `attruname`: EA name
* `fd`: open descriptor of the file, or -1 to use uname

Returns: AFP_OK, AFPERR_PARAM for a missing EA name, or the result of the last module that failed

Calls: [vfs_ea_attrname_invalid](vfs.c.md#vfs_ea_attrname_invalid)

Calls through [`vfs_ops::vfs_ea_getsize`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_easize](ea_ad.c.md#get_easize), [sys_get_easize](ea_sys.c.md#sys_get_easize)

Called through [`vfs_ops::vfs_ea_getsize`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_getcontent

```c
static int vfs_ea_getcontent(const struct vol *vol, char *rbuf, size_t *rbuflen, const char *uname, int oflag, const char *attruname, int maxreply, int fd)
```

Defined at lines 1600 to 1622.

Run the vfs_ea_getcontent hook of every VFS module of vol.

A missing EA name fails before any module runs. Only the EA backend module implements the hook, and it appends the EA content to rbuf.

Parameters:
* `vol`: volume
* `rbuf`: reply buffer receiving the content
* `rbuflen`: reply length, advanced by the bytes appended
* `uname`: file name
* `oflag`: O_NOFOLLOW to act on a symlink itself
* `attruname`: EA name
* `maxreply`: reply size limit for the EA content
* `fd`: open descriptor of the file, or -1 to use uname

Returns: AFP_OK, AFPERR_PARAM for a missing EA name, or the result of the last module that failed

Calls: [vfs_ea_attrname_invalid](vfs.c.md#vfs_ea_attrname_invalid)

Calls through [`vfs_ops::vfs_ea_getcontent`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_eacontent](ea_ad.c.md#get_eacontent), [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent)

Called through [`vfs_ops::vfs_ea_getcontent`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_list

```c
static int vfs_ea_list(const struct vol *vol, char *attrnamebuf, size_t *buflen, const char *uname, int oflag, int fd)
```

Defined at lines 1639 to 1657.

Run the vfs_ea_list hook of every VFS module of vol.

Only the EA backend module implements the hook, and it appends the EA names as consecutive C strings.

Parameters:
* `vol`: volume
* `attrnamebuf`: buffer receiving the names
* `buflen`: bytes in attrnamebuf, advanced by the names appended
* `uname`: file name
* `oflag`: O_NOFOLLOW to act on a symlink itself
* `fd`: open descriptor of the file, or -1 to use uname

Returns: AFP_OK, or the result of the last module that failed

Calls through [`vfs_ops::vfs_ea_list`](../../include/atalk/vfs.h.md#struct-vfs_ops): [list_eas](ea_ad.c.md#list_eas), [sys_list_eas](ea_sys.c.md#sys_list_eas)

Called through [`vfs_ops::vfs_ea_list`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_set

```c
static int vfs_ea_set(const struct vol *vol, const char *uname, const char *attruname, const char *ibuf, size_t attrsize, int oflag, int fd)
```

Defined at lines 1676 to 1698.

Run the vfs_ea_set hook of every VFS module of vol.

A missing EA name fails before any module runs. Only the EA backend module implements the hook.

Parameters:
* `vol`: volume
* `uname`: file name
* `attruname`: EA name
* `ibuf`: EA content
* `attrsize`: bytes in ibuf
* `oflag`: O_CREAT to refuse an existing EA, O_TRUNC to require one, O_NOFOLLOW to act on a symlink itself
* `fd`: open descriptor of the file, or -1 to use uname

Returns: AFP_OK, AFPERR_PARAM for a missing EA name, or the result of the last module that failed

Calls: [vfs_ea_attrname_invalid](vfs.c.md#vfs_ea_attrname_invalid)

Calls through [`vfs_ops::vfs_ea_set`](../../include/atalk/vfs.h.md#struct-vfs_ops): [set_ea](ea_ad.c.md#set_ea), [sys_set_ea](ea_sys.c.md#sys_set_ea)

Called through [`vfs_ops::vfs_ea_set`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_ea_remove

```c
static int vfs_ea_remove(const struct vol *vol, const char *uname, const char *attruname, int oflag, int fd)
```

Defined at lines 1714 to 1735.

Run the vfs_ea_remove hook of every VFS module of vol.

A missing EA name fails before any module runs. Only the EA backend module implements the hook.

Parameters:
* `vol`: volume
* `uname`: file name
* `attruname`: EA name
* `oflag`: O_NOFOLLOW to act on a symlink itself
* `fd`: open descriptor of the file, or -1 to use uname

Returns: AFP_OK, AFPERR_PARAM for a missing EA name, or the result of the last module that failed

Calls: [vfs_ea_attrname_invalid](vfs.c.md#vfs_ea_attrname_invalid)

Calls through [`vfs_ops::vfs_ea_remove`](../../include/atalk/vfs.h.md#struct-vfs_ops): [remove_ea](ea_ad.c.md#remove_ea), [sys_remove_ea](ea_sys.c.md#sys_remove_ea)

Called through [`vfs_ops::vfs_ea_remove`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### vfs_validupath

```c
static int vfs_validupath(const struct vol *vol, const char *name)
```

Defined at lines 1747 to 1750.

Whether clients may see name, as the AppleDouble layout module decides.

Only the first module answers, because the layout alone knows which names hold its metadata.

Parameters:
* `vol`: volume
* `name`: directory entry name

Returns: nonzero when clients may see name, 0 otherwise

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](vfs.c.md#validupath_adouble), [validupath_ea](vfs.c.md#validupath_ea)

Called through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [check_dirent](../../etc/afpd/enumerate.c.md#check_dirent), [check_name](../../etc/afpd/filedir.c.md#check_name), [dbd_readdir](../../bin/dbd/cmd_dbd_scanvol.c.md#dbd_readdir)

Dispatched via: [vfs_master_funcs](vfs.c.md#vfs_master_funcs)

### initvol_vfs

```c
void initvol_vfs(struct vol *vol)
```

Defined at lines 1963 to 2000.

Install the VFS dispatchers and modules of a volume.

Module 0 is the AppleDouble layout, version 2 or ea, together with its ad_path function. The EA backend that v_vfs_ea selects, native EAs or AppleDouble EA files, fills slot 1 and leaves it empty without EA support. A build with NFSv4 or POSIX ACLs puts its ACL module in slot 2.

Parameters:
* `vol`: volume to set up

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
