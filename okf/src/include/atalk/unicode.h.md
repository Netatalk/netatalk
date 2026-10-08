---
type: C Header File
title: "include/atalk/unicode.h"
description: "2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/unicode.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `errno.h`, `stdint.h`, `sys/param.h`

# Included by

* [bin/dbd/cmd_dbd_scanvol.c](../../bin/dbd/cmd_dbd_scanvol.c.md)
* [bin/getzones/getzones.c](../../bin/getzones/getzones.c.md)
* [bin/misc/netacnv.c](../../bin/misc/netacnv.c.md)
* [bin/nad/nad_find.c](../../bin/nad/nad_find.c.md)
* [bin/nad/nad_util.c](../../bin/nad/nad_util.c.md)
* [bin/nbp/nbplkup.c](../../bin/nbp/nbplkup.c.md)
* [bin/nbp/nbplkup_output.c](../../bin/nbp/nbplkup_output.c.md)
* [bin/nbp/nbplkup_output.h](../../bin/nbp/nbplkup_output.h.md)
* [bin/nbp/nbprgstr.c](../../bin/nbp/nbprgstr.c.md)
* [bin/nbp/nbpunrgstr.c](../../bin/nbp/nbpunrgstr.c.md)
* [etc/afpd/catsearch.c](../../etc/afpd/catsearch.c.md)
* [etc/afpd/status.c](../../etc/afpd/status.c.md)
* [etc/afpd/volume.h](../../etc/afpd/volume.h.md)
* [etc/atalkd/config.c](../../etc/atalkd/config.c.md)
* [etc/netatalk/afp_avahi.c](../../etc/netatalk/afp_avahi.c.md)
* [etc/netatalk/afp_mdns.c](../../etc/netatalk/afp_mdns.c.md)
* [etc/papd/lp.c](../../etc/papd/lp.c.md)
* [etc/papd/main.c](../../etc/papd/main.c.md)
* [etc/papd/print_cups.c](../../etc/papd/print_cups.c.md)
* [include/atalk/directory.h](directory.h.md)
* [include/atalk/globals.h](globals.h.md)
* [include/atalk/util.h](util.h.md)
* [include/atalk/volume.h](volume.h.md)
* [libatalk/unicode/charcnv.c](../../libatalk/unicode/charcnv.c.md)
* [libatalk/unicode/charsets/generic_cjk.h](../../libatalk/unicode/charsets/generic_cjk.h.md)
* [libatalk/unicode/charsets/generic_mb.c](../../libatalk/unicode/charsets/generic_mb.c.md)
* [libatalk/unicode/charsets/mac_centraleurope.c](../../libatalk/unicode/charsets/mac_centraleurope.c.md)
* [libatalk/unicode/charsets/mac_cyrillic.c](../../libatalk/unicode/charsets/mac_cyrillic.c.md)
* [libatalk/unicode/charsets/mac_greek.c](../../libatalk/unicode/charsets/mac_greek.c.md)
* [libatalk/unicode/charsets/mac_hebrew.c](../../libatalk/unicode/charsets/mac_hebrew.c.md)
* [libatalk/unicode/charsets/mac_roman.c](../../libatalk/unicode/charsets/mac_roman.c.md)
* [libatalk/unicode/charsets/mac_turkish.c](../../libatalk/unicode/charsets/mac_turkish.c.md)
* [libatalk/unicode/iconv.c](../../libatalk/unicode/iconv.c.md)
* [libatalk/unicode/utf16_case.c](../../libatalk/unicode/utf16_case.c.md)
* [libatalk/unicode/utf8.c](../../libatalk/unicode/utf8.c.md)
* [libatalk/unicode/util_unistr.c](../../libatalk/unicode/util_unistr.c.md)
* [libatalk/util/cnid.c](../../libatalk/util/cnid.c.md)

# Types

### struct atalk_iconv_t

