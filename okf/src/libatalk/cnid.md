---
type: Subsystem
title: "libatalk/cnid"
description: "4 files, 76 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/cnid"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [libatalk/cnid/cnid.c](cnid/cnid.c.md): 16 functions, includes 4 project headers.
* [libatalk/cnid/cnid_init.c](cnid/cnid_init.c.md): initialization stuff for CNID backends.
* [libatalk/cnid/mysql/cnid_mysql.c](cnid/mysql/cnid_mysql.c.md): 26 functions, includes 8 project headers.
* [libatalk/cnid/sqlite/cnid_sqlite.c](cnid/sqlite/cnid_sqlite.c.md): 33 functions, includes 8 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 23 includes

# Calls into

* [libatalk/compat](compat.md): 22 calls
* [libatalk/util](util.md): 6 calls

# Called from

* [etc/afpd](../etc/afpd.md): 39 calls
* [bin/nad](../bin/nad.md): 24 calls
* [bin/dbd](../bin/dbd.md): 9 calls
* [etc/spotlight](../etc/spotlight.md): 2 calls
* [libatalk/util](util.md): 1 calls

# Most called functions

* [cnid_init](cnid/cnid_init.c.md#cnid_init): 12 callers
* [cnid_get](cnid/cnid.c.md#cnid_get): 11 callers
* [cnid_sqlite_set_errno](cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_set_errno): 11 callers
* [cnid_mysql_set_errno](cnid/mysql/cnid_mysql.c.md#cnid_mysql_set_errno): 10 callers
* [cnid_resolve](cnid/cnid.c.md#cnid_resolve): 9 callers
* [cnid_delete](cnid/cnid.c.md#cnid_delete): 8 callers
* [cnid_sqlite_stmt_ready](cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_stmt_ready): 8 callers
* [cnid_sqlite_stmt_reset](cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_stmt_reset): 8 callers
* [cnid_sqlite_execute](cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_execute): 6 callers
* [cnid_add](cnid/cnid.c.md#cnid_add): 5 callers
