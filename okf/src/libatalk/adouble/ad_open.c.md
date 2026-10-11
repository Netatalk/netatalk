---
type: C Source File
title: "libatalk/adouble/ad_open.c"
description: "Part of Netatalk's AppleDouble implementatation."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_open.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `stdarg.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Function tables

### ad_adouble

Initialized at line 130 as `struct adouble_fops`. Assigns:

* `ad_header_read`: [ad_header_read](ad_open.c.md#ad_header_read)
* `ad_header_upgrade`: [ad_header_upgrade](ad_open.c.md#ad_header_upgrade)
* `ad_mkrf`: [ad_mkrf](ad_open.c.md#ad_mkrf)
* `ad_path`: [ad_path](ad_open.c.md#ad_path)
* `ad_rebuild_header`: [ad_rebuild_adouble_header_v2](ad_flush.c.md#ad_rebuild_adouble_header_v2)

### ad_adouble_ea

Initialized at line 138 as `struct adouble_fops`. Assigns:

* `ad_header_read`: [ad_header_read_ea](ad_open.c.md#ad_header_read_ea)
* `ad_header_upgrade`: [ad_header_upgrade_ea](ad_open.c.md#ad_header_upgrade_ea)
* `ad_mkrf`: [ad_mkrf_osx](ad_open.c.md#ad_mkrf_osx)
* `ad_path`: [ad_path_osx](ad_open.c.md#ad_path_osx)
* `ad_rebuild_header`: [ad_rebuild_adouble_header_ea](ad_flush.c.md#ad_rebuild_adouble_header_ea)

# Functions

### adflags2logstr

```c
const char * adflags2logstr(int adflags)
```

Defined at lines 188 to 308.

Render an ADFLAGS_* mask as text, for log output.

Parameters:
* `adflags`: AppleDouble open flags

Returns: a static buffer with the names of the set flags joined by "|"

Calls: [strlcat](../compat/strlcpy.c.md#strlcat)

Called by: [ad_close](ad_flush.c.md#ad_close), [ad_flush](ad_flush.c.md#ad_flush), [ad_flush_hf](ad_flush.c.md#ad_flush_hf), [ad_flush_rf](ad_flush.c.md#ad_flush_rf), [ad_open](ad_open.c.md#ad_open), [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

### openflags2logstr

```c
const char * openflags2logstr(int oflags)
```

Defined at lines 319 to 367.

Render open(2) flags as text, for log output.

Covers O_RDONLY, O_RDWR, O_CREAT, O_TRUNC, and O_EXCL.

Parameters:
* `oflags`: open flags

Returns: a static buffer with the names of the set flags joined by "|"

Calls: [strlcat](../compat/strlcpy.c.md#strlcat)

Called by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

### get_eid

```c
static uint32_t get_eid(uint32_t eid)
```

Defined at lines 379 to 402.

Map an on-disk entry ID to the in-memory entry index.

IDs up to 15 are the standard AppleDouble entries and pass through. The netatalk private IDs AD_DEV, AD_INO, AD_SYN, and AD_ID map to ADEID_PRIVDEV, ADEID_PRIVINO, ADEID_PRIVSYN, and ADEID_PRIVID.

Parameters:
* `eid`: entry ID as stored on disk

Returns: the entry index, or 0 for an unknown ID

Called by: [parse_entries](ad_open.c.md#parse_entries)

### ad_init_offsets

```c
int ad_init_offsets(struct adouble *ad)
```

Defined at lines 416 to 460.

Initialize the entry table of an AppleDouble for its version.

ad_init_offsets leaves a header that already carries AD_MAGIC alone. Otherwise it sets the magic, the version, and the valid data length. The entries then take the offsets and lengths of the version 2 or EA layout. Without HAVE_EAFD the EA form also gets the macOS resource fork offset.

Parameters:
* `ad`: AppleDouble with ad_vers set

Returns: 0, or -1 for a version other than AD_VERSION2 or AD_VERSION_EA

Called by: [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_init](ad_open.c.md#ad_init), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [new_ad_header](ad_open.c.md#new_ad_header)

Uses file-scope variables: `entry_order2`, `entry_order_ea`

### new_ad_header

```c
static int new_ad_header(struct adouble *ad, const char *path, struct stat *stp, int adflags)
```

Defined at lines 476 to 526.

Fill a fresh AppleDouble header for a path.

Initializes the entry table and marks a dot file invisible when the volume sets ADVOL_INVDOTS, the directory itself excepted. The create, modify, and access dates take the file's mtime, the backup date AD_DATE_START.

Parameters:
* `ad`: AppleDouble with ad_vers and ad_options set
* `path`: path of the file, for the dot check and for lstat
* `stp`: stat of the file, or NULL to lstat path here
* `adflags`: ADFLAGS_DIR when path is a directory

Returns: 0, or -1 when the version is unknown, FinderInfo cannot be addressed, or the stat fails

Calls: [ad_init_offsets](ad_open.c.md#ad_init_offsets), [ad_setattr](ad_attr.c.md#ad_setattr), [ad_setdate](ad_date.c.md#ad_setdate)

Called by: [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea)

### parse_entries

```c
static int parse_entries(struct adouble *ad, uint16_t nentries, size_t valid_data_len)
```

Defined at lines 539 to 573.

Parse the entry descriptors of an AppleDouble header into the entry table.

Every entry except the resource fork must lie inside valid_data_len.

Parameters:
* `ad`: AppleDouble whose ad_data holds the header
* `nentries`: number of entry descriptors in the header
* `valid_data_len`: bytes of header data present in ad_data

Returns: 0, or -1 for a malformed entry

Calls: [get_eid](ad_open.c.md#get_eid)

Called by: [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_ea](ad_open.c.md#ad_header_read_ea), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx)

### ad_header_read

```c
static int ad_header_read(const char *path, struct adouble *ad, const struct stat *hst)
```

Defined at lines 589 to 664.

Read and check the header of a version 2 AppleDouble file.

Reads up to AD_DATASZ2 bytes from the metadata file and checks the magic, the version, and the entry count. It then parses the entries and requires a resource fork entry whose offset lies inside the buffer. The resource fork sits at the end of the file: its length is the file size minus its offset.

Parameters:
* `path`: path of the data file, for log messages
* `ad`: AppleDouble whose metadata descriptor is open
* `hst`: stat of the metadata file, or NULL to fstat it here

Returns: 0 on success, -1 with errno EIO for a malformed header, or 1 when the stat fails

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [adf_pread](ad_read.c.md#adf_pread), [fullpathname](../util/unix.c.md#fullpathname), [parse_entries](ad_open.c.md#parse_entries)

Called through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_refresh](ad_open.c.md#ad_refresh)

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

### ad_valid_header_osx

```c
int ad_valid_header_osx(const char *path)
```

Defined at lines 677 to 730.

Check whether a ._ file carries a netatalk-written AppleDouble header.

Reads the header, checks the magic and the version, and requires the netatalk filler string. A ._ file that macOS itself wrote fails the check: it is not one of ours, so enumeration and access treat it as plain data.

Parameters:
* `path`: path of the ._ file

Returns: 0 for a netatalk header, 1 otherwise, including when the file cannot be read

Calls: [fullpathname](../util/unix.c.md#fullpathname)

Called by: [nad_rm](../../bin/nad/nad_rm.c.md#nad_rm), [rm](../../bin/nad/nad_rm.c.md#rm), [validupath_ea](../vfs/vfs.c.md#validupath_ea)

### ad_convert_pread_full

```c
static int ad_convert_pread_full(struct ad_fd *adf, void *buf, size_t count, off_t offset)
```

Defined at lines 744 to 770.

Read an exact byte range from an AppleDouble fork.

Retries interrupted and short reads. Reaching EOF before `count` bytes have been read is reported as an I/O error.

Parameters:
* `adf`: open AppleDouble fork
* `buf`: destination buffer
* `count`: number of bytes to read
* `offset`: starting file offset

Returns: 0 on success, -1 on error with errno set

Calls: [adf_pread](ad_read.c.md#adf_pread)

Called by: [ad_convert_move_rfork](ad_open.c.md#ad_convert_move_rfork)

### ad_convert_pwrite_full

```c
static int ad_convert_pwrite_full(struct ad_fd *adf, const void *buf, size_t count, off_t offset)
```

Defined at lines 784 to 810.

Write an exact byte range to an AppleDouble fork.

Retries interrupted and short writes. A zero-length write before `count` bytes have been written is reported as an I/O error.

Parameters:
* `adf`: open AppleDouble fork
* `buf`: source buffer
* `count`: number of bytes to write
* `offset`: starting file offset

Returns: 0 on success, -1 on error with errno set

Calls: [adf_pwrite](ad_write.c.md#adf_pwrite)

Called by: [ad_convert_move_rfork](ad_open.c.md#ad_convert_move_rfork), [ad_convert_osx](ad_open.c.md#ad_convert_osx)

### ad_convert_move_rfork

```c
static int ad_convert_move_rfork(struct ad_fd *adf, off_t src, off_t dst, off_t len)
```

Defined at lines 825 to 844.

Move a resource fork towards the start of its AppleDouble file.

The caller must validate both ranges and ensure `src` is not before `dst`. Copying proceeds forwards in bounded chunks so overlapping ranges are safe when moving data towards the start of the file.

Parameters:
* `adf`: open AppleDouble fork to modify
* `src`: source range offset
* `dst`: destination range offset
* `len`: number of bytes to move

Returns: 0 on success, -1 on read or write error with errno set

Calls: [ad_convert_pread_full](ad_open.c.md#ad_convert_pread_full), [ad_convert_pwrite_full](ad_open.c.md#ad_convert_pwrite_full)

Called by: [ad_convert_osx](ad_open.c.md#ad_convert_osx)

### ad_convert_osx

```c
static int ad_convert_osx(const char *path, struct adouble *ad, const struct stat *hst)
```

Defined at lines 861 to 1033.

Convert a macOS ._ AppleDouble file to the netatalk layout.

A macOS FinderInfo entry longer than 32 bytes carries packed xattrs, which netatalk cannot use. The conversion discards them and moves the resource fork to its canonical offset. Entry offsets and lengths come from the file, so ad_convert_osx validates them and refuses a malformed sidecar instead of rewriting it. [ad_open()](ad_open.c.md#ad_open) runs during the conversion and could recurse, so a static flag makes a nested call a no-op.

Parameters:
* `path`: path of the ._ file, for log messages
* `ad`: AppleDouble whose entries describe the file as read
* `hst`: stat of the open sidecar the header came from

Returns: -1 on error, 0 when nothing needed converting, 1 after converting

Calls: [ad_close](ad_flush.c.md#ad_close), [ad_convert_move_rfork](ad_open.c.md#ad_convert_move_rfork), [ad_convert_pwrite_full](ad_open.c.md#ad_convert_pwrite_full), [ad_flush](ad_flush.c.md#ad_flush), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_init_offsets](ad_open.c.md#ad_init_offsets), [ad_init_old](ad_open.c.md#ad_init_old), [ad_open](ad_open.c.md#ad_open), [ad_rebuild_adouble_header_osx](ad_flush.c.md#ad_rebuild_adouble_header_osx), [fullpathname](../util/unix.c.md#fullpathname)

Called by: [ad_header_read_osx](ad_open.c.md#ad_header_read_osx)

Mentioned in the documentation of: [ad_header_read_osx](ad_open.c.md#ad_header_read_osx)

### ad_header_read_osx

```c
static int ad_header_read_osx(const char *path, struct adouble *ad, struct stat *hst)
```

Defined at lines 1049 to 1150.

Read the header of a macOS ._ file and take its resource fork.

Only the resource fork comes from the ._ file. The FinderInfo lives in the file's extended attribute. A FinderInfo entry of the wrong length means a macOS-written file, which [ad_convert_osx()](ad_open.c.md#ad_convert_osx) rewrites once before a second read. The resource fork entry must sit at ADEDOFF_RFORK_OSX.

Parameters:
* `path`: path of the data file, for log messages
* `ad`: AppleDouble whose resource fork descriptor is open
* `hst`: stat of the ._ file, or NULL to fstat it here

Returns: 0 on success, -1 with errno EIO for a malformed or unconvertible header, or the errno of fstat or pread

Calls: [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_init_old](ad_open.c.md#ad_init_old), [adf_pread](ad_read.c.md#adf_pread), [fullpathname](../util/unix.c.md#fullpathname), [parse_entries](ad_open.c.md#parse_entries)

Called by: [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [ad_refresh](ad_open.c.md#ad_refresh)

### ad_header_read_ea

```c
static int ad_header_read_ea(const char *path, struct adouble *ad, const struct stat *hst)
```

Defined at lines 1168 to 1269.

Read and check the metadata EA of an AppleDouble ea file.

The EA comes from the metadata descriptor when one is open, otherwise from path. The header must hold AD_DATASZ_EA bytes, the version 2 magic and version, ADEID_NUM_EA entries, and every entry the ea layout defines. Without HAVE_EAFD the function forces the resource fork offset to ADEDOFF_RFORK_OSX. A malformed EA fails with EINVAL. When ad carries ADFLAGS_CREATE the function deletes the malformed EA as root and fails with ENOENT instead, so the caller creates a fresh one.

Parameters:
* `path`: file whose metadata EA to read
* `ad`: AppleDouble receiving the parsed header
* `hst`: unused

Returns: 0, or -1 with errno set

Calls: [become_root](../util/unix.c.md#become_root), [fullpathname](../util/unix.c.md#fullpathname), [parse_entries](ad_open.c.md#parse_entries), [sys_fgetxattr](../vfs/extattr.c.md#sys_fgetxattr), [sys_getxattr](../vfs/extattr.c.md#sys_getxattr), [sys_removexattr](../vfs/extattr.c.md#sys_removexattr), [unbecome_root](../util/unix.c.md#unbecome_root)

Called through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_refresh](ad_open.c.md#ad_refresh)

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_mkrf

```c
static int ad_mkrf(const char *path)
```

Defined at lines 1281 to 1301.

Create the .AppleDouble directory that holds a version 2 metadata file.

The directory is the parent of path. For "/dir/.AppleDouble/file" the call creates "/dir/.AppleDouble" through ad_mkdir.

Parameters:
* `path`: metadata file path, truncated at its last slash during the mkdir and restored only on success

Returns: 0, or -1 when path has no slash or ad_mkdir failed

Calls: [ad_mkdir](ad_open.c.md#ad_mkdir)

Called through [`adouble_fops::ad_mkrf`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

### ad_mkrf_osx

```c
static int ad_mkrf_osx(const char *path)
```

Defined at lines 1318 to 1321.

Create nothing, because the ea layout stores forks without a parent directory.

Parameters:
* `path`: unused

Returns: 0

Called through [`adouble_fops::ad_mkrf`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_chown

```c
static int ad_chown(const char *path, struct stat *stbuf)
```

Defined at lines 1337 to 1351.

Give a freshly created AppleDouble file the owner that the parent directory implies.

Only a root afpd changes ownership, and it signals that through ad_setfuid. The new owner is the configured default user, or the parent directory owner when the default is zero. The group always follows the parent directory.

Parameters:
* `path`: file to chown
* `stbuf`: stat of the parent directory

Returns: 0, or -1 when lchown failed

Called by: [ad_mkdir](ad_open.c.md#ad_mkdir), [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

Uses file-scope variables: `default_uid`

### ad_mode_st

```c
static int ad_mode_st(const char *path, mode_t *mode, struct stat *stbuf)
```

Defined at lines 1364 to 1377.

Restrict a creation mode to the permissions of the parent directory.

Parameters:
* `path`: file whose parent directory supplies the permissions
* `mode`: requested mode, masked with the parent directory mode, or with DEFMASK when the stat failed
* `stbuf`: receives the parent directory stat

Returns: 0, or -1 when mode is zero or the parent directory stat failed

Calls: [ad_stat](ad_open.c.md#ad_stat)

Called by: [ad_mkdir](ad_open.c.md#ad_mkdir), [ad_mode](ad_open.c.md#ad_mode), [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

### ad_header_upgrade

```c
static int ad_header_upgrade(struct adouble *ad, const char *name)
```

Defined at lines 1387 to 1390.

Upgrade nothing, because the version 2 layout has a single header format.

Parameters:
* `ad`: unused
* `name`: unused

Returns: 0

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

### ad_header_upgrade_ea

```c
static int ad_header_upgrade_ea(struct adouble *ad, const char *name)
```

Defined at lines 1399 to 1403.

Abort, because the ea layout never upgrades a header.

Parameters:
* `ad`: unused
* `name`: unused

Returns: 0, though AFP_PANIC ends the process first

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_error

```c
static int ad_error(struct adouble *ad, int adflags)
```

Defined at lines 1419 to 1442.

Decide whether a failed metadata open fails the entire ad_open.

A missing header is no error under ADFLAGS_NOHF, and a read-only ADFLAGS_RF open tolerates ENOENT. Only ENOENT and ENOATTR count as missing, so an unreadable header still fails. Any other failure also closes a data fork that this ad_open opened under ADFLAGS_DF, ADFLAGS_SETSHRMD, or ADFLAGS_CHECK_OF.

Parameters:
* `ad`: AppleDouble whose metadata open failed, errno still set
* `adflags`: ADFLAGS_* bits of the ad_open call

Returns: 0 when the flags tolerate the failure, -1 otherwise

Calls: [ad_close](ad_flush.c.md#ad_close)

Called by: [ad_open_hf](ad_open.c.md#ad_open_hf)

### ad2openflags

```c
static int ad2openflags(const struct adouble *ad, int adfile, int adflags)
```

Defined at lines 1452 to 1504.

Map ADFLAGS to open() flags.

Parameters:
* `ad`: the adouble structure
* `adfile`: the file you really want to open: ADFLAGS_DF or ADFLAGS_HF
* `adflags`: flags from ad_open(..., adflags, ...)

Returns: mapped flags suitable for calling open()

Called by: [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea)

### ad_open_df

```c
static int ad_open_df(const char *path, int adflags, mode_t mode, struct adouble *ad)
```

Defined at lines 1546 to 1666.

Open the data fork of an AppleDouble.

An already open fork gains a reference. A new open maps adflags through ad2openflags and, under ADFLAGS_CREATE, restricts mode to the parent directory permissions unless ADVOL_UNIXPRIV keeps it. When O_NOFOLLOW hits a symlink the fork stores the link target instead of a descriptor and marks it AD_SYMLINK. EACCES, EPERM, or EROFS on a read-only ADFLAGS_SETSHRMD open retries read-only without O_CREAT, O_EXCL, or O_TRUNC. Under ADFLAGS_CREATE, ad_chown then sets the owner of the opened file.

Parameters:
* `path`: data file
* `adflags`: ADFLAGS_* bits of the ad_open call
* `mode`: creation mode under ADFLAGS_CREATE
* `ad`: AppleDouble receiving the data fork descriptor

Returns: 0, or -1 with errno set

Calls: [ad2openflags](ad_open.c.md#ad2openflags), [ad_chown](ad_open.c.md#ad_chown), [ad_mode_st](ad_open.c.md#ad_mode_st), [adf_lock_init](ad_lock.c.md#adf_lock_init), [adflags2logstr](ad_open.c.md#adflags2logstr), [fullpathname](../util/unix.c.md#fullpathname)

Called by: [ad_open](ad_open.c.md#ad_open)

### ad_open_hf_v2

```c
static int ad_open_hf_v2(const char *path, int adflags, mode_t mode, struct adouble *ad)
```

Defined at lines 1689 to 1848.

Open or create the version 2 metadata file of an AppleDouble.

A symlink data fork short-circuits with nothing opened. An already open metadata file gains a reference after ad_refresh. Otherwise the call opens the ad_path file without O_CREAT or O_EXCL. EACCES, EPERM, or EROFS on a read-only ADFLAGS_SETSHRMD open retries read-only without O_TRUNC.

ENOENT under ADFLAGS_CREATE creates the .AppleDouble directory when needed, retries with O_CREAT and a mode from ad_hf_mode, and lets ad_chown set the owner. A new or empty file receives a fresh header through new_ad_header and ad_flush. Any other file goes through ad_header_read. A failure after the open closes the descriptor again.

Parameters:
* `path`: data file whose metadata file to open
* `adflags`: ADFLAGS_* bits of the ad_open call
* `mode`: creation mode for a new metadata file
* `ad`: AppleDouble receiving the metadata descriptor and header

Returns: 0, or -1 with errno set

Calls: [ad2openflags](ad_open.c.md#ad2openflags), [ad_chown](ad_open.c.md#ad_chown), [ad_flush](ad_flush.c.md#ad_flush), [ad_hf_mode](ad_open.c.md#ad_hf_mode), [ad_mode_st](ad_open.c.md#ad_mode_st), [ad_refresh](ad_open.c.md#ad_refresh), [adf_lock_init](ad_lock.c.md#adf_lock_init), [adflags2logstr](ad_open.c.md#adflags2logstr), [fullpathname](../util/unix.c.md#fullpathname), [new_ad_header](ad_open.c.md#new_ad_header), [openflags2logstr](ad_open.c.md#openflags2logstr)

Called by: [ad_open_hf](ad_open.c.md#ad_open_hf)

Calls through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_ea](ad_open.c.md#ad_header_read_ea)

Calls through [`adouble_fops::ad_mkrf`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_mkrf](ad_open.c.md#ad_mkrf), [ad_mkrf_osx](ad_open.c.md#ad_mkrf_osx)

Calls through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_path](ad_open.c.md#ad_path), [ad_path_osx](ad_open.c.md#ad_path_osx)

### ad_open_hf_ea

```c
static int ad_open_hf_ea(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 1872 to 1997.

Open or create the metadata EA of an AppleDouble ea file.

A symlink short-circuits with nothing opened. An already open metadata descriptor gains a reference. A write request on a closed descriptor opens the file itself, read-only for a directory so that fchdir still works. A read-only request reads the EA by path and opens nothing.

A missing EA fails with ENOENT unless ADFLAGS_CREATE allows creating one. Creation on an ADVOL_RO volume fails with EROFS. new_ad_header and ad_flush write the fresh header, and the descriptor flags gain O_CREAT. On macOS the native FinderInfo EA overwrites the FinderInfo entry. ad_rlen receives the resource fork size, and a failure after the open closes the descriptor again.

Parameters:
* `path`: file whose metadata EA to open
* `adflags`: ADFLAGS_* bits of the ad_open call
* `mode`: unused
* `ad`: AppleDouble receiving the metadata descriptor and header

Returns: 0, or -1 with errno set

Calls: [ad2openflags](ad_open.c.md#ad2openflags), [ad_flush](ad_flush.c.md#ad_flush), [ad_reso_size](ad_open.c.md#ad_reso_size), [adflags2logstr](ad_open.c.md#adflags2logstr), [fullpathname](../util/unix.c.md#fullpathname), [new_ad_header](ad_open.c.md#new_ad_header)

Called by: [ad_open_hf](ad_open.c.md#ad_open_hf)

Calls through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_ea](ad_open.c.md#ad_header_read_ea)

### ad_open_hf

```c
static int ad_open_hf(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 2012 to 2038.

Open AppleDouble metadata with the layout that ad_vers selects.

The metadata reference count rises before the layout-specific open runs and falls again when that open fails. ad_error then decides whether the failure is tolerable.

Parameters:
* `path`: data file whose metadata to open
* `adflags`: ADFLAGS_* bits of the ad_open call
* `mode`: creation mode for new metadata
* `ad`: AppleDouble receiving the metadata

Returns: 0, or -1 with errno set

Calls: [ad_error](ad_open.c.md#ad_error), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

Called by: [ad_open](ad_open.c.md#ad_open)

### ad_reso_size

```c
off_t ad_reso_size(const char *path, int adflags, struct adouble *ad)
```

Defined at lines 2053 to 2096.

Size of the resource fork of an AppleDouble ea file.

With HAVE_EAFD the size comes from the open resource fork descriptor. Failing that, it comes from the resource fork EA of the metadata descriptor or of path. Without HAVE_EAFD it is the size of the ad_path_osx file beyond its header.

Parameters:
* `path`: file whose resource fork to measure
* `adflags`: ADFLAGS_* bits, ADFLAGS_DIR yields 0
* `ad`: AppleDouble with possibly open descriptors, or NULL

Returns: the size in bytes, or 0 for a directory or when the lookup fails

Calls: [ad_path_osx](ad_open.c.md#ad_path_osx), [sys_fgetxattr](../vfs/extattr.c.md#sys_fgetxattr), [sys_lgetxattr](../vfs/extattr.c.md#sys_lgetxattr)

Called by: [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [getmetadata](../../etc/afpd/file.c.md#getmetadata)

### ad_open_rf_v2

```c
static int ad_open_rf_v2(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 2112 to 2134.

Take a resource fork reference on a version 2 AppleDouble.

The version 2 resource fork lives in the metadata file, so ad_open_hf_v2 already opened it. A closed metadata file fails the call unless ADFLAGS_NORF or ADFLAGS_RDONLY tolerates it, and then the count stays unchanged.

Parameters:
* `path`: data file, for logging only
* `adflags`: ADFLAGS_* bits of the ad_open call
* `mode`: unused
* `ad`: AppleDouble whose resource fork count rises

Returns: 0, or -1 when the metadata file did not open and the flags require it

Calls: [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [fullpathname](../util/unix.c.md#fullpathname)

Called by: [ad_open_rf](ad_open.c.md#ad_open_rf)

### ad_open_rf_ea

```c
static int ad_open_rf_ea(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 2158 to 2342.

Open or create the resource fork of an AppleDouble ea file.

An already open resource fork gains a reference and refreshes ad_rlen. A new open uses sys_getxattrfd on the metadata descriptor on Solaris, and open on the ad_path file elsewhere, both without O_CREAT. EACCES, EPERM, or EROFS on a read-only open retries read-only without O_TRUNC. ENOENT without ADFLAGS_CREATE returns 0 with no fork open. ADFLAGS_CREATE adds O_CREAT and creates the fork with mode.

Without HAVE_EAFD a created fork receives a header through new_ad_header and ad_flush, and an existing one goes through ad_header_read_osx. ad_rlen receives the fork size. A failure closes a descriptor this call opened, and ENOENT or ENOATTR under ADFLAGS_NORF then counts as success. Any other failure also closes the data and metadata forks that adflags name.

Parameters:
* `path`: data file whose resource fork to open
* `adflags`: ADFLAGS_* bits of the ad_open call
* `mode`: creation mode under ADFLAGS_CREATE
* `ad`: AppleDouble receiving the resource fork descriptor

Returns: 0, or -1 with errno set

Calls: [ad2openflags](ad_open.c.md#ad2openflags), [ad_close](ad_flush.c.md#ad_close), [ad_flush](ad_flush.c.md#ad_flush), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx), [ad_reso_size](ad_open.c.md#ad_reso_size), [fullpathname](../util/unix.c.md#fullpathname), [new_ad_header](ad_open.c.md#new_ad_header), [sys_getxattrfd](../vfs/extattr.c.md#sys_getxattrfd)

Called by: [ad_open_rf](ad_open.c.md#ad_open_rf)

Calls through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_path](ad_open.c.md#ad_path), [ad_path_osx](ad_open.c.md#ad_path_osx)

### ad_open_rf

```c
static int ad_open_rf(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 2354 to 2374.

Open the resource fork of an AppleDouble with the layout that ad_vers selects.

Parameters:
* `path`: data file whose resource fork to open
* `adflags`: ADFLAGS_* bits of the ad_open call
* `mode`: creation mode under ADFLAGS_CREATE
* `ad`: AppleDouble receiving the resource fork

Returns: 0, or -1 on failure or an unknown ad_vers

Calls: [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [ad_open_rf_v2](ad_open.c.md#ad_open_rf_v2)

Called by: [ad_open](ad_open.c.md#ad_open)

### ad_entry_check_size

```c
static bool ad_entry_check_size(uint32_t eid, size_t bufsize, uint32_t off, uint32_t got_len)
```

Defined at lines 2397 to 2494.

Check that entry eid with got_len bytes at off fits the ad_data buffer.

An empty entry always passes. Fixed-size entries must have exactly their defined length. FinderInfo must hold at least ADEDLEN_FINDERI bytes, because a macOS sidecar packs xattrs behind the canonical 32, and ad_convert_osx discards them later. Other variable entries must not exceed their defined length, and the buffer must have room for that maximum. The data and resource forks never live in the buffer, so they always pass.

Parameters:
* `eid`: entry id, ADEID_*
* `bufsize`: valid bytes in the ad_data buffer
* `off`: entry offset in the buffer
* `got_len`: entry length from the header

Returns: true when the entry fits, false when it does not or eid is out of range

Called by: [ad_entry_fits](ad_open.c.md#ad_entry_fits)

### ad_entry_fits

```c
bool ad_entry_fits(const struct adouble *ad, int eid, uint32_t len)
```

Defined at lines 2507 to 2512.

Whether entry eid in ad has room for len bytes at its offset.

This is the bounds check of [ad_entry](../../include/atalk/adouble.h.md#struct-ad_entry) against a caller-supplied length, so a caller can test a destination before it commits the length.

Parameters:
* `ad`: AppleDouble with its metadata loaded
* `eid`: entry id, ADEID_*
* `len`: length to test

Returns: true when the entry has an offset and the length fits

Calls: [ad_entry_check_size](ad_open.c.md#ad_entry_check_size), [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_copy_header](ad_flush.c.md#ad_copy_header), [ad_entry](ad_open.c.md#ad_entry)

### ad_entry

```c
void * ad_entry(const struct adouble *ad, int eid)
```

Defined at lines 2522 to 2535.

Pointer to entry eid inside the ad_data buffer of ad.

Parameters:
* `ad`: AppleDouble with its metadata loaded
* `eid`: entry id, ADEID_*

Returns: the entry start, or NULL when the entry is absent or does not fit the buffer

Calls: [ad_entry_fits](ad_open.c.md#ad_entry_fits), [ad_getentryoff](ad_open.c.md#ad_getentryoff)

### ad_getentryoff

```c
off_t ad_getentryoff(const struct adouble *ad, int eid)
```

Defined at lines 2549 to 2572.

Offset of entry eid in the layout of ad.

Version 2 returns the header offset. The ea layout returns 0 for the data fork and, with HAVE_EAFD, for the resource fork, because those forks live outside the metadata EA.

Parameters:
* `ad`: AppleDouble with its metadata loaded
* `eid`: entry id, ADEID_*

Returns: the entry offset, 0 for a fork kept outside the buffer or an absent entry

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_copy_header](ad_flush.c.md#ad_copy_header), [ad_entry](ad_open.c.md#ad_entry), [ad_entry_fits](ad_open.c.md#ad_entry_fits), [ad_flush_hf](ad_flush.c.md#ad_flush_hf), [ad_flush_rf](ad_flush.c.md#ad_flush_rf), [ad_fork_fileno](ad_open.c.md#ad_fork_fileno), [ad_getattr](ad_attr.c.md#ad_getattr), [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_getdate](ad_date.c.md#ad_getdate), [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx), [ad_lock](ad_lock.c.md#ad_lock), [ad_read](ad_read.c.md#ad_read), [ad_rebuild_adouble_header_v2](ad_flush.c.md#ad_rebuild_adouble_header_v2), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [ad_setattr](ad_attr.c.md#ad_setattr), [ad_setdate](ad_date.c.md#ad_setdate), [ad_setname](ad_attr.c.md#ad_setname), [ad_testlock_range](ad_lock.c.md#ad_testlock_range), [ad_tmplock](ad_lock.c.md#ad_tmplock), [ad_write](ad_write.c.md#ad_write), [copy_fork](ad_write.c.md#copy_fork), [getvolparams](../../etc/afpd/volume.c.md#getvolparams)

Mentioned in the documentation of: [ad_testlock_range](ad_lock.c.md#ad_testlock_range)

### ad_fork_fileno

```c
int ad_fork_fileno(const struct adouble *ad, int eid, off_t *off)
```

Defined at lines 2583 to 2591.

The descriptor that holds fork eid.

Parameters:
* `ad`: adouble holding the fork
* `eid`: ADEID_DFORK or ADEID_RFORK
* `off`: fork offset, returned as the offset in that descriptor

Returns: the data fork or resource fork descriptor

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_recvfile](ad_recvfile.c.md#ad_recvfile), [read_fork](../../etc/afpd/fork.c.md#read_fork)

### ad_path_ea

```c
const char * ad_path_ea(const char *path, int adflags)
```

Defined at lines 2601 to 2604.

Return path unchanged, because the Solaris ea layout keeps every fork in EAs of the file.

Parameters:
* `path`: file or directory
* `adflags`: unused

Returns: path

Called by: [init_null_vol](../../bin/nad/nad_util.c.md#init_null_vol), [initvol_vfs](../vfs/vfs.c.md#initvol_vfs), [set_fallback_volume](../../bin/nad/megatron.c.md#set_fallback_volume)

### ad_path_osx

```c
const char * ad_path_osx(const char *path, int adflags)
```

Defined at lines 2618 to 2658.

Sidecar path of the resource fork in the ea layout.

The result is "._name" beside the file, or "name/..namedfork/rsrc" on macOS where the kernel exposes the native resource fork as a file. A path of "." names the current directory and getcwd resolves it first. The result lives in a static buffer that the next call overwrites.

Parameters:
* `path`: file whose resource fork sidecar to name
* `adflags`: unused

Returns: the sidecar path in a static buffer

Calls: [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy), [strnlen](../compat/misc.c.md#strnlen)

Called by: [ad_reso_size](ad_open.c.md#ad_reso_size), [init_null_vol](../../bin/nad/nad_util.c.md#init_null_vol), [initvol_vfs](../vfs/vfs.c.md#initvol_vfs), [set_fallback_volume](../../bin/nad/megatron.c.md#set_fallback_volume)

Called through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_path

```c
const char * ad_path(const char *path, int adflags)
```

Defined at lines 2672 to 2722.

Metadata file path of the version 2 layout.

A file "a/b" maps to "a/.AppleDouble/b". A directory "a/b" maps to "a/b/.AppleDouble/.Parent". The result lives in a static buffer that the next call overwrites.

Parameters:
* `path`: file or directory
* `adflags`: ADFLAGS_DIR marks path as a directory

Returns: the metadata path in a static buffer, or NULL when it would exceed MAXPATHLEN

Calls: [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [ad_conv_v22ea](ad_conv.c.md#ad_conv_v22ea), [initvol_vfs](../vfs/vfs.c.md#initvol_vfs), [set_fallback_volume](../../bin/nad/megatron.c.md#set_fallback_volume)

Called through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

### ad_dir

```c
char * ad_dir(const char *path)
```

Defined at lines 2734 to 2790.

Parent directory of path, for permission inheritance.

The result drops the last component and any trailing slashes, and a bare file name yields ".". It lives in a static buffer that the next call overwrites.

Parameters:
* `path`: file or directory

Returns: the parent directory in a static buffer, or NULL with ENAMETOOLONG

Called by: [RF_setdirmode_adouble](../vfs/vfs.c.md#rf_setdirmode_adouble), [RF_setdirunixmode_adouble](../vfs/vfs.c.md#rf_setdirunixmode_adouble), [ad_stat](ad_open.c.md#ad_stat), [check_access](../../etc/afpd/directory.c.md#check_access)

### ad_setfuid

```c
int ad_setfuid(const uid_t id)
```

Defined at lines 2799 to 2803.

Register the user that ad_chown gives to new AppleDouble files.

Parameters:
* `id`: default owner, 0 to inherit from the parent directory, or -1 to disable

Returns: 0

Called by: [login](../../etc/afpd/auth.c.md#login), [main](../../bin/dbd/cmd_dbd.c.md#main)

Uses file-scope variables: `default_uid`

### ad_getfuid

```c
uid_t ad_getfuid(void)
```

Defined at lines 2810 to 2813.

The user that ad_setfuid registered.

Returns: the default owner, -1 before any ad_setfuid call

Uses file-scope variables: `default_uid`

### ad_stat

```c
int ad_stat(const char *path, struct stat *stbuf)
```

Defined at lines 2822 to 2832.

Stat the parent directory of path.

Parameters:
* `path`: file or directory
* `stbuf`: receives the parent directory stat

Returns: 0, or -1 when ad_dir or stat failed

Calls: [ad_dir](ad_open.c.md#ad_dir)

Called by: [ad_mode_st](ad_open.c.md#ad_mode_st)

### ad_mode

```c
int ad_mode(const char *path, mode_t mode)
```

Defined at lines 2842 to 2847.

Mask a creation mode with the parent directory permissions of path.

Parameters:
* `path`: file whose parent directory supplies the permissions
* `mode`: requested mode

Returns: the masked mode, limited to DEFMASK when the parent directory stat failed

Calls: [ad_mode_st](ad_open.c.md#ad_mode_st)

Called by: [afp_addappl](../../etc/afpd/appl.c.md#afp_addappl), [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [afp_rmvappl](../../etc/afpd/appl.c.md#afp_rmvappl), [applopen](../../etc/afpd/appl.c.md#applopen), [iconopen](../../etc/afpd/desktop.c.md#iconopen)

### ad_mkdir

```c
int ad_mkdir(const char *path, mode_t mode)
```

Defined at lines 2859 to 2875.

Create a directory with a mode inherited from its parent.

ad_mode_st restricts mode to the parent permissions, and ad_chown then gives the new directory the owner that the parent implies.

Parameters:
* `path`: directory to create
* `mode`: requested mode

Returns: 0, or -1 with errno from mkdir

Calls: [ad_chown](ad_open.c.md#ad_chown), [ad_mode_st](ad_open.c.md#ad_mode_st), [getcwdpath](../util/unix.c.md#getcwdpath)

Called by: [ad_mkrf](ad_open.c.md#ad_mkrf), [applopen](../../etc/afpd/appl.c.md#applopen), [iconopen](../../etc/afpd/desktop.c.md#iconopen), [netatalk_mkdir](../../etc/afpd/directory.c.md#netatalk_mkdir)

### ad_init_func

```c
static void ad_init_func(struct adouble *ad)
```

Defined at lines 2887 to 2912.

Select the fork layout of ad from ad_vers.

Version 2 shares the metadata descriptor with the resource fork, and the ea layout shares it with the data fork. Every descriptor starts closed, the reference count at 1, and the resource fork length at 0. An unknown version aborts.

Parameters:
* `ad`: AppleDouble with ad_vers set

Called by: [ad_init](ad_open.c.md#ad_init), [ad_init_old](ad_open.c.md#ad_init_old)

Uses file-scope variables: `ad_adouble`, `ad_adouble_ea`

### ad_init_old

```c
void ad_init_old(struct adouble *ad, int flags, int options)
```

Defined at lines 2924 to 2930.

Initialize an AppleDouble from explicit version and option values.

ad_init takes the same values from a volume. Unlike ad_init this function leaves the entry offset table empty until a header read fills it.

Parameters:
* `ad`: AppleDouble to initialize
* `flags`: AD_VERSION2 or AD_VERSION_EA
* `options`: ADVOL_* option bits

Calls: [ad_init_func](ad_open.c.md#ad_init_func)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx)

### ad_init

```c
void ad_init(struct adouble *ad, const struct vol *vol)
```

Defined at lines 2943 to 2954.

Initialize an AppleDouble for the layout and options of a volume.

ad_init_offsets fills the entry offset table at once, so [ad_entry](../../include/atalk/adouble.h.md#struct-ad_entry), ad_getattr, and ad_getdate work on a cache hit that never reads a header. ad_init_offsets ignores a header that already carries AD_MAGIC, so a later header read can call it again.

Parameters:
* `ad`: AppleDouble to initialize
* `vol`: volume supplying v_adouble and v_ad_options

Calls: [ad_init_func](ad_open.c.md#ad_init_func), [ad_init_offsets](ad_open.c.md#ad_init_offsets)

Called by: [RF_renamefile_adouble](../vfs/vfs.c.md#rf_renamefile_adouble), [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [adl_lkup](../../etc/afpd/catsearch.c.md#adl_lkup), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit), [copy](../../bin/nad/nad_cp.c.md#copy), [copy_source_header](../../bin/nad/nad_cp.c.md#copy_source_header), [copyfile](../../etc/afpd/file.c.md#copyfile), [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [dir_add](../../etc/afpd/directory.c.md#dir_add), [do_move](../../bin/nad/nad_mv.c.md#do_move), [ea_copyfile](../vfs/ea_ad.c.md#ea_copyfile), [ea_renamefile](../vfs/ea_ad.c.md#ea_renamefile), [getdirparams](../../etc/afpd/directory.c.md#getdirparams), [getfilparams](../../etc/afpd/file.c.md#getfilparams), [getvolparams](../../etc/afpd/volume.c.md#getvolparams), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open), [nad_set](../../bin/nad/nad_set.c.md#nad_set), [of_ad](../../etc/afpd/ofork.c.md#of_ad), [of_alloc](../../etc/afpd/ofork.c.md#of_alloc), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [print_flags](../../bin/nad/nad_ls.c.md#print_flags), [renamedir](../../etc/afpd/directory.c.md#renamedir), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

Mentioned in the documentation of: [ad_open](ad_open.c.md#ad_open)

### ad_open

```c
int ad_open(struct adouble *ad, const char *path, int adflags,...)
```

Defined at lines 3009 to 3100.

Open data-, metadata(header)- or resource fork.

```
ad_open(struct adouble *ad, const char *path, int adflags, int flags)
ad_open(struct adouble *ad, const char *path, int adflags, int flags, mode_t mode)
```

You must call [ad_init()](ad_open.c.md#ad_init) before ad_open, usually you'll just call it like this:

```
struct adoube ad;
ad_init(&ad, vol->v_adouble, vol->v_ad_options);
```

Open a files data fork, metadata fork or resource fork.

Regular adflags:

Parameters:
* `ad`: pointer to struct adouble
* `path`: Path to file or directory
* `adflags`: Flags specifying which fork to open, can be or'd (see below)
* `...`: mode used with O_CREATE

* ADFLAGS_DF: open data fork
* ADFLAGS_RF: open resource fork
* ADFLAGS_HF: open header (metadata) file
* ADFLAGS_NOHF: it's not an error if header file couldn't be opened
* ADFLAGS_NORF: it's not an error if reso fork couldn't be opened
* ADFLAGS_DIR: if path is a directory you MUST or ADFLAGS_DIR to adflags

Access mode for the forks:

* ADFLAGS_RDONLY: open read only
* ADFLAGS_RDWR: open read write

Creation flags:

* ADFLAGS_CREATE: create if not existing
* ADFLAGS_TRUNC: truncate

Special flags:

* ADFLAGS_CHECK_OF: check for open forks from us and other afpd's
* ADFLAGS_SETSHRMD: this adouble struct will be used to set sharemode locks. This basically results in the files being opened RW instead of RDONLY.

The open mode flags (rw vs ro) have to take into account all the following requirements:

* we remember open fds for files because me must avoid a single close releasing fcntl locks for other fds of the same file Bug: on Solaris (HAVE_EAFD) ADFLAGS_RF doesn't work without ADFLAGS_HF, because it checks whether ad_meta_fileno() is already opened. As a workaround pass ADFLAGS_SETSHRMD.

Returns: 0 on success, any other value indicates an error

Calls: [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf](ad_open.c.md#ad_open_hf), [ad_open_rf](ad_open.c.md#ad_open_rf), [ad_openforks](ad_lock.c.md#ad_openforks), [adflags2logstr](ad_open.c.md#adflags2logstr), [fullpathname](../util/unix.c.md#fullpathname)

Called by: [RF_renamefile_adouble](../vfs/vfs.c.md#rf_renamefile_adouble), [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_metadata](ad_open.c.md#ad_metadata), [ad_openat](ad_open.c.md#ad_openat), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [copy](../../bin/nad/nad_cp.c.md#copy), [copy_source_header](../../bin/nad/nad_cp.c.md#copy_source_header), [copyfile](../../etc/afpd/file.c.md#copyfile), [dir_add](../../etc/afpd/directory.c.md#dir_add), [do_move](../../bin/nad/nad_mv.c.md#do_move), [ea_copyfile](../vfs/ea_ad.c.md#ea_copyfile), [ea_renamefile](../vfs/ea_ad.c.md#ea_renamefile), [find_adouble](../../etc/afpd/file.c.md#find_adouble), [getvolparams](../../etc/afpd/volume.c.md#getvolparams), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open), [nad_set](../../bin/nad/nad_set.c.md#nad_set), [renamedir](../../etc/afpd/directory.c.md#renamedir), [renamefile](../../etc/afpd/file.c.md#renamefile), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

Mentioned in the documentation of: [ad_convert_osx](ad_open.c.md#ad_convert_osx)

### ad_metadata

```c
int ad_metadata(const char *name, int flags, struct adouble *adp)
```

Defined at lines 3115 to 3131.

Open only the metadata of a file or directory read-only, as root when needed.

The call keeps ADFLAGS_DIR and ADFLAGS_CHECK_OF from flags, adds ADFLAGS_HF and ADFLAGS_RDONLY, and retries as root after EACCES.

Parameters:
* `name`: file or directory
* `flags`: ADFLAGS_DIR when name is a directory, ADFLAGS_CHECK_OF to look for open forks of this or another afpd
* `adp`: initialized AppleDouble receiving the metadata

Returns: 0, or -1 with errno set

Calls: [ad_open](ad_open.c.md#ad_open), [become_root](../util/unix.c.md#become_root), [unbecome_root](../util/unix.c.md#unbecome_root)

Called by: [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached), [ad_metadataat](ad_open.c.md#ad_metadataat), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [print_flags](../../bin/nad/nad_ls.c.md#print_flags)

Mentioned in the documentation of: [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit)

### ad_metadataat

```c
int ad_metadataat(int dirfd, const char *name, int flags, struct adouble *adp)
```

Defined at lines 3148 to 3212.

ad_metadata relative to a directory descriptor

With a dirfd other than -1 the call changes into that directory around ad_metadata and restores the previous working directory afterwards. When the restore fails it anchors the process at "/" and reports the restore error only when ad_metadata itself succeeded.

Parameters:
* `dirfd`: directory that name is relative to, or -1 for the working directory
* `name`: file or directory
* `flags`: ADFLAGS_DIR and ADFLAGS_CHECK_OF, as for ad_metadata
* `adp`: initialized AppleDouble receiving the metadata

Returns: 0, or -1 with errno set

Calls: [ad_metadata](ad_open.c.md#ad_metadata)

Called by: [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit)

### ad_refresh

```c
int ad_refresh(const char *path, struct adouble *ad)
```

Defined at lines 3226 to 3286.

Reread the metadata of an open AppleDouble.

Version 2 rereads the header from the open metadata descriptor. The ea layout refreshes the resource fork length from the open resource fork, and without HAVE_EAFD rereads the sidecar header first. It then rereads the metadata EA.

Parameters:
* `path`: file, unused for version 2
* `ad`: AppleDouble with the forks to refresh open

Returns: 0, or -1 when a descriptor it needs stays closed or a read failed

Calls: [ad_header_read_osx](ad_open.c.md#ad_header_read_osx), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [afp_getforkparams](../../etc/afpd/fork.c.md#afp_getforkparams), [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams), [flushfork](../../etc/afpd/fork.c.md#flushfork), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

Calls through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_ea](ad_open.c.md#ad_header_read_ea)

### ad_openat

```c
int ad_openat(struct adouble *ad, int dirfd, const char *path, int adflags,...)
```

Defined at lines 3305 to 3384.

ad_open relative to a directory descriptor

With a dirfd other than -1 the call changes into that directory around ad_open and restores the previous working directory afterwards. When the restore fails it anchors the process at "/" and reports the restore error only when ad_open itself succeeded. ADFLAGS_CREATE takes the creation mode from the variable arguments, as ad_open does.

Parameters:
* `ad`: initialized AppleDouble
* `dirfd`: directory that path is relative to, or -1 for the working directory
* `path`: file or directory
* `adflags`: ADFLAGS_* bits, as for ad_open
* `...`: creation mode under ADFLAGS_CREATE

Returns: 0, or -1 with errno set

Calls: [ad_open](ad_open.c.md#ad_open)

Called by: [copyfile](../../etc/afpd/file.c.md#copyfile), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks)

### ad_hf_mode

```c
mode_t ad_hf_mode(mode_t mode)
```

Defined at lines 3395 to 3426.

Derive the metadata file mode from a data fork mode.

Execute bits go away. Any read permission also grants write, because fcntl locks need write access, and any write permission also grants read.

Parameters:
* `mode`: data fork mode

Returns: the metadata file mode

Called by: [RF_setdirmode_adouble](../vfs/vfs.c.md#rf_setdirmode_adouble), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [adouble_setfilmode](../vfs/vfs.c.md#adouble_setfilmode), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open)

# Types

### struct entry

Defined at line 107.
* `uint32_t id`
* `uint32_t offset`
* `uint32_t len`

# Macros

* `ADEDLEN_INIT`: initial lengths of some of the fields
* `DEFMASK`: be conservative
* `TIMEWARP_DELTA`: this is to prevent changing timezones from causing problems with localtime volumes.
* Undocumented: `ADEDOFF_AFPFILEI`, `ADEDOFF_AFPFILEI_EA`, `ADEDOFF_COMMENT_EA`, `ADEDOFF_COMMENT_V2`, `ADEDOFF_DID`, `ADEDOFF_FILEDATESI`, `ADEDOFF_FILEDATESI_EA`, `ADEDOFF_FILLER`, `ADEDOFF_FINDERI_EA`, `ADEDOFF_FINDERI_V2`, `ADEDOFF_MAGIC`, `ADEDOFF_NAME_V2`, `ADEDOFF_NENTRIES`, `ADEDOFF_PRIVDEV`, `ADEDOFF_PRIVDEV_EA`, `ADEDOFF_PRIVID`, `ADEDOFF_PRIVID_EA`, `ADEDOFF_PRIVINO`, `ADEDOFF_PRIVINO_EA`, `ADEDOFF_PRIVSYN`, `ADEDOFF_PRIVSYN_EA`, `ADEDOFF_PRODOSFILEI`, `ADEDOFF_RFORK_V2`, `ADEDOFF_SHORTNAME`, `ADEDOFF_VERSION`, `ADFLAGS2LOGSTRBUFSIZ`, `EMULATE_SUIDDIR`, `OPENFLAGS2LOGSTRBUFSIZ`

# File-scope variables

`default_uid`, `entry_order2`, `entry_order_ea`
