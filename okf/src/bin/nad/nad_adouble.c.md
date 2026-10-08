---
type: C Source File
title: "bin/nad/nad_adouble.c"
description: "16 functions, 1 type, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_adouble.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [megatron.h](megatron.h.md)
* [nad.h](nad.h.md)
* [nad_adouble.h](nad_adouble.h.md)
* [netatalk/endian.h](../../sys/netatalk/endian.h.md)
* System headers: `ctype.h`, `dirent.h`, `errno.h`, `stdint.h`, `stdio.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`, `unistd.h`

# Functions

### cap_hexval

```c
static int cap_hexval(unsigned char c)
```

Defined at lines 41 to 52.

Called by: [cap_decode_byte](nad_adouble.c.md#cap_decode_byte)

### cap_decode_byte

```c
static int cap_decode_byte(const unsigned char *in, unsigned char *out)
```

Defined at lines 54 to 66.

Calls: [cap_hexval](nad_adouble.c.md#cap_hexval)

Called by: [utompathcap](nad_adouble.c.md#utompathcap)

### cap_escape_byte

```c
static void cap_escape_byte(unsigned char c, unsigned char **out)
```

Defined at lines 68 to 73.

Called by: [mtoupathcap](nad_adouble.c.md#mtoupathcap)

Uses file-scope variables: `hexdig`

### mtoupathcap

```c
static char * mtoupathcap(char *mpath)
```

Defined at lines 82 to 108.

Calls: [cap_escape_byte](nad_adouble.c.md#cap_escape_byte)

Uses file-scope variables: `mtou_buf`

### utompathcap

```c
static char * utompathcap(char *upath)
```

Defined at lines 111 to 137.

Calls: [cap_decode_byte](nad_adouble.c.md#cap_decode_byte)

Uses file-scope variables: `utom_buf`

### nad_set_volume

```c
void nad_set_volume(const struct nad_volume *volume)
```

Defined at lines 154 to 158.

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert), [set_nad_volume_from_path](nad_stuffit.c.md#set_nad_volume_from_path)

Uses file-scope variables: `nad`

### nad_adouble_dir

```c
static int nad_adouble_dir(char *dir, size_t dirlen)
```

Defined at lines 160 to 178.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [nad_prepare_adouble_dir](nad_adouble.c.md#nad_prepare_adouble_dir)

Uses file-scope variables: `nad`

### nad_prepare_adouble_dir

```c
static int nad_prepare_adouble_dir(int parentfd, char *dir, size_t dirlen, mode_t mode, struct stat *st, int *created)
```

Defined at lines 180 to 219.

Calls: [nad_adouble_dir](nad_adouble.c.md#nad_adouble_dir)

Called by: [nad_open](nad_adouble.c.md#nad_open)

### nad_set_created_modes

```c
static int nad_set_created_modes(mode_t data_mode, mode_t header_mode, int dirfd, mode_t dir_mode, int created_dir, int created_header)
```

Defined at lines 221 to 240.

Called by: [nad_open](nad_adouble.c.md#nad_open)

Uses file-scope variables: `nad`

### nad_open

```c
int nad_open(char *path, int openflags, struct FHeader *fh, int options)
```

Defined at lines 242 to 430.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_hf_mode](../../libatalk/adouble/ad_open.c.md#ad_hf_mode), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_openat](../../libatalk/adouble/ad_open.c.md#ad_openat), [nad_header_read](nad_adouble.c.md#nad_header_read), [nad_header_write](nad_adouble.c.md#nad_header_write), [nad_prepare_adouble_dir](nad_adouble.c.md#nad_prepare_adouble_dir), [nad_set_created_modes](nad_adouble.c.md#nad_set_created_modes), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer), [extract_file_entry](nad_stuffit.c.md#extract_file_entry), [from_open](megatron.c.md#from_open), [to_open](megatron.c.md#to_open)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Uses file-scope variables: `nad`

### nad_header_read

```c
int nad_header_read(struct FHeader *fh)
```

Defined at lines 432 to 570.

Calls: [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [utompath](../../etc/afpd/desktop.c.md#utompath)

Called by: [nad_open](nad_adouble.c.md#nad_open)

Uses file-scope variables: `nad`

### nad_header_write

```c
int nad_header_write(struct FHeader *fh)
```

Defined at lines 572 to 659.

Calls: [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [nad_open](nad_adouble.c.md#nad_open)

Uses file-scope variables: `nad`

### nad_read

```c
ssize_t nad_read(int fork, char *forkbuf, size_t bufc)
```

Defined at lines 663 to 681.

Calls: [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read)

Called by: [from_read](megatron.c.md#from_read), [read_fork_bytes](nad_stuffit.c.md#read_fork_bytes)

Uses file-scope variables: `forkeid`, `nad`

### nad_write

```c
ssize_t nad_write(int fork, char *forkbuf, size_t bufc)
```

Defined at lines 683 to 718.

Calls: [ad_write](../../libatalk/adouble/ad_write.c.md#ad_write)

Called by: [to_write](megatron.c.md#to_write), [write_fork_bytes](nad_stuffit.c.md#write_fork_bytes)

Uses file-scope variables: `forkeid`, `nad`

### nad_update_cnid

```c
static int nad_update_cnid(void)
```

Defined at lines 720 to 764.

Calls: [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset)

Called by: [nad_close](nad_adouble.c.md#nad_close)

Uses file-scope variables: `nad`

### nad_close

```c
int nad_close(int status)
```

Defined at lines 766 to 808.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [nad_update_cnid](nad_adouble.c.md#nad_update_cnid)

Called by: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer), [extract_file_entry](nad_stuffit.c.md#extract_file_entry), [from_close](megatron.c.md#from_close), [to_close](megatron.c.md#to_close)

Uses file-scope variables: `nad`

# Types

### struct nad_file_data

Defined at line 143.
* `char macname`
* `char adpath`
* `int offset`
* `int closeflags`
* `int write_metadata`
* `const struct nad_volume * volume`
* `struct vol * vol`
* `struct adouble ad`

# File-scope variables

`_mtoupath`, `_utompath`, `forkeid`, `hexdig`, `mtou_buf`, `nad`, `utom_buf`
