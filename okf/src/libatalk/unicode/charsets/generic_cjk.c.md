---
type: C Source File
title: "libatalk/unicode/charsets/generic_cjk.c"
description: "8 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/generic_cjk.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/unicode](../../unicode.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [generic_cjk.h](generic_cjk.h.md)
* System headers: `string.h`

# Functions

### cjk_iconv

```c
static size_t cjk_iconv(void *cd, char **inbuf, char *end, char **outbuf, size_t *outbytesleft)
```

Defined at lines 29 to 40.

Called by: [cjk_generic_pull](generic_cjk.c.md#cjk_generic_pull), [cjk_generic_push](generic_cjk.c.md#cjk_generic_push)

### cjk_generic_push

```c
size_t cjk_generic_push(size_t(*char_func)(uint8_t *, const ucs2_t *, size_t *), void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 42 to 101.

Calls: [cjk_iconv](generic_cjk.c.md#cjk_iconv)

Called by: [mac_chinese_simp_push](mac_chinese_simp.c.md#mac_chinese_simp_push), [mac_chinese_trad_push](mac_chinese_trad.c.md#mac_chinese_trad_push), [mac_japanese_push](mac_japanese.c.md#mac_japanese_push), [mac_korean_push](mac_korean.c.md#mac_korean_push)

### cjk_generic_pull

```c
size_t cjk_generic_pull(size_t(*char_func)(ucs2_t *, const uint8_t *, size_t *), void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 103 to 162.

Calls: [cjk_iconv](generic_cjk.c.md#cjk_iconv)

Called by: [mac_chinese_simp_pull](mac_chinese_simp.c.md#mac_chinese_simp_pull), [mac_chinese_trad_pull](mac_chinese_trad.c.md#mac_chinese_trad_pull), [mac_japanese_pull](mac_japanese.c.md#mac_japanese_pull), [mac_korean_pull](mac_korean.c.md#mac_korean_pull)

### cjk_char_push

```c
size_t cjk_char_push(uint16_t c, uint8_t *out)
```

Defined at lines 164 to 183.

Called by: [mac_chinese_simp_char_push](mac_chinese_simp.c.md#mac_chinese_simp_char_push), [mac_chinese_trad_char_push](mac_chinese_trad.c.md#mac_chinese_trad_char_push), [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)

### cjk_char_pull

```c
size_t cjk_char_pull(ucs2_t wc, ucs2_t *out, const uint32_t *compose)
```

Defined at lines 185 to 208.

Called by: [mac_chinese_simp_char_pull](mac_chinese_simp.c.md#mac_chinese_simp_char_pull), [mac_chinese_trad_char_pull](mac_chinese_trad.c.md#mac_chinese_trad_char_pull), [mac_japanese_char_pull](mac_japanese.c.md#mac_japanese_char_pull), [mac_korean_char_pull](mac_korean.c.md#mac_korean_char_pull)

### cjk_lookup

```c
uint16_t cjk_lookup(uint16_t c, const cjk_index_t *index, const uint16_t *charset)
```

Defined at lines 210 to 236.

Called by: [mac_chinese_simp_char_pull](mac_chinese_simp.c.md#mac_chinese_simp_char_pull), [mac_chinese_simp_char_push](mac_chinese_simp.c.md#mac_chinese_simp_char_push), [mac_chinese_trad_char_pull](mac_chinese_trad.c.md#mac_chinese_trad_char_pull), [mac_chinese_trad_char_push](mac_chinese_trad.c.md#mac_chinese_trad_char_push), [mac_japanese_char_pull](mac_japanese.c.md#mac_japanese_char_pull), [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push), [mac_korean_char_pull](mac_korean.c.md#mac_korean_char_pull), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)

### cjk_compose

```c
ucs2_t cjk_compose(ucs2_t base, ucs2_t comb, const uint32_t *table, size_t size)
```

Defined at lines 238 to 258.

Called by: [cjk_compose_seq](generic_cjk.c.md#cjk_compose_seq), [mac_chinese_simp_char_push](mac_chinese_simp.c.md#mac_chinese_simp_char_push), [mac_chinese_trad_char_push](mac_chinese_trad.c.md#mac_chinese_trad_char_push), [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)

### cjk_compose_seq

```c
ucs2_t cjk_compose_seq(const ucs2_t *in, size_t *len, const uint32_t *table, size_t size)
```

Defined at lines 260 to 284.

Calls: [cjk_compose](generic_cjk.c.md#cjk_compose)

Called by: [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)
