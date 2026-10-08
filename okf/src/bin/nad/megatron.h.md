---
type: C Header File
title: "bin/nad/megatron.h"
description: "4 types, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/megatron.h"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* System headers: `stdbool.h`

# Included by

* [bin/nad/hqx.c](hqx.c.md)
* [bin/nad/macbin.c](macbin.c.md)
* [bin/nad/megatron.c](megatron.c.md)
* [bin/nad/nad_adouble.c](nad_adouble.c.md)
* [bin/nad/nad_stuffit.c](nad_stuffit.c.md)

# Types

### struct FHeader

Defined at line 59.
* `char name`
* `char comment`
* `uint32_t forklen`
* `uint32_t create_date`
* `uint32_t mod_date`
* `uint32_t backup_date`
* `unsigned int date_flags`
* `struct FInfo finder_info`
* `struct FXInfo finder_xinfo`

### struct FInfo

Defined at line 42.
* `uint32_t fdType`
* `uint32_t fdCreator`
* `uint16_t fdFlags`
* `uint32_t fdLocation`
* `uint16_t fdFldr`

### struct FXInfo

Defined at line 50.
* `uint16_t fdIconID`
* `uint16_t fdUnused`
* `uint8_t fdScript`
* `uint8_t fdXFlags`
* `uint16_t fdComment`
* `uint32_t fdPutAway`

### struct nad_volume

Defined at line 71.
* `struct vol * vol`
* `struct vol * cnid_vol`
* `char db_stamp`
* `bool owns_cdb`
* `bool fallback`
* `bool skip_cnid`

# Macros

* Undocumented: `BIN2NAD`, `DATA`, `FH_DATE_BACKUP`, `FH_DATE_CREATE`, `FH_DATE_MODIFY`, `FILEIOFF_ATTR`, `FILEIOFF_BACKUP`, `FILEIOFF_CREATE`, `FILEIOFF_MODIFY`, `FINDERIOFF_CREATOR`, `FINDERIOFF_FLAGS`, `FINDERIOFF_FLDR`, `FINDERIOFF_LOC`, `FINDERIOFF_SCRIPT`, `FINDERIOFF_TYPE`, `FINDERIOFF_XFLAGS`, `HEX2NAD`, `KEEP`, `NAD2BIN`, `NAD2HEX`, `NUMFORKS`, `OPTION_ADOUBLE`, `OPTION_HEADERONLY`, `OPTION_NONE`, `OPTION_NORF`, `OPTION_STDOUT`, `OPTION_VERBOSE`, `RESOURCE`, `STDIN`, `S_ISDIR`, `TRASH`, `mtoupath`, `utompath`
