---
type: C Source File
title: "libatalk/unicode/iconv.c"
description: "wrapper/stub for iconv character set conversion."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/iconv.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../unicode.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/byteorder.h](../../include/atalk/byteorder.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `ctype.h`, `errno.h`, `iconv.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `unistd.h`

# Function tables

### builtin_functions

Initialized at line 101. Dispatches to: [ascii_pull](iconv.c.md#ascii_pull), [ascii_push](iconv.c.md#ascii_push), [iconv_copy](iconv.c.md#iconv_copy)

# Functions

### find_charset_functions

```c
struct charset_functions * find_charset_functions(const char *name)
```

Defined at lines 123 to 136.

Called by: [atalk_iconv_open](iconv.c.md#atalk_iconv_open), [atalk_register_charset](iconv.c.md#atalk_register_charset), [get_charset_functions](charcnv.c.md#get_charset_functions), [volume_codepage](../../etc/afpd/volume.c.md#volume_codepage)

Uses file-scope variables: `charsets`

### atalk_register_charset

```c
int atalk_register_charset(struct charset_functions *funcs)
```

Defined at lines 138 to 154.

Calls: [find_charset_functions](iconv.c.md#find_charset_functions)

Called by: [lazy_initialize_iconv](iconv.c.md#lazy_initialize_iconv)

Uses file-scope variables: `charsets`

### lazy_initialize_iconv

```c
static void lazy_initialize_iconv(void)
```

Defined at lines 156 to 184.

Calls: [atalk_register_charset](iconv.c.md#atalk_register_charset)

Called by: [atalk_iconv_open](iconv.c.md#atalk_iconv_open)

Uses file-scope variables: `builtin_functions`, `charset_mac_centraleurope` in [libatalk/unicode/charsets/mac_centraleurope.c](charsets/mac_centraleurope.c.md), `charset_mac_chinese_simp` in [libatalk/unicode/charsets/mac_chinese_simp.c](charsets/mac_chinese_simp.c.md), `charset_mac_chinese_trad` in [libatalk/unicode/charsets/mac_chinese_trad.c](charsets/mac_chinese_trad.c.md), `charset_mac_cyrillic` in [libatalk/unicode/charsets/mac_cyrillic.c](charsets/mac_cyrillic.c.md), `charset_mac_greek` in [libatalk/unicode/charsets/mac_greek.c](charsets/mac_greek.c.md), `charset_mac_hebrew` in [libatalk/unicode/charsets/mac_hebrew.c](charsets/mac_hebrew.c.md), `charset_mac_japanese` in [libatalk/unicode/charsets/mac_japanese.c](charsets/mac_japanese.c.md), `charset_mac_korean` in [libatalk/unicode/charsets/mac_korean.c](charsets/mac_korean.c.md), `charset_mac_roman` in [libatalk/unicode/charsets/mac_roman.c](charsets/mac_roman.c.md), `charset_mac_turkish` in [libatalk/unicode/charsets/mac_turkish.c](charsets/mac_turkish.c.md), `charset_utf8` in [libatalk/unicode/utf8.c](utf8.c.md), `charset_utf8_mac` in [libatalk/unicode/utf8.c](utf8.c.md)

### sys_iconv

```c
static size_t sys_iconv(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 189 to 207.

Called by: [atalk_iconv_open](iconv.c.md#atalk_iconv_open)

### atalk_iconv

```c
size_t atalk_iconv(atalk_iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 215 to 250.

This is a simple portable iconv() implementaion.

It only knows about a very small number of character sets - just enough that netatalk works on systems that don't have iconv.

Called by: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [pull_charset_flags](charcnv.c.md#pull_charset_flags), [push_charset_flags](charcnv.c.md#push_charset_flags)

Calls through [`atalk_iconv_t::direct`](../../include/atalk/unicode.h.md#struct-atalk_iconv_t): no table assigns this field

Calls through [`atalk_iconv_t::pull`](../../include/atalk/unicode.h.md#struct-atalk_iconv_t): no table assigns this field

Calls through [`atalk_iconv_t::push`](../../include/atalk/unicode.h.md#struct-atalk_iconv_t): no table assigns this field

### atalk_iconv_open

```c
atalk_iconv_t atalk_iconv_open(const char *tocode, const char *fromcode)
```

Defined at lines 256 to 351.

simple iconv_open() wrapper

Calls: [find_charset_functions](iconv.c.md#find_charset_functions), [iconv_copy](iconv.c.md#iconv_copy), [lazy_initialize_iconv](iconv.c.md#lazy_initialize_iconv), [sys_iconv](iconv.c.md#sys_iconv)

Called by: [add_charset](charcnv.c.md#add_charset), [init_iconv](charcnv.c.md#init_iconv)

Uses file-scope variables: `charsets`

### atalk_iconv_close

```c
int atalk_iconv_close(atalk_iconv_t cd)
```

Defined at lines 356 to 378.

simple iconv_close() wrapper

### ascii_pull

```c
static size_t ascii_pull(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 385 to 411.

Dispatched via: [builtin_functions](iconv.c.md#builtin_functions)

### ascii_push

```c
static size_t ascii_push(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 413 to 446.

Dispatched via: [builtin_functions](iconv.c.md#builtin_functions)

### iconv_copy

```c
static size_t iconv_copy(void *, char **, size_t *, char **, size_t *)
```

Defined at lines 449 to 466.

Called by: [atalk_iconv_open](iconv.c.md#atalk_iconv_open)

Dispatched via: [builtin_functions](iconv.c.md#builtin_functions)

# Macros

* Undocumented: `CHARSET_WIDECHAR`, `DLIST_ADD`, `UCS2ICONV`

# File-scope variables

`charsets`
