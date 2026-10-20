---
type: C Source File
title: "libatalk/unicode/util_unistr.c"
description: "UTF-16 string functions, with Unicode precomposition and decomposition."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/util_unistr.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-20T20:07:02+11:00 }
---

Part of the [libatalk/unicode](../unicode.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/byteorder.h](../../include/atalk/byteorder.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [precompose.h](precompose.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`

# Functions

### strlower_w

```c
int strlower_w(ucs2_t *)
```

Defined at lines 42 to 72. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Lowercase a NUL-terminated UTF-16 string in place.

A surrogate pair changes as one character.

Parameters:
* `s`: UTF-16 string

Returns: 1 when a character changed, or 0

Calls: [tolower_sp](utf16_case.c.md#tolower_sp), [tolower_w](utf16_case.c.md#tolower_w)

Called by: [charset_strlower](charcnv.c.md#charset_strlower), [convert_charset](charcnv.c.md#convert_charset)

### strupper_w

```c
int strupper_w(ucs2_t *)
```

Defined at lines 82 to 112. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Uppercase a NUL-terminated UTF-16 string in place.

A surrogate pair changes as one character.

Parameters:
* `s`: UTF-16 string

Returns: 1 when a character changed, or 0

Calls: [toupper_sp](utf16_case.c.md#toupper_sp), [toupper_w](utf16_case.c.md#toupper_w)

Called by: [charset_strupper](charcnv.c.md#charset_strupper), [convert_charset](charcnv.c.md#convert_charset)

### strlen_w

```c
size_t strlen_w(const ucs2_t *)
```

Defined at lines 122 to 129. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Count the UTF-16 units of a NUL-terminated string, as strlen does.

A surrogate pair counts as two units.

Parameters:
* `src`: UTF-16 string

Returns: the number of units before the NUL

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [convert_string_internal](charcnv.c.md#convert_string_internal), [strcasestr_w](util_unistr.c.md#strcasestr_w), [strndup_w](util_unistr.c.md#strndup_w), [strstr_w](util_unistr.c.md#strstr_w), [ucs2_to_charset](charcnv.c.md#ucs2_to_charset), [ucs2_to_charset_allocate](charcnv.c.md#ucs2_to_charset_allocate)

### strnlen_w

```c
size_t strnlen_w(const ucs2_t *, size_t)
```

Defined at lines 141 to 148. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Count the UTF-16 units of a string up to a limit, as strnlen does.

A surrogate pair counts as two units. Without an earlier NUL the call reads the unit at index `max` before it stops.

Parameters:
* `src`: UTF-16 string
* `max`: limit

Returns: the number of units before the NUL, capped at `max`

### strchr_w

```c
ucs2_t * strchr_w(const ucs2_t *, ucs2_t)
```

Defined at lines 160 to 175. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Find a UTF-16 unit in a NUL-terminated string, as strchr does.

The search treats the two halves of a surrogate pair as separate units, and a NUL `c` finds the terminator.

Parameters:
* `s`: UTF-16 string
* `c`: unit to find

Returns: the first match, or NULL

Called by: [strstr_w](util_unistr.c.md#strstr_w)

### strcasechr_w

```c
ucs2_t * strcasechr_w(const ucs2_t *s, ucs2_t c)
```

Defined at lines 187 to 202. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Find a UTF-16 unit in a string, ignoring case.

The search compares single units only, and strcasechr_sp covers surrogate pairs.

Parameters:
* `s`: UTF-16 string
* `c`: unit to find

Returns: the first match, or NULL

Calls: [tolower_w](utf16_case.c.md#tolower_w)

Called by: [strcasestr_w](util_unistr.c.md#strcasestr_w)

### strcasechr_sp

```c
ucs2_t * strcasechr_sp(const ucs2_t *s, uint32_t c_sp)
```

Defined at lines 213 to 228.

Find a surrogate pair in a UTF-16 string, ignoring case.

The search compares the pair with each pair of neighboring units.

Parameters:
* `s`: UTF-16 string
* `c_sp`: pair to find, with the high surrogate in bits 31 to 16

Returns: the first match, or NULL

Calls: [tolower_sp](utf16_case.c.md#tolower_sp)

Called by: [strcasestr_w](util_unistr.c.md#strcasestr_w)

### strcmp_w

```c
int strcmp_w(const ucs2_t *, const ucs2_t *)
```

Defined at lines 238 to 249. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Compare two NUL-terminated UTF-16 strings by unit value, as strcmp does.

Parameters:
* `a`: first string
* `b`: second string

Returns: the difference of the first differing units, or 0

### strncmp_w

```c
int strncmp_w(const ucs2_t *, const ucs2_t *, size_t)
```

Defined at lines 259 to 270. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Compare up to `len` units of two UTF-16 strings, as strncmp does.

Parameters:
* `a`: first string
* `b`: second string
* `len`: limit in units

Returns: the difference of the first differing units, or 0

Called by: [strstr_w](util_unistr.c.md#strstr_w)

### strstr_w

```c
ucs2_t * strstr_w(const ucs2_t *s, const ucs2_t *ins)
```

Defined at lines 279 to 300. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Find a UTF-16 substring, as strstr does.

Parameters:
* `s`: UTF-16 string
* `ins`: substring to find

Returns: the first match, or NULL for no match or an empty or NULL argument

Calls: [strchr_w](util_unistr.c.md#strchr_w), [strlen_w](util_unistr.c.md#strlen_w), [strncmp_w](util_unistr.c.md#strncmp_w)

### strcasestr_w

```c
ucs2_t * strcasestr_w(const ucs2_t *, const ucs2_t *)
```

Defined at lines 312 to 349. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Find a UTF-16 substring, ignoring case, with surrogate pair support.

A substring that starts with a lone surrogate from 0xd800 to 0xdbff matches nothing.

Parameters:
* `s`: UTF-16 string
* `ins`: substring to find

Returns: the first match, or NULL for no match or an empty or NULL argument

Calls: [strcasechr_sp](util_unistr.c.md#strcasechr_sp), [strcasechr_w](util_unistr.c.md#strcasechr_w), [strlen_w](util_unistr.c.md#strlen_w), [strncasecmp_w](util_unistr.c.md#strncasecmp_w)

Called by: [crit_check](../../etc/afpd/catsearch.c.md#crit_check)

### strcasecmp_w

```c
int strcasecmp_w(const ucs2_t *, const ucs2_t *)
```

Defined at lines 359 to 387. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Compare two NUL-terminated UTF-16 strings, ignoring case, with surrogate pair support.

Parameters:
* `a`: first string
* `b`: second string

Returns: the difference of the first differing lowercase forms, or 0

Calls: [tolower_sp](utf16_case.c.md#tolower_sp), [tolower_w](utf16_case.c.md#tolower_w)

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [crit_check](../../etc/afpd/catsearch.c.md#crit_check)

### strncasecmp_w

```c
int strncasecmp_w(const ucs2_t *, const ucs2_t *, size_t)
```

Defined at lines 400 to 431. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Compare up to `len` units of two UTF-16 strings, ignoring case.

A surrogate pair counts as two units. When `len` ends inside a pair, the comparison covers both of its units.

Parameters:
* `a`: first string
* `b`: second string
* `len`: limit in units

Returns: the difference of the first differing lowercase forms, or 0

Calls: [tolower_sp](utf16_case.c.md#tolower_sp), [tolower_w](utf16_case.c.md#tolower_w)

Called by: [strcasestr_w](util_unistr.c.md#strcasestr_w), [uam_getname](../../etc/afpd/uam.c.md#uam_getname)

### strndup_w

```c
ucs2_t * strndup_w(const ucs2_t *, size_t)
```

Defined at lines 444 to 462. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Duplicate the first `len` units of a UTF-16 string into a buffer from malloc.

A `len` of 0 takes the full string. The call does not stop at an earlier NUL and does not check for a split surrogate pair.

Parameters:
* `src`: UTF-16 string
* `len`: number of units, or 0 for the full string

Returns: the new string, or NULL when memory runs out

Calls: [strlen_w](util_unistr.c.md#strlen_w)

Called by: [strdup_w](util_unistr.c.md#strdup_w)

### strdup_w

```c
ucs2_t * strdup_w(const ucs2_t *)
```

Defined at lines 470 to 473. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Duplicate a NUL-terminated UTF-16 string into a buffer from malloc.

Parameters:
* `src`: UTF-16 string

Returns: the new string, or NULL when memory runs out

Calls: [strndup_w](util_unistr.c.md#strndup_w)

Called by: [creatvol](../util/netatalk_conf.c.md#creatvol)

### do_precomposition

```c
static ucs2_t do_precomposition(unsigned int base, unsigned int comb)
```

Defined at lines 485 to 508.

Look up the precomposed BMP character for a base and a combining character.

The call searches the sorted precomposition table.

Parameters:
* `base`: base character
* `comb`: combining character

Returns: the precomposed character, or 0 for no entry

Called by: [precompose_w](util_unistr.c.md#precompose_w)

Uses file-scope variables: `base` in [libatalk/unicode/precompose.h](precompose.h.md), `comb` in [libatalk/unicode/precompose.h](precompose.h.md), `precompositions` in [libatalk/unicode/precompose.h](precompose.h.md)

### do_precomposition_sp

```c
static uint32_t do_precomposition_sp(unsigned int base_sp, unsigned int comb_sp)
```

Defined at lines 521 to 545.

Look up the precomposed surrogate pair for a base pair and a combining pair.

Each pair sits in one 32-bit value, with the high surrogate in bits 31 to 16.

Parameters:
* `base_sp`: base pair
* `comb_sp`: combining pair

Returns: the precomposed pair, or 0 for no entry

Called by: [precompose_w](util_unistr.c.md#precompose_w)

Uses file-scope variables: `base_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `comb_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `precompositions_sp` in [libatalk/unicode/precompose.h](precompose.h.md)

### do_decomposition

```c
static uint32_t do_decomposition(ucs2_t base)
```

Defined at lines 555 to 580.

Look up the base and the combining character of a precomposed BMP character.

Parameters:
* `base`: precomposed character

Returns: the base in bits 31 to 16 and the combining character in bits 15 to 0, or 0 for no entry

Called by: [decompose_w](util_unistr.c.md#decompose_w)

Uses file-scope variables: `base` in [libatalk/unicode/precompose.h](precompose.h.md), `comb` in [libatalk/unicode/precompose.h](precompose.h.md), `decompositions` in [libatalk/unicode/precompose.h](precompose.h.md)

### do_decomposition_sp

```c
static uint64_t do_decomposition_sp(unsigned int base_sp)
```

Defined at lines 591 to 618.

Look up the base pair and the combining pair of a precomposed surrogate pair.

Parameters:
* `base_sp`: precomposed pair, with the high surrogate in bits 31 to 16

Returns: the base pair in bits 63 to 32 and the combining pair in bits 31 to 0, or 0 for no entry

Called by: [decompose_w](util_unistr.c.md#decompose_w)

Uses file-scope variables: `base_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `comb_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `decompositions_sp` in [libatalk/unicode/precompose.h](precompose.h.md)

### precompose_w

```c
size_t precompose_w(ucs2_t *, size_t, ucs2_t *, size_t *)
```

Defined at lines 639 to 748. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Precompose a UTF-16 string by the netatalk tables.

The call combines each base with the combining characters after it, Hangul syllables included, and surrogate pairs through a separate table. It does no singleton mapping or canonical ordering. It ignores the composition exclusions, which would break round trips.

The tables leave out U+2000 to U+2FFF, U+FE30 to U+FE4F, and U+2F800 to U+2FA1F, as the AFP 3.x specification asks. The output ends with a NUL, which the returned length leaves out.

Parameters:
* `name`: UTF-16 input
* `inplen`: input length in bytes, an even number above 0
* `comp`: output buffer
* `outlen`: output space in bytes, which receives the space left

Returns: the output length in bytes, or `(size_t)-1` for an odd, empty, or oversized input or with E2BIG

Calls: [do_precomposition](util_unistr.c.md#do_precomposition), [do_precomposition_sp](util_unistr.c.md#do_precomposition_sp)

Called by: [charset_precompose](charcnv.c.md#charset_precompose), [convert_charset](charcnv.c.md#convert_charset), [convert_string](charcnv.c.md#convert_string), [convert_string_allocate](charcnv.c.md#convert_string_allocate)

Uses file-scope variables: `base` in [libatalk/unicode/precompose.h](precompose.h.md), `base_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `comb` in [libatalk/unicode/precompose.h](precompose.h.md), `comb_sp` in [libatalk/unicode/precompose.h](precompose.h.md)

### decompose_w

```c
size_t decompose_w(ucs2_t *, size_t, ucs2_t *, size_t *)
```

Defined at lines 765 to 874. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Decompose a UTF-16 string by the netatalk tables.

The call splits each precomposed character into its base and its combining characters, Hangul syllables included, and surrogate pairs through a separate table. ASCII stays as is. The output ends with a NUL, which the returned length leaves out.

Parameters:
* `name`: UTF-16 input
* `inplen`: input length in bytes, an even number above 0
* `comp`: output buffer
* `outlen`: output space in bytes, which receives the space left

Returns: the output length in bytes, or `(size_t)-1` for an odd or empty input or with E2BIG

Calls: [do_decomposition](util_unistr.c.md#do_decomposition), [do_decomposition_sp](util_unistr.c.md#do_decomposition_sp)

Called by: [charset_decompose](charcnv.c.md#charset_decompose), [convert_charset](charcnv.c.md#convert_charset), [convert_string](charcnv.c.md#convert_string), [convert_string_allocate](charcnv.c.md#convert_string_allocate)

Uses file-scope variables: `base` in [libatalk/unicode/precompose.h](precompose.h.md), `base_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `comb` in [libatalk/unicode/precompose.h](precompose.h.md)
