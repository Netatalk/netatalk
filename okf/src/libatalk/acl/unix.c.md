---
type: C Source File
title: "libatalk/acl/unix.c"
description: "chmod wrappers that keep explicit NFSv4 ACEs and set the POSIX ACL group entry from the mode"
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/acl/unix.c"
tags: ["libatalk/acl"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 56 to 112. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Read the NFSv4 ACL of a file or directory.

Symlinks and special files report 0 entries. The caller frees the returned array.

Parameters:
* `name`: path of the object
* `retAces`: receives a newly allocated array of the entries, or NULL

Returns: the number of entries, or -1 on error

Calls: [getcwdpath](../util/unix.c.md#getcwdpath)

Called by: [check_vol_acl_support](../util/netatalk_conf.c.md#check_vol_acl_support), [get_and_map_acl](../../etc/afpd/acls.c.md#get_and_map_acl), [nfsv4_chmod](unix.c.md#nfsv4_chmod), [remove_nfsv4_acl_vfs](../vfs/acl.c.md#remove_nfsv4_acl_vfs), [set_acl](../../etc/afpd/acls.c.md#set_acl), [solaris_acl_rights](../../etc/afpd/acls.c.md#solaris_acl_rights)

### concat_aces

```c
ace_t * concat_aces(ace_t *aces1, int ace1count, ace_t *aces2, int ace2count)
```

Defined at lines 123 to 150. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Join two ACE arrays into a newly allocated array.

Parameters:
* `aces1`: first array
* `ace1count`: number of entries in aces1
* `aces2`: second array, appended after aces1
* `ace2count`: number of entries in aces2

Returns: the joined array, or NULL when malloc fails

Called by: [nfsv4_chmod](unix.c.md#nfsv4_chmod)

### strip_trivial_aces

```c
int strip_trivial_aces(ace_t **saces, int sacecount)
```

Defined at lines 161 to 202. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Drop the owner@, group@, and everyone@ entries from an ACE array.

The call frees the old array and stores a new one in saces.

Parameters:
* `saces`: ACE array, replaced by the remaining entries
* `sacecount`: number of entries

Returns: the number of remaining entries, or -1 when malloc fails

Called by: [nfsv4_chmod](unix.c.md#nfsv4_chmod)

### strip_nontrivial_aces

```c
int strip_nontrivial_aces(ace_t **saces, int sacecount)
```

Defined at lines 213 to 252. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Keep only the owner@, group@, and everyone@ entries of an ACE array.

The call frees the old array and stores a new one in saces.

Parameters:
* `saces`: ACE array, replaced by the remaining entries
* `sacecount`: number of entries

Returns: the number of remaining entries, or -1 when malloc fails

Called by: [nfsv4_chmod](unix.c.md#nfsv4_chmod)

### nfsv4_chmod

```c
int nfsv4_chmod(char *name, mode_t mode)
```

Defined at lines 273 to 350. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Change the mode of a file and keep its explicit NFSv4 ACEs.

The call works in 7 steps:

1. Read the ACL. Without entries, a plain chmod finishes the call.
1. Drop the owner@, group@, and everyone@ entries, which the mode replaces.
1. Run chmod with the new mode.
1. Read the ACL again, as root after EACCES. Solaris up to onnv132 keeps the explicit entries here, and onnv145 and OpenIndiana drop them.
1. Keep only the owner@, group@, and everyone@ entries of the new ACL.
1. Join the explicit entries of the old ACL with the mode entries of the new one.
1. Set the joined ACL, as root after EACCES.

Parameters:
* `name`: path of the object
* `mode`: new mode

Returns: 0, or -1 with errno set

Calls: [become_root](../util/unix.c.md#become_root), [concat_aces](unix.c.md#concat_aces), [get_nfsv4_acl](unix.c.md#get_nfsv4_acl), [getcwdpath](../util/unix.c.md#getcwdpath), [strip_nontrivial_aces](unix.c.md#strip_nontrivial_aces), [strip_trivial_aces](unix.c.md#strip_trivial_aces), [unbecome_root](../util/unix.c.md#unbecome_root)

### posix_chmod

```c
int posix_chmod(const char *name, mode_t mode)
```

Defined at lines 373 to 493. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Change the mode of a file and keep its POSIX ACL group entry in step.

On an object with an extended ACL, chmod changes ACL_MASK instead of ACL_GROUP_OBJ, as POSIX 1003.1e draft 17 section 23.1.2 defines. macOS clients do not expect that. After chmod the call therefore sets ACL_GROUP_OBJ from the group bits of mode and recalculates ACL_MASK. BSD systems drop the bits above 0777 first, because ZFS on FreeBSD refuses the setgid bit there.

Parameters:
* `name`: path of the object
* `mode`: new mode

Returns: 0, or -1 on error

Calls: [fullpathname](../util/unix.c.md#fullpathname)

### posix_fchmod

```c
int posix_fchmod(int fd, mode_t mode)
```

Defined at lines 505 to 613. Declared in [include/atalk/acl.h](../../include/atalk/acl.h.md).

Change the mode of an open file and keep its POSIX ACL group entry in step.

The call works like posix_chmod on a descriptor, without the BSD bit mask.

Parameters:
* `fd`: descriptor of the object
* `mode`: new mode

Returns: 0, or -1 on error

# Macros

* Undocumented: `SEARCH_GROUP_OBJ`, `SEARCH_MASK`
