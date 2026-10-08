---
type: C Source File
title: "libatalk/acl/unix.c"
description: "7 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/acl/unix.c"
tags: ["libatalk/acl"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/acl](../acl.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/acl.h](../../include/atalk/acl.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/acl.h`, `sys/stat.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### get_nfsv4_acl

```c
int get_nfsv4_acl(const char *name, ace_t **retAces)
```

Defined at lines 45 to 101. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Get ACL.

Note: Allocates storage as needed. Caller must free.

Returns: no of ACEs or -1 on error.

Calls: [getcwdpath](../util/unix.c.md#getcwdpath)

Called by: [check_vol_acl_support](../util/netatalk_conf.c.md#check_vol_acl_support), [get_and_map_acl](../../etc/afpd/acls.c.md#get_and_map_acl), [nfsv4_chmod](unix.c.md#nfsv4_chmod), [remove_nfsv4_acl_vfs](../vfs/acl.c.md#remove_nfsv4_acl_vfs), [set_acl](../../etc/afpd/acls.c.md#set_acl), [solaris_acl_rights](../../etc/afpd/acls.c.md#solaris_acl_rights)

### concat_aces

```c
ace_t * concat_aces(ace_t *aces1, int ace1count, ace_t *aces2, int ace2count)
```

Defined at lines 106 to 133. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Concatenate ACEs

Called by: [nfsv4_chmod](unix.c.md#nfsv4_chmod)

### strip_trivial_aces

```c
int strip_trivial_aces(ace_t **saces, int sacecount)
```

Defined at lines 139 to 180. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Remove any trivial ACE "in-place".

Returns: no of non-trivial ACEs

Called by: [nfsv4_chmod](unix.c.md#nfsv4_chmod)

Mentioned in the documentation of: [nfsv4_chmod](unix.c.md#nfsv4_chmod)

### strip_nontrivial_aces

```c
int strip_nontrivial_aces(ace_t **saces, int sacecount)
```

Defined at lines 186 to 225. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Remove non-trivial ACEs "in-place".

Returns: no of trivial ACEs.

Called by: [nfsv4_chmod](unix.c.md#nfsv4_chmod)

Mentioned in the documentation of: [nfsv4_chmod](unix.c.md#nfsv4_chmod)

### nfsv4_chmod

```c
int nfsv4_chmod(char *name, mode_t mode)
```

Defined at lines 242 to 319. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Change mode of file preserving existing explicit ACEs.

nfsv4_chmod

1. reads objects ACL (acl1), may return 0 or -1 NFSv4 ACEs on e.g. UFS fs
1. removes all trivial ACEs from the ACL by calling [strip_trivial_aces()](unix.c.md#strip_trivial_aces), possibly leaving 0 ACEs in the ACL if there were only trivial ACEs as mapped from the mode
1. calls chmod() with mode, we're done if step (1) returned 0 for noaces
1. reads the changed ACL (acl2) which a. might still contain explicit ACEs (up to onnv132) b. will have any explicit ACE removed (starting with onnv145/Openindiana)
1. strip any explicit ACE from acl2 using [strip_nontrivial_aces()](unix.c.md#strip_nontrivial_aces)
1. merge acl2 and acl2
1. set the ACL merged ACL on the object

Calls: [become_root](../util/unix.c.md#become_root), [concat_aces](unix.c.md#concat_aces), [get_nfsv4_acl](unix.c.md#get_nfsv4_acl), [getcwdpath](../util/unix.c.md#getcwdpath), [strip_nontrivial_aces](unix.c.md#strip_nontrivial_aces), [strip_trivial_aces](unix.c.md#strip_trivial_aces), [unbecome_root](../util/unix.c.md#unbecome_root)

### posix_chmod

```c
int posix_chmod(const char *name, mode_t mode)
```

Defined at lines 341 to 461. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

POSIX ACL chmod.

This is a workaround for chmod() on filestystems supporting Posix 1003.1e draft 17 compliant ACLs. For objects with extended ACLs, e.g. objects with an ACL_MASK entry, chmod() manipulates ACL_MASK instead of ACL_GROUP_OBJ. As OS X isn't aware of this behavior calling FPSetFileDirParms may lead to unpredictable results. For more information see section 23.1.2 of Posix 1003.1e draft 17.

Note: accepts the same arguments as chmod()

Returns: 0 in case of success or -1 in case something went wrong.

Calls: [fullpathname](../util/unix.c.md#fullpathname)

### posix_fchmod

```c
int posix_fchmod(int fd, mode_t mode)
```

Defined at lines 469 to 577. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

POSIX ACL fchmod.

Note: accepts the same arguments as fchmod()

Returns: 0 in case of success or -1 in case something went wrong.

# Macros

* Undocumented: `SEARCH_GROUP_OBJ`, `SEARCH_MASK`
