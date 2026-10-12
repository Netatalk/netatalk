---
type: C Source File
title: "bin/nad/macbin.c"
description: "MacBinary encoding and decoding for the nad archive commands."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/macbin.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 83 to 87.

Combine two bytes into a 16-bit value in host byte order.

Parameters:
* `high`: bits 15 to 8
* `low`: bits 7 to 0

Returns: the combined value

Called by: [macbin_fdflags_from_header](macbin.c.md#macbin_fdflags_from_header)

### macbin_u32_from_header

```c
static uint32_t macbin_u32_from_header(const unsigned char *header, size_t offset)
```

Defined at lines 97 to 104.

Read a big-endian 32-bit field of the MacBinary header in host byte order.

Parameters:
* `header`: 128-byte MacBinary header
* `offset`: offset of the field

Returns: the field value

Called by: [test_header](macbin.c.md#test_header)

### macbin_fdflags_from_header

```c
static uint16_t macbin_fdflags_from_header(const unsigned char *header, int revision)
```

Defined at lines 117 to 135.

Read the Finder flags of a MacBinary header in network byte order.

Byte 73 holds bits 15 to 8. MacBinary II and III keep bits 7 to 0 at byte

1. A zero there falls back to byte 74, where some writers put them. MacBinary I has only byte 73.

Parameters:
* `header`: 128-byte MacBinary header
* `revision`: MacBinary revision of the header, 1 to 3

Returns: the Finder flags

Calls: [macbin_u16_from_bytes](macbin.c.md#macbin_u16_from_bytes)

Called by: [bin_header_read](macbin.c.md#bin_header_read)

### macbin_fdflags_to_header

```c
static void macbin_fdflags_to_header(unsigned char *header, uint16_t fdflags)
```

Defined at lines 145 to 153.

Store Finder flags in a MacBinary header.

Bits 15 to 8 go to byte 73, and bits 7 to 0 go to bytes 101 and 74.

Parameters:
* `header`: 128-byte MacBinary header
* `fdflags`: Finder flags in network byte order

Called by: [bin_header_write](macbin.c.md#bin_header_write)

### macbin_ad_date_from_header

```c
static int macbin_ad_date_from_header(uint32_t mac_date, uint32_t *ad_date)
```

Defined at lines 165 to 169.

Turn a MacBinary date into an AppleDouble date.

A MacBinary date counts local seconds from 1904, and an AppleDouble date counts UTC seconds from 2000. Both stay in network byte order.

Parameters:
* `mac_date`: MacBinary date
* `ad_date`: receives the AppleDouble date

Returns: 0, or -1 when the time conversion fails

Calls: [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset)

Called by: [bin_header_read](macbin.c.md#bin_header_read)

### macbin_ad_date_to_header

```c
static int macbin_ad_date_to_header(uint32_t ad_date, uint32_t *mac_date)
```

Defined at lines 181 to 189.

Turn an AppleDouble date into a MacBinary date.

The call converts UTC seconds from 2000 into local seconds from 1904, both in network byte order.

Parameters:
* `ad_date`: AppleDouble date
* `mac_date`: receives the MacBinary date

Returns: 0, or -1 when the time conversion fails

Calls: [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset)

Called by: [bin_header_write](macbin.c.md#bin_header_write)

### macbin_discard_bytes

```c
static int macbin_discard_bytes(size_t length)
```

Defined at lines 200 to 224.

Read and drop bytes from the MacBinary input.

Each read takes up to 128 bytes, and the call advances the tracked file position.

Parameters:
* `length`: number of bytes to drop

Returns: 0, or -1 for a read error or an early end of file

Called by: [macbin_skip_padding](macbin.c.md#macbin_skip_padding)

Uses file-scope variables: `bin`

### macbin_write_repeat

```c
static int macbin_write_repeat(unsigned char value, size_t length)
```

Defined at lines 236 to 260.

Write one byte value `length` times to the MacBinary output.

Each write takes up to 128 bytes, and the call advances the tracked file position.

Parameters:
* `value`: byte to write
* `length`: number of bytes

Returns: 0, or -1 for a failed write

Called by: [macbin_write_padding](macbin.c.md#macbin_write_padding)

Uses file-scope variables: `bin`

### macbin_skip_padding

```c
static int macbin_skip_padding(void)
```

Defined at lines 271 to 315.

Skip the input padding after the data fork up to the next 128-byte boundary.

A seekable input moves with lseek. On a pipe the call tracks the position itself and reads past the padding.

Returns: 0, or -1 for a read error or an early end of file

Calls: [macbin_discard_bytes](macbin.c.md#macbin_discard_bytes)

Called by: [bin_read](macbin.c.md#bin_read)

Uses file-scope variables: `bin`

### macbin_write_padding

```c
static int macbin_write_padding(void)
```

Defined at lines 326 to 359.

Fill the MacBinary output with 0x7f bytes up to the next 128-byte boundary.

The value 0x7f matches other converters. On a pipe the call tracks the position itself.

Returns: 0, or -1 for a failed write

Calls: [macbin_write_repeat](macbin.c.md#macbin_write_repeat)

Called by: [bin_write](macbin.c.md#bin_write)

Uses file-scope variables: `bin`

### bin_open

```c
int bin_open(char *binfile, int flags, struct FHeader *fh, int options)
```

Defined at lines 375 to 454.

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

Defined at lines 467 to 500.

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

Defined at lines 510 to 513.

Return the path of the MacBinary output that bin_open created.

The path is `-` for stdout. The archive commands use it to register the new file in the CNID database of its volume.

Returns: the output path

Called by: [created_file_path](megatron.c.md#created_file_path)

Uses file-scope variables: `bin`

### bin_read

```c
ssize_t bin_read(int fork, char *buffer, size_t length)
```

Defined at lines 530 to 587.

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

Defined at lines 602 to 666.

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

Defined at lines 680 to 763.

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

Defined at lines 776 to 870.

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

Defined at lines 884 to 972.

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

Defined at line 64.
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
