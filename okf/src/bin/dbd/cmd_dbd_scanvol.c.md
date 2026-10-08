---
type: C Source File
title: "bin/dbd/cmd_dbd_scanvol.c"
description: "14 functions, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/dbd/cmd_dbd_scanvol.c"
tags: ["bin/dbd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/dbd](../dbd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/acl.h](../../include/atalk/acl.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [cmd_dbd.h](cmd_dbd.h.md)
* System headers: `arpa/inet.h`, `dirent.h`, `errno.h`, `fcntl.h`, `setjmp.h`, `stdbool.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### check_netatalk_dirs

```c
static const char * check_netatalk_dirs(const char *name)
```

Defined at lines 69 to 80.

Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop".

Returns: pointer to name or NULL.

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `netatalk_dirs`

### check_special_dirs

```c
static const char * check_special_dirs(const char *name)
```

Defined at lines 86 to 97.

Check for special names.

Returns: pointer to name or NULL.

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `special_dirs`

### update_cnid

```c
static int update_cnid(cnid_t did, const struct stat *sp, const char *oldname, const char *newname)
```

Defined at lines 102 to 122.

We unCAPed a name, update CNID db

Calls: [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

### check_adfile

```c
static int check_adfile(const char *fname, const struct stat *st, const char **newname)
```

Defined at lines 127 to 213.

Check for .AppleDouble file, create if missing

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [convert_utf8_to_mac](../../libatalk/util/pathconv.c.md#convert_utf8_to_mac), [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### remove_eafiles

```c
static void remove_eafiles(const char *name, struct ea *ea)
```

Defined at lines 218 to 270.

Remove all files with file::EA* from adouble dir

Calls: [dbd_log](cmd_dbd.c.md#dbd_log), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [check_eafiles](cmd_dbd_scanvol.c.md#check_eafiles)

Uses file-scope variables: `cwdbuf`

### check_eafiles

```c
static int check_eafiles(const char *fname)
```

Defined at lines 275 to 332.

Check Extended Attributes files

Calls: [dbd_log](cmd_dbd.c.md#dbd_log), [ea_close](../../libatalk/vfs/ea_ad.c.md#ea_close), [ea_open](../../libatalk/vfs/ea_ad.c.md#ea_open), [ea_path](../../libatalk/vfs/ea_ad.c.md#ea_path), [remove_eafiles](cmd_dbd_scanvol.c.md#remove_eafiles)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### adouble_dir_unusable

```c
static bool adouble_dir_unusable(int err)
```

Defined at lines 339 to 342.

Whether a non-directory holds the AppleDouble dir name.

A regular file gives ENOTDIR, a symlink the platform's O_NOFOLLOW errno.

Called by: [check_addir](cmd_dbd_scanvol.c.md#check_addir)

### check_addir

```c
static int check_addir(int volroot)
```

Defined at lines 347 to 501.

Check for .AppleDouble folder and .Parent, create if missing

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [adouble_dir_unusable](cmd_dbd_scanvol.c.md#adouble_dir_unusable), [convert_utf8_to_mac](../../libatalk/util/pathconv.c.md#convert_utf8_to_mac), [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### check_eafile_in_adouble

```c
static int check_eafile_in_adouble(int parent_fd, int addir_fd, const char *name)
```

Defined at lines 509 to 566.

Check if file cotains "::EA" and if it does check if its correspondig data fork exists.

Returns: 0 = name is not an EA file

Returns: 1 = name is an EA file and no problem was found

Returns: -1 = name is an EA file and data fork is gone

Calls: [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [read_addir](cmd_dbd_scanvol.c.md#read_addir)

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### read_addir

```c
static int read_addir(void)
```

Defined at lines 573 to 686.

Check files and dirs inside .AppleDouble folder.

Note: remove orphaned files

Note: bail on dirs

Calls: [check_eafile_in_adouble](cmd_dbd_scanvol.c.md#check_eafile_in_adouble), [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### check_cnid

```c
static cnid_t check_cnid(const char *name, cnid_t did, struct stat *st, int adfile_ok)
```

Defined at lines 692 to 786.

Check CNID for a file/dir, both from db and from ad-file.

Returns: Correct CNID of object or CNID_INVALID (ie 0) on error

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getid](../../libatalk/adouble/ad_attr.c.md#ad_getid), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_add](../../libatalk/cnid/cnid.c.md#cnid_add), [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `cwdbuf`, `dbd_flags`, `jmp`, `stamp`

### check_orphaned

```c
static int check_orphaned(const char *name)
```

Defined at lines 788 to 806.

Calls: [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `cwdbuf`

### dbd_readdir

```c
static int dbd_readdir(int volroot, cnid_t did)
```

Defined at lines 814 to 1017.

This is called recursively for all dirs. volroot=1 means we're in the volume root dir, 0 means we aren't. We use this when checking for netatalk private folders like .AppleDB. did is our parent's CNID.

Calls: [check_addir](cmd_dbd_scanvol.c.md#check_addir), [check_adfile](cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](cmd_dbd_scanvol.c.md#check_cnid), [check_eafiles](cmd_dbd_scanvol.c.md#check_eafiles), [check_netatalk_dirs](cmd_dbd_scanvol.c.md#check_netatalk_dirs), [check_orphaned](cmd_dbd_scanvol.c.md#check_orphaned), [check_special_dirs](cmd_dbd_scanvol.c.md#check_special_dirs), [dbd_log](cmd_dbd.c.md#dbd_log), [read_addir](cmd_dbd_scanvol.c.md#read_addir), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [update_cnid](cmd_dbd_scanvol.c.md#update_cnid)

Called by: [cmd_dbd_scanvol](cmd_dbd_scanvol.c.md#cmd_dbd_scanvol)

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](../../libatalk/vfs/vfs.c.md#validupath_adouble), [validupath_ea](../../libatalk/vfs/vfs.c.md#validupath_ea), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath)

Uses file-scope variables: `alarmed` in [bin/dbd/cmd_dbd.c](cmd_dbd.c.md), `cwdbuf`, `dbd_flags`, `jmp`

### cmd_dbd_scanvol

```c
int cmd_dbd_scanvol(struct vol *vol, dbd_flags_t flags)
```

Defined at lines 1022 to 1087. Declared in [bin/dbd/cmd_dbd.h](cmd_dbd.h.md).

Main func called from [cmd_dbd.c](cmd_dbd.c.md)

Calls: [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [cnid_getstamp](../../libatalk/cnid/cnid.c.md#cnid_getstamp), [dbd_log](cmd_dbd.c.md#dbd_log), [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [main](cmd_dbd.c.md#main)

Uses file-scope variables: `cwdbuf`, `dbd_flags`, `jmp`, `stamp`

# Macros

* Undocumented: `ADDIR_OK`, `ADFILE_OK`

# File-scope variables

`cwdbuf`, `dbd_flags`, `jmp`, `netatalk_dirs`, `special_dirs`, `stamp`, `vol`
