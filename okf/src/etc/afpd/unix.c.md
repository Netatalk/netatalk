---
type: C Source File
title: "etc/afpd/unix.c"
description: "10 functions, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/unix.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 39 to 70.

Get the free space on a partition.

Called by: [getvolspace](volume.c.md#getvolspace)

### utombits

```c
static int utombits(mode_t bits)
```

Defined at lines 72 to 81.

Called by: [utommode](unix.c.md#utommode)

### utommode

```c
static void utommode(const AFPObj *obj, const struct stat *stat, struct maccess *ma)
```

Defined at lines 86 to 123.

Calls: [gmem](../../libatalk/util/unix.c.md#gmem), [utombits](unix.c.md#utombits)

Called by: [accessmode](unix.c.md#accessmode)

### accessmode

```c
void accessmode(const AFPObj *obj, const struct vol *vol, char *path, struct maccess *ma, struct dir *dir, struct stat *st)
```

Defined at lines 132 to 150.

Calculate the mode for a directory using a stat() call to estimate permission.

Note: the previous method, using access(), does not work correctly over NFS.

Calls: [acltoownermode](acls.c.md#acltoownermode), [ostat](../../libatalk/util/unix.c.md#ostat), [utommode](unix.c.md#utommode)

Called by: [afp_getsrvrparms](volume.c.md#afp_getsrvrparms), [check_access](directory.c.md#check_access), [file_access](directory.c.md#file_access), [getdirparams](directory.c.md#getdirparams), [getmetadata](file.c.md#getmetadata), [setdirparams](directory.c.md#setdirparams)

### mtoubits

```c
static mode_t mtoubits(uint8_t bits)
```

Defined at lines 152 to 161.

Called by: [mtoumode](unix.c.md#mtoumode)

### mtoumode

```c
mode_t mtoumode(struct maccess *)
```

Defined at lines 168 to 178. Declared in [etc/afpd/directory.h](directory.h.md).

Calls: [mtoubits](unix.c.md#mtoubits)

Called by: [setdirparams](directory.c.md#setdirparams)

### setfilunixmode

```c
int setfilunixmode(const struct vol *vol, struct path *path, mode_t mode)
```

Defined at lines 181 to 199.

Calls: [of_stat](ofork.c.md#of_stat), [setfilmode](../../libatalk/vfs/unix.c.md#setfilmode)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [setfilparams](file.c.md#setfilparams)

Calls through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setfilmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](../../libatalk/vfs/vfs.c.md#rf_setfilmode_ea), [ea_chmod_file](../../libatalk/vfs/ea_ad.c.md#ea_chmod_file), [vfs_setfilmode](../../libatalk/vfs/vfs.c.md#vfs_setfilmode)

### setdirunixmode

```c
int setdirunixmode(const struct vol *vol, char *name, mode_t mode)
```

Defined at lines 203 to 232.

Calls: [dir_rx_set](../../libatalk/vfs/unix.c.md#dir_rx_set), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [ochmod](../../libatalk/util/unix.c.md#ochmod)

Called by: [setdirparams](directory.c.md#setdirparams)

Calls through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirunixmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setdirunixmode_adouble), [RF_setdirunixmode_ea](../../libatalk/vfs/vfs.c.md#rf_setdirunixmode_ea), [ea_chmod_dir](../../libatalk/vfs/ea_ad.c.md#ea_chmod_dir), [vfs_setdirunixmode](../../libatalk/vfs/vfs.c.md#vfs_setdirunixmode)

### setfilowner

```c
int setfilowner(const struct vol *vol, const uid_t uid, const gid_t gid, struct path *path)
```

Defined at lines 235 to 251.

Calls: [ochown](../../libatalk/util/unix.c.md#ochown)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [setfilparams](file.c.md#setfilparams)

Calls through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_chown_adouble](../../libatalk/vfs/vfs.c.md#rf_chown_adouble), [RF_chown_ea](../../libatalk/vfs/vfs.c.md#rf_chown_ea), [ea_chown](../../libatalk/vfs/ea_ad.c.md#ea_chown), [vfs_chown](../../libatalk/vfs/vfs.c.md#vfs_chown)

### setdirowner

```c
int setdirowner(const struct vol *vol, const char *name, const uid_t uid, const gid_t gid)
```

Defined at lines 258 to 271.

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [ochown](../../libatalk/util/unix.c.md#ochown)

Called by: [setdirparams](directory.c.md#setdirparams)

Calls through [`vfs_ops::vfs_setdirowner`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setdirowner_adouble](../../libatalk/vfs/vfs.c.md#rf_setdirowner_adouble), [RF_setdirowner_ea](../../libatalk/vfs/vfs.c.md#rf_setdirowner_ea), [vfs_setdirowner](../../libatalk/vfs/vfs.c.md#vfs_setdirowner)
