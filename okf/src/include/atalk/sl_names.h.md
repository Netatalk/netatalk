---
type: C Header File
title: "include/atalk/sl_names.h"
description: "Spotlight's name comparisons: the value syntax, the c, d and w modifiers, and the && and || of a query."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/sl_names.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-11-22T14:39:48+01:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/unicode.h](unicode.h.md)
* System headers: `stdbool.h`, `stddef.h`

# Included by

* [bin/nad/nad_find.c](../../bin/nad/nad_find.c.md)
* [etc/spotlight/cnid/sl_cnid.c](../../etc/spotlight/cnid/sl_cnid.c.md)
* [libatalk/cnid/cnid.c](../../libatalk/cnid/cnid.c.md)
* [libatalk/cnid/cnid_mysql.c](../../libatalk/cnid/cnid_mysql.c.md)
* [libatalk/cnid/cnid_sqlite.c](../../libatalk/cnid/cnid_sqlite.c.md)
* [libatalk/util/sl_names.c](../../libatalk/util/sl_names.c.md)

# Typedefs and enums

* `typedef bool sl_name_narrows_fn`

# Macros

* `SL_NAMES_CASE`: c: letters ignore case
* `SL_NAMES_DIACRITIC`: d: letters ignore diacritics
* `SL_NAMES_NEGATE`: !=: the names that do not match
* `SL_NAMES_WORD`: w: the value anchors at words
