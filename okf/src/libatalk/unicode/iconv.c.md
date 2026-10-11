---
type: C Source File
title: "libatalk/unicode/iconv.c"
description: "wrapper/stub for iconv character set conversion."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/iconv.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 130 to 143.

Find the registered conversion functions of a charset by name, ignoring case.

Parameters:
* `name`: charset name

Returns: the functions, or NULL for a charset without them

Called by: [atalk_iconv_open](iconv.c.md#atalk_iconv_open), [atalk_register_charset](iconv.c.md#atalk_register_charset), [get_charset_functions](charcnv.c.md#get_charset_functions), [volume_codepage](../../etc/afpd/volume.c.md#volume_codepage)

Uses file-scope variables: `charsets`

### atalk_register_charset

```c
int atalk_register_charset(struct charset_functions *funcs)
```

Defined at lines 151 to 167.

Add a charset to the list of registered conversion functions.

Parameters:
* `funcs`: conversion functions, which the list links in

Returns: 0, -1 for NULL, or -2 for a name already in the list

Calls: [find_charset_functions](iconv.c.md#find_charset_functions)

Called by: [lazy_initialize_iconv](iconv.c.md#lazy_initialize_iconv)

Uses file-scope variables: `charsets`

### lazy_initialize_iconv

```c
static void lazy_initialize_iconv(void)
```

Defined at lines 175 to 203.

Register the built-in charsets once: UCS-2, ASCII, UTF8, UTF8-MAC, and the Mac charsets.

The CJK Mac charsets need a usable iconv.

Calls: [atalk_register_charset](iconv.c.md#atalk_register_charset)

Called by: [atalk_iconv_open](iconv.c.md#atalk_iconv_open)

Uses file-scope variables: `builtin_functions`, `charset_mac_centraleurope` in [libatalk/unicode/charsets/mac_centraleurope.c](charsets/mac_centraleurope.c.md), `charset_mac_chinese_simp` in [libatalk/unicode/charsets/mac_chinese_simp.c](charsets/mac_chinese_simp.c.md), `charset_mac_chinese_trad` in [libatalk/unicode/charsets/mac_chinese_trad.c](charsets/mac_chinese_trad.c.md), `charset_mac_cyrillic` in [libatalk/unicode/charsets/mac_cyrillic.c](charsets/mac_cyrillic.c.md), `charset_mac_greek` in [libatalk/unicode/charsets/mac_greek.c](charsets/mac_greek.c.md), `charset_mac_hebrew` in [libatalk/unicode/charsets/mac_hebrew.c](charsets/mac_hebrew.c.md), `charset_mac_japanese` in [libatalk/unicode/charsets/mac_japanese.c](charsets/mac_japanese.c.md), `charset_mac_korean` in [libatalk/unicode/charsets/mac_korean.c](charsets/mac_korean.c.md), `charset_mac_roman` in [libatalk/unicode/charsets/mac_roman.c](charsets/mac_roman.c.md), `charset_mac_turkish` in [libatalk/unicode/charsets/mac_turkish.c](charsets/mac_turkish.c.md), `charset_utf8` in [libatalk/unicode/utf8.c](utf8.c.md), `charset_utf8_mac` in [libatalk/unicode/utf8.c](utf8.c.md)

### sys_iconv

```c
static size_t sys_iconv(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 220 to 238.

Convert through the system iconv and reset the shift state after an error.

The reset keeps the shift state of a charset such as SJIS from leaking into the next conversion. Without a usable iconv the call fails with EINVAL.

Parameters:
* `cd`: system iconv descriptor
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: output position
* `outbytesleft`: output space

Returns: the result of iconv, or `(size_t)-1` with errno

Called by: [atalk_iconv_open](iconv.c.md#atalk_iconv_open)

### atalk_iconv

```c
size_t atalk_iconv(atalk_iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 255 to 290.

Convert through an atalk_iconv descriptor, with the iconv calling convention.

A descriptor with a direct function converts in one call. Otherwise the call pulls up to 2048 bytes of UCS2 at a time and pushes each chunk to the output.

Parameters:
* `cd`: descriptor from atalk_iconv_open
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: output position
* `outbytesleft`: output space

Returns: 0 or the result of the direct function, or `(size_t)-1` with errno

Called by: [convert_string_allocate_internal](charcnv.c.md#convert_string_allocate_internal), [convert_string_internal](charcnv.c.md#convert_string_internal), [pull_charset_flags](charcnv.c.md#pull_charset_flags), [push_charset_flags](charcnv.c.md#push_charset_flags)

Calls through [`atalk_iconv_t::direct`](../../include/atalk/unicode.h.md#struct-atalk_iconv_t): no table assigns this field

Calls through [`atalk_iconv_t::pull`](../../include/atalk/unicode.h.md#struct-atalk_iconv_t): no table assigns this field

Calls through [`atalk_iconv_t::push`](../../include/atalk/unicode.h.md#struct-atalk_iconv_t): no table assigns this field

### atalk_iconv_open

```c
atalk_iconv_t atalk_iconv_open(const char *tocode, const char *fromcode)
```

Defined at lines 305 to 400.

Open a conversion between two charsets through UCS2.

The descriptor uses the built-in functions of each charset, and the system iconv for a charset without them or with CHARSET_ICONV. Equal names give a direct byte transfer, and a UCS-2 side gives a direct single-step conversion.

Parameters:
* `tocode`: target charset name
* `fromcode`: source charset name

Returns: the descriptor, or `(atalk_iconv_t)-1` with ENOMEM or EINVAL

Calls: [find_charset_functions](iconv.c.md#find_charset_functions), [iconv_copy](iconv.c.md#iconv_copy), [lazy_initialize_iconv](iconv.c.md#lazy_initialize_iconv), [sys_iconv](iconv.c.md#sys_iconv)

Called by: [add_charset](charcnv.c.md#add_charset), [init_iconv](charcnv.c.md#init_iconv)

Uses file-scope variables: `charsets`

### atalk_iconv_close

```c
int atalk_iconv_close(atalk_iconv_t cd)
```

Defined at lines 409 to 431.

Close an atalk_iconv descriptor together with its system iconv descriptors.

Parameters:
* `cd`: descriptor to close

Returns: 0

### ascii_pull

```c
static size_t ascii_pull(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 448 to 474.

Convert ASCII to UCS2, where any byte from 0x80 fails with EILSEQ.

Parameters:
* `cd`: descriptor, unused
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with EILSEQ or E2BIG

Dispatched via: [builtin_functions](iconv.c.md#builtin_functions)

### ascii_push

```c
static size_t ascii_push(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 489 to 522.

Convert UCS2 to ASCII, where any character from U+0080 fails with EILSEQ.

A trailing odd byte fails with EINVAL.

Parameters:
* `cd`: descriptor, unused
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with EILSEQ, EINVAL, or E2BIG

Dispatched via: [builtin_functions](iconv.c.md#builtin_functions)

### iconv_copy

```c
static size_t iconv_copy(void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 538 to 555.

Pass bytes through unchanged, for a conversion between equal charsets.

The call moves as many bytes as fit, and input that stays fails with E2BIG.

Parameters:
* `cd`: descriptor, unused
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with E2BIG

Called by: [atalk_iconv_open](iconv.c.md#atalk_iconv_open)

Dispatched via: [builtin_functions](iconv.c.md#builtin_functions)

# Macros

* Undocumented: `CHARSET_WIDECHAR`, `DLIST_ADD`, `UCS2ICONV`

# File-scope variables

`charsets`
