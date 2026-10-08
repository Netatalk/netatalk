---
type: C Source File
title: "etc/afpd/spotlight_marshalling.c"
description: "38 functions, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/spotlight_marshalling.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 92 to 99.

Called by: [sivalc](spotlight_marshalling.c.md#sivalc), [sl_put_le64](spotlight_marshalling.c.md#sl_put_le64)

### sl_put_le64

```c
static void sl_put_le64(char *buf, off_t off, uint64_t val)
```

Defined at lines 101 to 105.

Calls: [sl_put_le32](spotlight_marshalling.c.md#sl_put_le32)

Called by: [slvalc](spotlight_marshalling.c.md#slvalc)

### sl_get_le64

```c
static uint64_t sl_get_le64(const char *buf, int offset)
```

Defined at lines 107 to 118.

Called by: [sl_unpack_uint64](spotlight_marshalling.c.md#sl_unpack_uint64)

### sl_get_be64

```c
static uint64_t sl_get_be64(const char *buf, int offset)
```

Defined at lines 120 to 131.

Called by: [sl_unpack_uint64](spotlight_marshalling.c.md#sl_unpack_uint64)

### sivalc

```c
static int sivalc(char *buf, off_t off, off_t maxoff, uint32_t val)
```

Defined at lines 133 to 148.

Calls: [sl_put_le32](spotlight_marshalling.c.md#sl_put_le32)

Called by: [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len)

### slvalc

```c
static int slvalc(char *buf, off_t off, off_t maxoff, uint64_t val)
```

Defined at lines 150 to 159.

Calls: [sl_put_le64](spotlight_marshalling.c.md#sl_put_le64)

Called by: [sl_pack_CNID](spotlight_marshalling.c.md#sl_pack_cnid), [sl_pack_array](spotlight_marshalling.c.md#sl_pack_array), [sl_pack_bool](spotlight_marshalling.c.md#sl_pack_bool), [sl_pack_date](spotlight_marshalling.c.md#sl_pack_date), [sl_pack_dict](spotlight_marshalling.c.md#sl_pack_dict), [sl_pack_filemeta](spotlight_marshalling.c.md#sl_pack_filemeta), [sl_pack_float](spotlight_marshalling.c.md#sl_pack_float), [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len), [sl_pack_nil](spotlight_marshalling.c.md#sl_pack_nil), [sl_pack_string](spotlight_marshalling.c.md#sl_pack_string), [sl_pack_uint64](spotlight_marshalling.c.md#sl_pack_uint64), [sl_pack_uuid](spotlight_marshalling.c.md#sl_pack_uuid)

### spotlight_get_utf16_string_encoding

```c
static uint spotlight_get_utf16_string_encoding(const char *buf, int offset, int query_length, uint encoding)
```

Defined at lines 165 to 184.

Returns the UTF-16 string encoding, by checking the 2-byte byte order mark.

Returns: If there is no byte order mark, -1 is returned.

Called by: [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx)

### sl_pack_tag

```c
static uint64_t sl_pack_tag(uint16_t type, uint16_t size_or_count, uint32_t val)
```

Defined at lines 192 to 196.

Called by: [sl_pack_CNID](spotlight_marshalling.c.md#sl_pack_cnid), [sl_pack_array](spotlight_marshalling.c.md#sl_pack_array), [sl_pack_bool](spotlight_marshalling.c.md#sl_pack_bool), [sl_pack_date](spotlight_marshalling.c.md#sl_pack_date), [sl_pack_dict](spotlight_marshalling.c.md#sl_pack_dict), [sl_pack_filemeta](spotlight_marshalling.c.md#sl_pack_filemeta), [sl_pack_float](spotlight_marshalling.c.md#sl_pack_float), [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len), [sl_pack_nil](spotlight_marshalling.c.md#sl_pack_nil), [sl_pack_string](spotlight_marshalling.c.md#sl_pack_string), [sl_pack_uint64](spotlight_marshalling.c.md#sl_pack_uint64), [sl_pack_uuid](spotlight_marshalling.c.md#sl_pack_uuid)

### sl_pack_float

```c
static int sl_pack_float(double d, char *buf, int offset, off_t maxoff)
```

Defined at lines 198 to 215.

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_uint64

```c
static int sl_pack_uint64(uint64_t u, char *buf, int offset, off_t maxoff)
```

Defined at lines 217 to 229.

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_bool

```c
static int sl_pack_bool(sl_bool_t bl, char *buf, int offset, off_t maxoff)
```

Defined at lines 231 to 243.

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_nil

```c
static int sl_pack_nil(char *buf, int offset, off_t maxoff)
```

Defined at lines 245 to 256.

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_date

```c
static int sl_pack_date(sl_time_t t, char *buf, int offset, off_t maxoff)
```

Defined at lines 258 to 278.

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_uuid

```c
static int sl_pack_uuid(sl_uuid_t *uuid, char *buf, int offset, off_t maxoff)
```

Defined at lines 280 to 297.

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_CNID

```c
static int sl_pack_CNID(sl_cnids_t *cnids, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 299 to 341.

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_array

```c
static int sl_pack_array(sl_array_t *array, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 343 to 363.

Calls: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop), [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_dict

```c
static int sl_pack_dict(sl_array_t *dict, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 365 to 387.

Calls: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop), [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_filemeta

```c
static int sl_pack_filemeta(sl_filemeta_t *fm, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 389 to 426.

Calls: [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len), [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_string

```c
static int sl_pack_string(char *s, char *buf, int offset, char *toc_buf, int *toc_idx, off_t maxoff)
```

Defined at lines 428 to 467.

Calls: [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop)

### sl_pack_loop

```c
static int sl_pack_loop(DALLOC_CTX *query, char *buf, int offset, off_t maxoff, char *toc_buf, int *toc_idx)
```

Defined at lines 469 to 523.

Calls: [sl_pack_CNID](spotlight_marshalling.c.md#sl_pack_cnid), [sl_pack_array](spotlight_marshalling.c.md#sl_pack_array), [sl_pack_bool](spotlight_marshalling.c.md#sl_pack_bool), [sl_pack_date](spotlight_marshalling.c.md#sl_pack_date), [sl_pack_dict](spotlight_marshalling.c.md#sl_pack_dict), [sl_pack_filemeta](spotlight_marshalling.c.md#sl_pack_filemeta), [sl_pack_float](spotlight_marshalling.c.md#sl_pack_float), [sl_pack_nil](spotlight_marshalling.c.md#sl_pack_nil), [sl_pack_string](spotlight_marshalling.c.md#sl_pack_string), [sl_pack_uint64](spotlight_marshalling.c.md#sl_pack_uint64), [sl_pack_uuid](spotlight_marshalling.c.md#sl_pack_uuid)

Called by: [sl_pack_array](spotlight_marshalling.c.md#sl_pack_array), [sl_pack_dict](spotlight_marshalling.c.md#sl_pack_dict), [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len)

### sl_unpack_range_valid

```c
static bool sl_unpack_range_valid(size_t buf_len, int offset, size_t length)
```

Defined at lines 529 to 534.

Called by: [sl_unpack_CNID](spotlight_marshalling.c.md#sl_unpack_cnid), [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked), [sl_unpack_uuid](spotlight_marshalling.c.md#sl_unpack_uuid)

### sl_unpack_uint64

```c
static uint64_t sl_unpack_uint64(const char *buf, int offset, uint encoding)
```

Defined at lines 536 to 543.

Calls: [sl_get_be64](spotlight_marshalling.c.md#sl_get_be64), [sl_get_le64](spotlight_marshalling.c.md#sl_get_le64)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

### sl_unpack_uint64_checked

```c
static int sl_unpack_uint64_checked(const char *buf, size_t buf_len, int offset, uint encoding, uint64_t *value)
```

Defined at lines 545 to 554.

Calls: [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64](spotlight_marshalling.c.md#sl_unpack_uint64)

Called by: [sl_unpack_CNID](spotlight_marshalling.c.md#sl_unpack_cnid), [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_date](spotlight_marshalling.c.md#sl_unpack_date), [sl_unpack_floats](spotlight_marshalling.c.md#sl_unpack_floats), [sl_unpack_ints](spotlight_marshalling.c.md#sl_unpack_ints), [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r), [sl_unpack_uuid](spotlight_marshalling.c.md#sl_unpack_uuid)

### sl_unpack_count

```c
static int sl_unpack_count(uint64_t query_data64, int query_length, size_t element_size, int *count)
```

Defined at lines 556 to 579.

Called by: [sl_unpack_date](spotlight_marshalling.c.md#sl_unpack_date), [sl_unpack_floats](spotlight_marshalling.c.md#sl_unpack_floats), [sl_unpack_ints](spotlight_marshalling.c.md#sl_unpack_ints), [sl_unpack_uuid](spotlight_marshalling.c.md#sl_unpack_uuid)

### sl_unpack_ints

```c
static int sl_unpack_ints(DALLOC_CTX *query, const char *buf, int offset, int query_length, size_t buf_len, uint encoding)
```

Defined at lines 581 to 608.

Calls: [sl_unpack_count](spotlight_marshalling.c.md#sl_unpack_count), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_date

```c
static int sl_unpack_date(DALLOC_CTX *query, const char *buf, int offset, int query_length, size_t buf_len, uint encoding)
```

Defined at lines 610 to 649.

Calls: [sl_unpack_count](spotlight_marshalling.c.md#sl_unpack_count), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_uuid

```c
static int sl_unpack_uuid(DALLOC_CTX *query, const char *buf, int offset, int query_length, size_t buf_len, uint encoding)
```

Defined at lines 651 to 679.

Calls: [sl_unpack_count](spotlight_marshalling.c.md#sl_unpack_count), [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_floats

```c
static int sl_unpack_floats(DALLOC_CTX *query, const char *buf, int offset, int query_length, size_t buf_len, uint encoding)
```

Defined at lines 681 to 712.

Calls: [sl_unpack_count](spotlight_marshalling.c.md#sl_unpack_count), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_CNID

```c
static int sl_unpack_CNID(DALLOC_CTX *query, const char *buf, int offset, int length, size_t buf_len, uint encoding)
```

Defined at lines 714 to 761.

Calls: [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx)

### spotlight_get_qtype_string

```c
static const char * spotlight_get_qtype_string(uint64_t query_type)
```

Defined at lines 763 to 790.

### spotlight_get_cpx_qtype_string

```c
static const char * spotlight_get_cpx_qtype_string(uint64_t cpx_query_type)
```

Defined at lines 792 to 816.

### sl_unpack_cpx

```c
static int sl_unpack_cpx(DALLOC_CTX *query, const char *buf, const int offset, uint cpx_query_type, uint cpx_query_count, size_t buf_len, const uint toc_offset, uint toc_count, const uint encoding, int depth)
```

Defined at lines 818 to 964.

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [dalloc_strndup](../../libatalk/dalloc/dalloc.c.md#dalloc_strndup), [sl_unpack_CNID](spotlight_marshalling.c.md#sl_unpack_cnid), [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r), [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked), [spotlight_get_utf16_string_encoding](spotlight_marshalling.c.md#spotlight_get_utf16_string_encoding)

Called by: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop)

### sl_unpack_loop

```c
static int sl_unpack_loop(DALLOC_CTX *query, const char *buf, int offset, uint count, size_t buf_len, const uint toc_offset, uint toc_count, const uint encoding, int depth)
```

Defined at lines 966 to 1133.

Calls: [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_date](spotlight_marshalling.c.md#sl_unpack_date), [sl_unpack_floats](spotlight_marshalling.c.md#sl_unpack_floats), [sl_unpack_ints](spotlight_marshalling.c.md#sl_unpack_ints), [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64](spotlight_marshalling.c.md#sl_unpack_uint64), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked), [sl_unpack_uuid](spotlight_marshalling.c.md#sl_unpack_uuid)

Called by: [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r)

### sl_pack_len

```c
static int sl_pack_len(DALLOC_CTX *query, char *buf, off_t maxoff)
```

Defined at lines 1139 to 1173.

Calls: [sivalc](spotlight_marshalling.c.md#sivalc), [sl_pack_loop](spotlight_marshalling.c.md#sl_pack_loop), [sl_pack_tag](spotlight_marshalling.c.md#sl_pack_tag), [slvalc](spotlight_marshalling.c.md#slvalc)

Called by: [sl_pack](spotlight_marshalling.c.md#sl_pack), [sl_pack_filemeta](spotlight_marshalling.c.md#sl_pack_filemeta)

### sl_pack

```c
int sl_pack(DALLOC_CTX *query, char *buf)
```

Defined at lines 1175 to 1178. Declared in [include/atalk/spotlight.h](../../include/atalk/spotlight.h.md).

Calls: [sl_pack_len](spotlight_marshalling.c.md#sl_pack_len)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

### sl_unpack_r

```c
static int sl_unpack_r(DALLOC_CTX *query, const char *buf, size_t buf_len, int depth)
```

Defined at lines 1180 to 1249.

Calls: [sl_unpack_loop](spotlight_marshalling.c.md#sl_unpack_loop), [sl_unpack_range_valid](spotlight_marshalling.c.md#sl_unpack_range_valid), [sl_unpack_uint64_checked](spotlight_marshalling.c.md#sl_unpack_uint64_checked)

Called by: [sl_unpack](spotlight_marshalling.c.md#sl_unpack), [sl_unpack_cpx](spotlight_marshalling.c.md#sl_unpack_cpx), [sl_unpack_len](spotlight_marshalling.c.md#sl_unpack_len)

### sl_unpack

```c
int sl_unpack(DALLOC_CTX *query, const char *buf)
```

Defined at lines 1251 to 1254. Declared in [include/atalk/spotlight.h](../../include/atalk/spotlight.h.md).

Calls: [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r)

### sl_unpack_len

```c
int sl_unpack_len(DALLOC_CTX *query, const char *buf, size_t buf_len)
```

Defined at lines 1256 to 1259. Declared in [include/atalk/spotlight.h](../../include/atalk/spotlight.h.md).

Calls: [sl_unpack_r](spotlight_marshalling.c.md#sl_unpack_r)

Called by: [afp_spotlight_rpc](spotlight.c.md#afp_spotlight_rpc)

# Macros

* Undocumented: `MAX_SLQ_DAT`, `MAX_SLQ_TOC`, `SL_OFFSET_DELTA`, `SPOTLIGHT_TIME_DELTA`, `SQ_CPX_TYPE_ARRAY`, `SQ_CPX_TYPE_CNIDS`, `SQ_CPX_TYPE_DICT`, `SQ_CPX_TYPE_FILEMETA`, `SQ_CPX_TYPE_STRING`, `SQ_CPX_TYPE_UTF16_STRING`, `SQ_TYPE_BOOL`, `SQ_TYPE_CNIDS`, `SQ_TYPE_COMPLEX`, `SQ_TYPE_DATA`, `SQ_TYPE_DATE`, `SQ_TYPE_FLOAT`, `SQ_TYPE_INT64`, `SQ_TYPE_NULL`, `SQ_TYPE_TOC`, `SQ_TYPE_UUID`, `SUBQ_SAFETY_LIM`
