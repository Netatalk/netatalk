---
type: C Header File
title: "etc/afpd/file.h"
description: "includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/file.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [directory.h](directory.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `sys/param.h`

# Included by

* [etc/afpd/appl.c](appl.c.md)
* [etc/afpd/catsearch.c](catsearch.c.md)
* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/enumerate.c](enumerate.c.md)
* [etc/afpd/fce_api.c](fce_api.c.md)
* [etc/afpd/fce_util.c](fce_util.c.md)
* [etc/afpd/file.c](file.c.md)
* [etc/afpd/filedir.c](filedir.c.md)
* [etc/afpd/fork.c](fork.c.md)
* [etc/afpd/switch.c](switch.c.md)
* [etc/afpd/virtual_icon.c](virtual_icon.c.md)
* [etc/afpd/volume.c](volume.c.md)

# Typedefs and enums

* `enum kTextEncoding_t`: `kTextEncodingMacRoman`, `kTextEncodingMacJapanese`, `kTextEncodingMacChineseTrad`, `kTextEncodingMacKorean`, `kTextEncodingMacArabic`, `kTextEncodingMacHebrew`, `kTextEncodingMacGreek`, `kTextEncodingMacCyrillic`, `kTextEncodingMacDevanagari`, `kTextEncodingMacGurmukhi`, `kTextEncodingMacGujarati`, `kTextEncodingMacOriya`, `kTextEncodingMacBengali`, `kTextEncodingMacTamil`, `kTextEncodingMacTelugu`, `kTextEncodingMacKannada`, `kTextEncodingMacMalayalam`, `kTextEncodingMacSinhalese`, `kTextEncodingMacBurmese`, `kTextEncodingMacKhmer`, `kTextEncodingMacThai`, `kTextEncodingMacLaotian`, `kTextEncodingMacGeorgian`, `kTextEncodingMacArmenian`, `kTextEncodingMacChineseSimp`, `kTextEncodingMacTibetan`, `kTextEncodingMacMongolian`, `kTextEncodingMacEthiopic`, `kTextEncodingMacCentralEurRoman`, `kTextEncodingMacVietnamese`, `kTextEncodingMacExtArabic`, `kTextEncodingMacSymbol`, `kTextEncodingMacDingbats`, `kTextEncodingMacTurkish`, `kTextEncodingMacCroatian`, `kTextEncodingMacIcelandic`, `kTextEncodingMacRomanian`, `kTextEncodingMacCeltic`, `kTextEncodingMacGaelic`, `kTextEncodingMacKeyboardGlyphs`, `kTextEncodingMacUnicode`, `kTextEncodingMacFarsi`, `kTextEncodingMacUkrainian`, `kTextEncodingMacInuit`, `kTextEncodingMacVT100`, `kTextEncodingMacHFS`, `kTextEncodingUnicodeDefault`, `kTextEncodingUnicodeV1_1`, `kTextEncodingISO10646_1993`, `kTextEncodingUnicodeV2_0`, `kTextEncodingUnicodeV2_1`, `kTextEncodingUnicodeV3_0`, `kTextEncodingISOLatin1`, `kTextEncodingISOLatin2`, `kTextEncodingISOLatin3`, `kTextEncodingISOLatin4`, `kTextEncodingISOLatinCyrillic`, `kTextEncodingISOLatinArabic`, `kTextEncodingISOLatinGreek`, `kTextEncodingISOLatinHebrew`, `kTextEncodingISOLatin5`, `kTextEncodingISOLatin6`, `kTextEncodingISOLatin7`, `kTextEncodingISOLatin8`, `kTextEncodingISOLatin9`, `kTextEncodingDOSLatinUS`, `kTextEncodingDOSGreek`, `kTextEncodingDOSBalticRim`, `kTextEncodingDOSLatin1`, `kTextEncodingDOSGreek1`, `kTextEncodingDOSLatin2`, `kTextEncodingDOSCyrillic`, `kTextEncodingDOSTurkish`, `kTextEncodingDOSPortuguese`, `kTextEncodingDOSIcelandic`, `kTextEncodingDOSHebrew`, `kTextEncodingDOSCanadianFrench`, `kTextEncodingDOSArabic`, `kTextEncodingDOSNordic`, `kTextEncodingDOSRussian`, `kTextEncodingDOSGreek2`, `kTextEncodingDOSThai`, `kTextEncodingDOSJapanese`, `kTextEncodingDOSChineseSimplif`, `kTextEncodingDOSKorean`, `kTextEncodingDOSChineseTrad`, `kTextEncodingWindowsLatin1`, `kTextEncodingWindowsANSI`, `kTextEncodingWindowsLatin2`, `kTextEncodingWindowsCyrillic`, `kTextEncodingWindowsGreek`, `kTextEncodingWindowsLatin5`, `kTextEncodingWindowsHebrew`, `kTextEncodingWindowsArabic`, `kTextEncodingWindowsBalticRim`, `kTextEncodingWindowsVietnamese`, `kTextEncodingWindowsKoreanJohab`, `kTextEncodingUS_ASCII`, `kTextEncodingJIS_X0201_76`, `kTextEncodingJIS_X0208_83`, `kTextEncodingJIS_X0208_90`

# Macros

* Undocumented: `FILPBIT_ATTR`, `FILPBIT_BDATE`, `FILPBIT_CDATE`, `FILPBIT_DFLEN`, `FILPBIT_EXTDFLEN`, `FILPBIT_EXTRFLEN`, `FILPBIT_FINFO`, `FILPBIT_FNUM`, `FILPBIT_LNAME`, `FILPBIT_MDATE`, `FILPBIT_PDID`, `FILPBIT_PDINFO`, `FILPBIT_RFLEN`, `FILPBIT_SNAME`, `FILPBIT_UNIXPR`, `kTextEncodingUTF8`
