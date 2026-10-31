---
type: C Source File
title: "libatalk/vfs/extattr.c"
description: "Platform wrappers for extended attribute system calls, from Samba."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/extattr.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-31T16:21:52+11:00 }
---

Part of the [libatalk/vfs](../vfs.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/types.h`

# Functions

### prefix

```c
static const char * prefix(const char *uname)
```

Defined at lines 99 to 107.

Platform EA name for uname in the user namespace.

Solaris and macOS use uname unchanged. Every other platform gets "user." in front, in a static buffer that the next call overwrites, and the buffer truncates names longer than 255 bytes.

Parameters:
* `uname`: EA name without a namespace

Returns: the platform EA name

Calls: [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [sys_fgetxattr](extattr.c.md#sys_fgetxattr), [sys_fremovexattr](extattr.c.md#sys_fremovexattr), [sys_fsetxattr](extattr.c.md#sys_fsetxattr), [sys_getxattr](extattr.c.md#sys_getxattr), [sys_lgetxattr](extattr.c.md#sys_lgetxattr), [sys_lremovexattr](extattr.c.md#sys_lremovexattr), [sys_lsetxattr](extattr.c.md#sys_lsetxattr), [sys_removexattr](extattr.c.md#sys_removexattr), [sys_setxattr](extattr.c.md#sys_setxattr)

Uses file-scope variables: `attr_name`

### sys_getxattrfd

```c
int sys_getxattrfd(int fd, const char *uname, int oflag,...)
```

Defined at lines 121 to 145. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

On Solaris, open EA uname of fd as a file descriptor.

O_CREAT takes the mode from the variable arguments. Every other platform fails with ENOSYS.

Parameters:
* `fd`: open file
* `uname`: EA name
* `oflag`: open flags, O_XATTR added
* `...`: creation mode under O_CREAT

Returns: the EA descriptor, or -1 with errno set

Called by: [ad_open_rf_ea](../adouble/ad_open.c.md#ad_open_rf_ea)

### sys_getxattr

```c
ssize_t sys_getxattr(const char *path, const char *name, void *value, size_t size)
```

Defined at lines 161 to 226. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Read EA uname of path, following symlinks.

A zero size returns the EA length without reading it. The BSD extattr interface truncates without an error, so that branch measures the EA first and fails with ERANGE when value is too small. A platform without EA support fails with ENOSYS.

Parameters:
* `path`: file
* `uname`: EA name without a namespace
* `value`: receives the EA bytes
* `size`: capacity of value, or 0 to query the length

Returns: the EA length, or -1 with errno set

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile), [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent), [sys_get_easize](ea_sys.c.md#sys_get_easize)

### sys_fgetxattr

```c
ssize_t sys_fgetxattr(int filedes, const char *name, void *value, size_t size)
```

Defined at lines 241 to 302. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Read EA uname of the file that filedes refers to.

A zero size returns the EA length without reading it. The BSD extattr branch measures the EA first and fails with ERANGE when value is too small. A platform without EA support fails with ENOSYS.

Parameters:
* `filedes`: open file
* `uname`: EA name without a namespace
* `value`: receives the EA bytes
* `size`: capacity of value, or 0 to query the length

Returns: the EA length, or -1 with errno set

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_flush_hf](../adouble/ad_flush.c.md#ad_flush_hf), [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [ad_reso_size](../adouble/ad_open.c.md#ad_reso_size), [copy_ea](unix.c.md#copy_ea), [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent), [sys_get_easize](ea_sys.c.md#sys_get_easize)

### sys_lgetxattr

```c
ssize_t sys_lgetxattr(const char *path, const char *name, void *value, size_t size)
```

Defined at lines 317 to 372. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Read EA uname of path without following a symlink.

A zero size returns the EA length without reading it. The BSD extattr branch measures the EA first and fails with ERANGE when value is too small. A platform without EA support fails with ENOSYS.

Parameters:
* `path`: file or symlink
* `uname`: EA name without a namespace
* `value`: receives the EA bytes
* `size`: capacity of value, or 0 to query the length

Returns: the EA length, or -1 with errno set

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_reso_size](../adouble/ad_open.c.md#ad_reso_size), [do_check_ea_support](../util/netatalk_conf.c.md#do_check_ea_support), [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent), [sys_get_easize](ea_sys.c.md#sys_get_easize)

### remove_user

```c
static ssize_t remove_user(ssize_t ret, char *list, size_t size)
```

Defined at lines 477 to 511.

Strip the "user." prefix from a listxattr result in place.

Names outside the user namespace drop out of the list. A failed call or a length query passes through unchanged. macOS lists EAs without a namespace, so the list stays unchanged there.

Parameters:
* `ret`: listxattr result, the list length or -1
* `list`: NUL-separated EA names
* `size`: capacity of list, 0 for a length query

Returns: the new list length, or ret unchanged

Called by: [sys_flistxattr](extattr.c.md#sys_flistxattr), [sys_listxattr](extattr.c.md#sys_listxattr), [sys_llistxattr](extattr.c.md#sys_llistxattr)

### sys_listxattr

```c
ssize_t sys_listxattr(const char *path, char *list, size_t size)
```

Defined at lines 526 to 557. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

List the EA names of path, following symlinks.

The names come NUL-separated and without a namespace prefix. A zero size returns the length the list needs. A platform without EA support fails with ENOSYS.

Parameters:
* `path`: file
* `list`: receives the names
* `size`: capacity of list, or 0 to query the length

Returns: the list length, or -1 with errno set

Calls: [remove_user](extattr.c.md#remove_user)

Called by: [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile), [sys_list_eas](ea_sys.c.md#sys_list_eas)

### sys_flistxattr

```c
ssize_t sys_flistxattr(int filedes, const char *path, char *list, size_t size)
```

Defined at lines 572 to 604. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

List the EA names of a file.

Solaris lists through filedes. Every other platform lists through path and follows symlinks. The names come NUL-separated and without a namespace prefix. A zero size returns the length the list needs.

Parameters:
* `filedes`: open file, used only on Solaris
* `path`: the same file, used on every other platform
* `list`: receives the names
* `size`: capacity of list, or 0 to query the length

Returns: the list length, or -1 with errno set

Calls: [remove_user](extattr.c.md#remove_user)

Called by: [sys_list_eas](ea_sys.c.md#sys_list_eas)

### sys_llistxattr

```c
ssize_t sys_llistxattr(const char *path, char *list, size_t size)
```

Defined at lines 618 to 649. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

List the EA names of path without following a symlink.

The names come NUL-separated and without a namespace prefix. A zero size returns the length the list needs. A platform without EA support fails with ENOSYS.

Parameters:
* `path`: file or symlink
* `list`: receives the names
* `size`: capacity of list, or 0 to query the length

Returns: the list length, or -1 with errno set

Calls: [remove_user](extattr.c.md#remove_user)

Called by: [sys_list_eas](ea_sys.c.md#sys_list_eas)

### sys_removexattr

```c
int sys_removexattr(const char *path, const char *name)
```

Defined at lines 660 to 688. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Delete EA uname of path, following symlinks.

A platform without EA support fails with ENOSYS.

Parameters:
* `path`: file
* `uname`: EA name without a namespace

Returns: 0, or -1 with errno set

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_convert_osx](../adouble/ad_open.c.md#ad_convert_osx), [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [ad_open_hf_ea](../adouble/ad_open.c.md#ad_open_hf_ea), [sys_remove_ea](ea_sys.c.md#sys_remove_ea)

### sys_fremovexattr

```c
int sys_fremovexattr(int filedes, const char *path, const char *name)
```

Defined at lines 701 to 729. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Delete EA uname of a file.

Solaris deletes through filedes. Every other platform deletes through path and follows symlinks.

Parameters:
* `filedes`: open file, used only on Solaris
* `path`: the same file, used on every other platform
* `uname`: EA name without a namespace

Returns: 0, or -1 with errno set

Calls: [prefix](extattr.c.md#prefix)

Called by: [createfile_meta](../../etc/afpd/file.c.md#createfile_meta), [sys_remove_ea](ea_sys.c.md#sys_remove_ea)

### sys_lremovexattr

```c
int sys_lremovexattr(const char *path, const char *name)
```

Defined at lines 740 to 766. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Delete EA uname of path without following a symlink.

A platform without EA support fails with ENOSYS.

Parameters:
* `path`: file or symlink
* `uname`: EA name without a namespace

Returns: 0, or -1 with errno set

Calls: [prefix](extattr.c.md#prefix)

Called by: [sys_remove_ea](ea_sys.c.md#sys_remove_ea)

### sys_setxattr

```c
int sys_setxattr(const char *path, const char *name, const void *value, size_t size, int flags)
```

Defined at lines 784 to 847. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Write EA uname of path, following symlinks.

XATTR_CREATE fails with EEXIST when the EA exists, and XATTR_REPLACE fails with ENOATTR when it does not. The BSD extattr branch emulates both flags with an existence test first. Solaris turns XATTR_CREATE into O_EXCL and creates the attribute file unless the caller passes XATTR_REPLACE. A platform without EA support fails with ENOSYS.

Parameters:
* `path`: file
* `uname`: EA name without a namespace
* `value`: EA bytes
* `size`: bytes in value
* `flags`: 0, XATTR_CREATE, or XATTR_REPLACE

Returns: 0, or -1 with errno set

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_conv_v22ea_hf](../adouble/ad_conv.c.md#ad_conv_v22ea_hf), [do_check_ea_support](../util/netatalk_conf.c.md#do_check_ea_support), [sys_ea_copyfile](ea_sys.c.md#sys_ea_copyfile), [sys_set_ea](ea_sys.c.md#sys_set_ea)

### sys_fsetxattr

```c
int sys_fsetxattr(int filedes, const char *name, const void *value, size_t size, int flags)
```

Defined at lines 865 to 931. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Write EA uname of the file that filedes refers to.

XATTR_CREATE fails with EEXIST when the EA exists, and XATTR_REPLACE fails with ENOATTR when it does not. The BSD extattr branch emulates both flags with an existence test first. Solaris turns XATTR_CREATE into O_EXCL and creates the attribute file unless the caller passes XATTR_REPLACE. A platform without EA support fails with ENOSYS.

Parameters:
* `filedes`: open file
* `uname`: EA name without a namespace
* `value`: EA bytes
* `size`: bytes in value
* `flags`: 0, XATTR_CREATE, or XATTR_REPLACE

Returns: 0, or -1 with errno set

Calls: [prefix](extattr.c.md#prefix)

Called by: [ad_flush_hf](../adouble/ad_flush.c.md#ad_flush_hf), [copy_ea](unix.c.md#copy_ea), [sys_set_ea](ea_sys.c.md#sys_set_ea)

### sys_lsetxattr

```c
int sys_lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags)
```

Defined at lines 949 to 1011. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Write EA uname of path without following a symlink.

XATTR_CREATE fails with EEXIST when the EA exists, and XATTR_REPLACE fails with ENOATTR when it does not. The BSD extattr branch emulates both flags with an existence test first. Solaris turns XATTR_CREATE into O_EXCL and creates the attribute file unless the caller passes XATTR_REPLACE. A platform without EA support fails with ENOSYS.

Parameters:
* `path`: file or symlink
* `uname`: EA name without a namespace
* `value`: EA bytes
* `size`: bytes in value
* `flags`: 0, XATTR_CREATE, or XATTR_REPLACE

Returns: 0, or -1 with errno set

Calls: [prefix](extattr.c.md#prefix)

Called by: [sys_set_ea](ea_sys.c.md#sys_set_ea)

# File-scope variables

`attr_name`
