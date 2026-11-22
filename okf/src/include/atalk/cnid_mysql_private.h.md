---
type: C Header File
title: "include/atalk/cnid_mysql_private.h"
description: "Private MySQL CNID backend state and status flags."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/cnid_mysql_private.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-11-23T02:29:08+11:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/uuid.h](uuid.h.md)
* System headers: `stdbool.h`

# Included by

* [libatalk/cnid/cnid_mysql.c](../../libatalk/cnid/cnid_mysql.c.md)

# Types

### struct CNID_mysql_private

Defined at line 24.
* `struct vol * vol`
* `uint32_t cnid_mysql_flags`
* `MYSQL * cnid_mysql_con`
* `char * cnid_mysql_voluuid_str`
* `char * cnid_mysql_volpath`: the volume path, escaped for a quoted SQL literal
* `cnid_t cnid_mysql_hint`
* `MYSQL_STMT * cnid_lookup_stmt`
* `MYSQL_STMT * cnid_add_stmt`
* `MYSQL_STMT * cnid_put_stmt`
* `MYSQL_STMT * cnid_get_stmt`
* `MYSQL_STMT * cnid_delete_stmt`
* `MYSQL_STMT * cnid_resolve_stmt`
* `MYSQL_STMT * cnid_purge_stmt`
* `MYSQL_STMT * cnid_dirlist_stmt`
* `MYSQL_STMT * cnid_delete_row_stmt`
* `MYSQL_STMT * cnid_update_row_stmt`
* `MYSQL_STMT * cnid_del_devino_other_stmt`
* `MYSQL_STMT * cnid_check_row_stmt`
* `bool cnid_find_scoped_unsupported`
* `bool cnid_mysql_folded`: the table has the Folded1 column
* `enum cnid_mysql_hold_kind cnid_mysql_hold_kind`
* `unsigned long cnid_mysql_stmt_thread`
* `char cnid_mysql_name_is`

# Typedefs and enums

* `typedef struct CNID_mysql_private CNID_mysql_private`
* `enum cnid_mysql_hold_kind`: `CNID_MYSQL_HOLD_ROW`, `CNID_MYSQL_HOLD_TABLE`, `CNID_MYSQL_HOLD_NONE`

# Macros

* `CNID_MYSQL_FLAG_DEPLETED`: CNID set overflowed.
* `CNID_MYSQL_FLAG_HINT_RANGE_LOGGED`: out-of-range hint warned once
* `CNID_MYSQL_FLAG_NEAR_DEPLETION`: approaching-ceiling warned once
