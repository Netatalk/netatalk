---
type: C Source File
title: "libatalk/unicode/util_unistr.c"
description: "21 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/util_unistr.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 34 to 64. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Convert a string to lower case.

Returns: True if any char is converted

Note: surrogate pair support

Calls: [tolower_sp](utf16_case.c.md#tolower_sp), [tolower_w](utf16_case.c.md#tolower_w)

Called by: [charset_strlower](charcnv.c.md#charset_strlower), [convert_charset](charcnv.c.md#convert_charset)

### strupper_w

```c
int strupper_w(ucs2_t *)
```

Defined at lines 71 to 101. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Convert a string to upper case.

Returns: True if any char is converted

Note: surrogate pair support

Calls: [toupper_sp](utf16_case.c.md#toupper_sp), [toupper_w](utf16_case.c.md#toupper_w)

Called by: [charset_strupper](charcnv.c.md#charset_strupper), [convert_charset](charcnv.c.md#convert_charset)

### strlen_w

```c
size_t strlen_w(const ucs2_t *)
```

Defined at lines 109 to 116. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strlen()

Count the number of characters in a UTF-16 string.

Note: one surrogate pair is two characters.

Called by: [convert_string_internal](charcnv.c.md#convert_string_internal), [strcasestr_w](util_unistr.c.md#strcasestr_w), [strndup_w](util_unistr.c.md#strndup_w), [strstr_w](util_unistr.c.md#strstr_w), [ucs2_to_charset](charcnv.c.md#ucs2_to_charset), [ucs2_to_charset_allocate](charcnv.c.md#ucs2_to_charset_allocate)

### strnlen_w

```c
size_t strnlen_w(const ucs2_t *, size_t)
```

Defined at lines 125 to 132. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide [strnlen()](../compat/misc.c.md#strnlen)

Count up to max number of characters in a UTF-16 string.

Note: one surrogate pair is two characters.

### strchr_w

```c
ucs2_t * strchr_w(const ucs2_t *, ucs2_t)
```

Defined at lines 138 to 153. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strchr()

Note: hi and lo of surrogate pair are separately processed.

Called by: [strstr_w](util_unistr.c.md#strstr_w)

### strcasechr_w

```c
ucs2_t * strcasechr_w(const ucs2_t *s, ucs2_t c)
```

Defined at lines 159 to 174. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strcasechr()

Note: separately process BMP and surrogate pair

Calls: [tolower_w](utf16_case.c.md#tolower_w)

Called by: [strcasestr_w](util_unistr.c.md#strcasestr_w)

### strcasechr_sp

```c
ucs2_t * strcasechr_sp(const ucs2_t *s, uint32_t c_sp)
```

Defined at lines 180 to 195.

sp strcasechr()

Note: separately process BMP and surrogate pair

Calls: [tolower_sp](utf16_case.c.md#tolower_sp)

Called by: [strcasestr_w](util_unistr.c.md#strcasestr_w)

### strcmp_w

```c
int strcmp_w(const ucs2_t *, const ucs2_t *)
```

Defined at lines 201 to 212. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strcmp()

Note: no problem of surrogate pair

### strncmp_w

```c
int strncmp_w(const ucs2_t *, const ucs2_t *, size_t)
```

Defined at lines 218 to 229. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strncmp()

Note: no problem of surrogate pair

Called by: [strstr_w](util_unistr.c.md#strstr_w)

### strstr_w

```c
ucs2_t * strstr_w(const ucs2_t *s, const ucs2_t *ins)
```

Defined at lines 235 to 256. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strstr()

Note: no problem of surrogate pair

Calls: [strchr_w](util_unistr.c.md#strchr_w), [strlen_w](util_unistr.c.md#strlen_w), [strncmp_w](util_unistr.c.md#strncmp_w)

### strcasestr_w

```c
ucs2_t * strcasestr_w(const ucs2_t *, const ucs2_t *)
```

Defined at lines 262 to 299. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strcasestr()

Note: surrogate pair support

Calls: [strcasechr_sp](util_unistr.c.md#strcasechr_sp), [strcasechr_w](util_unistr.c.md#strcasechr_w), [strlen_w](util_unistr.c.md#strlen_w), [strncasecmp_w](util_unistr.c.md#strncasecmp_w)

Called by: [crit_check](../../etc/afpd/catsearch.c.md#crit_check)

### strcasecmp_w

```c
int strcasecmp_w(const ucs2_t *, const ucs2_t *)
```

Defined at lines 307 to 335. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strcasecmp()

case insensitive string comparison

Note: surrogate pair support

Calls: [tolower_sp](utf16_case.c.md#tolower_sp), [tolower_w](utf16_case.c.md#tolower_w)

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [crit_check](../../etc/afpd/catsearch.c.md#crit_check)

### strncasecmp_w

```c
int strncasecmp_w(const ucs2_t *, const ucs2_t *, size_t)
```

Defined at lines 343 to 374. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strncasecmp()

case insensitive string comparison, length limited

Note: compare up to 'len+1' if 'len' isolate surrogate pair

Calls: [tolower_sp](utf16_case.c.md#tolower_sp), [tolower_w](utf16_case.c.md#tolower_w)

Called by: [strcasestr_w](util_unistr.c.md#strcasestr_w), [uam_getname](../../etc/afpd/uam.c.md#uam_getname)

### strndup_w

```c
ucs2_t * strndup_w(const ucs2_t *, size_t)
```

Defined at lines 383 to 401. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strndup()

duplicate string

Note: not check isolation of surrogate pair

Note: if len == 0 then duplicate the whole string

Calls: [strlen_w](util_unistr.c.md#strlen_w)

Called by: [strdup_w](util_unistr.c.md#strdup_w)

### strdup_w

```c
ucs2_t * strdup_w(const ucs2_t *)
```

Defined at lines 409 to 412. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

wide strdup()

duplicate string

Note: no problem of surrogate pair

Calls: [strndup_w](util_unistr.c.md#strndup_w)

Called by: [creatvol](../util/netatalk_conf.c.md#creatvol)

### do_precomposition

```c
static ucs2_t do_precomposition(unsigned int base, unsigned int comb)
```

Defined at lines 417 to 440.

binary search for pre|decomposition

Called by: [precompose_w](util_unistr.c.md#precompose_w)

Uses file-scope variables: `comb` in [libatalk/unicode/precompose.h](precompose.h.md), `precompositions` in [libatalk/unicode/precompose.h](precompose.h.md)

### do_precomposition_sp

```c
static uint32_t do_precomposition_sp(unsigned int base_sp, unsigned int comb_sp)
```

Defined at lines 443 to 467.

Called by: [precompose_w](util_unistr.c.md#precompose_w)

Uses file-scope variables: `base_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `comb_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `precompositions_sp` in [libatalk/unicode/precompose.h](precompose.h.md)

### do_decomposition

```c
static uint32_t do_decomposition(ucs2_t base)
```

Defined at lines 470 to 495.

Called by: [decompose_w](util_unistr.c.md#decompose_w)

Uses file-scope variables: `comb` in [libatalk/unicode/precompose.h](precompose.h.md), `decompositions` in [libatalk/unicode/precompose.h](precompose.h.md)

### do_decomposition_sp

```c
static uint64_t do_decomposition_sp(unsigned int base_sp)
```

Defined at lines 498 to 525.

Called by: [decompose_w](util_unistr.c.md#decompose_w)

Uses file-scope variables: `base_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `comb_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `decompositions_sp` in [libatalk/unicode/precompose.h](precompose.h.md)

### precompose_w

```c
size_t precompose_w(ucs2_t *, size_t, ucs2_t *, size_t *)
```

Defined at lines 541 to 650. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

pre|decomposition

we can't use static, this stuff needs to be reentrant static char comp[MAXPATHLEN +1];

We don't implement Singleton and Canonical Ordering. We ignore CompositionExclusions.txt because they cause the problem of the roundtrip such as Dancing Icon.

exclude U2000-U2FFF, UFE30-UFE4F and U2F800-U2FA1F ranges in [precompose.h](precompose.h.md) from composition according to AFP 3.x spec

Calls: [do_precomposition](util_unistr.c.md#do_precomposition), [do_precomposition_sp](util_unistr.c.md#do_precomposition_sp)

Called by: [charset_precompose](charcnv.c.md#charset_precompose), [convert_charset](charcnv.c.md#convert_charset), [convert_string](charcnv.c.md#convert_string), [convert_string_allocate](charcnv.c.md#convert_string_allocate)

Uses file-scope variables: `base_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `comb` in [libatalk/unicode/precompose.h](precompose.h.md), `comb_sp` in [libatalk/unicode/precompose.h](precompose.h.md)

### decompose_w

```c
size_t decompose_w(ucs2_t *, size_t, ucs2_t *, size_t *)
```

Defined at lines 653 to 762. Declared in [include/atalk/unicode.h](../../include/atalk/unicode.h.md).

Calls: [do_decomposition](util_unistr.c.md#do_decomposition), [do_decomposition_sp](util_unistr.c.md#do_decomposition_sp)

Called by: [charset_decompose](charcnv.c.md#charset_decompose), [convert_charset](charcnv.c.md#convert_charset), [convert_string](charcnv.c.md#convert_string), [convert_string_allocate](charcnv.c.md#convert_string_allocate)

Uses file-scope variables: `base_sp` in [libatalk/unicode/precompose.h](precompose.h.md), `comb` in [libatalk/unicode/precompose.h](precompose.h.md)
