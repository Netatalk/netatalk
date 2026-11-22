---
type: C Source File
title: "libatalk/util/sl_names.c"
description: "Spotlight name comparisons: reading a query and matching a name."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/sl_names.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-11-22T14:39:48+01:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/sl_names.h](../../include/atalk/sl_names.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* System headers: `ctype.h`, `stdbool.h`, `stdint.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### sl_name_converts

```c
static bool sl_name_converts(charset_t charset)
```

Defined at lines 115 to 118.

Tell whether text of a charset reads as UTF-8 only once converted.

UTF-8 and UTF8-MAC read as they are. UCS-2 never holds a name, so a query that names no charset, its root zeroed, takes it for UTF-8 too.

Parameters:
* `charset`: the charset

Returns: true for every other charset

Called by: [sl_name_fold_in](sl_names.c.md#sl_name_fold_in), [sl_name_query_match](sl_names.c.md#sl_name_query_match), [sl_name_query_parse_in](sl_names.c.md#sl_name_query_parse_in), [sl_name_query_term_in](sl_names.c.md#sl_name_query_term_in)

### sl_name_mark

```c
static bool sl_name_mark(ucs2_t u)
```

Defined at lines 131 to 146.

Tell whether a unit is a combining mark that 'd' drops.

The Hebrew and Arabic ranges are Unicode 17.0's nonspacing marks of those blocks: the points, the cantillation marks and the harakat, and the hamza and madda of a letter that decomposes with them, so أ folds to ا.

Parameters:
* `u`: UTF-16 unit

Returns: true for a combining diacritical mark, a kana voicing mark, or a nonspacing mark of Hebrew or Arabic

Called by: [sl_name_fold_units](sl_names.c.md#sl_name_fold_units), [sl_name_joins](sl_names.c.md#sl_name_joins)

Mentioned in the documentation of: [sl_name_fold_units](sl_names.c.md#sl_name_fold_units), [sl_name_joins](sl_names.c.md#sl_name_joins)

### sl_name_joins

```c
static bool sl_name_joins(ucs2_t u)
```

Defined at lines 155 to 170.

Tell whether a unit joins the character before it.

Parameters:
* `u`: UTF-16 unit

Returns: true for a mark [sl_name_mark()](sl_names.c.md#sl_name_mark) knows, or a combining mark of Thai, Lao, Myanmar or Khmer

Calls: [sl_name_mark](sl_names.c.md#sl_name_mark)

Called by: [sl_name_boundary](sl_names.c.md#sl_name_boundary), [sl_name_char_end](sl_names.c.md#sl_name_char_end), [sl_name_glob](sl_names.c.md#sl_name_glob)

### sl_name_next

```c
static uint32_t sl_name_next(const unsigned char **p, const unsigned char *end)
```

Defined at lines 182 to 219.

Decode the next UTF-8 character.

A byte that starts no valid sequence in its shortest form decodes alone, as U+DC80 to U+DCFF, so the bytes of a name that is not UTF-8 stay apart.

Parameters:
* `p`: position, moved past the character
* `end`: end of the string

Returns: the code point

Called by: [sl_name_fold_units](sl_names.c.md#sl_name_fold_units), [sl_name_pattern_new](sl_names.c.md#sl_name_pattern_new)

### sl_name_case

```c
static ucs2_t sl_name_case(ucs2_t u)
```

Defined at lines 232 to 301.

Fold the case of a unit.

The tree's lowercase map leaves a few letters apart from the ones they fold to: the micro sign, long s, final sigma, the Greek symbol forms of beta, theta, phi, pi, kappa, rho and epsilon, the Cyrillic small letter variants, long s with dot above and Greek prosgegrammeni.

Parameters:
* `u`: UTF-16 unit

Returns: the unit that a letter of either case folds to

Calls: [tolower_w](../unicode/utf16_case.c.md#tolower_w)

Called by: [sl_name_fold_units](sl_names.c.md#sl_name_fold_units)

### sl_name_atomic

```c
static bool sl_name_atomic(ucs2_t u)
```

Defined at lines 313 to 316.

Tell whether a unit is a CJK ideograph or a Hangul syllable.

A comparison needs neither decomposed: an ideograph has no canonical decomposition, a syllable composes again from its jamo, and no composition takes either as its second character.

Parameters:
* `u`: the unit

Returns: true for U+3400 to U+9FFF and U+AC00 to U+D7A3

Called by: [sl_name_fold_units](sl_names.c.md#sl_name_fold_units)

### sl_name_fold_units

```c
static size_t sl_name_fold_units(const char *str, size_t len, unsigned int flags, ucs2_t *out, ucs2_t *tmp)
```

Defined at lines 343 to 428.

Fold UTF-8 characters into UTF-16 units.

SL_NAMES_CASE folds the case of each character first, so a letter that lowers to a precomposed one decomposes as that one does; I with dot above lowers to i and its dot, as its decomposition does. SL_NAMES_DIACRITIC then decomposes each character but a CJK ideograph or a Hangul syllable and drops the marks [sl_name_mark()](sl_names.c.md#sl_name_mark) knows. A variation selector only picks a glyph and folds away, so ❤ and its emoji form ❤️ fold alike. The units then compose, so a name stored precomposed and one stored decomposed in canonical order fold alike; [precompose_w()](../unicode/util_unistr.c.md#precompose_w) composes neighbours only, so marks in another order, or a letter stored part composed, fold apart wherever the marks stay. Units below U+0300, ideographs and syllables never compose with the unit before them, so a fold of those alone skips composing. Under SL_NAME_INDEX a Hangul syllable decomposes too and nothing composes: a dropped mark could otherwise let its neighbours compose, which a run of a value, folded alone, does not.

Parameters:
* `str`: the characters
* `len`: bytes of str
* `flags`: SL_NAMES_CASE, SL_NAMES_DIACRITIC and SL_NAME_INDEX
* `out`: receives the units, 2 * len + SL_NAME_UNITS of room
* `tmp`: scratch of the same room

Returns: the number of units

Calls: [decompose_w](../unicode/util_unistr.c.md#decompose_w), [precompose_w](../unicode/util_unistr.c.md#precompose_w), [sl_name_atomic](sl_names.c.md#sl_name_atomic), [sl_name_case](sl_names.c.md#sl_name_case), [sl_name_mark](sl_names.c.md#sl_name_mark), [sl_name_next](sl_names.c.md#sl_name_next), [tolower_sp](../unicode/utf16_case.c.md#tolower_sp)

Called by: [sl_name_flush](sl_names.c.md#sl_name_flush), [sl_name_fold](sl_names.c.md#sl_name_fold), [sl_name_folds_match](sl_names.c.md#sl_name_folds_match), [sl_name_pattern_match](sl_names.c.md#sl_name_pattern_match)

### sl_name_char_end

```c
static size_t sl_name_char_end(const ucs2_t *s, size_t i, size_t n)
```

Defined at lines 438 to 447.

Step past a unit or a surrogate pair and the units that join it.

Parameters:
* `s`: folded units
* `i`: start of the character, below n
* `n`: number of units

Returns: the position after the character

Calls: [sl_name_joins](sl_names.c.md#sl_name_joins)

Called by: [sl_name_glob](sl_names.c.md#sl_name_glob)

### sl_name_alnum

```c
static bool sl_name_alnum(ucs2_t u)
```

Defined at lines 570 to 593.

Tell whether a folded unit belongs to a word.

Parameters:
* `u`: UTF-16 unit

Returns: true for an ASCII letter or digit, and for a non-ASCII unit that sl_name_nonword holds no range of: a letter, a mark, a number, a surrogate, a private-use unit or one Unicode leaves unassigned

Called by: [sl_name_word](sl_names.c.md#sl_name_word)

Uses file-scope variables: `sl_name_nonword`

### sl_name_unspaced

```c
static bool sl_name_unspaced(ucs2_t u)
```

Defined at lines 604 to 616.

Tell whether a folded unit starts a character of an unspaced script.

Parameters:
* `u`: UTF-16 unit

Returns: true for Thai, Lao, Myanmar, Khmer, kana, Bopomofo, the CJK ideographs and the fullwidth and halfwidth forms; Hangul's compatibility and halfwidth jamo stay out, as Korean spaces its words

Called by: [sl_name_boundary](sl_names.c.md#sl_name_boundary)

### sl_name_pair_tail

```c
static bool sl_name_pair_tail(const ucs2_t *s, size_t i)
```

Defined at lines 625 to 629.

Tell whether a folded unit is the low half of a surrogate pair.

Parameters:
* `s`: folded units
* `i`: position

Returns: true for a low surrogate after a high one

Called by: [sl_name_boundary](sl_names.c.md#sl_name_boundary), [sl_name_flush](sl_names.c.md#sl_name_flush), [sl_name_word](sl_names.c.md#sl_name_word)

### sl_name_word

```c
static bool sl_name_word(const ucs2_t *s, size_t i, size_t n)
```

Defined at lines 643 to 668.

Tell whether the character at a position belongs to a word.

A surrogate pair takes the class of its code point, so an emoji ends a word as a BMP symbol does.

Parameters:
* `s`: folded units
* `i`: start of the character, below n
* `n`: number of units

Returns: true for a letter, a mark, a number, a surrogate, a private-use or an unassigned code point

Calls: [sl_name_alnum](sl_names.c.md#sl_name_alnum), [sl_name_pair_tail](sl_names.c.md#sl_name_pair_tail)

Called by: [sl_name_boundary](sl_names.c.md#sl_name_boundary)

Uses file-scope variables: `sl_name_nonword_sp`

### sl_name_boundary

```c
static bool sl_name_boundary(const ucs2_t *s, size_t i, size_t n)
```

Defined at lines 685 to 705.

Tell whether a position of folded units is a word boundary.

Inside the text of a script written without spaces every character starts a word, as a search for a word within such text expects. A character of no word stands alone, so "(2)" starts after the space of "photo (2).jpg". The character before a position takes the class of its base, whatever marks join it.

Parameters:
* `s`: folded units
* `i`: position
* `n`: number of units

Returns: true at either end, and between two characters when either is of no word or of a script written without spaces

Calls: [sl_name_joins](sl_names.c.md#sl_name_joins), [sl_name_pair_tail](sl_names.c.md#sl_name_pair_tail), [sl_name_unspaced](sl_names.c.md#sl_name_unspaced), [sl_name_word](sl_names.c.md#sl_name_word)

Called by: [sl_name_glob](sl_names.c.md#sl_name_glob)

### sl_name_glob

```c
static bool sl_name_glob(const struct sl_name_pattern *pat, const ucs2_t *s, size_t n, bool word)
```

Defined at lines 722 to 763.

Match a value against folded units.

The value takes every unit up to the end, or with `word` up to any word boundary, and an anchored start with `word` may fall on any word boundary. A '*' backtracks one whole character at a time, and a start moves to the next word boundary. A literal run cannot end on a character that a joining unit follows.

Parameters:
* `pat`: the value
* `s`: folded units
* `n`: number of units
* `word`: the value's anchored ends fall on word boundaries

Returns: true when the value matches

Calls: [sl_name_boundary](sl_names.c.md#sl_name_boundary), [sl_name_char_end](sl_names.c.md#sl_name_char_end), [sl_name_joins](sl_names.c.md#sl_name_joins)

Called by: [sl_name_units_match](sl_names.c.md#sl_name_units_match)

### sl_name_put_utf8

```c
static char * sl_name_put_utf8(char *o, const ucs2_t *u, size_t n)
```

Defined at lines 779 to 808.

Write folded units as UTF-8, a stray byte's unit as U+FFFD.

U+DC80 to U+DCFF stand for the bytes that start no UTF-8 character. The index text and its patterns then hold whole characters only, which SQLite's GLOB and trigrams read one by one, as the matcher reads the units; a stray byte written as itself would merge with the bytes after it.

Parameters:
* `o`: output position
* `u`: the units
* `n`: number of units

Returns: the output position after them

Called by: [sl_name_flush](sl_names.c.md#sl_name_flush), [sl_name_fold](sl_names.c.md#sl_name_fold)

### sl_name_flush

```c
static void sl_name_flush(struct sl_name_pattern *pat, const char *run, size_t *runlen, ucs2_t *units, ucs2_t *tmp, char **gp, char **ip)
```

Defined at lines 822 to 865.

Fold a run of literal characters into tokens and the index patterns.

Parameters:
* `pat`: the value, receiving the tokens and their folded bytes
* `run`: the characters
* `runlen`: bytes of run, reset to 0
* `units`: scratch of 2 * run bytes + SL_NAME_UNITS units
* `tmp`: scratch of the same room
* `gp`: end of the GLOB index pattern, moved past the run
* `ip`: end of the LIKE index pattern, moved past the run

Calls: [sl_name_fold_units](sl_names.c.md#sl_name_fold_units), [sl_name_pair_tail](sl_names.c.md#sl_name_pair_tail), [sl_name_put_utf8](sl_names.c.md#sl_name_put_utf8)

Called by: [sl_name_pattern_new](sl_names.c.md#sl_name_pattern_new)

### sl_name_pattern_new

```c
struct sl_name_pattern * sl_name_pattern_new(const char *value, size_t len, unsigned int flags)
```

Defined at lines 883 to 1006.

Compile a value with its modifiers.

The LIKE pattern keeps the value's literal ASCII characters but letters, each '', '_' and '!' escaped with '!', between '': a collation can fold a letter otherwise than the matcher, or read two letters as one, but compares the others as themselves, and the matcher drops variation selectors, and under SL_NAMES_DIACRITIC combining marks, between any two characters. So every match satisfies the pattern under any collation.

Parameters:
* `value`: the value between its quotes, escapes included
* `len`: bytes of value
* `flags`: SL_NAMES_* flags

Returns: the comparison, to [sl_name_pattern_free()](sl_names.c.md#sl_name_pattern_free), or NULL when out of memory

Calls: [sl_name_flush](sl_names.c.md#sl_name_flush), [sl_name_next](sl_names.c.md#sl_name_next), [sl_name_pattern_free](sl_names.c.md#sl_name_pattern_free)

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison), [sl_name_query_value](sl_names.c.md#sl_name_query_value)

### sl_name_pattern_free

```c
void sl_name_pattern_free(struct sl_name_pattern *pattern)
```

Defined at lines 1013 to 1021.

Free a compiled name comparison.

Parameters:
* `pattern`: the comparison, or NULL

Called by: [sl_name_pattern_new](sl_names.c.md#sl_name_pattern_new), [sl_name_query_free](sl_names.c.md#sl_name_query_free), [sl_name_query_parse_in](sl_names.c.md#sl_name_query_parse_in)

Mentioned in the documentation of: [sl_name_pattern_new](sl_names.c.md#sl_name_pattern_new)

### sl_name_holds

```c
static bool sl_name_holds(const struct sl_name_pattern *pat, const ucs2_t *s, size_t n)
```

Defined at lines 1032 to 1050.

Tell whether folded units hold a value's longest literal run.

Parameters:
* `pat`: the value
* `s`: folded units
* `n`: number of units

Returns: true when the run occurs, as it does in every match, or when the value has no literal

Called by: [sl_name_units_match](sl_names.c.md#sl_name_units_match)

### sl_name_pattern_negated

```c
static bool sl_name_pattern_negated(const struct sl_name_pattern *pattern)
```

Defined at lines 1058 to 1061.

Tell whether a compiled comparison is a !=.

Parameters:
* `pattern`: the comparison

Returns: true for SL_NAMES_NEGATE

Called by: [sl_name_pattern_match](sl_names.c.md#sl_name_pattern_match), [sl_name_seed](sl_names.c.md#sl_name_seed), [sl_name_units_match](sl_names.c.md#sl_name_units_match)

### sl_name_units_match

```c
static bool sl_name_units_match(const struct sl_name_pattern *pattern, const ucs2_t *s, size_t n)
```

Defined at lines 1071 to 1078.

Tell whether a folded name matches a compiled comparison.

Parameters:
* `pattern`: the comparison
* `s`: the name's units, folded as the comparison asks
* `n`: number of units

Returns: whether the name matches, the opposite for SL_NAMES_NEGATE

Calls: [sl_name_glob](sl_names.c.md#sl_name_glob), [sl_name_holds](sl_names.c.md#sl_name_holds), [sl_name_pattern_negated](sl_names.c.md#sl_name_pattern_negated)

Called by: [sl_name_folds_match](sl_names.c.md#sl_name_folds_match), [sl_name_pattern_match](sl_names.c.md#sl_name_pattern_match)

### sl_name_pattern_match

```c
bool sl_name_pattern_match(const struct sl_name_pattern *pattern, const char *name, size_t len)
```

Defined at lines 1092 to 1105.

Tell whether a name matches a compiled comparison.

With SL_NAMES_WORD an end of the value without a wildcard falls on a word boundary of the name instead of its end. A name over MAXPATHLEN bytes matches no comparison but a !=.

Parameters:
* `pattern`: the comparison
* `name`: UTF-8 name
* `len`: bytes of name

Returns: whether the name matches, the opposite for SL_NAMES_NEGATE

Calls: [sl_name_fold_units](sl_names.c.md#sl_name_fold_units), [sl_name_pattern_negated](sl_names.c.md#sl_name_pattern_negated), [sl_name_units_match](sl_names.c.md#sl_name_units_match)

Called by: [sl_name_folds_match](sl_names.c.md#sl_name_folds_match)

### sl_name_folds_match

```c
static bool sl_name_folds_match(struct sl_name_folds *f, const struct sl_name_pattern *pattern)
```

Defined at lines 1133 to 1148.

Tell whether the name of `f` matches a compiled comparison.

The folding for the comparison's modifiers is made once and kept in `f`, but for a name over SL_NAME_KEEP bytes, which folds anew.

Parameters:
* `f`: the name and its foldings so far
* `pattern`: the comparison

Returns: whether the name matches, the opposite for SL_NAMES_NEGATE

Calls: [sl_name_fold_units](sl_names.c.md#sl_name_fold_units), [sl_name_pattern_match](sl_names.c.md#sl_name_pattern_match), [sl_name_units_match](sl_names.c.md#sl_name_units_match)

Called by: [sl_name_node_match](sl_names.c.md#sl_name_node_match)

### sl_name_pattern_literals

```c
size_t sl_name_pattern_literals(const struct sl_name_pattern *pattern)
```

Defined at lines 1157 to 1160.

Bytes of a compiled comparison's literals, as its modifiers fold them.

Parameters:
* `pattern`: the comparison

Returns: the UTF-8 bytes, escapes, wildcards and the marks 'd' drops left out

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison)

### sl_name_fold

```c
char * sl_name_fold(const char *name, size_t len)
```

Defined at lines 1169 to 1188.

Fold a name for case and diacritics, decomposed, as the index does.

Parameters:
* `name`: UTF-8 name
* `len`: bytes of name

Returns: the folded UTF-8 name, to free(), or NULL when out of memory

Calls: [sl_name_fold_units](sl_names.c.md#sl_name_fold_units), [sl_name_put_utf8](sl_names.c.md#sl_name_put_utf8)

Called by: [sl_name_fold_in](sl_names.c.md#sl_name_fold_in)

Mentioned in the documentation of: [sl_name_fold_in](sl_names.c.md#sl_name_fold_in)

### sl_name_fold_in

```c
char * sl_name_fold_in(const char *name, size_t len, charset_t charset)
```

Defined at lines 1199 to 1211.

Fold a name of a charset as [sl_name_fold()](sl_names.c.md#sl_name_fold) folds its UTF-8.

Parameters:
* `name`: the name
* `len`: bytes of name
* `charset`: its charset; a name that does not convert folds its bytes

Returns: the folded name, to free(), or NULL when out of memory

Calls: [convert_string_quiet](../unicode/charcnv.c.md#convert_string_quiet), [sl_name_converts](sl_names.c.md#sl_name_converts), [sl_name_fold](sl_names.c.md#sl_name_fold)

Called by: [cnid_mysql_fold_param](../cnid/cnid_mysql.c.md#cnid_mysql_fold_param), [cnid_sqlite_fold](../cnid/cnid_sqlite.c.md#cnid_sqlite_fold)

### sl_name_pattern_index_glob

```c
const char * sl_name_pattern_index_glob(const struct sl_name_pattern *pattern)
```

Defined at lines 1223 to 1226.

The GLOB pattern that the folded name of every match satisfies.

'*' joins the value's runs of folded literal characters, so a '' or a '_' in them compares as itself; a '*', '?' or '[' in them stands as '?'. An end of the value that a wildcard or w leaves open takes '*' as well.

Parameters:
* `pattern`: the comparison

Returns: the pattern, "*" for a !=, which lasts as long as `pattern`

Called by: [cnid_sqlite_candidates](../cnid/cnid_sqlite.c.md#cnid_sqlite_candidates), [cnid_sqlite_find_bind](../cnid/cnid_sqlite.c.md#cnid_sqlite_find_bind), [cnid_sqlite_indexable](../cnid/cnid_sqlite.c.md#cnid_sqlite_indexable)

### sl_name_pattern_index_like

```c
const char * sl_name_pattern_index_like(const struct sl_name_pattern *pattern)
```

Defined at lines 1239 to 1242.

The LIKE pattern, ESCAPE '!

', that every match's folded name satisfies

'' joins the value's runs of folded literal characters, each '', '_' and '!' of them escaped with '!', for a column of folded names that compares bytes. An end of the value that a wildcard or w leaves open takes '' as well.

Parameters:
* `pattern`: the comparison

Returns: the pattern, "%" for a !=, which lasts as long as `pattern`

Called by: [cnid_mysql_find](../cnid/cnid_mysql.c.md#cnid_mysql_find), [cnid_mysql_folded_narrows](../cnid/cnid_mysql.c.md#cnid_mysql_folded_narrows), [cnid_mysql_like](../cnid/cnid_mysql.c.md#cnid_mysql_like)

### sl_name_pattern_like

```c
const char * sl_name_pattern_like(const struct sl_name_pattern *pattern)
```

Defined at lines 1250 to 1253.

The LIKE pattern, ESCAPE '!

', every match satisfies under any collation

Parameters:
* `pattern`: the comparison

Returns: the pattern, "%" for a !=, which lasts as long as `pattern`

Called by: [cnid_mysql_like](../cnid/cnid_mysql.c.md#cnid_mysql_like)

### sl_name_pattern_same

```c
static bool sl_name_pattern_same(const struct sl_name_pattern *a, const struct sl_name_pattern *b)
```

Defined at lines 1262 to 1276.

Tell whether two compiled comparisons match the same names.

Parameters:
* `a`: a comparison
* `b`: another

Returns: true for the same modifiers and the same folded value

Called by: [sl_name_chain_holds](sl_names.c.md#sl_name_chain_holds), [sl_name_comparison](sl_names.c.md#sl_name_comparison), [sl_name_seed](sl_names.c.md#sl_name_seed)

### sl_name_space

```c
static void sl_name_space(struct sl_name_parse *ps)
```

Defined at lines 1283 to 1289.

Skip white space.

Parameters:
* `ps`: parser state

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison), [sl_name_query_parse_in](sl_names.c.md#sl_name_query_parse_in), [sl_name_take](sl_names.c.md#sl_name_take)

### sl_name_take

```c
static bool sl_name_take(struct sl_name_parse *ps, const char *token)
```

Defined at lines 1298 to 1309.

Take a token after white space.

Parameters:
* `ps`: parser state
* `token`: the token

Returns: true when the token follows and is taken

Calls: [sl_name_space](sl_names.c.md#sl_name_space)

Called by: [sl_name_and](sl_names.c.md#sl_name_and), [sl_name_comparison](sl_names.c.md#sl_name_comparison), [sl_name_or](sl_names.c.md#sl_name_or), [sl_name_primary](sl_names.c.md#sl_name_primary)

### sl_name_skip_string

```c
static const char * sl_name_skip_string(struct sl_name_parse *ps)
```

Defined at lines 1318 to 1337.

Read past a quoted string, its escapes included.

Parameters:
* `ps`: parser state, at the opening quote; failed for a string without its closing quote

Returns: the position of the closing quote, or of the end of the query

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison), [sl_name_skip_operand](sl_names.c.md#sl_name_skip_operand)

### sl_name_skip_operand

```c
static void sl_name_skip_operand(struct sl_name_parse *ps)
```

Defined at lines 1347 to 1375.

Read past an operand the matcher does not take.

A word, a number, a string or a call such as $time.iso(...).

Parameters:
* `ps`: parser state; failed for a call without its closing parenthesis

Calls: [sl_name_skip_string](sl_names.c.md#sl_name_skip_string)

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison)

### sl_name_modifiers

```c
static unsigned int sl_name_modifiers(struct sl_name_parse *ps)
```

Defined at lines 1384 to 1408.

Read modifier letters.

Parameters:
* `ps`: parser state

Returns: the SL_NAMES_* flags of c, d and w in either case; other letters add none

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison)

### sl_name_phrase

```c
static size_t sl_name_phrase(const char *value, size_t len, char *out)
```

Defined at lines 1421 to 1445.

Drop the Finder's phrase delimiters from a value.

The Finder sends a phrase as "…"* or *"…"*; without the trailing '*', as anywhere else, an escaped quote is a quote.

Parameters:
* `value`: the value between its quotes
* `len`: bytes of value
* `out`: receives the value, len bytes of room

Returns: the bytes written to out

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison)

### sl_name_ascii_same

```c
static bool sl_name_ascii_same(const char *a, const char *b, size_t n)
```

Defined at lines 1459 to 1479.

Compare bytes, ASCII letters in either case, whatever the locale.

strncasecmp() folds through the locale, and a Turkish one keeps I apart from i, so a query that a volume of `casefold = tolower` lowered would name no attribute.

Parameters:
* `a`: bytes
* `b`: bytes
* `n`: number of bytes

Returns: true when the bytes agree

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison)

### sl_name_never

```c
static struct sl_name_query * sl_name_never(struct sl_name_parse *ps)
```

Defined at lines 1487 to 1498.

Make the node of a comparison that matches no name.

Parameters:
* `ps`: parser state, failed when out of memory

Returns: the node, or NULL when out of memory

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison)

### sl_name_comparison

```c
static struct sl_name_query * sl_name_comparison(struct sl_name_parse *ps)
```

Defined at lines 1518 to 1671.

Read one comparison, a node when it compares a name with a value.

A name attribute, ==, = or != and a quoted value with its modifiers make a node; any other comparison, function or operand is read to its end and makes none. A repeat of a comparison, one that compiles to the same modifiers and folded value, takes no room again. A comparison past ps->max, or one without SL_NAMES_NEGATE under ps->min bytes of folded literal characters, matches no name. A string or a call without its closing quote or parenthesis, a name attribute without an operator, an operator without a value, modifiers in brackets without the closing bracket, the SL_NAME_NODES + 1st comparison, a token that starts no comparison, the end of the query and a closing parenthesis among them, and a failed allocation, fail the query.

Parameters:
* `ps`: parser state

Returns: the node, or NULL

Calls: [sl_name_ascii_same](sl_names.c.md#sl_name_ascii_same), [sl_name_modifiers](sl_names.c.md#sl_name_modifiers), [sl_name_never](sl_names.c.md#sl_name_never), [sl_name_pattern_literals](sl_names.c.md#sl_name_pattern_literals), [sl_name_pattern_new](sl_names.c.md#sl_name_pattern_new), [sl_name_pattern_same](sl_names.c.md#sl_name_pattern_same), [sl_name_phrase](sl_names.c.md#sl_name_phrase), [sl_name_query_free](sl_names.c.md#sl_name_query_free), [sl_name_query_value](sl_names.c.md#sl_name_query_value), [sl_name_skip_operand](sl_names.c.md#sl_name_skip_operand), [sl_name_skip_string](sl_names.c.md#sl_name_skip_string), [sl_name_space](sl_names.c.md#sl_name_space), [sl_name_take](sl_names.c.md#sl_name_take)

Called by: [sl_name_primary](sl_names.c.md#sl_name_primary)

### sl_name_chain_holds

```c
static bool sl_name_chain_holds(const struct sl_name_query *chain, char op, const struct sl_name_query *leaf)
```

Defined at lines 1681 to 1691.

Tell whether a chain of one operator holds a comparison.

Parameters:
* `chain`: the chain's node
* `op`: '&' or '|'
* `leaf`: a comparison's node

Returns: true when a comparison of the chain matches the same names

Calls: [sl_name_pattern_same](sl_names.c.md#sl_name_pattern_same)

Called by: [sl_name_join](sl_names.c.md#sl_name_join)

### sl_name_join

```c
static struct sl_name_query * sl_name_join(struct sl_name_parse *ps, char op, struct sl_name_query *a, struct sl_name_query *b)
```

Defined at lines 1708 to 1747.

Join two nodes.

A side that the reading dropped gives way to the other. A side that matches no name makes an && match none and gives way in an ||. A comparison that its chain of the same operator holds already, as the Finder's pairs of name attributes send it, gives way too, so the match reads it once.

Parameters:
* `ps`: parser state, failed when out of memory
* `op`: '&' or '|'
* `a`: left node, or NULL
* `b`: right node, or NULL

Returns: the joined node, the one that is not NULL, or NULL

Calls: [sl_name_chain_holds](sl_names.c.md#sl_name_chain_holds), [sl_name_query_free](sl_names.c.md#sl_name_query_free)

Called by: [sl_name_and](sl_names.c.md#sl_name_and), [sl_name_or](sl_names.c.md#sl_name_or)

### sl_name_primary

```c
static struct sl_name_query * sl_name_primary(struct sl_name_parse *ps)
```

Defined at lines 1758 to 1780.

Read a comparison or a parenthesised expression.

An expression without its closing parenthesis, or one nested deeper than SL_NAME_DEPTH, fails the query.

Parameters:
* `ps`: parser state

Returns: the node, or NULL

Calls: [sl_name_comparison](sl_names.c.md#sl_name_comparison), [sl_name_or](sl_names.c.md#sl_name_or), [sl_name_take](sl_names.c.md#sl_name_take)

Called by: [sl_name_and](sl_names.c.md#sl_name_and)

### sl_name_and

```c
static struct sl_name_query * sl_name_and(struct sl_name_parse *ps)
```

Defined at lines 1788 to 1797.

Read comparisons joined by &&.

Parameters:
* `ps`: parser state

Returns: the node, or NULL

Calls: [sl_name_join](sl_names.c.md#sl_name_join), [sl_name_primary](sl_names.c.md#sl_name_primary), [sl_name_take](sl_names.c.md#sl_name_take)

Called by: [sl_name_or](sl_names.c.md#sl_name_or)

### sl_name_or

```c
static struct sl_name_query * sl_name_or(struct sl_name_parse *ps)
```

Defined at lines 1805 to 1814.

Read conjunctions joined by ||.

Parameters:
* `ps`: parser state

Returns: the node, or NULL

Calls: [sl_name_and](sl_names.c.md#sl_name_and), [sl_name_join](sl_names.c.md#sl_name_join), [sl_name_take](sl_names.c.md#sl_name_take)

Called by: [sl_name_primary](sl_names.c.md#sl_name_primary), [sl_name_query_parse_in](sl_names.c.md#sl_name_query_parse_in)

### sl_name_query_parse

```c
struct sl_name_query * sl_name_query_parse(const char *query, size_t min_literals, int max_comparisons)
```

Defined at lines 1836 to 1842.

Read the name comparisons of a Spotlight query.

kMDItemFSName, kMDItemDisplayName, _kMDItemFileName and the any-attribute '*' compare names; every other comparison drops out of the && or || that holds it. A comparison other than != under the minimum, or one past the maximum, matches no name; a repeated one counts once. A query that does not read to its end, lacks a closing parenthesis or an operand, nests parentheses deeper than SL_NAME_DEPTH, holds more than SL_NAME_NODES name comparisons or runs out of memory matches no name. Attribute names and modifiers read without regard to case: a volume that folds its names to one case converts the query the same way.

Parameters:
* `query`: the query string
* `min_literals`: folded literal bytes a comparison without != needs
* `max_comparisons`: comparisons the query keeps at most

Returns: the comparisons, to [sl_name_query_free()](sl_names.c.md#sl_name_query_free), or NULL when the query holds no name comparison or no name can match

Calls: [sl_name_query_parse_in](sl_names.c.md#sl_name_query_parse_in)

### sl_name_utf8_query

```c
static char * sl_name_utf8_query(const char *query, charset_t charset)
```

Defined at lines 1867 to 1915.

Convert a query of a charset to UTF-8, whatever its length.

A longer query converts in pieces of SL_NAME_PIECE bytes at most. A piece that ends inside a character of more bytes does not convert, as a character cut short is no character of an ASCII-compatible charset, so it ends up to 3 bytes earlier instead, before that character. A piece that still does not convert halves, for a charset whose character decodes to several units, as one of Mac Japanese decodes to five. A try that fails logs at the debug level alone.

Parameters:
* `query`: the query, NUL-terminated
* `charset`: its charset

Returns: the UTF-8 query, to free(), or NULL when a byte of it does not convert, or out of memory

Calls: [convert_string_quiet](../unicode/charcnv.c.md#convert_string_quiet)

Called by: [sl_name_query_parse_in](sl_names.c.md#sl_name_query_parse_in)

### sl_name_query_parse_in

```c
struct sl_name_query * sl_name_query_parse_in(const char *query, size_t min_literals, int max_comparisons, charset_t charset)
```

Defined at lines 1934 to 1985.

Read the name comparisons of a Spotlight query in a charset.

The query converts to UTF-8 before it is read, so a byte that continues a character, such as the trailing 0x5C of a Shift_JIS character, escapes nothing; each name converts as the matcher reads it, and one that does not convert compares its bytes.

Parameters:
* `query`: the query string
* `min_literals`: folded literal bytes a comparison without != needs
* `max_comparisons`: comparisons the query keeps at most
* `charset`: charset of the query and the names

Returns: the comparisons, to [sl_name_query_free()](sl_names.c.md#sl_name_query_free), or NULL when the query holds no name comparison, no name can match or the query does not convert

Calls: [sl_name_converts](sl_names.c.md#sl_name_converts), [sl_name_or](sl_names.c.md#sl_name_or), [sl_name_pattern_free](sl_names.c.md#sl_name_pattern_free), [sl_name_query_free](sl_names.c.md#sl_name_query_free), [sl_name_space](sl_names.c.md#sl_name_space), [sl_name_utf8_query](sl_names.c.md#sl_name_utf8_query)

Called by: [sl_cnid_open_query](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_open_query), [sl_name_query_parse](sl_names.c.md#sl_name_query_parse)

### sl_name_query_value

```c
struct sl_name_query * sl_name_query_value(const char *value, size_t len, unsigned int flags)
```

Defined at lines 1995 to 2008.

Make a query of one name comparison.

Parameters:
* `value`: the value, escapes included
* `len`: bytes of value
* `flags`: SL_NAMES_* flags

Returns: the query, to [sl_name_query_free()](sl_names.c.md#sl_name_query_free), or NULL when out of memory

Calls: [sl_name_pattern_new](sl_names.c.md#sl_name_pattern_new)

Called by: [sl_name_comparison](sl_names.c.md#sl_name_comparison), [sl_name_query_term_in](sl_names.c.md#sl_name_query_term_in)

### sl_name_query_term

```c
struct sl_name_query * sl_name_query_term(const char *term, size_t len, bool exact, unsigned int flags)
```

Defined at lines 2024 to 2028.

Make a query of one name comparison from a search term.

A term for names that contain it keeps its wildcards and escapes, and a backslash at its end means itself. A term for whole names takes every character literally.

Parameters:
* `term`: the term
* `len`: bytes of term, MAXPATHLEN at most
* `exact`: whether the term is a whole name
* `flags`: SL_NAMES_* flags

Returns: the query, to [sl_name_query_free()](sl_names.c.md#sl_name_query_free), or NULL when out of memory or for a longer term

Calls: [sl_name_query_term_in](sl_names.c.md#sl_name_query_term_in)

### sl_name_query_term_in

```c
struct sl_name_query * sl_name_query_term_in(const char *term, size_t len, bool exact, unsigned int flags, charset_t charset)
```

Defined at lines 2043 to 2089.

Make a query of one name comparison from a search term in a charset.

The term converts to UTF-8 first, and keeps its bytes when it does not.

Parameters:
* `term`: the term
* `len`: bytes of term, MAXPATHLEN at most, as UTF-8 too
* `exact`: whether the term is a whole name
* `flags`: SL_NAMES_* flags
* `charset`: charset of the term and the names

Returns: the query, to [sl_name_query_free()](sl_names.c.md#sl_name_query_free), or NULL when out of memory or for a longer term

Calls: [convert_string](../unicode/charcnv.c.md#convert_string), [sl_name_converts](sl_names.c.md#sl_name_converts), [sl_name_query_value](sl_names.c.md#sl_name_query_value)

Called by: [cnid_find_string](../cnid/cnid.c.md#cnid_find_string), [nad_find](../../bin/nad/nad_find.c.md#nad_find), [sl_name_query_term](sl_names.c.md#sl_name_query_term)

### sl_name_query_free

```c
void sl_name_query_free(struct sl_name_query *query)
```

Defined at lines 2096 to 2104.

Free a query's name comparisons.

Parameters:
* `query`: the comparisons, or NULL

Calls: [sl_name_pattern_free](sl_names.c.md#sl_name_pattern_free)

Called by: [cnid_find_string](../cnid/cnid.c.md#cnid_find_string), [nad_find](../../bin/nad/nad_find.c.md#nad_find), [sl_cnid_open_query](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_open_query), [sl_cnid_query_free](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_query_free), [sl_name_comparison](sl_names.c.md#sl_name_comparison), [sl_name_join](sl_names.c.md#sl_name_join), [sl_name_query_parse_in](sl_names.c.md#sl_name_query_parse_in)

Mentioned in the documentation of: [sl_name_query_parse](sl_names.c.md#sl_name_query_parse), [sl_name_query_parse_in](sl_names.c.md#sl_name_query_parse_in), [sl_name_query_seeds](sl_names.c.md#sl_name_query_seeds), [sl_name_query_term](sl_names.c.md#sl_name_query_term), [sl_name_query_term_in](sl_names.c.md#sl_name_query_term_in), [sl_name_query_value](sl_names.c.md#sl_name_query_value)

### sl_name_node_match

```c
static bool sl_name_node_match(const struct sl_name_query *query, struct sl_name_folds *f)
```

Defined at lines 2113 to 2128.

Tell whether a folded name satisfies a node of a query.

Parameters:
* `query`: the node
* `f`: the name and its foldings so far

Returns: the truth of the && and || of the node's comparisons

Calls: [sl_name_folds_match](sl_names.c.md#sl_name_folds_match)

Called by: [sl_name_query_match](sl_names.c.md#sl_name_query_match)

### sl_name_query_match

```c
bool sl_name_query_match(const struct sl_name_query *query, const char *name, size_t len)
```

Defined at lines 2140 to 2165.

Tell whether a name satisfies a query's name comparisons.

A name of SL_NAME_KEEP bytes at most folds once for each set of modifiers.

Parameters:
* `query`: the comparisons
* `name`: the name, in the charset of the query
* `len`: bytes of name

Returns: the truth of the && and || of the comparisons

Calls: [convert_string_quiet](../unicode/charcnv.c.md#convert_string_quiet), [sl_name_converts](sl_names.c.md#sl_name_converts), [sl_name_node_match](sl_names.c.md#sl_name_node_match)

Called by: [cnid_mysql_find](../cnid/cnid_mysql.c.md#cnid_mysql_find), [cnid_sqlite_match](../cnid/cnid_sqlite.c.md#cnid_sqlite_match), [filesystem_find](../../bin/nad/nad_find.c.md#filesystem_find), [sl_cnid_fill_results](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_fill_results)

### sl_name_seed

```c
static bool sl_name_seed(const struct sl_name_query *query, const struct sl_name_pattern **seeds, int *n, int max, sl_name_narrows_fn *narrows, void *ctx)
```

Defined at lines 2185 to 2222.

Collect the seeds that cover every match of a node.

A seed is a positive comparison the backend narrows by, collected once; every match of the node satisfies one of them. Either side of an && covers its matches: the left one when the backend narrows by all of its seeds, else the right one when it does. Each side is read once, and the backend asked once about each seed, so the work grows with the nodes of the query.

Parameters:
* `query`: the node
* `seeds`: receives the comparisons
* `n`: comparisons in seeds
* `max`: room in seeds
* `narrows`: the backend's test of a seed, or NULL
* `ctx`: the test's context

Returns: false when no such set fits, as under a !=, a comparison the backend does not narrow by, or an && neither side of which fits

Calls: [sl_name_pattern_negated](sl_names.c.md#sl_name_pattern_negated), [sl_name_pattern_same](sl_names.c.md#sl_name_pattern_same)

Called by: [sl_name_query_seeds](sl_names.c.md#sl_name_query_seeds)

### sl_name_query_seeds

```c
int sl_name_query_seeds(const struct sl_name_query *query, const struct sl_name_pattern **seeds, int max, sl_name_narrows_fn *narrows, void *ctx)
```

Defined at lines 2238 to 2244.

Pick the comparisons a database search runs for a query.

Parameters:
* `query`: the comparisons
* `seeds`: receives positive comparisons that every match satisfies one of, which belong to `query` and last until [sl_name_query_free()](sl_names.c.md#sl_name_query_free) frees it
* `max`: room in seeds, of which SL_NAME_NODES at most are used
* `narrows`: tells whether the backend narrows a search by a seed, or NULL
* `ctx`: context for narrows

Returns: the number of seeds, every one of which narrows the search, or 0 when every name is a candidate

Calls: [sl_name_seed](sl_names.c.md#sl_name_seed)

Called by: [cnid_find_scoped](../cnid/cnid.c.md#cnid_find_scoped), [cnid_mysql_find](../cnid/cnid_mysql.c.md#cnid_mysql_find)

Mentioned in the documentation of: [cnid_find_narrows](../cnid/cnid.c.md#cnid_find_narrows)

# Types

### struct sl_name_folds

Defined at line 1116.
* `const char * name`
* `size_t len`
* `size_t n`: units of each folding, SIZE_MAX until made
* `ucs2_t s`

### struct sl_name_parse

Defined at line 91.
* `const char * p`: position in the UTF-8 query
* `size_t min`: literal bytes a comparison other than != needs
* `int max`: distinct comparisons kept
* `int nseen`: comparisons compiled in seen
* `int nodes`: name comparisons read, repeats included
* `int depth`: parentheses open
* `bool failed`: no name can match the query
* `struct sl_name_pattern ** seen`: the distinct comparisons so far

### struct sl_name_pattern

Defined at line 60.
* `unsigned int flags`
* `size_t literals`: bytes of the value's folded literals
* `size_t req`: first token of the longest literal run
* `size_t reqlen`: its tokens, 0 without a literal
* `char * like`: LIKE pattern every match satisfies.
* `char * index_glob`: GLOB pattern over names [sl_name_fold()](sl_names.c.md#sl_name_fold) folds.
* `char * index_like`: LIKE pattern over them, ESCAPE '!
* `size_t ntok`
* `struct sl_name_tok tok`

### struct sl_name_query

Defined at line 75.
* `char op`
* `charset_t charset`
* `struct sl_name_pattern * pattern`
* `struct sl_name_query * left`
* `struct sl_name_query * right`

### struct sl_name_tok

Defined at line 54.
* `ucs2_t ch`
* `unsigned char kind`
* `bool run_end`: last unit of a run of literal characters

# Typedefs and enums

* `enum (anonymous)`: `SL_TOK_CHAR`, `SL_TOK_ONE`, `SL_TOK_ANY`

# Macros

* Undocumented: `SL_NAME_DEPTH`, `SL_NAME_FOLDING`, `SL_NAME_INDEX`, `SL_NAME_KEEP`, `SL_NAME_NEVER`, `SL_NAME_NODES`, `SL_NAME_PIECE`, `SL_NAME_PIECE_UTF8`, `SL_NAME_UNITS`

# File-scope variables

`sl_name_nonword`, `sl_name_nonword_sp`
