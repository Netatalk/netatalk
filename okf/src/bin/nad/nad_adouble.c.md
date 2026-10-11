---
type: C Source File
title: "bin/nad/nad_adouble.c"
description: "16 functions, 1 type, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_adouble.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 47 to 58.

Return the value of a lowercase hex digit of a CAP escape.

Parameters:
* `c`: character to decode

Returns: 0 to 15, or -1 for another character

Called by: [cap_decode_byte](nad_adouble.c.md#cap_decode_byte)

### cap_decode_byte

```c
static int cap_decode_byte(const unsigned char *in, unsigned char *out)
```

Defined at lines 67 to 79.

Decode the two hex digits after the colon of a CAP escape.

Parameters:
* `in`: two characters
* `out`: receives the byte

Returns: 1 for a valid pair, or 0

Calls: [cap_hexval](nad_adouble.c.md#cap_hexval)

Called by: [utompathcap](nad_adouble.c.md#utompathcap)

### cap_escape_byte

```c
static void cap_escape_byte(unsigned char c, unsigned char **out)
```

Defined at lines 87 to 92.

Write a byte as a CAP escape, a colon and two lowercase hex digits.

Parameters:
* `c`: byte to escape
* `out`: write position, advanced by three characters

Called by: [mtoupathcap](nad_adouble.c.md#mtoupathcap)

Uses file-scope variables: `hexdig`

### mtoupathcap

```c
static char * mtoupathcap(char *mpath)
```

Defined at lines 111 to 137.

Encode a Mac file name as a CAP-style host name.

Bytes that do not print, slashes, and a leading dot become `:xx` escapes. The result lives in a static buffer that the next call overwrites, and a name that does not fit ends early.

Parameters:
* `mpath`: Mac name

Returns: the host name in the static buffer

Calls: [cap_escape_byte](nad_adouble.c.md#cap_escape_byte)

Uses file-scope variables: `mtou_buf`

### utompathcap

```c
static char * utompathcap(char *upath)
```

Defined at lines 150 to 176.

Decode a CAP-style host name into a Mac file name.

Each `:xx` escape with lowercase hex digits becomes its byte, and every other character stays. The result lives in a static buffer that the next call overwrites.

Parameters:
* `upath`: host name

Returns: the Mac name in the static buffer

Calls: [cap_decode_byte](nad_adouble.c.md#cap_decode_byte)

Uses file-scope variables: `utom_buf`

### nad_set_volume

```c
void nad_set_volume(const struct nad_volume *volume)
```

Defined at lines 198 to 202.

Select the volume state for the following nad_open calls.

Parameters:
* `volume`: volume state, or NULL for none

Called by: [nad_archive_convert](megatron.c.md#nad_archive_convert), [set_nad_volume_from_path](nad_stuffit.c.md#set_nad_volume_from_path)

Uses file-scope variables: `nad`

### nad_adouble_dir

```c
static int nad_adouble_dir(char *dir, size_t dirlen)
```

Defined at lines 212 to 230.

Derive the directory part of the AppleDouble header path of the open file.

Parameters:
* `dir`: receives the directory, .AppleDouble for adouble:v2
* `dirlen`: size of `dir`

Returns: 0, or -1 with ENAMETOOLONG or EINVAL

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [nad_prepare_adouble_dir](nad_adouble.c.md#nad_prepare_adouble_dir)

Uses file-scope variables: `nad`

### nad_prepare_adouble_dir

```c
static int nad_prepare_adouble_dir(int parentfd, char *dir, size_t dirlen, mode_t mode, struct stat *st, int *created)
```

Defined at lines 248 to 287.

Open the .AppleDouble directory of a new file and create it when missing.

A new directory starts with owner rights added, so the following ad_open can create the header in it. Later, nad_set_created_modes sets the inherited mode.

Parameters:
* `parentfd`: descriptor of the current directory
* `dir`: receives the directory path
* `dirlen`: size of `dir`
* `mode`: mode of the current directory
* `st`: receives the stat result of the directory
* `created`: receives 1 when the call created the directory, or 0

Returns: the directory descriptor, or -1 with errno

Calls: [nad_adouble_dir](nad_adouble.c.md#nad_adouble_dir)

Called by: [nad_open](nad_adouble.c.md#nad_open)

### nad_set_created_modes

```c
static int nad_set_created_modes(mode_t data_mode, mode_t header_mode, int dirfd, mode_t dir_mode, int created_dir, int created_header)
```

Defined at lines 304 to 323.

Set the inherited modes on the new data file, header, and .AppleDouble directory.

The data file always gets `data_mode`. The header and the directory get their modes only when this conversion created them.

Parameters:
* `data_mode`: mode of the data file
* `header_mode`: mode of a new header
* `dirfd`: descriptor of the .AppleDouble directory
* `dir_mode`: mode of a new directory
* `created_dir`: nonzero when the conversion created the directory
* `created_header`: nonzero when the conversion created the header

Returns: 0, or -1 for a failed fchmod

Called by: [nad_open](nad_adouble.c.md#nad_open)

Uses file-scope variables: `nad`

### nad_open

```c
int nad_open(char *path, int openflags, struct FHeader *fh, int options)
```

Defined at lines 344 to 532.

Open a file and its Mac metadata as the input or the output of a conversion.

For reading, the call opens `path` with its forks and its metadata and fills `fh` through nad_header_read. For writing, it creates the file that the Mac name of `fh` names in the current directory, and an existing file fails. The new files take the mode of the current directory without its execute bits. On adouble:v2 the call also prepares the .AppleDouble directory. It then writes the header with nad_header_write.

Parameters:
* `path`: input path for reading, unused for writing
* `openflags`: O_RDONLY for reading, or the create flags for writing
* `fh`: file header to fill or to write
* `options`: OPTION_* flags, where OPTION_NORF leaves out the resource fork

Returns: 0, or -1 for an error

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_hf_mode](../../libatalk/adouble/ad_open.c.md#ad_hf_mode), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_openat](../../libatalk/adouble/ad_open.c.md#ad_openat), [nad_header_read](nad_adouble.c.md#nad_header_read), [nad_header_write](nad_adouble.c.md#nad_header_write), [nad_prepare_adouble_dir](nad_adouble.c.md#nad_prepare_adouble_dir), [nad_set_created_modes](nad_adouble.c.md#nad_set_created_modes), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer), [extract_file_entry](nad_stuffit.c.md#extract_file_entry), [from_open](megatron.c.md#from_open), [to_open](megatron.c.md#to_open)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Uses file-scope variables: `nad`

### nad_header_read

```c
int nad_header_read(struct FHeader *fh)
```

Defined at lines 545 to 683.

Fill a file header from the open file and its metadata.

The name comes from the AppleDouble name entry, or from the decoded host name when the entry is empty. Each fork length has to fit in 32 bits. The call reads the three dates and the Finder info, and a missing Finder info gives zeros. The comment stays empty.

Parameters:
* `fh`: file header to fill

Returns: 0, or -1 for an error

Calls: [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [utompath](../../etc/afpd/desktop.c.md#utompath)

Called by: [nad_open](nad_adouble.c.md#nad_open)

Uses file-scope variables: `nad`

### nad_header_write

```c
int nad_header_write(struct FHeader *fh)
```

Defined at lines 695 to 782.

Write a file header into the metadata of the new file.

The call stores the Mac name on adouble:v2, the comment, the resource fork length, and the Finder info. It also sets the creation and the modification dates and then flushes the metadata.

Parameters:
* `fh`: file header

Returns: 0, or -1 for a name or a comment that is too long

Calls: [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [nad_open](nad_adouble.c.md#nad_open)

Uses file-scope variables: `nad`

### nad_read

```c
ssize_t nad_read(int fork, char *forkbuf, size_t bufc)
```

Defined at lines 795 to 813.

Read the next bytes of a fork of the open file.

Parameters:
* `fork`: DATA or RESOURCE
* `forkbuf`: destination buffer
* `bufc`: size of `forkbuf`

Returns: the number of bytes read, 0 at the end of the fork, or -1 for an error

Calls: [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read)

Called by: [from_read](megatron.c.md#from_read), [read_fork_bytes](nad_stuffit.c.md#read_fork_bytes)

Uses file-scope variables: `forkeid`, `nad`

### nad_write

```c
ssize_t nad_write(int fork, char *forkbuf, size_t bufc)
```

Defined at lines 826 to 861.

Write bytes to a fork of the new file at its tracked offset.

The call writes until all bytes are out, and a write of zero bytes fails as a short write.

Parameters:
* `fork`: DATA or RESOURCE
* `forkbuf`: bytes to write
* `bufc`: number of bytes

Returns: `bufc`, or a negative value for an error

Calls: [ad_write](../../libatalk/adouble/ad_write.c.md#ad_write)

Called by: [to_write](megatron.c.md#to_write), [write_fork_bytes](nad_stuffit.c.md#write_fork_bytes)

Uses file-scope variables: `forkeid`, `nad`

### nad_update_cnid

```c
static int nad_update_cnid(void)
```

Defined at lines 873 to 917.

Give a new file its CNID and store the CNID in its metadata.

A file open for reading, the stand-in volume of force, and a volume without CNIDs need nothing. The call resolves the CNID of the data file path and writes it into the metadata with the parent ID and the database stamp.

Returns: 0, or -1 with errno for an error

Calls: [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset)

Called by: [nad_close](nad_adouble.c.md#nad_close)

Uses file-scope variables: `nad`

### nad_close

```c
int nad_close(int status)
```

Defined at lines 929 to 971.

Close the open file and keep or delete it.

KEEP flushes the metadata, gives a new file its CNID, and closes the forks. TRASH closes the forks and deletes the data file and its header file.

Parameters:
* `status`: KEEP or TRASH

Returns: 0, or -1 for an error or another status

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [nad_update_cnid](nad_adouble.c.md#nad_update_cnid)

Called by: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer), [extract_file_entry](nad_stuffit.c.md#extract_file_entry), [from_close](megatron.c.md#from_close), [to_close](megatron.c.md#to_close)

Uses file-scope variables: `nad`

# Types

### struct nad_file_data

Defined at line 182.
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
