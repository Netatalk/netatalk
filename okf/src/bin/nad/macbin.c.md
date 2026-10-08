---
type: C Source File
title: "bin/nad/macbin.c"
description: "18 functions, 1 type, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/macbin.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [crc16.h](crc16.h.md)
* [macbin.h](macbin.h.md)
* [megatron.h](megatron.h.md)
* [nad.h](nad.h.md)
* [netatalk/endian.h](../../sys/netatalk/endian.h.md)
* System headers: `ctype.h`, `stdio.h`, `string.h`, `strings.h`, `sys/param.h`, `sys/types.h`, `sys/uio.h`, `unistd.h`

# Functions

### macbin_u16_from_bytes

```c
static uint16_t macbin_u16_from_bytes(unsigned char high, unsigned char low)
```

Defined at lines 71 to 75.

Called by: [macbin_fdflags_from_header](macbin.c.md#macbin_fdflags_from_header)

### macbin_u32_from_header

```c
static uint32_t macbin_u32_from_header(const unsigned char *header, size_t offset)
```

Defined at lines 77 to 84.

Called by: [test_header](macbin.c.md#test_header)

### macbin_fdflags_from_header

```c
static uint16_t macbin_fdflags_from_header(const unsigned char *header, int revision)
```

Defined at lines 86 to 104.

Calls: [macbin_u16_from_bytes](macbin.c.md#macbin_u16_from_bytes)

Called by: [bin_header_read](macbin.c.md#bin_header_read)

### macbin_fdflags_to_header

```c
static void macbin_fdflags_to_header(unsigned char *header, uint16_t fdflags)
```

Defined at lines 106 to 114.

Called by: [bin_header_write](macbin.c.md#bin_header_write)

### macbin_ad_date_from_header

```c
static int macbin_ad_date_from_header(uint32_t mac_date, uint32_t *ad_date)
```

Defined at lines 116 to 120.

Calls: [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset)

Called by: [bin_header_read](macbin.c.md#bin_header_read)

### macbin_ad_date_to_header

```c
static int macbin_ad_date_to_header(uint32_t ad_date, uint32_t *mac_date)
```

Defined at lines 122 to 130.

Calls: [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset)

Called by: [bin_header_write](macbin.c.md#bin_header_write)

### macbin_discard_bytes

```c
static int macbin_discard_bytes(size_t length)
```

Defined at lines 132 to 156.

Called by: [macbin_skip_padding](macbin.c.md#macbin_skip_padding)

Uses file-scope variables: `bin`

### macbin_write_repeat

```c
static int macbin_write_repeat(unsigned char value, size_t length)
```

Defined at lines 158 to 182.

Called by: [macbin_write_padding](macbin.c.md#macbin_write_padding)

Uses file-scope variables: `bin`

### macbin_skip_padding

```c
static int macbin_skip_padding(void)
```

Defined at lines 184 to 228.

Calls: [macbin_discard_bytes](macbin.c.md#macbin_discard_bytes)

Called by: [bin_read](macbin.c.md#bin_read)

Uses file-scope variables: `bin`

### macbin_write_padding

```c
static int macbin_write_padding(void)
```

Defined at lines 230 to 263.

Calls: [macbin_write_repeat](macbin.c.md#macbin_write_repeat)

Called by: [bin_write](macbin.c.md#bin_write)

Uses file-scope variables: `bin`

### bin_open

```c
int bin_open(char *binfile, int flags, struct FHeader *fh, int options)
```

Defined at lines 279 to 358.

Open a MacBinary file for reading or writing.

This must be called before other MacBinary operations. When opening for reading, it validates and reads the MacBinary header. When opening for writing, it initializes output state and writes the header.

Parameters:
* `binfile`: input file path, or `-` for standard input
* `flags`: open flags; `O_RDONLY` selects read mode
* `fh`: file header to read or write
* `options`: output options, such as `OPTION_STDOUT`

Returns: 0 on success, -1 on error

Calls: [bin_close](macbin.c.md#bin_close), [bin_header_read](macbin.c.md#bin_header_read), [bin_header_write](macbin.c.md#bin_header_write), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [test_header](macbin.c.md#test_header)

Called by: [from_open](megatron.c.md#from_open), [to_open](megatron.c.md#to_open)

Uses file-scope variables: `bin`

Mentioned in the documentation of: [bin_close](macbin.c.md#bin_close), [bin_header_read](macbin.c.md#bin_header_read), [bin_header_write](macbin.c.md#bin_header_write)

### bin_close

```c
int bin_close(int keepflag)
```

Defined at lines 371 to 404.

Close the active MacBinary file.

This must be called before opening another file with [bin_open()](macbin.c.md#bin_open). `KEEP` closes the file and preserves it. `TRASH` closes the file and removes incomplete output.

Parameters:
* `keepflag`: `KEEP` to keep the output, `TRASH` to discard it

Returns: 0 on success, -1 on error

Called by: [bin_open](macbin.c.md#bin_open), [from_close](megatron.c.md#from_close), [to_close](megatron.c.md#to_close)

Uses file-scope variables: `bin`

### bin_path

```c
const char * bin_path(void)
```

Defined at lines 406 to 409.

Called by: [created_file_path](megatron.c.md#created_file_path)

Uses file-scope variables: `bin`

### bin_read

```c
ssize_t bin_read(int fork, char *buffer, size_t length)
```

Defined at lines 426 to 483.

Read data from a MacBinary fork.

Call this until it returns zero for each fork. When the data fork is complete, it skips MacBinary padding before the resource fork begins.

Note: [bin_read()](macbin.c.md#bin_read) must be called enough times to return zero, and no more than that, for each fork.

Parameters:
* `fork`: fork selector, `DATA` or `RESOURCE`
* `buffer`: destination buffer
* `length`: maximum number of bytes to read

Returns: number of bytes read, 0 when the fork is complete, -1 on error

Calls: [macbin_skip_padding](macbin.c.md#macbin_skip_padding)

Called by: [from_read](megatron.c.md#from_read)

Uses file-scope variables: `bin`, `forkname` in [bin/nad/megatron.c](megatron.c.md)

### bin_write

```c
ssize_t bin_write(int fork, char *buffer, size_t length)
```

Defined at lines 498 to 562.

Write data to a MacBinary fork.

Data must be written in fork order. The resource fork cannot be written until the data fork is complete. Padding is written when each fork is finished.

Parameters:
* `fork`: fork selector, `DATA` or `RESOURCE`
* `buffer`: source buffer
* `length`: number of bytes to write

Returns: number of bytes written, -1 on error

Calls: [macbin_write_padding](macbin.c.md#macbin_write_padding)

Called by: [to_write](megatron.c.md#to_write)

Uses file-scope variables: `bin`, `forkname` in [bin/nad/megatron.c](megatron.c.md)

### bin_header_read

```c
int bin_header_read(struct FHeader *fh, int revision)
```

Defined at lines 576 to 659.

Read a MacBinary header into a file header.

This is called by [bin_open()](macbin.c.md#bin_open) before any fork data is read. It validates the MacBinary revision, decodes header fields, and initializes fork lengths.

Parameters:
* `fh`: file header to populate
* `revision`: MacBinary revision returned by [test_header()](macbin.c.md#test_header)

Returns: 0 on success, -1 on error

Calls: [macbin_ad_date_from_header](macbin.c.md#macbin_ad_date_from_header), [macbin_fdflags_from_header](macbin.c.md#macbin_fdflags_from_header)

Called by: [bin_open](macbin.c.md#bin_open)

Uses file-scope variables: `bin`, `head_buf`

### bin_header_write

```c
int bin_header_write(struct FHeader *fh)
```

Defined at lines 672 to 766.

Write a file header as a MacBinary header.

This is called by [bin_open()](macbin.c.md#bin_open) before any fork data is written. It encodes the file header as a MacBinary III header and initializes fork lengths.

Parameters:
* `fh`: file header to write

Returns: 0 on success, -1 on error

Calls: [crc16_xmodem_update](crc16.c.md#crc16_xmodem_update), [macbin_ad_date_to_header](macbin.c.md#macbin_ad_date_to_header), [macbin_fdflags_to_header](macbin.c.md#macbin_fdflags_to_header), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [bin_open](macbin.c.md#bin_open)

Uses file-scope variables: `bin`, `head_buf`

### test_header

```c
int test_header(void)
```

Defined at lines 780 to 868.

Test the input header for a supported MacBinary revision.

Reads the first 128 bytes and determines whether the file is MacBinary, MacBinary II, MacBinary III, or not a MacBinary file.

Note: Apple's MacBinary II files can have a non-zero value at byte 74, so the byte 74 check is not very useful.

Returns: 1 for MacBinary, 2 for MacBinary II, 3 for MacBinary III, -1 if the header is invalid

Calls: [crc16_xmodem_update](crc16.c.md#crc16_xmodem_update), [macbin_u32_from_header](macbin.c.md#macbin_u32_from_header)

Called by: [bin_open](macbin.c.md#bin_open)

Uses file-scope variables: `bin`, `head_buf`

Mentioned in the documentation of: [bin_header_read](macbin.c.md#bin_header_read)

# Types

### struct bin_file_data

Defined at line 59.
* `uint32_t forklen`
* `char path`
* `int filed`
* `int owns_fd`
* `off_t filepos`
* `unsigned short headercrc`

# Macros

* Undocumented: `HEADBUFSIZ`, `MACBINARY_PLAY_NICE_WITH_OTHERS`, `MACBIN_NAMELEN`, `NOWAY`, `SURETHANG`

# File-scope variables

`bin`, `head_buf`
