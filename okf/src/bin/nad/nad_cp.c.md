---
type: C Source File
title: "bin/nad/nad_cp.c"
description: "The nad cp command: copy entries and their metadata, assigning CNIDs resolved for the destination paths."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_cp.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
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

Defined at lines 110 to 114.

Step the directory IDs of the walk back to the parent after nftw leaves a directory.

Uses file-scope variables: `did`, `pdid`, `ppdid`

### usage_cp

```c
static void usage_cp(void)
```

Defined at lines 119 to 152.

Print the usage and the options of nad cp, then exit with EXIT_FAILURE.

Called by: [nad_cp](nad_cp.c.md#nad_cp)

### copy_source_header

```c
static int copy_source_header(struct adouble *dest, const char *path, int flags)
```

Defined at lines 167 to 212.

Transfer the AppleDouble header and the Finder comment of a source to a new entry.

The call reads the source metadata when the source volume uses adouble:v2 or adouble:ea. It adds the comment, which ad_copy_header leaves out.

Parameters:
* `dest`: open metadata of the new entry
* `path`: source path
* `flags`: ADFLAGS_DIR for a directory, or 0

Returns: 1 after the transfer, 0 when the source has no metadata, or -1 for a read error

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_copy_header](../../libatalk/adouble/ad_flush.c.md#ad_copy_header), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open)

Called by: [copy](nad_cp.c.md#copy)

Uses file-scope variables: `svolume`

### nad_cp

```c
int nad_cp(int argc, char *argv[], AFPObj *obj)
```

Defined at lines 232 to 404. Declared in [bin/nad/nad.h](nad.h.md).

Run the nad cp command on files and directory trees with their Mac metadata and CNIDs.

The options follow cp: -a, -f, -i, -n, -p, -R, -v, and -x. The last argument names the target. A single source goes to a target file, to a new target directory under -R, or into an existing directory. More than one source needs an existing directory.

The call walks each source with the nftw of nad, without FTW_CHDIR, so both paths can be relative. Each source and the target get separate volume states.

Parameters:
* `argc`: argument count, from the cp command on
* `argv`: argument vector, from the cp command on
* `obj`: AFP configuration with the volumes

Returns: 0, or 1 when a source fails, while usage and fatal errors exit

Calls: [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [copy](nad_cp.c.md#copy), [nftw](ftw.c.md#nftw), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [usage_cp](nad_cp.c.md#usage_cp)

Called by: [copy](nad_mv.c.md#copy), [main](nad.c.md#main)

Uses file-scope variables: `Rflag`, `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `badcp`, `did`, `dvolume`, `fflag`, `ftw_options`, `iflag` in [bin/nad/nad.h](nad.h.md), `nflag` in [bin/nad/nad.h](nad.h.md), `pdid`, `pflag` in [bin/nad/nad.h](nad.h.md), `ppdid`, `rval`, `svolume`, `to`, `type`, `vflag` in [bin/nad/nad.h](nad.h.md)

### copy

```c
static int copy(const char *path, const struct stat *statp, int tflag, struct FTW *ftw)
```

Defined at lines 428 to 807.

Recreate one source entry at the target during the nftw walk.

The call builds the target path from the target base and the part of the source path below the walk root. A new target directory under -R drops the name of the source root. A name that the target volume does not accept skips its subtree. Regular files and symbolic links get their data or their link text, and directories get created. Devices, sockets, and FIFOs only log.

On a target volume with AppleDouble metadata the call also transfers the metadata and registers the new entry with the CNID database. The metadata then holds the CNID and, on adouble:v2, the Mac name. When the source has no metadata, the dates come from the modification time of the target.

Parameters:
* `path`: source path
* `statp`: lstat result of the source
* `tflag`: nftw type flag, unused
* `ftw`: walk position with the base and the level

Returns: 0 to go on, FTW_SKIP_SUBTREE to skip a directory, or -1 to stop the walk

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [convert_dots_encoding](nad_util.c.md#convert_dots_encoding), [convert_utf8_to_mac](../../libatalk/util/pathconv.c.md#convert_utf8_to_mac), [copy_source_header](nad_cp.c.md#copy_source_header), [ftw_copy_file](nad_cp.c.md#ftw_copy_file), [ftw_copy_link](nad_cp.c.md#ftw_copy_link), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset), [setfile](nad_cp.c.md#setfile), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [nad_cp](nad_cp.c.md#nad_cp)

Uses file-scope variables: `Rflag`, `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `badcp`, `did`, `dvolume`, `nflag` in [bin/nad/nad.h](nad.h.md), `pdid`, `pflag` in [bin/nad/nad.h](nad.h.md), `ppdid`, `rval`, `svolume`, `to`, `type`, `vflag` in [bin/nad/nad.h](nad.h.md)

### ftw_copy_file

```c
static int ftw_copy_file(const struct FTW *entp, const char *spath, const struct stat *sp, int dne)
```

Defined at lines 836 to 1038.

Write the data of a source file to the target path.

Under -n an existing target stays, and under -i it needs a yes. Under -f the call deletes it and its metadata and creates it anew. A new target takes the mode of the source without the setuid and setgid bits. An error leaves the target in place.

Files up to 8 MiB go through mmap when it works. Other files go through a buffer of MAXPHYS bytes, or up to 2 MiB on hosts with more than 32 GiB of memory. Under -p the call then applies setfile.

Parameters:
* `entp`: walk position, unused
* `spath`: source path
* `sp`: stat result of the source
* `dne`: nonzero when the target does not exist

Returns: 0, or 1 for an error or a declined overwrite

Calls: [setfile](nad_cp.c.md#setfile)

Called by: [copy](nad_cp.c.md#copy)

Uses file-scope variables: `dvolume`, `fflag`, `iflag` in [bin/nad/nad.h](nad.h.md), `nflag` in [bin/nad/nad.h](nad.h.md), `pflag` in [bin/nad/nad.h](nad.h.md), `to`, `vflag` in [bin/nad/nad.h](nad.h.md)

### ftw_copy_link

```c
static int ftw_copy_link(const struct FTW *p, const char *spath, const struct stat *sstp, int exists)
```

Defined at lines 1052 to 1083.

Recreate a symbolic link at the target path.

The call reads the link text of the source, deletes an existing target, and creates the new link. Under -p it applies setfile.

Parameters:
* `p`: walk position, unused
* `spath`: source path
* `sstp`: lstat result of the source
* `exists`: nonzero when the target exists

Returns: 0, or 1 for an error

Calls: [setfile](nad_cp.c.md#setfile)

Called by: [copy](nad_cp.c.md#copy)

Uses file-scope variables: `pflag` in [bin/nad/nad.h](nad.h.md), `to`

### setfile

```c
static int setfile(const struct stat *fs, int fd)
```

Defined at lines 1098 to 1177.

Give the target the times, the owner, and the mode of the source under -p.

The times go through utimes, which follows a symbolic link. A differing owner drops the setuid and setgid bits from the mode, and a chown that fails with EPERM passes quietly. The file flags branch depends on HAVE_ST_FLAGS, which no build defines.

Parameters:
* `fs`: stat result of the source
* `fd`: open target, or -1 to work by path

Returns: 0, or 1 for an error

Calls: [atalk_stat_atime_timespec](../../include/atalk/compat.h.md#atalk_stat_atime_timespec), [atalk_stat_mtime_timespec](../../include/atalk/compat.h.md#atalk_stat_mtime_timespec), [atalk_timespec_to_timeval](../../include/atalk/compat.h.md#atalk_timespec_to_timeval)

Called by: [copy](nad_cp.c.md#copy), [ftw_copy_file](nad_cp.c.md#ftw_copy_file), [ftw_copy_link](nad_cp.c.md#ftw_copy_link)

Uses file-scope variables: `to`

# Typedefs and enums

* `enum op`: `FILE_TO_FILE`, `FILE_TO_DIR`, `DIR_TO_DNE`

# Macros

* `BUFSIZE_MAX`: Maximum buffer size in bytes - do not allow it to grow larger than this.
* `MAXPHYS`: Small (default) buffer size in bytes.
* Undocumented: `BUFSIZE_SMALL`, `STRIP_TRAILING_SLASH`, `YESNO`

# File-scope variables

`Rflag`, `badcp`, `did`, `dvolume`, `emptystring`, `fflag`, `ftw_options`, `iflag`, `nflag`, `pdid`, `pflag`, `ppdid`, `rval`, `svolume`, `to`, `type`, `vflag`
