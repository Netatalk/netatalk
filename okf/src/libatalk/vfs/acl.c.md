---
type: C Source File
title: "libatalk/vfs/acl.c"
description: "2 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/acl.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 38 to 94.

Removes all non-trivial ACLs from object.

Returns: full AFPERR code

Calls: [get_nfsv4_acl](../acl/unix.c.md#get_nfsv4_acl)

Called by: [RF_solaris_remove_acl](vfs.c.md#rf_solaris_remove_acl), [remove_acl](../../etc/afpd/acls.c.md#remove_acl)

### remove_posix_acl_vfs

```c
int remove_posix_acl_vfs(const char *name)
```

Defined at lines 107 to 151.

Remove any ACL_USER, ACL_GROUP, ACL_MASK or ACL_TYPE_DEFAULT ACEs from an object.

Parameters:
* `name`: filesystem object name

Returns: AFP error code, AFP_OK (= 0) on success, AFPERR_MISC on error

Called by: [RF_posix_remove_acl](vfs.c.md#rf_posix_remove_acl), [remove_acl](../../etc/afpd/acls.c.md#remove_acl)
