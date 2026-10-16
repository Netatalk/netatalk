---
type: Subsystem
title: "libatalk/unicode"
description: "31 files, 113 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/unicode"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

# Files

* [libatalk/unicode/charcnv.c](unicode/charcnv.c.md): Character-set conversion routines based on samba iconv wrappers.
* [libatalk/unicode/charsets/generic_cjk.c](unicode/charsets/generic_cjk.c.md): Shared conversion code of the CJK charsets.
* [libatalk/unicode/charsets/generic_cjk.h](unicode/charsets/generic_cjk.h.md): CJK mapping table types and character conversion helpers.
* [libatalk/unicode/charsets/generic_mb.c](unicode/charsets/generic_mb.c.md): Shared conversion code of the single-byte Mac charsets.
* [libatalk/unicode/charsets/generic_mb.h](unicode/charsets/generic_mb.h.md): Multibyte conversion interface using per-character callbacks.
* [libatalk/unicode/charsets/mac_centraleurope.c](unicode/charsets/mac_centraleurope.c.md): Mac Central European charset conversion.
* [libatalk/unicode/charsets/mac_centraleurope.h](unicode/charsets/mac_centraleurope.h.md): Mac Central Europe mapping tables and inline conversion helpers.
* [libatalk/unicode/charsets/mac_chinese_simp.c](unicode/charsets/mac_chinese_simp.c.md): Mac Chinese Simplified charset conversion.
* [libatalk/unicode/charsets/mac_chinese_simp.h](unicode/charsets/mac_chinese_simp.h.md): Mac Chinese Simplified character mapping tables.
* [libatalk/unicode/charsets/mac_chinese_trad.c](unicode/charsets/mac_chinese_trad.c.md): Mac Chinese Traditional charset conversion.
* [libatalk/unicode/charsets/mac_chinese_trad.h](unicode/charsets/mac_chinese_trad.h.md): Mac Chinese Traditional character mapping tables.
* [libatalk/unicode/charsets/mac_cyrillic.c](unicode/charsets/mac_cyrillic.c.md): Mac Cyrillic charset conversion.
* [libatalk/unicode/charsets/mac_cyrillic.h](unicode/charsets/mac_cyrillic.h.md): Mac Cyrillic character mapping tables.
* [libatalk/unicode/charsets/mac_greek.c](unicode/charsets/mac_greek.c.md): Mac Greek charset conversion.
* [libatalk/unicode/charsets/mac_greek.h](unicode/charsets/mac_greek.h.md): Mac Greek character mapping tables.
* [libatalk/unicode/charsets/mac_hebrew.c](unicode/charsets/mac_hebrew.c.md): Mac Hebrew charset conversion.
* [libatalk/unicode/charsets/mac_hebrew.h](unicode/charsets/mac_hebrew.h.md): Mac Hebrew character mapping tables.
* [libatalk/unicode/charsets/mac_japanese.c](unicode/charsets/mac_japanese.c.md): Mac Japanese charset conversion.
* [libatalk/unicode/charsets/mac_japanese.h](unicode/charsets/mac_japanese.h.md): Mac Japanese character mapping tables.
* [libatalk/unicode/charsets/mac_korean.c](unicode/charsets/mac_korean.c.md): Mac Korean charset conversion.
* [libatalk/unicode/charsets/mac_korean.h](unicode/charsets/mac_korean.h.md): Mac Korean character mapping tables.
* [libatalk/unicode/charsets/mac_roman.c](unicode/charsets/mac_roman.c.md): Mac Roman charset conversion.
* [libatalk/unicode/charsets/mac_roman.h](unicode/charsets/mac_roman.h.md): Mac Roman character mapping tables.
* [libatalk/unicode/charsets/mac_turkish.c](unicode/charsets/mac_turkish.c.md): Mac Turkish charset conversion.
* [libatalk/unicode/charsets/mac_turkish.h](unicode/charsets/mac_turkish.h.md): Mac Turkish character mapping tables.
* [libatalk/unicode/iconv.c](unicode/iconv.c.md): wrapper/stub for iconv character set conversion.
* [libatalk/unicode/precompose.h](unicode/precompose.h.md): Unicode composition and decomposition tables and Hangul constants.
* [libatalk/unicode/utf16_case.c](unicode/utf16_case.c.md): UTF-16 case mapping tables generated from the Unicode data.
* [libatalk/unicode/utf16_casetable.h](unicode/utf16_casetable.h.md): UTF-16 upper and lower case lookup tables.
* [libatalk/unicode/utf8.c](unicode/utf8.c.md): UTF-8 charset conversion.
* [libatalk/unicode/util_unistr.c](unicode/util_unistr.c.md): UTF-16 string functions, with Unicode precomposition and decomposition.

# Includes headers from

* [include/atalk](../include/atalk.md): 29 includes

# Called from

* [etc/afpd](../etc/afpd.md): 37 calls
* [bin/nbp](../bin/nbp.md): 10 calls
* [etc/papd](../etc/papd.md): 8 calls
* [libatalk/util](util.md): 7 calls
* [bin/netacnv](../bin/netacnv.md): 6 calls
* [bin/getzones](../bin/getzones.md): 5 calls
* [etc/atalkd](../etc/atalkd.md): 3 calls
* [libatalk/vfs](vfs.md): 3 calls
* [bin/nad](../bin/nad.md): 2 calls
* [etc/netatalk](../etc/netatalk.md): 2 calls
* [etc/uams](../etc/uams.md): 1 calls

# Most called functions

* [convert_string_allocate](unicode/charcnv.c.md#convert_string_allocate): 21 callers
* [convert_string](unicode/charcnv.c.md#convert_string): 18 callers
* [convert_charset](unicode/charcnv.c.md#convert_charset): 15 callers
* [add_charset](unicode/charcnv.c.md#add_charset): 10 callers
* [charset_name](unicode/charcnv.c.md#charset_name): 9 callers
* [cjk_lookup](unicode/charsets/generic_cjk.c.md#cjk_lookup): 8 callers
* [set_charset_name](unicode/charcnv.c.md#set_charset_name): 8 callers
* [atalk_iconv](unicode/iconv.c.md#atalk_iconv): 7 callers
* [atalk_iconv_open](unicode/iconv.c.md#atalk_iconv_open): 6 callers
* [convert_string_internal](unicode/charcnv.c.md#convert_string_internal): 6 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[charset_precompose](unicode/charcnv.c.md#charset_precompose), [charset_strlower](unicode/charcnv.c.md#charset_strlower), [charset_strupper](unicode/charcnv.c.md#charset_strupper), [strcmp_w](unicode/util_unistr.c.md#strcmp_w), [strnlen_w](unicode/util_unistr.c.md#strnlen_w), [strstr_w](unicode/util_unistr.c.md#strstr_w)
