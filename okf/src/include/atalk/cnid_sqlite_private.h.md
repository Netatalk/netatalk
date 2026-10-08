---
type: C Header File
title: "include/atalk/cnid_sqlite_private.h"
description: "1 type, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/cnid_sqlite_private.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/uuid.h](uuid.h.md)

# Included by

* [libatalk/cnid/sqlite/cnid_sqlite.c](../../libatalk/cnid/sqlite/cnid_sqlite.c.md)

# Types

### struct CNID_sqlite_private

Defined at line 10.
* `struct vol * vol`
* `uint32_t cnid_sqlite_flags`
* `sqlite3 * cnid_sqlite_con`
* `char * cnid_sqlite_voluuid_str`
* `cnid_t cnid_sqlite_hint`
* `sqlite3_stmt * cnid_lookup_stmt`
* `sqlite3_stmt * cnid_add_stmt`
* `sqlite3_stmt * cnid_put_stmt`
* `sqlite3_stmt * cnid_get_stmt`
* `sqlite3_stmt * cnid_resolve_stmt`
* `sqlite3_stmt * cnid_delete_stmt`
* `sqlite3_stmt * cnid_getstamp_stmt`
* `sqlite3_stmt * cnid_find_stmt`
* `sqlite3_stmt * cnid_find_scoped_stmt`
* `sqlite3_stmt * cnid_update_stmt`
* `sqlite3_stmt * cnid_del_didname_stmt`
* `sqlite3_stmt * cnid_del_devino_stmt`

# Typedefs and enums

* `typedef struct CNID_sqlite_private CNID_sqlite_private`

# Macros

* Undocumented: `CNID_SQLITE_FLAG_DEPLETED`, `CNID_SQLITE_FLAG_HINT_RANGE_LOGGED`, `CNID_SQLITE_FLAG_NEAR_DEPLETION`
