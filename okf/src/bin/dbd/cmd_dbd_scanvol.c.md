---
type: C Source File
title: "bin/dbd/cmd_dbd_scanvol.c"
description: "Volume scan of dbd: CNID and AppleDouble checks and repairs."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/dbd/cmd_dbd_scanvol.c"
tags: ["bin/dbd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-26T21:19:58+01:00 }
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

Defined at lines 77 to 88.

Tell whether a name is one of the netatalk folders .AppleDB and .AppleDesktop.

Parameters:
* `name`: directory entry name

Returns: the matching table entry, or NULL

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `netatalk_dirs`

### check_special_dirs

```c
static const char * check_special_dirs(const char *name)
```

Defined at lines 97 to 108.

Tell whether a name is a special directory that the scan leaves out, such as .zfs.

Parameters:
* `name`: directory entry name

Returns: the matching table entry, or NULL

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `special_dirs`

### update_cnid

```c
static int update_cnid(cnid_t did, const struct stat *sp, const char *oldname, const char *newname)
```

Defined at lines 122 to 142.

Move the CNID of a renamed entry from its old name to its new name.

The scan calls it after it decodes a CAP-encoded name. An entry that the database does not know needs no update.

Parameters:
* `did`: parent directory ID
* `sp`: stat result of the entry
* `oldname`: name before the rename
* `newname`: name after the rename

Returns: 0, or -1 when the update fails

Calls: [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

### check_adfile

```c
static int check_adfile(const char *fname, const struct stat *st, const char **newname)
```

Defined at lines 161 to 250.

Check the AppleDouble file of an entry, or convert it to extended attributes with -c.

On an adouble:ea volume the call converts the adouble:v2 data with -c and otherwise does nothing. On an adouble:v2 volume a missing AppleDouble file gets a log line. Outside -s, the call creates it with the Mac name and the owner of the entry. An existing file has to open. Under -s it is opened strictly read-only, so an empty header is reported rather than repaired.

Parameters:
* `fname`: entry name in the current directory
* `st`: stat result of the entry
* `newname`: receives the new name when the conversion renames the entry, or NULL

Returns: 0, or -1 for an access error, a missing file during -s, or a file that fails to open or create

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [convert_utf8_to_mac](../../libatalk/util/pathconv.c.md#convert_utf8_to_mac), [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### remove_eafiles

```c
static void remove_eafiles(const char *name, struct ea *ea)
```

Defined at lines 262 to 314.

Delete the extended attribute files of an entry from the .AppleDouble directory.

The call deletes every file in .AppleDouble whose name contains `name::EA`.

Parameters:
* `name`: entry name
* `ea`: EA state of the entry, unused

Calls: [dbd_log](cmd_dbd.c.md#dbd_log), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [check_eafiles](cmd_dbd_scanvol.c.md#check_eafiles)

Uses file-scope variables: `cwdbuf`

### check_eafiles

```c
static int check_eafiles(const char *fname)
```

Defined at lines 327 to 384.

Check the extended attribute files of an entry against its EA header.

An EA whose file is missing, or whose size differs from the header, drops out of the header. The call also deletes a file of the wrong size. Both changes happen even under -s. When the EA header does not open for another reason than a missing file, the call deletes the EA files, except under -s.

Parameters:
* `fname`: entry name

Returns: 0, or -1 when the EA header does not open

Calls: [dbd_log](cmd_dbd.c.md#dbd_log), [ea_close](../../libatalk/vfs/ea_ad.c.md#ea_close), [ea_open](../../libatalk/vfs/ea_ad.c.md#ea_open), [ea_path](../../libatalk/vfs/ea_ad.c.md#ea_path), [remove_eafiles](cmd_dbd_scanvol.c.md#remove_eafiles)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### adouble_dir_unusable

```c
static bool adouble_dir_unusable(int err)
```

Defined at lines 396 to 399.

Tell whether an open error means that a non-directory holds the .AppleDouble name.

A regular file gives ENOTDIR, and a symbolic link gives the O_NOFOLLOW error of the platform.

Parameters:
* `err`: errno of the failed open

Returns: true for such an error

Called by: [check_addir](cmd_dbd_scanvol.c.md#check_addir)

### check_addir

```c
static int check_addir(int volroot)
```

Defined at lines 415 to 574.

Check the .AppleDouble directory and its .Parent file in the current directory, and create them when missing.

An adouble:ea volume needs neither. A non-directory under the .AppleDouble name, which a non-AFP client may leave, skips the AppleDouble checks of the directory. Under -s a missing directory fails and a missing .Parent passes. Otherwise the call creates both with the Mac name of the directory and its owner. A directory that the call created but cannot open again is an error.

Parameters:
* `volroot`: nonzero in the volume root, unused

Returns: 0, 1 to skip the AppleDouble checks of the directory, or -1 for an error

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [adouble_dir_unusable](cmd_dbd_scanvol.c.md#adouble_dir_unusable), [convert_utf8_to_mac](../../libatalk/util/pathconv.c.md#convert_utf8_to_mac), [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### check_eafile_in_adouble

```c
static int check_eafile_in_adouble(int parent_fd, int addir_fd, const char *name)
```

Defined at lines 590 to 647.

Check whether a .AppleDouble entry is an EA file and whether its data file still exists.

Only a volume with `ea = ad` has EA files, and on others the call returns

1. An EA file whose data file is gone gets a log line and goes away, except under -s.

Parameters:
* `parent_fd`: open current directory
* `addir_fd`: open .AppleDouble directory
* `name`: entry name in .AppleDouble

Returns: 0 for a name without `::EA`, 1 for an EA file with its data file, or -1 for an orphan or an access error

Calls: [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [read_addir](cmd_dbd_scanvol.c.md#read_addir)

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### read_addir

```c
static int read_addir(void)
```

Defined at lines 660 to 773.

Check the entries of the .AppleDouble directory of the current directory.

Directories inside .AppleDouble only log, and EA files go to check_eafile_in_adouble. An AppleDouble file whose data file is gone goes away, except under -s. On an adouble:ea volume a missing or unusable .AppleDouble passes.

Returns: 0, or -1 when a directory does not open

Calls: [check_eafile_in_adouble](cmd_dbd_scanvol.c.md#check_eafile_in_adouble), [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `cwdbuf`, `dbd_flags`

### check_cnid

```c
static cnid_t check_cnid(const char *name, cnid_t did, struct stat *st, int adfile_ok)
```

Defined at lines 792 to 887.

Make sure that an entry has a CNID in the database and that its AppleDouble file agrees.

The call reads the CNID hint of the AppleDouble file, unless -i ignores it. Under -s, the metadata read cannot initialize or repair the header. It then adds or looks up the entry in the database with that hint. A mismatch rewrites the AppleDouble file with the database CNID, except under -s. An emptied CNID table ends the scan through longjmp, since the rest of the volume would get new IDs.

Parameters:
* `name`: entry name
* `did`: CNID of the parent directory
* `st`: stat result of the entry
* `adfile_ok`: 0 when the AppleDouble file of the entry is usable

Returns: the CNID of the entry, or CNID_INVALID for an error

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getid](../../libatalk/adouble/ad_attr.c.md#ad_getid), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_add](../../libatalk/cnid/cnid.c.md#cnid_add), [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `cwdbuf`, `dbd_flags`, `jmp`, `stamp`

### check_orphaned

```c
static int check_orphaned(const char *name)
```

Defined at lines 897 to 915.

Delete a `._name` file whose `name` no longer exists.

The call deletes the file even under -s.

Parameters:
* `name`: name that starts with `._`

Returns: 1 for an orphaned file, or 0

Calls: [dbd_log](cmd_dbd.c.md#dbd_log)

Called by: [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir)

Uses file-scope variables: `cwdbuf`

### dbd_readdir

```c
static int dbd_readdir(int volroot, cnid_t did)
```

Defined at lines 934 to 1137.

Scan the current directory, check each entry, and descend into the subdirectories.

The call checks the .AppleDouble directory first, then every entry. It skips the netatalk folders, .zfs in the volume root, and names that the volume does not accept. Types other than files, directories, and symbolic links go away, except under -s. The call checks the AppleDouble file of each entry, and for entries other than symbolic links the CNID and the EA files. It descends into each directory that has a CNID.

A signal ends the scan through longjmp.

Parameters:
* `volroot`: nonzero in the volume root
* `did`: CNID of the current directory

Returns: 0, or -1 for an error that stops the scan

Calls: [check_addir](cmd_dbd_scanvol.c.md#check_addir), [check_adfile](cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](cmd_dbd_scanvol.c.md#check_cnid), [check_eafiles](cmd_dbd_scanvol.c.md#check_eafiles), [check_netatalk_dirs](cmd_dbd_scanvol.c.md#check_netatalk_dirs), [check_orphaned](cmd_dbd_scanvol.c.md#check_orphaned), [check_special_dirs](cmd_dbd_scanvol.c.md#check_special_dirs), [dbd_log](cmd_dbd.c.md#dbd_log), [read_addir](cmd_dbd_scanvol.c.md#read_addir), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [update_cnid](cmd_dbd_scanvol.c.md#update_cnid)

Called by: [cmd_dbd_scanvol](cmd_dbd_scanvol.c.md#cmd_dbd_scanvol)

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](../../libatalk/vfs/vfs.c.md#validupath_adouble), [validupath_ea](../../libatalk/vfs/vfs.c.md#validupath_ea), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath)

Uses file-scope variables: `alarmed` in [bin/dbd/cmd_dbd.c](cmd_dbd.c.md), `cwdbuf`, `dbd_flags`, `jmp`

### cmd_dbd_scanvol

```c
int cmd_dbd_scanvol(struct vol *vol, dbd_flags_t flags)
```

Defined at lines 1152 to 1217. Declared in [bin/dbd/cmd_dbd.h](cmd_dbd.h.md).

Scan a whole volume from its root.

Only Unicode volumes qualify. The scan reads the stamp of the CNID database, and with -c it converts the AppleDouble data of the root. It then runs dbd_readdir from the root with CNID 2. The umask stays 0 during the scan, so repaired AppleDouble files take their permissions from the files they belong to. A signal ends the scan with 0.

Parameters:
* `vol_in`: volume
* `flags`: DBD_FLAGS_* options

Returns: 0, or -1 for a failed scan or an emptied CNID table

Calls: [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [cnid_getstamp](../../libatalk/cnid/cnid.c.md#cnid_getstamp), [dbd_log](cmd_dbd.c.md#dbd_log), [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [main](cmd_dbd.c.md#main)

Uses file-scope variables: `cwdbuf`, `dbd_flags`, `jmp`, `stamp`

# Macros

* Undocumented: `ADDIR_OK`, `ADFILE_OK`

# File-scope variables

`cwdbuf`, `dbd_flags`, `jmp`, `netatalk_dirs`, `special_dirs`, `stamp`, `vol`
