---
type: Subsystem
title: "libatalk/unicode"
description: "31 files, 108 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/unicode"
tags: ["libatalk/unicode"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [libatalk/unicode/charcnv.c](unicode/charcnv.c.md): 21 functions, includes 5 project headers.
* [libatalk/unicode/charsets/generic_cjk.c](unicode/charsets/generic_cjk.c.md): 8 functions, includes 1 project header.
* [libatalk/unicode/charsets/generic_cjk.h](unicode/charsets/generic_cjk.h.md): 1 type, includes 2 project headers.
* [libatalk/unicode/charsets/generic_mb.c](unicode/charsets/generic_mb.c.md): 2 functions, includes 4 project headers.
* [libatalk/unicode/charsets/generic_mb.h](unicode/charsets/generic_mb.h.md): No functions or types.
* [libatalk/unicode/charsets/mac_centraleurope.c](unicode/charsets/mac_centraleurope.c.md): 2 functions, includes 3 project headers.
* [libatalk/unicode/charsets/mac_centraleurope.h](unicode/charsets/mac_centraleurope.h.md): 2 functions.
* [libatalk/unicode/charsets/mac_chinese_simp.c](unicode/charsets/mac_chinese_simp.c.md): 4 functions, includes 2 project headers.
* [libatalk/unicode/charsets/mac_chinese_simp.h](unicode/charsets/mac_chinese_simp.h.md): No functions or types.
* [libatalk/unicode/charsets/mac_chinese_trad.c](unicode/charsets/mac_chinese_trad.c.md): 4 functions, includes 2 project headers.
* [libatalk/unicode/charsets/mac_chinese_trad.h](unicode/charsets/mac_chinese_trad.h.md): No functions or types.
* [libatalk/unicode/charsets/mac_cyrillic.c](unicode/charsets/mac_cyrillic.c.md): 2 functions, includes 3 project headers.
* [libatalk/unicode/charsets/mac_cyrillic.h](unicode/charsets/mac_cyrillic.h.md): 2 functions.
* [libatalk/unicode/charsets/mac_greek.c](unicode/charsets/mac_greek.c.md): 4 functions, includes 3 project headers.
* [libatalk/unicode/charsets/mac_greek.h](unicode/charsets/mac_greek.h.md): No functions or types.
* [libatalk/unicode/charsets/mac_hebrew.c](unicode/charsets/mac_hebrew.c.md): 4 functions, includes 4 project headers.
* [libatalk/unicode/charsets/mac_hebrew.h](unicode/charsets/mac_hebrew.h.md): MacHebrew.
* [libatalk/unicode/charsets/mac_japanese.c](unicode/charsets/mac_japanese.c.md): 4 functions, includes 2 project headers.
* [libatalk/unicode/charsets/mac_japanese.h](unicode/charsets/mac_japanese.h.md): No functions or types.
* [libatalk/unicode/charsets/mac_korean.c](unicode/charsets/mac_korean.c.md): 4 functions, includes 2 project headers.
* [libatalk/unicode/charsets/mac_korean.h](unicode/charsets/mac_korean.h.md): No functions or types.
* [libatalk/unicode/charsets/mac_roman.c](unicode/charsets/mac_roman.c.md): 4 functions, includes 3 project headers.
* [libatalk/unicode/charsets/mac_roman.h](unicode/charsets/mac_roman.h.md): No functions or types.
* [libatalk/unicode/charsets/mac_turkish.c](unicode/charsets/mac_turkish.c.md): 2 functions, includes 3 project headers.
* [libatalk/unicode/charsets/mac_turkish.h](unicode/charsets/mac_turkish.h.md): 2 functions.
* [libatalk/unicode/iconv.c](unicode/iconv.c.md): wrapper/stub for iconv character set conversion.
* [libatalk/unicode/precompose.h](unicode/precompose.h.md): No functions or types.
* [libatalk/unicode/utf16_case.c](unicode/utf16_case.c.md): 4 functions, includes 2 project headers.
* [libatalk/unicode/utf16_casetable.h](unicode/utf16_casetable.h.md): No functions or types.
* [libatalk/unicode/utf8.c](unicode/utf8.c.md): 2 functions, includes 3 project headers.
* [libatalk/unicode/util_unistr.c](unicode/util_unistr.c.md): 21 functions, includes 4 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 29 includes

# Called from

* [etc/afpd](../etc/afpd.md): 37 calls
* [bin/nbp](../bin/nbp.md): 10 calls
* [etc/papd](../etc/papd.md): 8 calls
* [libatalk/util](util.md): 7 calls
* [bin/getzones](../bin/getzones.md): 5 calls
* [bin/misc](../bin/misc.md): 3 calls
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
* [cjk_lookup](unicode/charsets/generic_cjk.c.md#cjk_lookup): 8 callers
* [set_charset_name](unicode/charcnv.c.md#set_charset_name): 8 callers
* [charset_name](unicode/charcnv.c.md#charset_name): 6 callers
* [convert_string_internal](unicode/charcnv.c.md#convert_string_internal): 6 callers
* [strlen_w](unicode/util_unistr.c.md#strlen_w): 6 callers
* [cjk_compose](unicode/charsets/generic_cjk.c.md#cjk_compose): 5 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[atalk_iconv_close](unicode/iconv.c.md#atalk_iconv_close), [charset_precompose](unicode/charcnv.c.md#charset_precompose), [charset_strlower](unicode/charcnv.c.md#charset_strlower), [charset_strupper](unicode/charcnv.c.md#charset_strupper), [strcmp_w](unicode/util_unistr.c.md#strcmp_w), [strnlen_w](unicode/util_unistr.c.md#strnlen_w), [strstr_w](unicode/util_unistr.c.md#strstr_w)
