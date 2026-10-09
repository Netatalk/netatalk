---
type: C Source File
title: "bin/nad/nad_cp.c"
description: "AFP-aware copying of files and directory trees for nad."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_cp.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-09T22:45:01+11:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/bstrlib_compat.h](../../include/atalk/bstrlib_compat.h.md)
* [atalk/queue.h](../../include/atalk/queue.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [ftw.h](ftw.h.md)
* [nad.h](nad.h.md)
* System headers: `errno.h`, `libgen.h`, `limits.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### upfunc

```c
static void upfunc(void)
```

Defined at lines 113 to 117.

Called by: [nad_cp](nad_cp.c.md#nad_cp)

Uses file-scope variables: `did`, `pdid`, `ppdid`

Mentioned in the documentation of: [nad_cp](nad_cp.c.md#nad_cp)

### usage_cp

```c
static void usage_cp(void)
```

Defined at lines 119 to 152.

Called by: [nad_cp](nad_cp.c.md#nad_cp)

### copy_source_header

```c
static int copy_source_header(struct adouble *, const char *, int)
```

Defined at lines 155 to 200.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_copy_header](../../libatalk/adouble/ad_flush.c.md#ad_copy_header), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open)

Called by: [copy](nad_cp.c.md#copy)

Uses file-scope variables: `svolume`

### nad_cp

```c
int nad_cp(int argc, char *argv[], AFPObj *obj)
```

Defined at lines 215 to 387. Declared in [bin/nad/nad.h](nad.h.md).

execute the nad cp command

Parses options and initializes the destination base in to, distinguishing a single destination file, an existing destination directory, and a new destination directory for a recursive copy. Multiple sources require an existing destination directory.

Uses the custom [nftw()](ftw.h.md#nftw) implementation to traverse each source separately, with [copy()](nad_cp.c.md#copy) processing entries and [upfunc()](nad_cp.c.md#upfunc) tracking directory CNID state. Traversal runs without FTW_CHDIR, so source and destination paths may be absolute or relative to the caller's working directory.

Calls: [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [copy](nad_cp.c.md#copy), [nftw](ftw.h.md#nftw), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [upfunc](nad_cp.c.md#upfunc), [usage_cp](nad_cp.c.md#usage_cp)

Called by: [copy](nad_mv.c.md#copy), [main](nad.c.md#main)

Uses file-scope variables: `Rflag`, `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `badcp`, `did`, `dvolume`, `fflag`, `ftw_options`, `iflag`, `nflag`, `pdid`, `pflag`, `ppdid`, `rval`, `svolume`, `to`, `type`, `vflag`

### copy

```c
static int copy(const char *path, const struct stat *statp, int tflag, struct FTW *ftw)
```

Defined at lines 401 to 780.

process a source entry during the [nftw()](ftw.h.md#nftw) traversal

Constructs the current destination path in to. For an existing destination directory, appends the source basename and any descendant path to the destination base. When copying a directory to a new destination directory, omits the source directory's basename and appends only the descendant path. For a file-to-file copy, uses the destination path directly.

Dispatches copying by entry type and handles destination volume metadata and CNID registration where applicable.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [convert_dots_encoding](nad_util.c.md#convert_dots_encoding), [convert_utf8_to_mac](../../libatalk/util/pathconv.c.md#convert_utf8_to_mac), [copy_source_header](nad_cp.c.md#copy_source_header), [ftw_copy_file](nad_cp.c.md#ftw_copy_file), [ftw_copy_link](nad_cp.c.md#ftw_copy_link), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset), [setfile](nad_cp.c.md#setfile), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [nad_cp](nad_cp.c.md#nad_cp)

Calls through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_copyfile_adouble](../../libatalk/vfs/vfs.c.md#rf_copyfile_adouble), [RF_copyfile_ea](../../libatalk/vfs/vfs.c.md#rf_copyfile_ea), [ea_copyfile](../../libatalk/vfs/ea_ad.c.md#ea_copyfile), [sys_ea_copyfile](../../libatalk/vfs/ea_sys.c.md#sys_ea_copyfile), [vfs_copyfile](../../libatalk/vfs/vfs.c.md#vfs_copyfile)

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](../../libatalk/vfs/vfs.c.md#validupath_adouble), [validupath_ea](../../libatalk/vfs/vfs.c.md#validupath_ea), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath)

Uses file-scope variables: `Rflag`, `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `badcp`, `did`, `dvolume`, `nflag`, `pdid`, `pflag`, `ppdid`, `rval`, `svolume`, `to`, `type`, `vflag`

Mentioned in the documentation of: [nad_cp](nad_cp.c.md#nad_cp)

### ftw_copy_file

```c
static int ftw_copy_file(const struct FTW *, const char *, const struct stat *, int)
```

Defined at lines 791 to 993.

Calls: [setfile](nad_cp.c.md#setfile)

Called by: [copy](nad_cp.c.md#copy)

Calls through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletefile_adouble](../../libatalk/vfs/vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](../../libatalk/vfs/vfs.c.md#rf_deletefile_ea), [ea_deletefile](../../libatalk/vfs/ea_ad.c.md#ea_deletefile), [vfs_deletefile](../../libatalk/vfs/vfs.c.md#vfs_deletefile)

Uses file-scope variables: `dvolume`, `fflag`, `iflag`, `nflag`, `pflag`, `to`, `vflag`

### ftw_copy_link

```c
static int ftw_copy_link(const struct FTW *, const char *, const struct stat *, int)
```

Defined at lines 995 to 1026.

Calls: [setfile](nad_cp.c.md#setfile)

Called by: [copy](nad_cp.c.md#copy)

Uses file-scope variables: `pflag`, `to`

### setfile

```c
static int setfile(const struct stat *, int)
```

Defined at lines 1028 to 1107.

Calls: [atalk_stat_atime_timespec](../../include/atalk/compat.h.md#atalk_stat_atime_timespec), [atalk_stat_mtime_timespec](../../include/atalk/compat.h.md#atalk_stat_mtime_timespec), [atalk_timespec_to_timeval](../../include/atalk/compat.h.md#atalk_timespec_to_timeval)

Called by: [copy](nad_cp.c.md#copy), [ftw_copy_file](nad_cp.c.md#ftw_copy_file), [ftw_copy_link](nad_cp.c.md#ftw_copy_link)

Uses file-scope variables: `to`

# Typedefs and enums

* `enum op`: `FILE_TO_FILE`, `FILE_TO_DIR`, `DIR_TO_DNE`

# Macros

* Undocumented: `BUFSIZE_MAX`, `BUFSIZE_SMALL`, `MAXPHYS`, `STRIP_TRAILING_SLASH`, `YESNO`

# File-scope variables

`Rflag`, `badcp`, `did`, `dvolume`, `emptystring`, `fflag`, `ftw_options`, `iflag`, `nflag`, `pdid`, `pflag`, `ppdid`, `rval`, `svolume`, `to`, `type`, `vflag`