Defined at line 17.
* `size_t(* direct`: Called through by [atalk_iconv](../../libatalk/unicode/iconv.c.md#atalk_iconv).
* `size_t(* pull`: Called through by [atalk_iconv](../../libatalk/unicode/iconv.c.md#atalk_iconv).
* `size_t(* push`: Called through by [atalk_iconv](../../libatalk/unicode/iconv.c.md#atalk_iconv).
* `void * cd_direct`
* `void * cd_pull`
* `void * cd_push`
* `char * from_name`
* `char * to_name`

### struct charset_functions

Defined at line 66.
* `const char * name`
* `const uint32_t kTextEncoding`
* `size_t(* pull`: Assigned in [charset_mac_centraleurope](../../libatalk/unicode/charsets/mac_centraleurope.c.md#charset_mac_centraleurope), [charset_mac_chinese_simp](../../libatalk/unicode/charsets/mac_chinese_simp.c.md#charset_mac_chinese_simp), [charset_mac_chinese_trad](../../libatalk/unicode/charsets/mac_chinese_trad.c.md#charset_mac_chinese_trad), [charset_mac_cyrillic](../../libatalk/unicode/charsets/mac_cyrillic.c.md#charset_mac_cyrillic), [charset_mac_greek](../../libatalk/unicode/charsets/mac_greek.c.md#charset_mac_greek), [charset_mac_hebrew](../../libatalk/unicode/charsets/mac_hebrew.c.md#charset_mac_hebrew), [charset_mac_japanese](../../libatalk/unicode/charsets/mac_japanese.c.md#charset_mac_japanese), [charset_mac_korean](../../libatalk/unicode/charsets/mac_korean.c.md#charset_mac_korean), [charset_mac_roman](../../libatalk/unicode/charsets/mac_roman.c.md#charset_mac_roman), [charset_mac_turkish](../../libatalk/unicode/charsets/mac_turkish.c.md#charset_mac_turkish), [charset_utf8](../../libatalk/unicode/utf8.c.md#charset_utf8), [charset_utf8_mac](../../libatalk/unicode/utf8.c.md#charset_utf8_mac).
* `size_t(* push`: Assigned in [charset_mac_centraleurope](../../libatalk/unicode/charsets/mac_centraleurope.c.md#charset_mac_centraleurope), [charset_mac_chinese_simp](../../libatalk/unicode/charsets/mac_chinese_simp.c.md#charset_mac_chinese_simp), [charset_mac_chinese_trad](../../libatalk/unicode/charsets/mac_chinese_trad.c.md#charset_mac_chinese_trad), [charset_mac_cyrillic](../../libatalk/unicode/charsets/mac_cyrillic.c.md#charset_mac_cyrillic), [charset_mac_greek](../../libatalk/unicode/charsets/mac_greek.c.md#charset_mac_greek), [charset_mac_hebrew](../../libatalk/unicode/charsets/mac_hebrew.c.md#charset_mac_hebrew), [charset_mac_japanese](../../libatalk/unicode/charsets/mac_japanese.c.md#charset_mac_japanese), [charset_mac_korean](../../libatalk/unicode/charsets/mac_korean.c.md#charset_mac_korean), [charset_mac_roman](../../libatalk/unicode/charsets/mac_roman.c.md#charset_mac_roman), [charset_mac_turkish](../../libatalk/unicode/charsets/mac_turkish.c.md#charset_mac_turkish), [charset_utf8](../../libatalk/unicode/utf8.c.md#charset_utf8), [charset_utf8_mac](../../libatalk/unicode/utf8.c.md#charset_utf8_mac).
* `uint32_t flags`
* `const char * iname`
* `struct charset_functions * prev`
* `struct charset_functions * next`

# Typedefs and enums

* `enum charset_t`: `CH_UCS2`, `CH_UTF8`, `CH_MAC`, `CH_UNIX`, `CH_UTF8_MAC`

# Macros

* Undocumented: `CHARSET_CLIENT`, `CHARSET_DECOMPOSED`, `CHARSET_ICONV`, `CHARSET_MULTIBYTE`, `CHARSET_PRECOMPOSED`, `CHARSET_VOLUME`, `CHARSET_WIDECHAR`, `CONV_DECOMPOSE`, `CONV_ESCAPEDOTS`, `CONV_ESCAPEHEX`, `CONV_FORCE`, `CONV_IGNORE`, `CONV_PRECOMPOSE`, `CONV_REQESCAPE`, `CONV_REQMANGLE`, `CONV_TOLOWER`, `CONV_TOUPPER`, `CONV_UNESCAPEHEX`, `CONV__EILSEQ`, `EILSEQ`, `IGNORE_CHAR`, `NUM_CHARSETS`, `SAFE_FREE`, `ucs2_t`
