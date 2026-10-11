---
type: C Source File
title: "etc/afpd/unix.c"
description: "10 functions, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/unix.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [acls.h](acls.h.md)
* [atalk/acl.h](../../include/atalk/acl.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [auth.h](auth.h.md)
* [directory.h](directory.h.md)
* [fork.h](fork.h.md)
* [unix.h](unix.h.md)
* [volume.h](volume.h.md)
* System headers: `errno.h`, `inttypes.h`, `limits.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### ustatfs_getvolspace

```c
int ustatfs_getvolspace(const struct vol *vol, VolSpace *bfree, VolSpace *btotal, uint32_t *bsize)
```

Defined at lines 47 to 78.

Free and total space of the file system that holds the volume.

The values come from statfs in bytes, saturated at UINT64_MAX.

Parameters:
* `vol`: volume
* `bfree`: receives the bytes available to the user
* `btotal`: receives the total bytes
* `bsize`: receives the fragment size

Returns: AFP_OK, or AFPERR_PARAM when statfs failed

Called by: [getvolspace](volume.c.md#getvolspace)

### utombits

```c
static int utombits(mode_t bits)
```

Defined at lines 86 to 95.

Map the three permission bits of one Unix class to AFP access rights.

Parameters:
* `bits`: mode, shifted so that the class sits in the low three bits

Returns: AR_UREAD, AR_UWRITE, and AR_USEARCH as the bits allow

Called by: [utommode](unix.c.md#utommode)

### utommode

```c
static void utommode(const AFPObj *obj, const struct stat *stat, struct maccess *ma)
```

Defined at lines 110 to 147.

Derive the AFP access rights of owner, group, world, and user from a stat.

Root gets every right. The owner gets the owner bits and AR_UOWN, a group member gets the group bits, and anyone else gets the world bits. A file that root owns also grants AR_UOWN to the user. The mapping follows page 63 of the AFP 3.0 specification.

Parameters:
* `obj`: session object with the user and group IDs
* `stat`: stat of the file or directory
* `ma`: receives the access rights

Calls: [gmem](../../libatalk/util/unix.c.md#gmem), [utombits](unix.c.md#utombits)

Called by: [accessmode](unix.c.md#accessmode)

### accessmode

```c
void accessmode(const AFPObj *obj, const struct vol *vol, char *path, struct maccess *ma, struct dir *dir, struct stat *st)
```

Defined at lines 163 to 181.

AFP access rights of a file or directory from its stat and ACLs.

The rights come from the Unix mode through utommode, and with ACL support acltoownermode refines them. The access call gives wrong answers over NFS, so the rights come from a stat. A failed stat leaves every right at zero.

Parameters:
* `obj`: session object
* `vol`: volume of the path
* `path`: file or directory
* `ma`: receives the access rights
* `dir`: unused
* `st`: stat of path, or NULL to stat it here

Calls: [acltoownermode](acls.c.md#acltoownermode), [ostat](../../libatalk/util/unix.c.md#ostat), [utommode](unix.c.md#utommode)

Called by: [afp_getsrvrparms](volume.c.md#afp_getsrvrparms), [check_access](directory.c.md#check_access), [file_access](directory.c.md#file_access), [getdirparams](directory.c.md#getdirparams), [getmetadata](file.c.md#getmetadata), [setdirparams](directory.c.md#setdirparams)

### mtoubits

```c
static mode_t mtoubits(uint8_t bits)
```

Defined at lines 192 to 201.

Map AFP access rights to the three permission bits of one Unix class.

Read and write each also grant search, because a Mac client cannot set the search right separately.

Parameters:
* `bits`: AR_UREAD and AR_UWRITE rights

Returns: the mode bits in the low three positions

Called by: [mtoumode](unix.c.md#mtoumode)

### mtoumode

```c
mode_t mtoumode(struct maccess *)
```

Defined at lines 214 to 224. Declared in [etc/afpd/directory.h](directory.h.md).

Build a Unix mode from AFP access rights.

The owner bits get the owner and world rights, the group bits the group and world rights, and the other bits the world rights. Read and write each also grant search. The mapping follows the Finder sharing window and page 63 of the AFP 3.0 specification.

Parameters:
* `ma`: AFP access rights

Returns: the permission bits of the mode

Calls: [mtoubits](unix.c.md#mtoubits)

Called by: [setdirparams](directory.c.md#setdirparams)

### setfilunixmode

```c
int setfilunixmode(const struct vol *vol, struct path *path, mode_t mode)
```

Defined at lines 238 to 256.

Apply a Unix mode to a file and to its AppleDouble metadata.

The mode gains the volume file permissions. The vfs_setfilmode hooks then derive the metadata mode, which needs write access wherever read access exists.

Parameters:
* `vol`: volume of the file
* `path`: file, its stat filled when missing
* `mode`: new mode

Returns: 0, or -1 when the stat or a mode change failed

Calls: [of_stat](ofork.c.md#of_stat), [setfilmode](../../libatalk/vfs/unix.c.md#setfilmode)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [setfilparams](file.c.md#setfilparams)

Calls through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setfilmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](../../libatalk/vfs/vfs.c.md#rf_setfilmode_ea), [ea_chmod_file](../../libatalk/vfs/ea_ad.c.md#ea_chmod_file), [vfs_setfilmode](../../libatalk/vfs/vfs.c.md#vfs_setfilmode)

### setdirunixmode

```c
int setdirunixmode(const struct vol *vol, char *name, mode_t mode)
```

Defined at lines 272 to 301.

Apply a Unix mode to a directory and to its AppleDouble metadata.

The mode gains the volume directory permissions and DIRBITS, minus the volume umask. A mode that grants the owner read and search access changes the directory before the vfs_setdirunixmode hooks. Any other mode changes it after them, so the metadata stays reachable.

Parameters:
* `vol`: volume of the directory
* `name`: directory
* `mode`: new mode

Returns: 0, or -1 when a mode change failed

Calls: [dir_rx_set](../../libatalk/vfs/unix.c.md#dir_rx_set), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [ochmod](../../libatalk/util/unix.c.md#ochmod)

Called by: [setdirparams](directory.c.md#setdirparams)

Calls through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirunixmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setdirunixmode_adouble), [RF_setdirunixmode_ea](../../libatalk/vfs/vfs.c.md#rf_setdirunixmode_ea), [ea_chmod_dir](../../libatalk/vfs/ea_ad.c.md#ea_chmod_dir), [vfs_setdirunixmode](../../libatalk/vfs/vfs.c.md#vfs_setdirunixmode)

### setfilowner

```c
int setfilowner(const struct vol *vol, const uid_t uid, const gid_t gid, struct path *path)
```

Defined at lines 314 to 330.

Give a file and its AppleDouble metadata a new owner and group.

EPERM counts as success for both changes.

Parameters:
* `vol`: volume of the file
* `uid`: new owner
* `gid`: new group
* `path`: file

Returns: 0, or -1 when a change failed with another error

Calls: [ochown](../../libatalk/util/unix.c.md#ochown)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [setfilparams](file.c.md#setfilparams)

Calls through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_chown_adouble](../../libatalk/vfs/vfs.c.md#rf_chown_adouble), [RF_chown_ea](../../libatalk/vfs/vfs.c.md#rf_chown_ea), [ea_chown](../../libatalk/vfs/ea_ad.c.md#ea_chown), [vfs_chown](../../libatalk/vfs/vfs.c.md#vfs_chown)

### setdirowner

```c
int setdirowner(const struct vol *vol, const char *name, const uid_t uid, const gid_t gid)
```

Defined at lines 345 to 358.

Give a directory and its AppleDouble metadata a new owner and group.

A failed change of the directory itself only logs. The vfs_setdirowner hooks then update the metadata. A uid or gid of 0 gets no special treatment.

Parameters:
* `vol`: volume of the directory
* `name`: directory
* `uid`: new owner
* `gid`: new group

Returns: 0, or -1 when a hook failed

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [ochown](../../libatalk/util/unix.c.md#ochown)

Called by: [setdirparams](directory.c.md#setdirparams)

Calls through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirowner_adouble](../../libatalk/vfs/vfs.c.md#rf_setdirowner_adouble), [RF_setdirowner_ea](../../libatalk/vfs/vfs.c.md#rf_setdirowner_ea), [vfs_setdirowner](../../libatalk/vfs/vfs.c.md#vfs_setdirowner)
