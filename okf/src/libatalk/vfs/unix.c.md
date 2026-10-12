---
type: C Source File
title: "libatalk/vfs/unix.c"
description: "Unix file operations of the VFS layer: modes, duplication, deletion, and renaming."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/unix.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 41 to 44. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Whether mode grants the owner read and search permission.

Parameters:
* `mode`: directory mode

Returns: 1 when mode carries both S_IRUSR and S_IXUSR, 0 otherwise

Called by: [RF_setdirmode_adouble](vfs.c.md#rf_setdirmode_adouble), [RF_setdirunixmode_adouble](vfs.c.md#rf_setdirunixmode_adouble), [setdeskmode](../../etc/afpd/desktop.c.md#setdeskmode), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode)

### setfilmode

```c
int setfilmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 60 to 87. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Set the permission bits of name and keep its other mode bits.

The volume umask clears bits from the new mode, and st supplies the file type and special bits. The ochmod call skips a symlink unless the volume follows symlinks. The volume options decide whether it preserves ACLs or skips the change. EPERM counts as success.

Parameters:
* `vol`: volume holding the file
* `name`: file to change
* `mode`: new permission bits
* `st`: stat whose non-permission bits to keep, or NULL to stat name

Returns: 0, or -1 with errno set

Calls: [ochmod](../util/unix.c.md#ochmod)

Called by: [adouble_setfilmode](vfs.c.md#adouble_setfilmode), [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir), [ea_chmod_file](ea_ad.c.md#ea_chmod_file), [setdirmode_adouble_loop](vfs.c.md#setdirmode_adouble_loop), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode)

### netatalk_rmdir_all_errors

```c
int netatalk_rmdir_all_errors(int dirfd, const char *name)
```

Defined at lines 100 to 132. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Delete an empty directory and map the error to an AFP result code.

Unlike netatalk_rmdir, a missing directory fails with AFPERR_NOOBJ.

Parameters:
* `dirfd`: directory that name is relative to, or -1 for the working directory
* `name`: directory to delete

Returns: AFP_OK, AFPERR_NOOBJ, AFPERR_DIRNEMPT, AFPERR_ACCESS, AFPERR_VLOCK, or AFPERR_PARAM

Called by: [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [netatalk_rmdir](unix.c.md#netatalk_rmdir)

### netatalk_rmdir

```c
int netatalk_rmdir(int dirfd, const char *name)
```

Defined at lines 146 to 155. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Delete an empty directory and treat a missing one as success.

The errors map as in netatalk_rmdir_all_errors, except that AFPERR_NOOBJ becomes AFP_OK.

Parameters:
* `dirfd`: directory that name is relative to, or -1 for the working directory
* `name`: directory to delete

Returns: AFP_OK, AFPERR_DIRNEMPT, AFPERR_ACCESS, AFPERR_VLOCK, or AFPERR_PARAM

Calls: [netatalk_rmdir_all_errors](unix.c.md#netatalk_rmdir_all_errors)

Called by: [RF_deletecurdir_adouble](vfs.c.md#rf_deletecurdir_adouble), [deletedir](../../etc/afpd/directory.c.md#deletedir)

### netatalk_unlink

```c
int netatalk_unlink(const char *name)
```

Defined at lines 166 to 186. Declared in [etc/afpd/directory.h](../../etc/afpd/directory.h.md).

Delete a file and map the error to an AFP result code.

A missing file counts as success.

Parameters:
* `name`: file to delete, relative to the working directory

Returns: AFP_OK, AFPERR_VLOCK on a read-only file system, AFPERR_ACCESS, or AFPERR_PARAM

Called by: [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [deletecurdir_adouble_loop](vfs.c.md#deletecurdir_adouble_loop), [deletecurdir_ea_osx_loop](vfs.c.md#deletecurdir_ea_osx_loop), [replace_with_symlink](../../etc/afpd/file.c.md#replace_with_symlink)

### copy_file_fd

```c
int copy_file_fd(int sfd, int dfd)
```

Defined at lines 202 to 237. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Transfer all remaining data from one descriptor to another.

The loop reads and writes in NETATALK_DIOSZ_STACK chunks and retries after EINTR.

Parameters:
* `sfd`: source descriptor, read from its current position
* `dfd`: destination descriptor, written at its current position

Returns: 0, or -1 when a read or write failed

Called by: [copy_file](unix.c.md#copy_file)

### copy_file

```c
int copy_file(int sfd, const char *src, const char *dst, mode_t mode)
```

Defined at lines 253 to 302. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Duplicate the data of file src into dst.

The open creates dst or truncates an existing one. A missing source logs only at debug level, because callers probe AppleDouble sidecars that plain files lack and ignore ENOENT.

Parameters:
* `dirfd`: directory that src is relative to, or -1 for the working directory
* `src`: source file
* `dst`: destination file, relative to the working directory
* `mode`: creation mode of dst

Returns: 0, or -1 with errno set

Calls: [copy_file_fd](unix.c.md#copy_file_fd)

Called by: [RF_copyfile_adouble](vfs.c.md#rf_copyfile_adouble), [RF_copyfile_ea](vfs.c.md#rf_copyfile_ea), [ea_copyfile](ea_ad.c.md#ea_copyfile)

### copy_ea

```c
int copy_ea(const char *ea, int sfd, const char *src, const char *dst, mode_t mode)
```

Defined at lines 317 to 351. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Transfer one EA from src to an existing dst.

A zero-length EA leaves dst unchanged.

Parameters:
* `ea`: EA name
* `dirfd`: directory that src is relative to, or -1 for the working directory
* `src`: source file
* `dst`: destination file, relative to the working directory
* `mode`: unused, because the open of dst does not create it

Returns: 0, or -1 when an open, the EA read, or the EA write failed

Calls: [sys_fgetxattr](extattr.c.md#sys_fgetxattr), [sys_fsetxattr](extattr.c.md#sys_fsetxattr)

### netatalk_unlinkat

```c
int netatalk_unlinkat(int dirfd, const char *name)
```

Defined at lines 365 to 389. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Delete a file relative to a directory descriptor and map the error to an AFP result code.

A missing file counts as success.

Parameters:
* `dirfd`: directory that name is relative to, or -1 for the working directory
* `name`: file to delete

Returns: AFP_OK, AFPERR_VLOCK on a read-only file system, AFPERR_ACCESS, or AFPERR_PARAM

Called by: [RF_deletefile_adouble](vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](vfs.c.md#rf_deletefile_ea), [delete_vetoed_files_at](../../etc/afpd/filedir.c.md#delete_vetoed_files_at), [deletedir](../../etc/afpd/directory.c.md#deletedir), [deletefile](../../etc/afpd/file.c.md#deletefile), [ea_close](ea_ad.c.md#ea_close)

### unix_rename

```c
int unix_rename(int sfd, const char *oldpath, int dfd, const char *newpath)
```

Defined at lines 402 to 417. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Rename oldpath to newpath through renameat.

Parameters:
* `sfd`: directory that oldpath is relative to, or -1 for the working directory
* `oldpath`: current name
* `dfd`: directory that newpath is relative to, or -1 for the working directory
* `newpath`: new name

Returns: 0, or -1 with errno set

Called by: [RF_renamefile_adouble](vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](vfs.c.md#rf_renamefile_ea), [ea_renamefile](ea_ad.c.md#ea_renamefile), [renamedir](../../etc/afpd/directory.c.md#renamedir), [renamefile](../../etc/afpd/file.c.md#renamefile)

### statat

```c
int statat(int dirfd, const char *path, struct stat *st)
```

Defined at lines 428 to 435. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Stat path relative to a directory descriptor, following symlinks.

Parameters:
* `dirfd`: directory that path is relative to, or -1 for the working directory
* `path`: file or directory
* `st`: receives the stat

Returns: 0, or -1 with errno set

Called by: [ea_close](ea_ad.c.md#ea_close)

### opendirat

```c
DIR * opendirat(int dirfd, const char *path)
```

Defined at lines 449 to 475. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Open a directory stream relative to a directory descriptor.

With a dirfd other than -1 the call changes into that directory for the opendir and back afterwards. A failed return ends the process with EXITERR_SYS.

Parameters:
* `dirfd`: directory that path is relative to, or -1 for the working directory
* `path`: directory to open

Returns: the directory stream, or NULL with errno set

Called by: [copydir](../../etc/afpd/directory.c.md#copydir), [deletedir](../../etc/afpd/directory.c.md#deletedir)
