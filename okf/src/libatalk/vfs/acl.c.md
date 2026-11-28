---
type: C Source File
title: "libatalk/vfs/acl.c"
description: "Reducing ACLs to the entries that mirror the mode bits, in the VFS layer."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/acl.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-11-28T10:52:17+01:00 }
---

Part of the [libatalk/vfs](../vfs.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/acl.h](../../include/atalk/acl.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### remove_nfsv4_acl_vfs

```c
int remove_nfsv4_acl_vfs(const char *name)
```

Defined at lines 52 to 108.

Keep only the owner@, group@, and everyone@ entries in the NFSv4 ACL of name.

Parameters:
* `name`: file or directory

Returns: AFP_OK, AFPERR_ACCESS, AFPERR_NOITEM for a missing name, or AFPERR_MISC

Calls: [get_nfsv4_acl](../acl/unix.c.md#get_nfsv4_acl)

Called by: [RF_solaris_remove_acl](vfs.c.md#rf_solaris_remove_acl), [remove_acl](../../etc/afpd/acls.c.md#remove_acl)

### limit_posix_acl_entry

```c
static int limit_posix_acl_entry(acl_entry_t entry, mode_t bits)
```

Defined at lines 123 to 135.

Clear permissions from a POSIX ACL entry according to mode bits.

Use the low three bits of bits to limit read, write, and execute permissions. The entry belongs to the caller's ACL and changes in place.

Parameters:
* `entry`: ACL entry to restrict
* `bits`: permitted rights in the low three bits

Returns: 0 on success, or -1 on an ACL error

Called by: [limit_posix_acl](acl.c.md#limit_posix_acl)

### limit_posix_acl

```c
static int limit_posix_acl(acl_t acl, mode_t mode)
```

Defined at lines 147 to 195.

Limit the ACL's effective permissions to the destination mode.

For an extended ACL, the mask bounds the owning group and named entries. For a basic ACL, the owning group entry holds the group permissions.

Parameters:
* `acl`: valid POSIX access ACL to restrict in place
* `mode`: destination file mode

Returns: 1 for an extended ACL, 0 for a basic ACL, or -1 with errno set

Calls: [limit_posix_acl_entry](acl.c.md#limit_posix_acl_entry)

Called by: [copy_posix_acl](acl.c.md#copy_posix_acl)

### apply_basic_posix_acl_mode

```c
static int apply_basic_posix_acl_mode(int fd, acl_t acl, mode_t mode)
```

Defined at lines 208 to 260.

Apply a basic POSIX access ACL as file mode bits.

Convert the owner, group, and other entries to permission bits and apply them with fchmod, carrying over the destination's special mode bits.

Parameters:
* `fd`: open destination file descriptor, kept open
* `acl`: basic access ACL with owner, group, and other entries
* `mode`: destination mode supplying the special bits

Returns: 0 on success, or -1 with errno set

Called by: [copy_posix_acl](acl.c.md#copy_posix_acl)

### copy_posix_acl

```c
int copy_posix_acl(int srcfd, int dstfd)
```

Defined at lines 274 to 301.

Copy a file's native POSIX access ACL to a newly created file.

A basic source ACL replaces any inherited ACL. Honor the destination's creation mode so the copy cannot widen permissions. If the destination does not support ACLs, apply a basic ACL as mode bits.

Parameters:
* `srcfd`: open source data fork, kept open
* `dstfd`: open destination data fork, kept open

Returns: 0 on success or when the source has no ACL support, or -1 with errno on error

Calls: [apply_basic_posix_acl_mode](acl.c.md#apply_basic_posix_acl_mode), [limit_posix_acl](acl.c.md#limit_posix_acl)

Called by: [copyfile](../../etc/afpd/file.c.md#copyfile)

### remove_posix_acl_vfs

```c
int remove_posix_acl_vfs(const char *name)
```

Defined at lines 311 to 355.

Remove any ACL_USER, ACL_GROUP, ACL_MASK or ACL_TYPE_DEFAULT ACEs from an object.

Parameters:
* `name`: filesystem object name

Returns: AFP error code, AFP_OK (= 0) on success, AFPERR_MISC on error

Called by: [RF_posix_remove_acl](vfs.c.md#rf_posix_remove_acl), [remove_acl](../../etc/afpd/acls.c.md#remove_acl)
