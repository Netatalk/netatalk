---
type: C Source File
title: "libatalk/vfs/unix.c"
description: "12 functions, includes 10 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/unix.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/vfs](../vfs.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/acl.h](../../include/atalk/acl.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### dir_rx_set

```c
int dir_rx_set(mode_t mode)
```

Defined at lines 30 to 33. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Called by: [RF_setdirmode_adouble](vfs.c.md#rf_setdirmode_adouble), [RF_setdirunixmode_adouble](vfs.c.md#rf_setdirunixmode_adouble), [setdeskmode](../../etc/afpd/desktop.c.md#setdeskmode), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode)

### setfilmode

```c
int setfilmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 36 to 63. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Calls: [ochmod](../util/unix.c.md#ochmod)

Called by: [adouble_setfilmode](vfs.c.md#adouble_setfilmode), [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir), [ea_chmod_file](ea_ad.c.md#ea_chmod_file), [setdirmode_adouble_loop](vfs.c.md#setdirmode_adouble_loop), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode)

### netatalk_rmdir_all_errors

```c
int netatalk_rmdir_all_errors(int dirfd, const char *name)
```

Defined at lines 70 to 102. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

system rmdir with afp error code.

Note: Supports *at semantics (cf openat). Pass dirfd=-1 to ignore this.

Called by: [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [netatalk_rmdir](unix.c.md#netatalk_rmdir)

### netatalk_rmdir

```c
int netatalk_rmdir(int dirfd, const char *name)
```

Defined at lines 109 to 118. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

System rmdir with afp error code, but ENOENT is not an error.

Note: Supports *at semantics (cf openat). Pass dirfd=-1 to ignore this.

Calls: [netatalk_rmdir_all_errors](unix.c.md#netatalk_rmdir_all_errors)

Called by: [RF_deletecurdir_adouble](vfs.c.md#rf_deletecurdir_adouble), [deletedir](../../etc/afpd/directory.c.md#deletedir)

### netatalk_unlink

```c
int netatalk_unlink(const char *name)
```

Defined at lines 124 to 144. Declared in [etc/afpd/directory.h](../../etc/afpd/directory.h.md).

system unlink with afp error code.

Note: ENOENT is not an error.

Called by: [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [deletecurdir_adouble_loop](vfs.c.md#deletecurdir_adouble_loop), [deletecurdir_ea_osx_loop](vfs.c.md#deletecurdir_ea_osx_loop), [replace_with_symlink](../../etc/afpd/file.c.md#replace_with_symlink)

### copy_file_fd

```c
int copy_file_fd(int sfd, int dfd)
```

Defined at lines 151 to 186. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Copy all file data from one file fd to another

Called by: [copy_file](unix.c.md#copy_file)

### copy_file

```c
int copy_file(int sfd, const char *src, const char *dst, mode_t mode)
```

Defined at lines 191 to 240. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Supports *at semantics, pass dirfd=-1 to ignore this

Calls: [copy_file_fd](unix.c.md#copy_file_fd)

Called by: [RF_copyfile_adouble](vfs.c.md#rf_copyfile_adouble), [RF_copyfile_ea](vfs.c.md#rf_copyfile_ea), [ea_copyfile](ea_ad.c.md#ea_copyfile)

### copy_ea

```c
int copy_ea(const char *ea, int sfd, const char *src, const char *dst, mode_t mode)
```

Defined at lines 247 to 281. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Copy an EA from one file to another.

Note: Supports *at semantics, pass dirfd=-1 to ignore this

Calls: [sys_fgetxattr](extattr.c.md#sys_fgetxattr), [sys_fsetxattr](extattr.c.md#sys_fsetxattr)

### netatalk_unlinkat

```c
int netatalk_unlinkat(int dirfd, const char *name)
```

Defined at lines 286 to 310. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

at wrapper for netatalk_unlink

Called by: [RF_deletefile_adouble](vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](vfs.c.md#rf_deletefile_ea), [delete_vetoed_files_at](../../etc/afpd/filedir.c.md#delete_vetoed_files_at), [deletedir](../../etc/afpd/directory.c.md#deletedir), [deletefile](../../etc/afpd/file.c.md#deletefile), [ea_close](ea_ad.c.md#ea_close)

### unix_rename

```c
int unix_rename(int sfd, const char *oldpath, int dfd, const char *newpath)
```

Defined at lines 322 to 337. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

This is equivalent of unix rename()

unix_rename mulitplexes rename and renameat.

Parameters:
* `sfd`: -1 gives AT_FDCWD
* `oldpath`: guess what
* `dfd`: same as sfd
* `newpath`: guess what

Called by: [RF_renamefile_adouble](vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](vfs.c.md#rf_renamefile_ea), [ea_renamefile](ea_ad.c.md#ea_renamefile), [renamedir](../../etc/afpd/directory.c.md#renamedir), [renamefile](../../etc/afpd/file.c.md#renamefile)

### statat

```c
int statat(int dirfd, const char *path, struct stat *st)
```

Defined at lines 348 to 355. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

stat/fsstatat multiplexer

statat mulitplexes stat and fstatat.

Parameters:
* `dirfd`: -1 gives AT_FDCWD
* `path`: pathname
* `st`: pointer to struct stat

Called by: [ea_close](ea_ad.c.md#ea_close)

### opendirat

```c
DIR * opendirat(int dirfd, const char *path)
```

Defined at lines 365 to 391. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

opendir wrapper for *at semantics support

opendirat chdirs to dirfd if dirfd != -1 before calling opendir on path.

Parameters:
* `dirfd`: if != -1, chdir(dirfd) before opendir(path)
* `path`: pathname

Called by: [copydir](../../etc/afpd/directory.c.md#copydir), [deletedir](../../etc/afpd/directory.c.md#deletedir)
