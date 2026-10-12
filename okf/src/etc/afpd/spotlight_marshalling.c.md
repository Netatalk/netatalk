---
type: C Source File
title: "etc/afpd/spotlight_marshalling.c"
description: "Packing and unpacking of Spotlight RPC requests and replies."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/spotlight_marshalling.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/byteorder.h](../../include/atalk/byteorder.h.md)
* [atalk/dalloc.h](../../include/atalk/dalloc.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/spotlight.h](../../include/atalk/spotlight.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `errno.h`, `inttypes.h`, `limits.h`, `math.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `strings.h`, `talloc.h`

# Functions

### sl_put_le32

```c
static void sl_put_le32(char *buf, off_t off, uint32_t val)
```

Defined at lines 106 to 113.

Write a 32-bit value in little-endian order at an offset.

The caller checks the bounds.

Parameters:
* `buf`: buffer
* `off`: byte offset
* `val`: value in host order

Called by: [sivalc](spotlight_marshalling.c.md#sivalc), [sl_put_le64](spotlight_marshalling.c.md#sl_put_le64)

### sl_put_le64

```c
static void sl_put_le64(char *buf, off_t off, uint64_t val)
```

Defined at lines 124 to 128.

Write a 64-bit value in little-endian order at an offset.

The caller checks the bounds.

Parameters:
* `buf`: buffer
* `off`: byte offset
* `val`: value in host order

Calls: [sl_put_le32](spotlight_marshalling.c.md#sl_put_le32)

Called by: [slvalc](spotlight_marshalling.c.md#slvalc)

### sl_get_le64

```c
static uint64_t sl_get_le64(const char *buf, int offset)
```

Defined at lines 137 to 148.

Read a little-endian 64-bit value at an offset.

Parameters:
* `buf`: buffer
* `offset`: byte offset

Returns: the value in host order

Called by: [sl_unpack_uint64](spotlight_marshalling.c.md#sl_unpack_uint64)

### sl_get_be64

```c
static uint64_t sl_get_be64(const char *buf, int offset)
```

Defined at lines 157 to 168.

Read a big-endian 64-bit value at an offset.

Parameters:
* `buf`: buffer
* `offset`: byte offset

Returns: the value in host order

Called by: [sl_unpack_uint64](spotlight_marshalling.c.md#sl_unpack_uint64)

### sivalc

```c
static int sivalc(char *buf, off_t off, off_t maxoff, uint32_t val)
```

Defined at lines 182 to 197.

Write a 32-bit value in little-endian order after a bounds check.

The packed Spotlight payload, variant 432130dm, carries its numbers in little-endian order on every host.

Parameters:
* `buf`: buffer
* `off`: byte offset
* `maxoff`: end of the usable buffer
* `val`: value in host order

Returns: 0, or -1 when the value would cross maxoff

Calls: [sl_put_le32](spotlight_marshalling.c.md#sl_put_le32)

Called by: [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len)

### slvalc

```c
static int slvalc(char *buf, off_t off, off_t maxoff, uint64_t val)
```

Defined at lines 208 to 217.

Write a 64-bit value in little-endian order after a bounds check.

Parameters:
* `buf`: buffer
* `off`: byte offset
* `maxoff`: end of the usable buffer
* `val`: value in host order

Returns: 0, or -1 when the value would cross maxoff

Calls: [sl_put_le64](spotlight_marshalling.c.md#sl_put_le64)

Called by: [sl_pack_CNID](spotlight_marshalling.c.md#sl_pack_cnid), [sl_pack_array](spotlight_marshalling.c.md#sl_pack_array), [sl_pack_bool](spotlight_marshalling.c.md#sl_pack_bool), [sl_pack_date](spotlight_marshalling.c.md#sl_pack_date), [sl_pack_dict](spotlight_marshalling.c.md#sl_pack_dict), [sl_pack_filemeta](spotlight_marshalling.c.md#sl_pack_filemeta), [sl_pack_float](spotlight_marshalling.c.md#sl_pack_float), [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len), [sl_pack_nil](spotlight_marshalling.c.md#sl_pack_nil), [sl_pack_string](spotlight_marshalling.c.md#sl_pack_string), [sl_pack_uint64](spotlight_marshalling.c.md#sl_pack_uint64), [sl_pack_uuid](spotlight_marshalling.c.md#sl_pack_uuid)

### spotlight_get_utf16_string_encoding

```c
static uint spotlight_get_utf16_string_encoding(const char *buf, int offset, int query_length, uint encoding)
```

Defined at lines 232 to 251.

Read the UTF-16 encoding of a string from its byte order mark.

A string without a byte order mark counts as little-endian, without the SL_ENC_UTF_16 flag.

Parameters:
* `buf`: packed request
* `offset`: offset of the string
* `query_length`: length of the string in bytes
* `encoding`: encoding of the request, unused

Returns: SL_ENC_UTF_16 with the byte order of the mark, or SL_ENC_LITTLE_ENDIAN alone

Called by: [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx)

### sl_pack_tag

```c
static uint64_t sl_pack_tag(uint16_t type, uint16_t size_or_count, uint32_t val)
```

Defined at lines 268 to 272.

Build the 8-byte tag of a packed element.

Parameters:
* `type`: element type, in bits 16 to 31
* `size_or_count`: size in 8-byte units or element count, in bits 0 to 15
* `val`: value, in bits 32 to 63

Returns: the tag

Called by: [sl_pack_CNID](spotlight_marshalling.c.md#sl_pack_cnid), [sl_pack_array](spotlight_marshalling.c.md#sl_pack_array), [sl_pack_bool](spotlight_marshalling.c.md#sl_pack_bool), [sl_pack_date](spotlight_marshalling.c.md#sl_pack_date), [sl_pack_dict](spotlight_marshalling.c.md#sl_pack_dict), [sl_pack_filemeta](spotlight_marshalling.c.md#sl_pack_filemeta), [sl_pack_float](spotlight_marshalling.c.md#sl_pack_float), [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len), [sl_pack_nil](spotlight_marshalling.c.md#sl_pack_nil), [sl_pack_string](spotlight_marshalling.c.md#sl_pack_string), [sl_pack_uint64](spotlight_marshalling.c.md#sl_pack_uint64), [sl_pack_uuid](spotlight_marshalling.c.md#sl_pack_uuid)

### sl_pack_float

```c
static int sl_pack_float(double d, char *buf, int offset, off_t maxoff)
```

Defined at lines 283 to 300.

Pack a double as a float element, a tag and the IEEE 754 bits.

Parameters:
* `d`: value
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer

Returns: the offset after the element, or -1 when it does not fit

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_uint64

```c
static int sl_pack_uint64(uint64_t u, char *buf, int offset, off_t maxoff)
```

Defined at lines 311 to 323.

Pack a 64-bit integer element, a tag and the value.

Parameters:
* `u`: value
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer

Returns: the offset after the element, or -1 when it does not fit

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_bool

```c
static int sl_pack_bool(sl_bool_t bl, char *buf, int offset, off_t maxoff)
```

Defined at lines 334 to 346.

Pack a boolean element, whose value sits in its tag.

Parameters:
* `bl`: value
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer

Returns: the offset after the element, or -1 when it does not fit

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_nil

```c
static int sl_pack_nil(char *buf, int offset, off_t maxoff)
```

Defined at lines 356 to 367.

Pack a nil element, a lone tag.

Parameters:
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer

Returns: the offset after the element, or -1 when it does not fit

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_date

```c
static int sl_pack_date(sl_time_t t, char *buf, int offset, off_t maxoff)
```

Defined at lines 379 to 399.

Pack a time as a date element, a double of seconds since 2001-01-01 UTC.

Parameters:
* `t`: time with microseconds
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer

Returns: the offset after the element, or -1 when it does not fit

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_uuid

```c
static int sl_pack_uuid(sl_uuid_t *uuid, char *buf, int offset, off_t maxoff)
```

Defined at lines 410 to 427.

Pack a UUID element, a tag and the 16 bytes.

Parameters:
* `uuid`: UUID
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer

Returns: the offset after the element, or -1 when it does not fit

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_CNID

```c
static int sl_pack_CNID(sl_cnids_t *cnids, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 443 to 485.

Pack a CNID list as a complex element with its table of contents entry.

The data holds a CNIDs tag. A nonempty list adds a header with the count and the context, then the CNIDs.

Parameters:
* `cnids`: CNID list
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer
* `toc_buf`: table of contents
* `toc_idx`: next free table of contents slot

Returns: the offset after the element, or -1 when it does not fit

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_array

```c
static int sl_pack_array(sl_array_t *array, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 498 to 518.

Pack an array as an SQ_TYPE_COMPLEX element, then pack its members.

Parameters:
* `array`: array to pack
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer
* `toc_buf`: table of contents
* `toc_idx`: next free table of contents slot

Returns: the offset after the members, or -1 when one does not fit

Calls: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop), [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_dict

```c
static int sl_pack_dict(sl_array_t *dict, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 534 to 556.

Pack a dictionary as an SQ_TYPE_COMPLEX element, then pack its keys and values.

Keys and values alternate in the dalloc array.

Parameters:
* `dict`: dictionary to pack
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer
* `toc_buf`: table of contents
* `toc_idx`: next free table of contents slot

Returns: the offset after the members, or -1 when one does not fit

Calls: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop), [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_filemeta

```c
static int sl_pack_filemeta(sl_filemeta_t *fm, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 574 to 611.

Pack file metadata as an SQ_TYPE_COMPLEX element that holds a nested packed payload.

The metadata packs as a separate complete payload behind a data tag. A payload of 40 bytes or less carries only its header, so the element then keeps no data.

Parameters:
* `fm`: file metadata to pack
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer
* `toc_buf`: table of contents
* `toc_idx`: next free table of contents slot

Returns: the offset after the element, or -1 when it does not fit

Calls: [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len), [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_string

```c
static int sl_pack_string(char *s, char *buf, int offset, char *toc_buf, int *toc_idx, off_t maxoff)
```

Defined at lines 628 to 667.

Pack a string as a complex element padded to 8-byte units.

The talloc size of s bounds the string, which must end in a NUL inside it. The table of contents entry records how many bytes the last unit uses.

Parameters:
* `s`: NUL-terminated talloc string
* `buf`: buffer
* `offset`: write offset
* `toc_buf`: table of contents
* `toc_idx`: next free table of contents slot
* `maxoff`: end of the usable buffer

Returns: the offset after the element, or -1 for an unterminated string or when it does not fit

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_loop

```c
static int sl_pack_loop(DALLOC_CTX *query, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 682 to 736.

Pack every element of a dalloc array by its talloc type name.

An element of an unknown type drops out without notice.

Parameters:
* `query`: array to pack
* `buf`: buffer
* `offset`: write offset
* `maxoff`: end of the usable buffer
* `toc_buf`: table of contents
* `toc_idx`: next free table of contents slot

Returns: the offset after the elements, or -1 when one does not fit

Calls: [sl_pack_CNID](spotlight_marshalling.c.md#sl_pack_cnid), [sl_pack_array](spotlight_marshalling.c.md#sl_pack_array), [sl_pack_bool](spotlight_marshalling.c.md#sl_pack_bool), [sl_pack_date](spotlight_marshalling.c.md#sl_pack_date), [sl_pack_dict](spotlight_marshalling.c.md#sl_pack_dict), [sl_pack_filemeta](spotlight_marshalling.c.md#sl_pack_filemeta), [sl_pack_float](spotlight_marshalling.c.md#sl_pack_float), [sl_pack_nil](spotlight_marshalling.c.md#sl_pack_nil), [sl_pack_string](spotlight_marshalling.c.md#sl_pack_string), [sl_pack_uint64](spotlight_marshalling.c.md#sl_pack_uint64), [sl_pack_uuid](spotlight_marshalling.c.md#sl_pack_uuid)

Called by: [sl_pack_array](spotlight_marshalling.c.md#sl_pack_array), [sl_pack_dict](spotlight_marshalling.c.md#sl_pack_dict), [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len)

### sl_unpack_range_valid

```c
static bool sl_unpack_range_valid(size_t buf_len, int offset, size_t length)
```

Defined at lines 751 to 756.

Tell whether a range lies inside a buffer, without overflow.

Parameters:
* `buf_len`: buffer length
* `offset`: range start
* `length`: range length

Returns: true when offset is not negative and the range ends inside the buffer

Called by: [sl_unpack_CNID](spotlight_marshalling.c.md#sl_unpack_cnid), [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked), [sl_unpack_uuid](spotlight_marshalling.c.md#sl_unpack_uuid)

### sl_unpack_uint64

```c
static uint64_t sl_unpack_uint64(const char *buf, int offset, uint encoding)
```

Defined at lines 766 to 773.

Read a 64-bit value in the byte order of the request.

Parameters:
* `buf`: packed request
* `offset`: byte offset, checked by the caller
* `encoding`: SL_ENC_LITTLE_ENDIAN, or any other value for big-endian

Returns: the value in host order

Calls: [sl_get_be64](spotlight_marshalling.c.md#sl_get_be64), [sl_get_le64](spotlight_marshalling.c.md#sl_get_le64)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

### sl_unpack_uint64_checked

```c
static int sl_unpack_uint64_checked(const char *buf, size_t buf_len, int offset, uint encoding, uint64_t *value)
```

Defined at lines 786 to 795.

Read a 64-bit value in the byte order of the request after a bounds check.

Parameters:
* `buf`: packed request
* `buf_len`: length of buf
* `offset`: byte offset
* `encoding`: byte order of the request
* `value`: receives the value

Returns: 0, or -1 when the value lies outside buf

Calls: [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64](spotlight_marshalling.c.md#sl_unpack_uint64)

Called by: [sl_unpack_CNID](spotlight_marshalling.c.md#sl_unpack_cnid), [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_date](spotlight_marshalling.c.md#sl_unpack_date), [sl_unpack_floats](spotlight_marshalling.c.md#sl_unpack_floats), [sl_unpack_ints](spotlight_marshalling.c.md#sl_unpack_ints), [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r), [sl_unpack_uuid](spotlight_marshalling.c.md#sl_unpack_uuid)

### sl_unpack_count

```c
static int sl_unpack_count(uint64_t query_data64, int query_length, size_t element_size, int *count)
```

Defined at lines 808 to 831.

Take the value count of an element from the upper 32 bits of its tag.

The count must fit in an int and in the bytes that follow the tag.

Parameters:
* `query_data64`: element tag
* `query_length`: element length in bytes, tag included
* `element_size`: size of one value
* `count`: receives the count

Returns: 0, or -1 when the values would overrun the element

Called by: [sl_unpack_date](spotlight_marshalling.c.md#sl_unpack_date), [sl_unpack_floats](spotlight_marshalling.c.md#sl_unpack_floats), [sl_unpack_ints](spotlight_marshalling.c.md#sl_unpack_ints), [sl_unpack_uuid](spotlight_marshalling.c.md#sl_unpack_uuid)

### sl_unpack_ints

```c
static int sl_unpack_ints(DALLOC_CTX *query, const char *buf, int offset, int query_length, size_t buf_len, uint encoding)
```

Defined at lines 844 to 871.

Unpack the 64-bit integers of an int64 element into a dalloc array.

Parameters:
* `query`: array that receives the values
* `buf`: packed request
* `offset`: offset of the element tag
* `query_length`: element length in bytes
* `buf_len`: length of buf
* `encoding`: byte order of the request

Returns: the number of values, or -1 for an invalid element

Calls: [sl_unpack_count](spotlight_marshalling.c.md#sl_unpack_count), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_date

```c
static int sl_unpack_date(DALLOC_CTX *query, const char *buf, int offset, int query_length, size_t buf_len, uint encoding)
```

Defined at lines 886 to 925.

Unpack the dates of a date element into sl_time_t values.

Each date is a double of seconds since 2001-01-01 UTC.

Parameters:
* `query`: array that receives the times
* `buf`: packed request
* `offset`: offset of the element tag
* `query_length`: element length in bytes
* `buf_len`: length of buf
* `encoding`: byte order of the request

Returns: the number of dates, or -1 for an invalid element

Calls: [sl_unpack_count](spotlight_marshalling.c.md#sl_unpack_count), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_uuid

```c
static int sl_unpack_uuid(DALLOC_CTX *query, const char *buf, int offset, int query_length, size_t buf_len, uint encoding)
```

Defined at lines 938 to 966.

Unpack the 16-byte UUIDs of a UUID element into a dalloc array.

Parameters:
* `query`: array that receives the UUIDs
* `buf`: packed request
* `offset`: offset of the element tag
* `query_length`: element length in bytes
* `buf_len`: length of buf
* `encoding`: byte order of the request

Returns: the number of UUIDs, or -1 for an invalid element

Calls: [sl_unpack_count](spotlight_marshalling.c.md#sl_unpack_count), [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_floats

```c
static int sl_unpack_floats(DALLOC_CTX *query, const char *buf, int offset, int query_length, size_t buf_len, uint encoding)
```

Defined at lines 979 to 1010.

Unpack the doubles of a float element into a dalloc array.

Parameters:
* `query`: array that receives the values
* `buf`: packed request
* `offset`: offset of the element tag
* `query_length`: element length in bytes
* `buf_len`: length of buf
* `encoding`: byte order of the request

Returns: the number of values, or -1 for an invalid element

Calls: [sl_unpack_count](spotlight_marshalling.c.md#sl_unpack_count), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_CNID

```c
static int sl_unpack_CNID(DALLOC_CTX *query, const char *buf, int offset, int length, size_t buf_len, uint encoding)
```

Defined at lines 1027 to 1074.

Unpack a CNIDs element into an [sl_cnids_t](../../include/atalk/spotlight.h.md#struct-sl_cnids_t).

An element of 16 bytes or less is an empty list, which a reply without results carries. A nonempty element needs a count of 1 or more that fits the element length.

Parameters:
* `query`: array that receives the list
* `buf`: packed request
* `offset`: offset of the CNID header
* `length`: element length in bytes
* `buf_len`: length of buf
* `encoding`: byte order of the request

Returns: 0, or -1 for an invalid element or a failed allocation

Calls: [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx)

### spotlight_get_qtype_string

```c
static const char * spotlight_get_qtype_string(uint64_t query_type)
```

Defined at lines 1082 to 1109.

Name a packed element type for the log.

Parameters:
* `query_type`: element type

Returns: the name, or "unknown"

### spotlight_get_cpx_qtype_string

```c
static const char * spotlight_get_cpx_qtype_string(uint64_t cpx_query_type)
```

Defined at lines 1117 to 1141.

Name an SQ_TYPE_COMPLEX element subtype for the log.

Parameters:
* `cpx_query_type`: subtype from the table of contents

Returns: the name, or "unknown"

### sl_unpack_cpx

```c
static int sl_unpack_cpx(DALLOC_CTX *query, const char *buf, const int offset, uint cpx_query_type, uint cpx_query_count, size_t buf_len, const uint toc_offset, uint toc_count, const uint encoding, int depth)
```

Defined at lines 1164 to 1310.

Unpack an SQ_TYPE_COMPLEX element, its subtype taken from the table of contents.

Arrays and dictionaries unpack their members recursively, down to SUBQ_SAFETY_LIM levels. Strings must end before the table of contents. UTF-16 strings must be little-endian and convert to UTF-8. File metadata holds a nested payload, and a header-only element of 8 bytes is empty.

Parameters:
* `query`: array that receives the element
* `buf`: packed data region
* `offset`: offset after the element tag
* `cpx_query_type`: subtype from the table of contents
* `cpx_query_count`: member count of an array or dictionary
* `buf_len`: length of buf
* `toc_offset`: offset of the first table of contents entry
* `toc_count`: number of table of contents entries
* `encoding`: byte order of the request
* `depth`: nesting depth

Returns: the offset after the element, or -1 for an invalid element

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [dalloc_strndup](../../libatalk/dalloc/dalloc.c.md#dalloc_strndup), [sl_unpack_CNID](spotlight_marshalling.c.md#sl_unpack_cnid), [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r), [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked), [spotlight_get_utf16_string_encoding](spotlight_marshalling.c.md#spotlight_get_utf16_string_encoding)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_loop

```c
static int sl_unpack_loop(DALLOC_CTX *query, const char *buf, int offset, uint count, size_t buf_len, const uint toc_offset, uint toc_count, const uint encoding, int depth)
```

Defined at lines 1331 to 1498.

Unpack count elements into a dalloc array.

The loop stops at the table of contents. A null element stands for up to 64 nils. Integer, UUID, float, and date elements hold 1 or more values each, and every value counts toward count.

Parameters:
* `query`: array that receives the elements
* `buf`: packed data region
* `offset`: offset of the first element tag
* `count`: number of values to unpack
* `buf_len`: length of buf
* `toc_offset`: offset of the first table of contents entry
* `toc_count`: number of table of contents entries
* `encoding`: byte order of the request
* `depth`: nesting depth

Returns: the offset after the elements, or -1 for an invalid element or a depth of SUBQ_SAFETY_LIM

Calls: [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_date](spotlight_marshalling.c.md#sl_unpack_date), [sl_unpack_floats](spotlight_marshalling.c.md#sl_unpack_floats), [sl_unpack_ints](spotlight_marshalling.c.md#sl_unpack_ints), [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64](spotlight_marshalling.c.md#sl_unpack_uint64), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked), [sl_unpack_uuid](spotlight_marshalling.c.md#sl_unpack_uuid)

Called by: [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r)

### sl_pack_len

```c
static int sl_pack_len(DALLOC_CTX *query, char *buf, off_t maxoff)
```

Defined at lines 1516 to 1550.

Pack a dalloc tree into the Spotlight wire format.

The payload starts with the magic 432130dm and 2 32-bit lengths in 8-byte units, of the full payload and of the data. The packed elements follow, then the table of contents with its SQ_TYPE_TOC tag.

Parameters:
* `query`: tree to pack
* `buf`: buffer
* `maxoff`: size of buf

Returns: the packed length, or -1 when the payload does not fit

Calls: [sivalc](spotlight_marshalling.c.md#sivalc), [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop), [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack](spotlight_marshalling.c.md#sl_pack), [sl_pack_filemeta](spotlight_marshalling.c.md#sl_pack_filemeta)

### sl_pack

```c
int sl_pack(DALLOC_CTX *query, char *buf)
```

Defined at lines 1559 to 1562. Declared in [include/atalk/spotlight.h](../../include/atalk/spotlight.h.md).

Pack a dalloc tree into a buffer of MAX_SLQ_DAT bytes.

Parameters:
* `query`: tree to pack
* `buf`: buffer of MAX_SLQ_DAT bytes

Returns: the packed length, or -1 when the payload does not fit

Calls: [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_unpack_r

```c
static int sl_unpack_r(DALLOC_CTX *query, const char *buf, size_t buf_len, int depth)
```

Defined at lines 1578 to 1647.

Unpack a Spotlight payload into a dalloc tree.

The magic md031234 marks a big-endian payload, and any other value a little-endian one. The header gives the table of contents offset, which must point at an SQ_TYPE_TOC tag inside the data. The data then unpacks as 1 element.

Parameters:
* `query`: tree that receives the elements
* `buf`: packed payload
* `buf_len`: length of buf
* `depth`: nesting depth of a nested payload

Returns: 0, or -1 for an invalid payload

Calls: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack](spotlight_marshalling.c.md#sl_unpack), [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_len](spotlight_marshalling.c.md#sl_unpack_len)

### sl_unpack

```c
int sl_unpack(DALLOC_CTX *query, const char *buf)
```

Defined at lines 1656 to 1659. Declared in [include/atalk/spotlight.h](../../include/atalk/spotlight.h.md).

Unpack a Spotlight payload from a buffer of MAX_SLQ_DAT bytes.

Parameters:
* `query`: tree that receives the elements
* `buf`: packed payload with MAX_SLQ_DAT readable bytes

Returns: 0, or -1 for an invalid payload

Calls: [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r)

### sl_unpack_len

```c
int sl_unpack_len(DALLOC_CTX *query, const char *buf, size_t buf_len)
```

Defined at lines 1669 to 1672. Declared in [include/atalk/spotlight.h](../../include/atalk/spotlight.h.md).

Unpack a Spotlight payload of a known length.

Parameters:
* `query`: tree that receives the elements
* `buf`: packed payload
* `buf_len`: length of buf

Returns: 0, or -1 for an invalid payload

Calls: [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

# Macros

* Undocumented: `MAX_SLQ_DAT`, `MAX_SLQ_TOC`, `SL_OFFSET_DELTA`, `SPOTLIGHT_TIME_DELTA`, `SQ_CPX_TYPE_ARRAY`, `SQ_CPX_TYPE_CNIDS`, `SQ_CPX_TYPE_DICT`, `SQ_CPX_TYPE_FILEMETA`, `SQ_CPX_TYPE_STRING`, `SQ_CPX_TYPE_UTF16_STRING`, `SQ_TYPE_BOOL`, `SQ_TYPE_CNIDS`, `SQ_TYPE_COMPLEX`, `SQ_TYPE_DATA`, `SQ_TYPE_DATE`, `SQ_TYPE_FLOAT`, `SQ_TYPE_INT64`, `SQ_TYPE_NULL`, `SQ_TYPE_TOC`, `SQ_TYPE_UUID`, `SUBQ_SAFETY_LIM`
