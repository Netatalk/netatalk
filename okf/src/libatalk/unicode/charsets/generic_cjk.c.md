---
type: C Source File
title: "libatalk/unicode/charsets/generic_cjk.c"
description: "Shared conversion code of the CJK charsets."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/unicode/charsets/generic_cjk.c"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 47 to 58.

Convert the pending run from `*inbuf` to `end` with iconv.

A failed conversion resets the shift state of the descriptor.

Parameters:
* `cd`: iconv descriptor
* `inbuf`: start of the run, advanced past the converted bytes
* `end`: end of the run
* `outbuf`: output position
* `outbytesleft`: output space

Returns: the number of bytes of the run that stay unconverted

Called by: [cjk_generic_pull](generic_cjk.c.md#cjk_generic_pull), [cjk_generic_push](generic_cjk.c.md#cjk_generic_push)

### cjk_generic_push

```c
size_t cjk_generic_push(size_t(*char_func)(uint8_t *, const ucs2_t *, size_t *), void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 78 to 137.

Convert UCS2 to a CJK charset with a per-character table function and iconv for the rest.

The table function maps one character, or a composed sequence, to up to CJK_PUSH_BUFFER bytes. Characters that it does not know collect into a run, and iconv converts each run. The call follows the iconv calling convention.

Parameters:
* `char_func`: table function of the charset
* `cd`: iconv descriptor for the runs
* `inbuf`: UCS2 input position
* `inbytesleft`: input bytes left
* `outbuf`: output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with E2BIG, with EINVAL for a trailing odd byte, or with the error of a conversion

Calls: [cjk_iconv](generic_cjk.c.md#cjk_iconv)

Called by: [mac_chinese_simp_push](mac_chinese_simp.c.md#mac_chinese_simp_push), [mac_chinese_trad_push](mac_chinese_trad.c.md#mac_chinese_trad_push), [mac_japanese_push](mac_japanese.c.md#mac_japanese_push), [mac_korean_push](mac_korean.c.md#mac_korean_push)

### cjk_generic_pull

```c
size_t cjk_generic_pull(size_t(*char_func)(ucs2_t *, const uint8_t *, size_t *), void *cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
```

Defined at lines 156 to 215.

Convert a CJK charset to UCS2 with a per-character table function and iconv for the rest.

The table function maps one character of one or two bytes to up to CJK_PULL_BUFFER UCS2 characters. Bytes that it does not know collect into a run, and iconv converts each run. The call follows the iconv calling convention.

Parameters:
* `char_func`: table function of the charset
* `cd`: iconv descriptor for the runs
* `inbuf`: input position
* `inbytesleft`: input bytes left
* `outbuf`: UCS2 output position
* `outbytesleft`: output space

Returns: 0, or `(size_t)-1` with E2BIG or with the error of a conversion

Calls: [cjk_iconv](generic_cjk.c.md#cjk_iconv)

Called by: [mac_chinese_simp_pull](mac_chinese_simp.c.md#mac_chinese_simp_pull), [mac_chinese_trad_pull](mac_chinese_trad.c.md#mac_chinese_trad_pull), [mac_japanese_pull](mac_japanese.c.md#mac_japanese_pull), [mac_korean_pull](mac_korean.c.md#mac_korean_pull)

### cjk_char_push

```c
size_t cjk_char_push(uint16_t c, uint8_t *out)
```

Defined at lines 227 to 246.

Write a table value of a CJK charset as one or two bytes.

The value 0 means that the table has no entry, and 0xffff marks a character that the charset cannot hold.

Parameters:
* `c`: table value
* `out`: receives the bytes, bits 15 to 8 first

Returns: 1 or 2, 0 for no entry, or `(size_t)-1` with EILSEQ

Called by: [mac_chinese_simp_char_push](mac_chinese_simp.c.md#mac_chinese_simp_char_push), [mac_chinese_trad_char_push](mac_chinese_trad.c.md#mac_chinese_trad_char_push), [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)

### cjk_char_pull

```c
size_t cjk_char_pull(ucs2_t wc, ucs2_t *out, const uint32_t *compose)
```

Defined at lines 260 to 283.

Expand a table value of a CJK charset into UCS2 characters.

Values from 0xe000 to 0xefff index the composition table, whose entries chain a base character to combining characters. The call unrolls the chain into up to CJK_PULL_BUFFER characters.

Parameters:
* `wc`: table value
* `out`: receives the characters
* `compose`: composition table of the charset

Returns: the number of characters, or 0 for no entry

Called by: [mac_chinese_simp_char_pull](mac_chinese_simp.c.md#mac_chinese_simp_char_pull), [mac_chinese_trad_char_pull](mac_chinese_trad.c.md#mac_chinese_trad_char_pull), [mac_japanese_char_pull](mac_japanese.c.md#mac_japanese_char_pull), [mac_korean_char_pull](mac_korean.c.md#mac_korean_char_pull)

### cjk_lookup

```c
uint16_t cjk_lookup(uint16_t c, const cjk_index_t *index, const uint16_t *charset)
```

Defined at lines 296 to 322.

Look up a character in the sparse table of a CJK charset.

Each index range points to summary pairs that cover 16 characters each: a bitmap of the present ones and the offset of the first in `charset`.

Parameters:
* `c`: character to look up
* `index`: ranges of the table, ended by a NULL summary
* `charset`: packed table values

Returns: the table value, or 0 for no entry

Called by: [mac_chinese_simp_char_pull](mac_chinese_simp.c.md#mac_chinese_simp_char_pull), [mac_chinese_simp_char_push](mac_chinese_simp.c.md#mac_chinese_simp_char_push), [mac_chinese_trad_char_pull](mac_chinese_trad.c.md#mac_chinese_trad_char_pull), [mac_chinese_trad_char_push](mac_chinese_trad.c.md#mac_chinese_trad_char_push), [mac_japanese_char_pull](mac_japanese.c.md#mac_japanese_char_pull), [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push), [mac_korean_char_pull](mac_korean.c.md#mac_korean_char_pull), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)

### cjk_compose

```c
ucs2_t cjk_compose(ucs2_t base, ucs2_t comb, const uint32_t *table, size_t size)
```

Defined at lines 336 to 356.

Find the composed private-use character for a base and a combining character.

The call searches the sorted composition table for the pair.

Parameters:
* `base`: base character, which can itself be a composed one
* `comb`: combining character
* `table`: sorted composition table
* `size`: number of table entries

Returns: 0xe000 plus the table index, or 0 for no match

Called by: [cjk_compose_seq](generic_cjk.c.md#cjk_compose_seq), [mac_chinese_simp_char_push](mac_chinese_simp.c.md#mac_chinese_simp_char_push), [mac_chinese_trad_char_push](mac_chinese_trad.c.md#mac_chinese_trad_char_push), [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)

### cjk_compose_seq

```c
ucs2_t cjk_compose_seq(const ucs2_t *in, size_t *len, const uint32_t *table, size_t size)
```

Defined at lines 372 to 396.

Compose the sequence at `in` into one private-use character.

The low 3 bits of the first character pick the sequence length, from 3 to 5 characters.

Parameters:
* `in`: input characters
* `len`: characters available, which receives the sequence length
* `table`: sorted composition table
* `size`: number of table entries

Returns: the composed character, or 0 with EINVAL for a short input or with EILSEQ for no match

Calls: [cjk_compose](generic_cjk.c.md#cjk_compose)

Called by: [mac_japanese_char_push](mac_japanese.c.md#mac_japanese_char_push), [mac_korean_char_push](mac_korean.c.md#mac_korean_char_push)
