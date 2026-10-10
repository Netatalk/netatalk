---
type: C Source File
title: "libatalk/adouble/ad_open.c"
description: "Part of Netatalk's AppleDouble implementatation."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_open.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
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

Defined at lines 182 to 302.

Calls: [strlcat](../compat/strlcpy.c.md#strlcat)

Called by: [ad_close](ad_flush.c.md#ad_close), [ad_flush](ad_flush.c.md#ad_flush), [ad_flush_hf](ad_flush.c.md#ad_flush_hf), [ad_flush_rf](ad_flush.c.md#ad_flush_rf), [ad_open](ad_open.c.md#ad_open), [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

### openflags2logstr

```c
const char * openflags2logstr(int oflags)
```

Defined at lines 305 to 353.

Calls: [strlcat](../compat/strlcpy.c.md#strlcat)

Called by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

### get_eid

```c
static uint32_t get_eid(uint32_t eid)
```

Defined at lines 355 to 378.

Called by: [parse_entries](ad_open.c.md#parse_entries)

### ad_init_offsets

```c
int ad_init_offsets(struct adouble *ad)
```

Defined at lines 384 to 428.

Initialize offset pointers

Called by: [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_init](ad_open.c.md#ad_init), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [new_ad_header](ad_open.c.md#new_ad_header)

Uses file-scope variables: `entry_order2`, `entry_order_ea`

Mentioned in the documentation of: [ad_rebuild_from_cache](../../etc/afpd/ad_cache.c.md#ad_rebuild_from_cache)

### new_ad_header

```c
static int new_ad_header(struct adouble *ad, const char *path, struct stat *stp, int adflags)
```

Defined at lines 431 to 481.

Calls: [ad_init_offsets](ad_open.c.md#ad_init_offsets), [ad_setattr](ad_attr.c.md#ad_setattr), [ad_setdate](ad_date.c.md#ad_setdate)

Called by: [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea)

### parse_entries

```c
static int parse_entries(struct adouble *ad, uint16_t nentries, size_t valid_data_len)
```

Defined at lines 487 to 521.

Read an AppleDouble buffer.

Returns: 0 on success, -1 if an entry was malformatted

Calls: [get_eid](ad_open.c.md#get_eid)

Called by: [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_ea](ad_open.c.md#ad_header_read_ea), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx)

### ad_header_read

```c
static int ad_header_read(const char *path, struct adouble *ad, const struct stat *hst)
```

Defined at lines 530 to 605.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [adf_pread](ad_read.c.md#adf_pread), [fullpathname](../util/unix.c.md#fullpathname), [parse_entries](ad_open.c.md#parse_entries)

Called through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_refresh](ad_open.c.md#ad_refresh)

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

### ad_valid_header_osx

```c
int ad_valid_header_osx(const char *path)
```

Defined at lines 608 to 661.

Calls: [fullpathname](../util/unix.c.md#fullpathname)

Called by: [nad_rm](../../bin/nad/nad_rm.c.md#nad_rm), [rm](../../bin/nad/nad_rm.c.md#rm), [validupath_ea](../vfs/vfs.c.md#validupath_ea)

### ad_convert_pread_full

```c
static int ad_convert_pread_full(struct ad_fd *adf, void *buf, size_t count, off_t offset)
```

Defined at lines 675 to 701.

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

Defined at lines 715 to 741.

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

Defined at lines 756 to 775.

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

Defined at lines 790 to 962.

Convert from Apple's ._ file to Netatalk.

Apple's AppleDouble may contain a FinderInfo entry longer then 32 bytes containing packed xattrs. Netatalk can't deal with that, so we simply discard the packed xattrs.

As we call [ad_open()](ad_open.c.md#ad_open) which might result in a recursion, just to be sure use static variable in_conversion to check for that.

Returns: -1 in case an error occured, 0 if no conversion was done, 1 otherwise

Calls: [ad_close](ad_flush.c.md#ad_close), [ad_convert_move_rfork](ad_open.c.md#ad_convert_move_rfork), [ad_convert_pwrite_full](ad_open.c.md#ad_convert_pwrite_full), [ad_flush](ad_flush.c.md#ad_flush), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_init_offsets](ad_open.c.md#ad_init_offsets), [ad_init_old](ad_open.c.md#ad_init_old), [ad_open](ad_open.c.md#ad_open), [ad_rebuild_adouble_header_osx](ad_flush.c.md#ad_rebuild_adouble_header_osx), [fullpathname](../util/unix.c.md#fullpathname)

Called by: [ad_header_read_osx](ad_open.c.md#ad_header_read_osx)

### ad_header_read_osx

```c
static int ad_header_read_osx(const char *path, struct adouble *ad, struct stat *hst)
```

Defined at lines 965 to 1066.

Read an ._ file, only uses the resofork, finderinfo is taken from EA

Calls: [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_init_old](ad_open.c.md#ad_init_old), [adf_pread](ad_read.c.md#adf_pread), [fullpathname](../util/unix.c.md#fullpathname), [parse_entries](ad_open.c.md#parse_entries)

Called by: [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [ad_refresh](ad_open.c.md#ad_refresh)

### ad_header_read_ea

```c
static int ad_header_read_ea(const char *path, struct adouble *ad, const struct stat *hst)
```

Defined at lines 1068 to 1169.

Calls: [become_root](../util/unix.c.md#become_root), [fullpathname](../util/unix.c.md#fullpathname), [parse_entries](ad_open.c.md#parse_entries), [sys_fgetxattr](../vfs/extattr.c.md#sys_fgetxattr), [sys_getxattr](../vfs/extattr.c.md#sys_getxattr), [sys_removexattr](../vfs/extattr.c.md#sys_removexattr), [unbecome_root](../util/unix.c.md#unbecome_root)

Called through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_refresh](ad_open.c.md#ad_refresh)

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_mkrf

```c
static int ad_mkrf(const char *path)
```

Defined at lines 1179 to 1199.

Takes a path to an AppleDouble file and creates the parent .AppleDouble directory.

Example:

path: "/path/.AppleDouble/file" => mkdir("/path/.AppleDouble/") (in [ad_mkdir()](ad_open.c.md#ad_mkdir))

Calls: [ad_mkdir](ad_open.c.md#ad_mkdir)

Called through [`adouble_fops::ad_mkrf`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

### ad_mkrf_osx

```c
static int ad_mkrf_osx(const char *path)
```

Defined at lines 1209 to 1212.

Called through [`adouble_fops::ad_mkrf`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_chown

```c
static int ad_chown(const char *path, struct stat *stbuf)
```

Defined at lines 1224 to 1238.

if we are root change path user/ group

use fstat and fchown or lchown with linux?

Note: It can be a native function for BSD cf. FAQ.Q10

Parameters:
* `path`: pathname to chown
* `stbuf`: parent directory inode

Called by: [ad_mkdir](ad_open.c.md#ad_mkdir), [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

Uses file-scope variables: `default_uid`

### ad_mode_st

```c
static int ad_mode_st(const char *path, mode_t *mode, struct stat *stbuf)
```

Defined at lines 1245 to 1258.

return access right and inode of path parent directory

Calls: [ad_stat](ad_open.c.md#ad_stat)

Called by: [ad_mkdir](ad_open.c.md#ad_mkdir), [ad_mode](ad_open.c.md#ad_mode), [ad_open_df](ad_open.c.md#ad_open_df), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

### ad_header_upgrade

```c
static int ad_header_upgrade(struct adouble *ad, const char *name)
```

Defined at lines 1261 to 1264.

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

### ad_header_upgrade_ea

```c
static int ad_header_upgrade_ea(struct adouble *ad, const char *name)
```

Defined at lines 1266 to 1270.

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_error

```c
static int ad_error(struct adouble *ad, int adflags)
```

Defined at lines 1281 to 1304.

Error handling for adouble header(=metadata) file open error.

We're called because opening ADFLAGS_HF caused an error.

1. In case ad_open is called with ADFLAGS_NOHF the error is suppressed.
1. Open non-existent resource fork, this will just result in first read return EOF
1. If ad_open was called with ADFLAGS_DF we may have opened the datafork and thus ought to close it before returning with an error condition.

Calls: [ad_close](ad_flush.c.md#ad_close)

Called by: [ad_open_hf](ad_open.c.md#ad_open_hf)

### ad2openflags

```c
static int ad2openflags(const struct adouble *ad, int adfile, int adflags)
```

Defined at lines 1314 to 1366.

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

Defined at lines 1391 to 1511.

Calls: [ad2openflags](ad_open.c.md#ad2openflags), [ad_chown](ad_open.c.md#ad_chown), [ad_mode_st](ad_open.c.md#ad_mode_st), [adf_lock_init](ad_lock.c.md#adf_lock_init), [adflags2logstr](ad_open.c.md#adflags2logstr), [fullpathname](../util/unix.c.md#fullpathname)

Called by: [ad_open](ad_open.c.md#ad_open)

### ad_open_hf_v2

```c
static int ad_open_hf_v2(const char *path, int adflags, mode_t mode, struct adouble *ad)
```

Defined at lines 1513 to 1672.

Calls: [ad2openflags](ad_open.c.md#ad2openflags), [ad_chown](ad_open.c.md#ad_chown), [ad_flush](ad_flush.c.md#ad_flush), [ad_hf_mode](ad_open.c.md#ad_hf_mode), [ad_mode_st](ad_open.c.md#ad_mode_st), [ad_refresh](ad_open.c.md#ad_refresh), [adf_lock_init](ad_lock.c.md#adf_lock_init), [adflags2logstr](ad_open.c.md#adflags2logstr), [fullpathname](../util/unix.c.md#fullpathname), [new_ad_header](ad_open.c.md#new_ad_header), [openflags2logstr](ad_open.c.md#openflags2logstr)

Called by: [ad_open_hf](ad_open.c.md#ad_open_hf)

Calls through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_ea](ad_open.c.md#ad_header_read_ea)

Calls through [`adouble_fops::ad_mkrf`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_mkrf](ad_open.c.md#ad_mkrf), [ad_mkrf_osx](ad_open.c.md#ad_mkrf_osx)

Calls through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_path](ad_open.c.md#ad_path), [ad_path_osx](ad_open.c.md#ad_path_osx)

### ad_open_hf_ea

```c
static int ad_open_hf_ea(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 1674 to 1799.

Calls: [ad2openflags](ad_open.c.md#ad2openflags), [ad_flush](ad_flush.c.md#ad_flush), [ad_reso_size](ad_open.c.md#ad_reso_size), [adflags2logstr](ad_open.c.md#adflags2logstr), [fullpathname](../util/unix.c.md#fullpathname), [new_ad_header](ad_open.c.md#new_ad_header)

Called by: [ad_open_hf](ad_open.c.md#ad_open_hf)

Calls through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_ea](ad_open.c.md#ad_header_read_ea)

### ad_open_hf

```c
static int ad_open_hf(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 1801 to 1827.

Calls: [ad_error](ad_open.c.md#ad_error), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2)

Called by: [ad_open](ad_open.c.md#ad_open)

### ad_reso_size

```c
off_t ad_reso_size(const char *path, int adflags, struct adouble *ad)
```

Defined at lines 1832 to 1875.

Get resofork length for adouble:ea, parameter 'ad' may be NULL

Calls: [ad_path_osx](ad_open.c.md#ad_path_osx), [sys_fgetxattr](../vfs/extattr.c.md#sys_fgetxattr), [sys_lgetxattr](../vfs/extattr.c.md#sys_lgetxattr)

Called by: [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [getmetadata](../../etc/afpd/file.c.md#getmetadata)

### ad_open_rf_v2

```c
static int ad_open_rf_v2(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 1877 to 1899.

Calls: [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [fullpathname](../util/unix.c.md#fullpathname)

Called by: [ad_open_rf](ad_open.c.md#ad_open_rf)

### ad_open_rf_ea

```c
static int ad_open_rf_ea(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 1901 to 2085.

Calls: [ad2openflags](ad_open.c.md#ad2openflags), [ad_close](ad_flush.c.md#ad_close), [ad_flush](ad_flush.c.md#ad_flush), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx), [ad_reso_size](ad_open.c.md#ad_reso_size), [fullpathname](../util/unix.c.md#fullpathname), [new_ad_header](ad_open.c.md#new_ad_header), [sys_getxattrfd](../vfs/extattr.c.md#sys_getxattrfd)

Called by: [ad_open_rf](ad_open.c.md#ad_open_rf)

Calls through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_path](ad_open.c.md#ad_path), [ad_path_osx](ad_open.c.md#ad_path_osx)

### ad_open_rf

```c
static int ad_open_rf(const char *path, int adflags, int mode, struct adouble *ad)
```

Defined at lines 2090 to 2110.

Open resource fork.

Calls: [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [ad_open_rf_v2](ad_open.c.md#ad_open_rf_v2)

Called by: [ad_open](ad_open.c.md#ad_open)

### ad_entry_check_size

```c
static bool ad_entry_check_size(uint32_t eid, size_t bufsize, uint32_t off, uint32_t got_len)
```

Defined at lines 2124 to 2221.

Called by: [ad_entry_fits](ad_open.c.md#ad_entry_fits)

### ad_entry_fits

```c
bool ad_entry_fits(const struct adouble *ad, int eid, uint32_t len)
```

Defined at lines 2229 to 2234.

Whether an entry of `len` bytes fits at eid's offset in ad.

[ad_entry()](../../include/atalk/adouble.h.md#struct-ad_entry)'s bounds check against a caller-supplied length, so a destination can be checked before its length is committed.

Calls: [ad_entry_check_size](ad_open.c.md#ad_entry_check_size), [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_copy_header](ad_flush.c.md#ad_copy_header), [ad_entry](ad_open.c.md#ad_entry)

### ad_entry

```c
void * ad_entry(const struct adouble *ad, int eid)
```

Defined at lines 2236 to 2249.

Calls: [ad_entry_fits](ad_open.c.md#ad_entry_fits), [ad_getentryoff](ad_open.c.md#ad_getentryoff)

### ad_getentryoff

```c
off_t ad_getentryoff(const struct adouble *ad, int eid)
```

Defined at lines 2251 to 2274.

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_copy_header](ad_flush.c.md#ad_copy_header), [ad_entry](ad_open.c.md#ad_entry), [ad_entry_fits](ad_open.c.md#ad_entry_fits), [ad_flush_hf](ad_flush.c.md#ad_flush_hf), [ad_flush_rf](ad_flush.c.md#ad_flush_rf), [ad_fork_fileno](ad_open.c.md#ad_fork_fileno), [ad_getattr](ad_attr.c.md#ad_getattr), [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_getdate](ad_date.c.md#ad_getdate), [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx), [ad_lock](ad_lock.c.md#ad_lock), [ad_read](ad_read.c.md#ad_read), [ad_rebuild_adouble_header_v2](ad_flush.c.md#ad_rebuild_adouble_header_v2), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [ad_setattr](ad_attr.c.md#ad_setattr), [ad_setdate](ad_date.c.md#ad_setdate), [ad_setname](ad_attr.c.md#ad_setname), [ad_testlock_range](ad_lock.c.md#ad_testlock_range), [ad_tmplock](ad_lock.c.md#ad_tmplock), [ad_write](ad_write.c.md#ad_write), [copy_fork](ad_write.c.md#copy_fork), [getvolparams](../../etc/afpd/volume.c.md#getvolparams)

Mentioned in the documentation of: [ad_testlock_range](ad_lock.c.md#ad_testlock_range)

### ad_fork_fileno

```c
int ad_fork_fileno(const struct adouble *ad, int eid, off_t *off)
```

Defined at lines 2285 to 2293.

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

Defined at lines 2295 to 2298.

Called by: [init_null_vol](../../bin/nad/nad_util.c.md#init_null_vol), [initvol_vfs](../vfs/vfs.c.md#initvol_vfs), [set_fallback_volume](../../bin/nad/megatron.c.md#set_fallback_volume)

### ad_path_osx

```c
const char * ad_path_osx(const char *path, int adflags)
```

Defined at lines 2300 to 2340.

Calls: [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy), [strnlen](../compat/misc.c.md#strnlen)

Called by: [ad_reso_size](ad_open.c.md#ad_reso_size), [init_null_vol](../../bin/nad/nad_util.c.md#init_null_vol), [initvol_vfs](../vfs/vfs.c.md#initvol_vfs), [set_fallback_volume](../../bin/nad/megatron.c.md#set_fallback_volume)

Called through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_path

```c
const char * ad_path(const char *path, int adflags)
```

Defined at lines 2351 to 2401.

Put the .AppleDouble where it needs to be:

```
    /   a/.AppleDouble/b
a/b
    \   b/.AppleDouble/.Parent
```

Bug: should do something for pathname > MAXPATHLEN

Calls: [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [ad_conv_v22ea](ad_conv.c.md#ad_conv_v22ea), [initvol_vfs](../vfs/vfs.c.md#initvol_vfs), [set_fallback_volume](../../bin/nad/megatron.c.md#set_fallback_volume)

Called through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

### ad_dir

```c
char * ad_dir(const char *path)
```

Defined at lines 2408 to 2464.

Support inherited protection modes for AppleDouble files. The supplied mode is ANDed with the parent directory's mask value in lieu of "umask", and that value is returned.

Called by: [RF_setdirmode_adouble](../vfs/vfs.c.md#rf_setdirmode_adouble), [RF_setdirunixmode_adouble](../vfs/vfs.c.md#rf_setdirunixmode_adouble), [ad_stat](ad_open.c.md#ad_stat), [check_access](../../etc/afpd/directory.c.md#check_access)

### ad_setfuid

```c
int ad_setfuid(const uid_t id)
```

Defined at lines 2466 to 2470.

Called by: [login](../../etc/afpd/auth.c.md#login), [main](../../bin/dbd/cmd_dbd.c.md#main)

Uses file-scope variables: `default_uid`

### ad_getfuid

```c
uid_t ad_getfuid(void)
```

Defined at lines 2473 to 2476.

Uses file-scope variables: `default_uid`

### ad_stat

```c
int ad_stat(const char *path, struct stat *stbuf)
```

Defined at lines 2481 to 2491.

stat path parent directory

Calls: [ad_dir](ad_open.c.md#ad_dir)

Called by: [ad_mode_st](ad_open.c.md#ad_mode_st)

### ad_mode

```c
int ad_mode(const char *path, mode_t mode)
```

Defined at lines 2496 to 2501.

return access right of path parent directory

Calls: [ad_mode_st](ad_open.c.md#ad_mode_st)

Called by: [afp_addappl](../../etc/afpd/appl.c.md#afp_addappl), [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [afp_rmvappl](../../etc/afpd/appl.c.md#afp_rmvappl), [applopen](../../etc/afpd/appl.c.md#applopen), [iconopen](../../etc/afpd/desktop.c.md#iconopen)

Mentioned in the documentation of: [ad_mkdir](ad_open.c.md#ad_mkdir)

### ad_mkdir

```c
int ad_mkdir(const char *path, mode_t mode)
```

Defined at lines 2506 to 2522.

Use mkdir() with mode bits taken from [ad_mode()](ad_open.c.md#ad_mode).

Calls: [ad_chown](ad_open.c.md#ad_chown), [ad_mode_st](ad_open.c.md#ad_mode_st), [getcwdpath](../util/unix.c.md#getcwdpath)

Called by: [ad_mkrf](ad_open.c.md#ad_mkrf), [applopen](../../etc/afpd/appl.c.md#applopen), [iconopen](../../etc/afpd/desktop.c.md#iconopen), [netatalk_mkdir](../../etc/afpd/directory.c.md#netatalk_mkdir)

Mentioned in the documentation of: [ad_mkrf](ad_open.c.md#ad_mkrf)

### ad_init_func

```c
static void ad_init_func(struct adouble *ad)
```

Defined at lines 2524 to 2549.

Called by: [ad_init](ad_open.c.md#ad_init), [ad_init_old](ad_open.c.md#ad_init_old)

Uses file-scope variables: `ad_adouble`, `ad_adouble_ea`

### ad_init_old

```c
void ad_init_old(struct adouble *ad, int flags, int options)
```

Defined at lines 2551 to 2557.

Calls: [ad_init_func](ad_open.c.md#ad_init_func)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_header_read_osx](ad_open.c.md#ad_header_read_osx)

### ad_init

```c
void ad_init(struct adouble *ad, const struct vol *vol)
```

Defined at lines 2559 to 2570.

Calls: [ad_init_func](ad_open.c.md#ad_init_func), [ad_init_offsets](ad_open.c.md#ad_init_offsets)

Called by: [RF_renamefile_adouble](../vfs/vfs.c.md#rf_renamefile_adouble), [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [adl_lkup](../../etc/afpd/catsearch.c.md#adl_lkup), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit), [copy](../../bin/nad/nad_cp.c.md#copy), [copy_source_header](../../bin/nad/nad_cp.c.md#copy_source_header), [copyfile](../../etc/afpd/file.c.md#copyfile), [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [dir_add](../../etc/afpd/directory.c.md#dir_add), [do_move](../../bin/nad/nad_mv.c.md#do_move), [ea_copyfile](../vfs/ea_ad.c.md#ea_copyfile), [ea_renamefile](../vfs/ea_ad.c.md#ea_renamefile), [getdirparams](../../etc/afpd/directory.c.md#getdirparams), [getfilparams](../../etc/afpd/file.c.md#getfilparams), [getvolparams](../../etc/afpd/volume.c.md#getvolparams), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open), [nad_set](../../bin/nad/nad_set.c.md#nad_set), [of_ad](../../etc/afpd/ofork.c.md#of_ad), [of_alloc](../../etc/afpd/ofork.c.md#of_alloc), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [print_flags](../../bin/nad/nad_ls.c.md#print_flags), [renamedir](../../etc/afpd/directory.c.md#renamedir), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

Mentioned in the documentation of: [ad_open](ad_open.c.md#ad_open), [ad_rebuild_from_cache](../../etc/afpd/ad_cache.c.md#ad_rebuild_from_cache)

### ad_open

```c
int ad_open(struct adouble *ad, const char *path, int adflags,...)
```

Defined at lines 2625 to 2716.

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

Mentioned in the documentation of: [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached)

### ad_metadata

```c
int ad_metadata(const char *name, int flags, struct adouble *adp)
```

Defined at lines 2729 to 2745.

open metadata, possibly as root

Return only metadata but try very hard i.e. at first try as user, then try as root.

Parameters:
* `name`: name of file/dir
* `flags`: ADFLAGS_DIR: name is a directory ADFLAGS_CHECK_OF: test if name is open by us or another afpd process
* `adp`: pointer to struct adouble

Calls: [ad_open](ad_open.c.md#ad_open), [become_root](../util/unix.c.md#become_root), [unbecome_root](../util/unix.c.md#unbecome_root)

Called by: [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached), [ad_metadataat](ad_open.c.md#ad_metadataat), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [print_flags](../../bin/nad/nad_ls.c.md#print_flags)

Mentioned in the documentation of: [ad_rlen_meta_absent](../../include/atalk/directory.h.md#ad_rlen_meta_absent), [ad_store_to_cache](../../etc/afpd/ad_cache.c.md#ad_store_to_cache), [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit)

### ad_metadataat

```c
int ad_metadataat(int dirfd, const char *name, int flags, struct adouble *adp)
```

Defined at lines 2750 to 2814.

openat like wrapper for ad_metadata

Calls: [ad_metadata](ad_open.c.md#ad_metadata)

Called by: [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit)

### ad_refresh

```c
int ad_refresh(const char *path, struct adouble *ad)
```

Defined at lines 2816 to 2876.

Calls: [ad_header_read_osx](ad_open.c.md#ad_header_read_osx), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [afp_getforkparams](../../etc/afpd/fork.c.md#afp_getforkparams), [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams), [flushfork](../../etc/afpd/fork.c.md#flushfork), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

Calls through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_ea](ad_open.c.md#ad_header_read_ea)

### ad_openat

```c
int ad_openat(struct adouble *ad, int dirfd, const char *path, int adflags,...)
```

Defined at lines 2878 to 2957.

Calls: [ad_open](ad_open.c.md#ad_open)

Called by: [copyfile](../../etc/afpd/file.c.md#copyfile), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks)

### ad_hf_mode

```c
mode_t ad_hf_mode(mode_t mode)
```

Defined at lines 2963 to 2994.

build a resource fork mode from the data fork mode: remove X mode and extend header to RW if R or W (W if R for locking),

Called by: [RF_setdirmode_adouble](../vfs/vfs.c.md#rf_setdirmode_adouble), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [adouble_setfilmode](../vfs/vfs.c.md#adouble_setfilmode), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open)

# Types

### struct entry

Defined at line 107.
* `uint32_t id`
* `uint32_t offset`
* `uint32_t len`

# Macros

* Undocumented: `ADEDLEN_INIT`, `ADEDOFF_AFPFILEI`, `ADEDOFF_AFPFILEI_EA`, `ADEDOFF_COMMENT_EA`, `ADEDOFF_COMMENT_V2`, `ADEDOFF_DID`, `ADEDOFF_FILEDATESI`, `ADEDOFF_FILEDATESI_EA`, `ADEDOFF_FILLER`, `ADEDOFF_FINDERI_EA`, `ADEDOFF_FINDERI_V2`, `ADEDOFF_MAGIC`, `ADEDOFF_NAME_V2`, `ADEDOFF_NENTRIES`, `ADEDOFF_PRIVDEV`, `ADEDOFF_PRIVDEV_EA`, `ADEDOFF_PRIVID`, `ADEDOFF_PRIVID_EA`, `ADEDOFF_PRIVINO`, `ADEDOFF_PRIVINO_EA`, `ADEDOFF_PRIVSYN`, `ADEDOFF_PRIVSYN_EA`, `ADEDOFF_PRODOSFILEI`, `ADEDOFF_RFORK_V2`, `ADEDOFF_SHORTNAME`, `ADEDOFF_VERSION`, `ADFLAGS2LOGSTRBUFSIZ`, `DEFMASK`, `EMULATE_SUIDDIR`, `OPENFLAGS2LOGSTRBUFSIZ`, `TIMEWARP_DELTA`

# File-scope variables

`default_uid`, `entry_order2`, `entry_order_ea`
