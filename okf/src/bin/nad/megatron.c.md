---
type: C Source File
title: "bin/nad/megatron.c"
description: "25 functions, 1 type, includes 11 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/megatron.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [hqx.h](hqx.h.md)
* [macbin.h](macbin.h.md)
* [megatron.h](megatron.h.md)
* [nad.h](nad.h.md)
* [nad_adouble.h](nad_adouble.h.md)
* [netatalk/endian.h](../../sys/netatalk/endian.h.md)
* System headers: `errno.h`, `fcntl.h`, `pwd.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### usage_archive

```c
static void usage_archive(FILE *out)
```

Defined at lines 53 to 72.

Called by: [nad_archive](megatron.c.md#nad_archive)

### parse_archive_mode

```c
static int parse_archive_mode(const char *name)
```

Defined at lines 74 to 83.

Uses file-scope variables: `archive_modes`

### mode_writes_metadata

```c
static int mode_writes_metadata(int module)
```

Defined at lines 85 to 88.

Called by: [mode_creates_volume_file](megatron.c.md#mode_creates_volume_file), [nad_archive_convert](megatron.c.md#nad_archive_convert), [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

### mode_reads_metadata

```c
static int mode_reads_metadata(int module)
```

Defined at lines 90 to 93.

Called by: [mode_creates_volume_file](megatron.c.md#mode_creates_volume_file), [nad_archive](megatron.c.md#nad_archive), [nad_archive_convert](megatron.c.md#nad_archive_convert)

### mode_creates_volume_file

```c
static int mode_creates_volume_file(int module, int flags)
```

Defined at lines 95 to 106.

Calls: [mode_reads_metadata](megatron.c.md#mode_reads_metadata), [mode_writes_metadata](megatron.c.md#mode_writes_metadata)

Called by: [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

### from_open

```c
static int from_open(int un, char *file, struct FHeader *fh, int flags)
```

Defined at lines 108 to 124.

Calls: [bin_open](macbin.c.md#bin_open), [hqx_open](hqx.c.md#hqx_open), [nad_open](nad_adouble.c.md#nad_open)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### from_read

```c
static ssize_t from_read(int un, int fork, char *buf, size_t len)
```

Defined at lines 126 to 142.

Calls: [bin_read](macbin.c.md#bin_read), [hqx_read](hqx.c.md#hqx_read), [nad_read](nad_adouble.c.md#nad_read)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### from_close

```c
static int from_close(int un)
```

Defined at lines 144 to 160.

Calls: [bin_close](macbin.c.md#bin_close), [hqx_close](hqx.c.md#hqx_close), [nad_close](nad_adouble.c.md#nad_close)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### to_open

```c
static int to_open(int to, char *file, struct FHeader *fh, int flags)
```

Defined at lines 162 to 178.

Calls: [bin_open](macbin.c.md#bin_open), [hqx_open](hqx.c.md#hqx_open), [nad_open](nad_adouble.c.md#nad_open)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### to_write

```c
static ssize_t to_write(int to, int fork, size_t bufc)
```

Defined at lines 180 to 196.

Calls: [bin_write](macbin.c.md#bin_write), [hqx_write](hqx.c.md#hqx_write), [nad_write](nad_adouble.c.md#nad_write)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

Uses file-scope variables: `forkbuf`

### to_close

```c
static int to_close(int to, int keepflag)
```

Defined at lines 198 to 214.

Calls: [bin_close](macbin.c.md#bin_close), [hqx_close](hqx.c.md#hqx_close), [nad_close](nad_adouble.c.md#nad_close)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### created_file_path

```c
static const char * created_file_path(int module)
```

Defined at lines 216 to 228.

Calls: [bin_path](macbin.c.md#bin_path), [hqx_path](hqx.c.md#hqx_path)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### update_created_file_cnid

```c
static int update_created_file_cnid(const struct nad_volume *volume, const char *path)
```

Defined at lines 230 to 302.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### print_header_date

```c
static void print_header_date(const char *label, uint32_t ad_date, int available)
```

Defined at lines 304 to 330.

Called by: [print_header](megatron.c.md#print_header)

### print_header

```c
static void print_header(const struct FHeader *fh)
```

Defined at lines 332 to 352.

Calls: [print_header_date](megatron.c.md#print_header_date)

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert)

### nad_archive_convert

```c
static int nad_archive_convert(char *path, int module, const char *newname, int flags, const struct nad_volume *volume)
```

Defined at lines 354 to 437.

Calls: [created_file_path](megatron.c.md#created_file_path), [from_close](megatron.c.md#from_close), [from_open](megatron.c.md#from_open), [from_read](megatron.c.md#from_read), [mode_reads_metadata](megatron.c.md#mode_reads_metadata), [mode_writes_metadata](megatron.c.md#mode_writes_metadata), [nad_set_volume](nad_adouble.c.md#nad_set_volume), [print_header](megatron.c.md#print_header), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [to_close](megatron.c.md#to_close), [to_open](megatron.c.md#to_open), [to_write](megatron.c.md#to_write), [update_created_file_cnid](megatron.c.md#update_created_file_cnid)

Called by: [nad_archive](megatron.c.md#nad_archive)

Uses file-scope variables: `forkbuf`

### validate_volume_metadata

```c
static int validate_volume_metadata(const char *volpath, const struct vol *vol)
```

Defined at lines 441 to 450.

Called by: [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

### set_fallback_volume

```c
static void set_fallback_volume(struct nad_volume *volume, int flags)
```

Defined at lines 454 to 475.

Calls: [ad_path](../../libatalk/adouble/ad_open.c.md#ad_path), [ad_path_ea](../../libatalk/adouble/ad_open.c.md#ad_path_ea), [ad_path_osx](../../libatalk/adouble/ad_open.c.md#ad_path_osx)

Called by: [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

Uses file-scope variables: `fallback_volume`

### resolve_metadata_volume

```c
static int resolve_metadata_volume(AFPObj *obj, int module, const char *path, int flags, struct nad_volume *volume)
```

Defined at lines 477 to 573.

Calls: [close_metadata_volume](megatron.c.md#close_metadata_volume), [cnid_getstamp](../../libatalk/cnid/cnid.c.md#cnid_getstamp), [cnid_open](../../libatalk/cnid/cnid.c.md#cnid_open), [cnid_scheme_registered](../../libatalk/cnid/cnid.c.md#cnid_scheme_registered), [getvolbypath](../../libatalk/util/netatalk_conf.c.md#getvolbypath), [mode_creates_volume_file](megatron.c.md#mode_creates_volume_file), [mode_writes_metadata](megatron.c.md#mode_writes_metadata), [nad_not_inside_volume](nad_util.c.md#nad_not_inside_volume), [set_fallback_volume](megatron.c.md#set_fallback_volume), [validate_volume_metadata](megatron.c.md#validate_volume_metadata)

Called by: [nad_archive](megatron.c.md#nad_archive)

Uses file-scope variables: `forceflag` in [bin/nad/nad_util.c](nad_util.c.md)

### close_metadata_volume

```c
static void close_metadata_volume(struct nad_volume *volume)
```

Defined at lines 575 to 583.

Calls: [cnid_close](../../libatalk/cnid/cnid.c.md#cnid_close)

Called by: [nad_archive](megatron.c.md#nad_archive), [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

### arg_is_option_with_value

```c
static int arg_is_option_with_value(const char *arg)
```

Defined at lines 585 to 588.

Called by: [nad_archive](megatron.c.md#nad_archive)

### arg_is_flag_option

```c
static int arg_is_flag_option(const char *arg)
```

Defined at lines 590 to 596.

Called by: [nad_archive](megatron.c.md#nad_archive)

### set_newname

```c
static int set_newname(char *newname, size_t newname_len, const char *name)
```

Defined at lines 598 to 606.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [nad_archive](megatron.c.md#nad_archive)

### option_after_file

```c
static int option_after_file(const char *arg)
```

Defined at lines 608 to 613.

Called by: [nad_archive](megatron.c.md#nad_archive)

### nad_archive

```c
int nad_archive(int argc, char **argv, AFPObj *obj)
```

Defined at lines 615 to 767.

Calls: [arg_is_flag_option](megatron.c.md#arg_is_flag_option), [arg_is_option_with_value](megatron.c.md#arg_is_option_with_value), [close_metadata_volume](megatron.c.md#close_metadata_volume), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [mode_reads_metadata](megatron.c.md#mode_reads_metadata), [nad_archive_convert](megatron.c.md#nad_archive_convert), [option_after_file](megatron.c.md#option_after_file), [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume), [set_newname](megatron.c.md#set_newname), [usage_archive](megatron.c.md#usage_archive)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `nad_log_verbose` in [bin/nad/nad_util.c](nad_util.c.md)

# Types

### struct archive_mode

Defined at line 41.
* `const char * name`

# File-scope variables

`archive_modes`, `fallback_volume`, `forkbuf`, `forkname`
