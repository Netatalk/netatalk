---
type: C Source File
title: "bin/nad/hqx.c"
description: "BinHex 4.0 encoding and decoding for the nad archive commands."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/hqx.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [crc16.h](crc16.h.md)
* [hqx.h](hqx.h.md)
* [megatron.h](megatron.h.md)
* [nad.h](nad.h.md)
* [nad_adouble.h](nad_adouble.h.md)
* [netatalk/endian.h](../../sys/netatalk/endian.h.md)
* System headers: `ctype.h`, `netinet/in.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`, `time.h`, `unistd.h`

# Functions

### hqx_write_all

```c
static int hqx_write_all(const void *buf, size_t len)
```

Defined at lines 110 to 126.

Write a whole buffer to the BinHex output.

The output is the file or the stdout that hqx_open picked. A short write continues where it stopped. A write that fails or writes nothing ends the call, and EINTR counts as a failure.

Parameters:
* `buf`: bytes to write
* `len`: number of bytes

Returns: 0, or -1 for a failed write

Called by: [hqx_close](hqx.c.md#hqx_close), [hqx_open](hqx.c.md#hqx_open), [hqx_put_char](hqx.c.md#hqx_put_char)

Uses file-scope variables: `hqx`

### hqx_put_char

```c
static int hqx_put_char(unsigned char c)
```

Defined at lines 137 to 153.

Write one BinHex character to the output and start a new line after 63 characters.

A newline goes in before the character once the line holds 63 characters.

Parameters:
* `c`: character to write

Returns: 0, or -1 for a failed write

Calls: [hqx_write_all](hqx.c.md#hqx_write_all)

Called by: [hqx_emit_6bit](hqx.c.md#hqx_emit_6bit), [hqx_flush_6bit](hqx.c.md#hqx_flush_6bit)

Uses file-scope variables: `hqx`

### hqx_emit_6bit

```c
static int hqx_emit_6bit(unsigned char b)
```

Defined at lines 164 to 185.

Add one byte to the 6-bit encoder of the BinHex output.

The encoder collects three bytes and then writes them as four characters of the BinHex alphabet.

Parameters:
* `b`: byte to encode

Returns: 0, or -1 for a failed write

Calls: [hqx_put_char](hqx.c.md#hqx_put_char)

Called by: [hqx_emit_byte](hqx.c.md#hqx_emit_byte)

Uses file-scope variables: `hqx`, `hqxchars`

### hqx_emit_byte

```c
static int hqx_emit_byte(unsigned char b)
```

Defined at lines 196 to 207.

Encode one data byte and escape the 0x90 run marker.

A literal 0x90 becomes 0x90 followed by 0, which BinHex decoders read as the byte itself. The encoder writes no compressed runs.

Parameters:
* `b`: data byte

Returns: 0, or -1 for a failed write

Calls: [hqx_emit_6bit](hqx.c.md#hqx_emit_6bit)

Called by: [hqx_emit_data](hqx.c.md#hqx_emit_data)

### hqx_emit_data

```c
static int hqx_emit_data(const void *buf, size_t len)
```

Defined at lines 216 to 227.

Encode a buffer of data bytes with hqx_emit_byte.

Parameters:
* `buf`: data bytes
* `len`: number of bytes

Returns: 0, or -1 for a failed write

Calls: [hqx_emit_byte](hqx.c.md#hqx_emit_byte)

Called by: [hqx_finish_fork](hqx.c.md#hqx_finish_fork), [hqx_header_write](hqx.c.md#hqx_header_write), [hqx_write](hqx.c.md#hqx_write)

### hqx_flush_6bit

```c
static int hqx_flush_6bit(void)
```

Defined at lines 237 to 261.

Write the one or two bytes that the encoder still holds.

The call fills the missing bits with zeros and writes two characters for one byte or three characters for two bytes.

Returns: 0, or -1 for a failed write

Calls: [hqx_put_char](hqx.c.md#hqx_put_char)

Called by: [hqx_close](hqx.c.md#hqx_close)

Uses file-scope variables: `hqx`, `hqxchars`

### hqx_finish_fork

```c
static int hqx_finish_fork(int fork)
```

Defined at lines 272 to 292.

Write the CRC of a fork after its last declared byte.

A finished fork needs nothing, and a fork with bytes left fails. The CRC goes through the encoder in network byte order.

Parameters:
* `fork`: DATA or RESOURCE

Returns: 0, or -1 for missing bytes or a failed write

Calls: [hqx_emit_data](hqx.c.md#hqx_emit_data)

Called by: [hqx_close](hqx.c.md#hqx_close), [hqx_write](hqx.c.md#hqx_write)

Uses file-scope variables: `hqx`

### hqx_open

```c
int hqx_open(char *hqxfile, int flags, struct FHeader *fh, int options)
```

Defined at lines 309 to 388.

Open a BinHex file for reading or writing.

This must be called before other hqx operations. When opening for reading, it skips the preamble and reads the BinHex header. When opening for writing, it initializes output state and writes the BinHex preamble and header.

Parameters:
* `hqxfile`: input file path, or `-` for standard input
* `flags`: open flags; `O_RDONLY` selects read mode
* `fh`: file header to read or write
* `options`: output options, such as `OPTION_STDOUT`

Returns: 0 on success, -1 on error

Calls: [hqx_close](hqx.c.md#hqx_close), [hqx_header_read](hqx.c.md#hqx_header_read), [hqx_header_write](hqx.c.md#hqx_header_write), [hqx_write_all](hqx.c.md#hqx_write_all), [skip_junk](hqx.c.md#skip_junk), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [from_open](megatron.c.md#from_open), [to_open](megatron.c.md#to_open)

Uses file-scope variables: `first_flag`, `hqx`

Mentioned in the documentation of: [hqx_close](hqx.c.md#hqx_close), [hqx_header_read](hqx.c.md#hqx_header_read), [skip_junk](hqx.c.md#skip_junk)

### hqx_close

```c
int hqx_close(int keepflag)
```

Defined at lines 402 to 438.

Close the active BinHex file.

This must be called before opening another file with [hqx_open()](hqx.c.md#hqx_open). In write mode, `KEEP` finishes any remaining fork data, flushes the encoder, and writes the BinHex terminator. `TRASH` closes the output and removes the incomplete file.

Parameters:
* `keepflag`: `KEEP` to keep the output, `TRASH` to discard it

Returns: 0 on success, -1 on error

Calls: [hqx_finish_fork](hqx.c.md#hqx_finish_fork), [hqx_flush_6bit](hqx.c.md#hqx_flush_6bit), [hqx_write_all](hqx.c.md#hqx_write_all)

Called by: [from_close](megatron.c.md#from_close), [hqx_open](hqx.c.md#hqx_open), [to_close](megatron.c.md#to_close)

Uses file-scope variables: `hqx`

### hqx_path

```c
const char * hqx_path(void)
```

Defined at lines 448 to 451.

Return the path of the BinHex output that hqx_open created.

The path is `-` for stdout. The archive commands use it to register the new file in the CNID database of its volume.

Returns: the output path

Called by: [created_file_path](megatron.c.md#created_file_path)

Uses file-scope variables: `hqx`

### hqx_read

```c
ssize_t hqx_read(int fork, char *buffer, size_t length)
```

Defined at lines 469 to 533.

Read decoded data from a BinHex fork.

Call this until it returns zero for each fork. When no fork data remains, the stored fork CRC is read and compared with the calculated CRC before returning zero.

Note: [hqx_read()](hqx.c.md#hqx_read) must be called enough times to return zero, and no more than that, for each fork.

Parameters:
* `fork`: fork selector, `DATA` or `RESOURCE`
* `buffer`: destination buffer
* `length`: maximum number of bytes to read

Returns: number of bytes read, 0 when the fork is complete, -1 on error

Calls: [crc16_xmodem_update](crc16.c.md#crc16_xmodem_update), [hqx_7tobin](hqx.c.md#hqx_7tobin)

Called by: [from_read](megatron.c.md#from_read)

Uses file-scope variables: `forkname` in [bin/nad/megatron.c](megatron.c.md), `hqx`

Mentioned in the documentation of: [hqx_7tobin](hqx.c.md#hqx_7tobin)

### hqx_header_read

```c
int hqx_header_read(struct FHeader *fh)
```

Defined at lines 546 to 664.

Read and validate a BinHex header.

This is called by [hqx_open()](hqx.c.md#hqx_open) before any fork data is read. It decodes the header fields, initializes fork lengths and CRC state, and verifies the header CRC.

Parameters:
* `fh`: file header to populate

Returns: 0 on success, negative value on error

Calls: [crc16_xmodem_update](crc16.c.md#crc16_xmodem_update), [hqx_7tobin](hqx.c.md#hqx_7tobin)

Called by: [hqx_open](hqx.c.md#hqx_open)

Uses file-scope variables: `hqx`

Mentioned in the documentation of: [hqx_7tobin](hqx.c.md#hqx_7tobin)

### hqx_header_write

```c
int hqx_header_write(struct FHeader *fh)
```

Defined at lines 673 to 711.

Encode and write a BinHex header.

Parameters:
* `fh`: file header to write

Returns: 0 on success, -1 on error

Calls: [crc16_xmodem_update](crc16.c.md#crc16_xmodem_update), [hqx_emit_data](hqx.c.md#hqx_emit_data), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [hqx_open](hqx.c.md#hqx_open)

### hqx_write

```c
ssize_t hqx_write(int fork, char *buffer, size_t length)
```

Defined at lines 725 to 753.

Encode fork data into the BinHex output.

The data fork comes first, so the first resource bytes finish the data fork. A write past the declared length of a fork, or to a finished fork, fails. The last declared byte of a fork finishes it with its CRC.

Parameters:
* `fork`: DATA or RESOURCE
* `buffer`: fork bytes
* `length`: number of bytes

Returns: `length`, or -1 for an error

Calls: [crc16_xmodem_update](crc16.c.md#crc16_xmodem_update), [hqx_emit_data](hqx.c.md#hqx_emit_data), [hqx_finish_fork](hqx.c.md#hqx_finish_fork)

Called by: [to_write](megatron.c.md#to_write)

Uses file-scope variables: `hqx`

### hqx7_fill

```c
ssize_t hqx7_fill(unsigned char *hqx7_ptr)
```

Defined at lines 766 to 786.

Fill the BinHex text input buffer.

This is called from [skip_junk()](hqx.c.md#skip_junk) and [hqx_7tobin()](hqx.c.md#hqx_7tobin). It reads from the BinHex file into the hqx7 buffer and updates hqx7_first and hqx7_last to delimit valid data.

Parameters:
* `hqx7_ptr`: pointer within hqx7_buf where reading should start

Returns: number of bytes read, 0 on end of file, -1 on error

Called by: [hqx_7tobin](hqx.c.md#hqx_7tobin), [skip_junk](hqx.c.md#skip_junk)

Uses file-scope variables: `hqx`, `hqx7_buf`, `hqx7_first`, `hqx7_last`

### skip_junk

```c
int skip_junk(int line)
```

Defined at lines 843 to 940.

Skip non-BinHex text until encoded data is found.

This is called from [hqx_open()](hqx.c.md#hqx_open) to find the first valid BinHex line, and from [hqx_7tobin()](hqx.c.md#hqx_7tobin) to find subsequent encoded lines.

Parameters:
* `line`: `FIRST` for the first encoded line, `OTHER` thereafter

Returns: 0 on success, -1 if valid encoded data is not found

Calls: [hqx7_fill](hqx.c.md#hqx7_fill)

Called by: [hqx_7tobin](hqx.c.md#hqx_7tobin), [hqx_open](hqx.c.md#hqx_open)

Uses file-scope variables: `hqx7_buf`, `hqx7_first`, `hqx7_last`, `hqxlookup`

Mentioned in the documentation of: [hqx7_fill](hqx.c.md#hqx7_fill)

### hqx_7tobin

```c
size_t hqx_7tobin(char *outbuf, size_t datalen)
```

Defined at lines 954 to 1083.

Decode BinHex text data to binary.

This is called by [hqx_header_read()](hqx.c.md#hqx_header_read) for header data and by [hqx_read()](hqx.c.md#hqx_read) for fork data and CRC fields. It buffers decoded data so callers get the requested length unless end of file is reached.

Parameters:
* `outbuf`: destination buffer for decoded bytes
* `datalen`: number of decoded bytes requested

Returns: number of decoded bytes written to `outbuf`

Calls: [hqx7_fill](hqx.c.md#hqx7_fill), [skip_junk](hqx.c.md#skip_junk)

Called by: [hqx_header_read](hqx.c.md#hqx_header_read), [hqx_read](hqx.c.md#hqx_read)

Uses file-scope variables: `first_flag`, `hqx7_buf`, `hqx7_first`, `hqx7_last`, `hqxlookup`

Mentioned in the documentation of: [hqx7_fill](hqx.c.md#hqx7_fill), [skip_junk](hqx.c.md#skip_junk)

# Types

### struct hqx_file_data

Defined at line 77.
* `uint32_t forklen`
* `unsigned short forkcrc`
* `int forkdone`
* `char path`
* `unsigned short headercrc`
* `int filed`
* `int owns_fd`
* `unsigned char outbuf`
* `int outcnt`
* `int linecol`
* `int writing`

# Macros

* Undocumented: `BHH_CRCSIZ`, `BHH_DATASIZ`, `BHH_FLAGSIZ`, `BHH_HEADSIZ`, `BHH_RESSIZ`, `BHH_TCSIZ`, `BHH_VERSION`, `FIRST`, `NOWAY`, `OTHER`, `RUNCHAR`, `SURETHANG`

# File-scope variables

`first_flag`, `hqx`, `hqx7_buf`, `hqx7_first`, `hqx7_last`, `hqxchars`, `hqxlookup`
