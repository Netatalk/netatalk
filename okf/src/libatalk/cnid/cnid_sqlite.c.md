---
type: C Source File
title: "libatalk/cnid/cnid_sqlite.c"
description: "SQLite CNID backend."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/cnid_sqlite.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-11-22T14:39:48+01:00 }
---

Part of the [libatalk/cnid](../cnid.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid_sqlite_private.h](../../include/atalk/cnid_sqlite_private.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/sl_names.h](../../include/atalk/sl_names.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `bstrlib.h`, `ctype.h`, `errno.h`, `fcntl.h`, `inttypes.h`, `net/if.h`, `netdb.h`, `netinet/in.h`, `netinet/tcp.h`, `sqlite3.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/stat.h`, `sys/time.h`, `sys/uio.h`, `sys/un.h`, `time.h`

# Function tables

### cnid_sqlite_module

Initialized at line 38 as `struct _cnid_module`. Assigns:

* `cnid_open`: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

# Functions

### cnid_sqlite_fchmod_regular

```c
static int cnid_sqlite_fchmod_regular(const char *path, mode_t mode, bool missing_ok)
```

Defined at lines 212 to 259.

Change the mode of a SQLite file without following a symlink.

Local users share the database directory, so afpd never passes a path to chmod while privileged. The call opens the file with O_NOFOLLOW. It refuses a file that is not regular or has more than 1 link, because a hard link could point outside the CNID directory.

Parameters:
* `path`: file to change
* `mode`: new permission bits
* `missing_ok`: true to accept a missing file

Returns: 0, or -1 with errno set, EMLINK for a file with more than 1 link

Called by: [cnid_sqlite_fchmod_companions](cnid_sqlite.c.md#cnid_sqlite_fchmod_companions), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_fchmod_companions

```c
static int cnid_sqlite_fchmod_companions(const char *dbpath, mode_t mode)
```

Defined at lines 271 to 292.

Check and set the mode of the -wal and -shm files of a SQLite database.

The call runs before SQLite opens them, to reject unsafe files, and again after SQLite creates them, to set their mode.

Parameters:
* `dbpath`: path of the database file
* `mode`: permission bits

Returns: 0, or -1 for an unsafe companion or a path that is too long

Calls: [cnid_sqlite_fchmod_regular](cnid_sqlite.c.md#cnid_sqlite_fchmod_regular)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_match

```c
static void cnid_sqlite_match(sqlite3_context *ctx, int argc, sqlite3_value **argv)
```

Defined at lines 302 to 311.

SQL function netatalk_match(name, query): whether the name matches.

Parameters:
* `ctx`: receives 1 or 0
* `argc`: 2
* `argv`: the name, and the query bound with sqlite3_bind_pointer()

Calls: [sl_name_query_match](../util/sl_names.c.md#sl_name_query_match)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_fold

```c
static void cnid_sqlite_fold(sqlite3_context *ctx, int argc, sqlite3_value **argv)
```

Defined at lines 322 to 341.

SQL function netatalk_fold(name): the name as the name index holds it.

The function's data is the volume, in whose charset the name comes.

Parameters:
* `ctx`: receives the folded name
* `argc`: 1
* `argv`: the name

Calls: [sl_name_fold_in](../util/sl_names.c.md#sl_name_fold_in)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_prepare_one

```c
static int cnid_sqlite_prepare_one(CNID_sqlite_private *db, sqlite3_stmt **stmt, const char *tag, const char *sql_fmt)
```

Defined at lines 353 to 374.

Prepare one statement of the volume; the caller reports a failure.

Parameters:
* `db`: SQLite backend state
* `stmt`: statement to finalize and prepare again
* `tag`: statement name for the log
* `sql_fmt`: SQL with s for the table name of the volume, or without one

Returns: the SQLite result, SQLITE_NOMEM when the SQL cannot be built

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [cnid_sqlite_index_prepare](cnid_sqlite.c.md#cnid_sqlite_index_prepare), [init_prepared_stmt_one](cnid_sqlite.c.md#init_prepared_stmt_one)

### init_prepared_stmt_one

```c
static int init_prepared_stmt_one(CNID_sqlite_private *db, sqlite3_stmt **stmt, const char *tag, const char *sql_fmt)
```

Defined at lines 389 to 403.

Prepare one statement of the volume, finalizing the previous one.

Preparing loads the schema, so a corrupt or read-only database fails here, and errno gets the same classification as a failed step.

Parameters:
* `db`: SQLite backend state
* `stmt`: statement to finalize and prepare again
* `tag`: statement name for the log
* `sql_fmt`: SQL with s for the table name of the volume, or without one

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_prepare_one](cnid_sqlite.c.md#cnid_sqlite_prepare_one), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Called by: [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt)

### init_prepared_stmt

```c
static int init_prepared_stmt(CNID_sqlite_private *db)
```

Defined at lines 417 to 473.

Prepare all 13 statements of the volume, at open and after a wipe.

The searches through the name index prepare last, where the table has one, and a failure of theirs leaves name searches to the scans. A failure of the 13 leaves the later statements NULL, which cnid_sqlite_stmt_ready then refuses. The scoped find walks the subtree with a recursive CTE, which needs SQLite 3.8.3 or later.

Parameters:
* `db`: SQLite backend state

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_index_prepare](cnid_sqlite.c.md#cnid_sqlite_index_prepare), [init_prepared_stmt_one](cnid_sqlite.c.md#init_prepared_stmt_one)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_index_retire

```c
static void cnid_sqlite_index_retire(CNID_sqlite_private *db)
```

Defined at lines 480 to 491.

Finalize the name index's searches and clear their pointers.

Parameters:
* `db`: SQLite backend state

Called by: [close_prepared_stmt](cnid_sqlite.c.md#close_prepared_stmt), [cnid_sqlite_index_abandon](cnid_sqlite.c.md#cnid_sqlite_index_abandon), [cnid_sqlite_index_prepare](cnid_sqlite.c.md#cnid_sqlite_index_prepare)

### cnid_sqlite_index_stmt

```c
static int cnid_sqlite_index_stmt(const CNID_sqlite_private *db, int nseeds, bool scoped, sqlite3_stmt **stmt)
```

Defined at lines 506 to 544.

Prepare the search through the name index for a number of seeds.

Each seed takes an arm of the index's candidates, a last arm the rows whose Ids the log holds, and the arms join by UNION.

Parameters:
* `db`: SQLite backend state
* `nseeds`: number of seeds, 1 to CNID_FIND_MAX_SEEDS
* `scoped`: whether the search walks its matches up to a directory
* `stmt`: receives the statement

Returns: the result of the prepare, SQLITE_NOMEM when the SQL cannot be built

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_index_prepare](cnid_sqlite.c.md#cnid_sqlite_index_prepare)

### close_prepared_stmt

```c
static void close_prepared_stmt(CNID_sqlite_private *db)
```

Defined at lines 551 to 580.

Finalize every prepared statement and clear the pointers.

Parameters:
* `db`: SQLite backend state

Calls: [cnid_sqlite_index_retire](cnid_sqlite.c.md#cnid_sqlite_index_retire)

Called by: [cnid_sqlite_close](cnid_sqlite.c.md#cnid_sqlite_close), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_hint_usable

```c
static bool cnid_sqlite_hint_usable(cnid_t hint)
```

Defined at lines 593 to 597.

Tell whether a CNID hint from AppleDouble or EA metadata is safe to bind.

The call rejects the reserved range under CNID_START. It also rejects any value above CNID_SQLITE_MAX_HINT, which as an explicit rowid would push the AUTOINCREMENT high-water mark toward the depletion reset.

Parameters:
* `hint`: CNID in network order

Returns: true when CNID_START <= hint <= CNID_SQLITE_MAX_HINT in host order

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add)

### cnid_sqlite_rowid_ok

```c
static bool cnid_sqlite_rowid_ok(uint64_t rowid)
```

Defined at lines 609 to 612.

Tell whether a rowid from the database still fits a CNID.

The Id column holds 64-bit integers, and a CNID has 32 bits. A narrowed rowid would name an unrelated live row, which the caller could then hand to a client or remove.

Parameters:
* `rowid`: rowid from the database

Returns: true when 1 <= rowid <= UINT32_MAX

Called by: [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve)

### cnid_sqlite_uuid_usable

```c
static bool cnid_sqlite_uuid_usable(const char *uuid)
```

Defined at lines 626 to 641.

Tell whether a volume UUID is safe to place in SQL text as a table name.

A table name cannot bind as a parameter. The uuid_strip_dashes call yields 32 hex digits. UUIDs read back from the volumes table are only as trustworthy as the database file, which local users can write. The call refuses any other value instead of quoting it.

Parameters:
* `uuid`: UUID string, or NULL

Returns: true for exactly 32 hex digits

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_set_errno

```c
static void cnid_sqlite_set_errno(int sqlite_return)
```

Defined at lines 658 to 692.

Map a SQLite result code onto the CNID_ERR_* contract in errno.

CNID_ERR_BUSY covers states that clear by themselves: contention, a full disk, I/O errors, out of memory, SQLITE_PROTOCOL, and the READONLY recovery states. SQLITE_READONLY maps to EROFS, which afp_deleteid answers with AFPERR_VLOCK. SQLITE_LOCKED stays out of the busy set, because a retry meets the same conflict.

Every other code fails the single operation as CNID_ERR_CORRUPT. CNID_ERR_DB, which ends the session, means an unreachable backend, and a local database file has no such state.

Parameters:
* `sqlite_return`: extended SQLite result code

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_delete_by_uuid](cnid_sqlite.c.md#cnid_sqlite_delete_by_uuid), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_index_create](cnid_sqlite.c.md#cnid_sqlite_index_create), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [init_prepared_stmt_one](cnid_sqlite.c.md#init_prepared_stmt_one)

### cnid_sqlite_execute

```c
static int cnid_sqlite_execute(sqlite3 *con, const char *sql)
```

Defined at lines 701 to 715.

Run one SQL statement and classify a failure into errno.

Parameters:
* `con`: SQLite connection
* `sql`: SQL text

Returns: 0, or -1 with errno set by cnid_sqlite_set_errno

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_empty_table](cnid_sqlite.c.md#cnid_sqlite_empty_table), [cnid_sqlite_index_drop](cnid_sqlite.c.md#cnid_sqlite_index_drop), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_delete_by_uuid

```c
static int cnid_sqlite_delete_by_uuid(sqlite3 *con, const char *sql, const char *uuid)
```

Defined at lines 729 to 757.

Remove a row by UUID through a bound parameter.

The UUID comes from the database file, which local users can write, so it never enters the SQL text. A crafted value would otherwise run as SQL under the root privileges of the cleanup.

Parameters:
* `con`: SQLite connection
* `sql`: SQL statement with 1 placeholder
* `uuid`: UUID of the row

Returns: 0, or -1 with errno set by cnid_sqlite_set_errno

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Called by: [cnid_sqlite_delete_sequence_row](cnid_sqlite.c.md#cnid_sqlite_delete_sequence_row), [cnid_sqlite_delete_volumes_row](cnid_sqlite.c.md#cnid_sqlite_delete_volumes_row)

### cnid_sqlite_delete_volumes_row

```c
static int cnid_sqlite_delete_volumes_row(sqlite3 *con, const char *uuid)
```

Defined at lines 766 to 770.

Remove the row of a stale volume from the volumes table.

Parameters:
* `con`: SQLite connection
* `uuid`: UUID of the stale volume

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_delete_by_uuid](cnid_sqlite.c.md#cnid_sqlite_delete_by_uuid)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_delete_sequence_row

```c
static int cnid_sqlite_delete_sequence_row(sqlite3 *con, const char *uuid)
```

Defined at lines 779 to 783.

Remove the AUTOINCREMENT high-water mark of a stale volume.

Parameters:
* `con`: SQLite connection
* `uuid`: UUID of the stale volume

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_delete_by_uuid](cnid_sqlite.c.md#cnid_sqlite_delete_by_uuid)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_stmt_reset

```c
static void cnid_sqlite_stmt_reset(sqlite3_stmt *stmt)
```

Defined at lines 794 to 799.

Reset a prepared statement and keep errno.

The sqlite3_reset call enters the VFS and can leave errno from a speculative syscall. Cleanup paths run it after errno holds the classification, the only failure signal of the caller.

Parameters:
* `stmt`: prepared statement

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update)

### cnid_sqlite_stmt_ready

```c
static bool cnid_sqlite_stmt_ready(sqlite3_stmt *stmt, const char *caller)
```

Defined at lines 812 to 823.

Tell whether a prepared statement is available to bind and step.

The statements stay NULL when the reprepare of cnid_sqlite_wipe stopped partway. Binding tolerates a NULL statement, but sqlite3_step on one invokes undefined behavior, so every entry point checks the statements it uses.

Parameters:
* `stmt`: prepared statement
* `caller`: function name for the log

Returns: true, or false with errno CNID_ERR_CLOSE

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_dirlist](cnid_sqlite.c.md#cnid_sqlite_dirlist), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update)

### cnid_sqlite_begin

```c
static int cnid_sqlite_begin(sqlite3 *con, int *owned)
```

Defined at lines 836 to 844.

Open a transaction and mark it as owned by the calling function.

A failed COMMIT can leave the connection inside a transaction. The owner flag lets a cleanup path roll back only a transaction that its function opened, never one of a caller.

Parameters:
* `con`: SQLite connection
* `owned`: set to 1 once the transaction opens

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_index_changed](cnid_sqlite.c.md#cnid_sqlite_index_changed), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_commit

```c
static int cnid_sqlite_commit(sqlite3 *con, int *owned)
```

Defined at lines 856 to 868.

Commit the owned transaction and release ownership on success only.

SQLITE_BUSY on COMMIT leaves the transaction open, and only the cleanup rollback can close it, so ownership survives a failed COMMIT.

Parameters:
* `con`: SQLite connection
* `owned`: cleared after a successful COMMIT

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_index_changed](cnid_sqlite.c.md#cnid_sqlite_index_changed), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_rollback

```c
static void cnid_sqlite_rollback(sqlite3 *con, int *owned)
```

Defined at lines 879 to 892.

Roll back an owned transaction and keep errno.

Cleanup paths call it after errno holds the classification, and the ROLLBACK enters the VFS, which could change errno.

Parameters:
* `con`: SQLite connection, or NULL
* `owned`: owner flag, cleared by the call

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_index_changed](cnid_sqlite.c.md#cnid_sqlite_index_changed), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_seed_sequence

```c
static int cnid_sqlite_seed_sequence(CNID_sqlite_private *db)
```

Defined at lines 905 to 929.

Reset the AUTOINCREMENT sequence of the volume to CNID_START - 1.

The call updates the sequence row and inserts it only when the UPDATE changed no row, because sqlite_sequence has no UNIQUE constraint for an upsert. It runs inside the transaction of the caller, which also owns the rollback.

Parameters:
* `db`: SQLite backend state

Returns: 0, or -1 with errno classified

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_empty_table](cnid_sqlite.c.md#cnid_sqlite_empty_table), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_count

```c
static int cnid_sqlite_count(const CNID_sqlite_private *db, const char *fmt)
```

Defined at lines 939 to 957.

Read the count that a SQL template of the volume selects.

Parameters:
* `db`: SQLite backend state
* `fmt`: SQL with %1$s for the table name, selecting the count

Returns: the count, or -1 when it cannot be read, with the reason in sqlite3_errmsg()

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [cnid_sqlite_candidates](cnid_sqlite.c.md#cnid_sqlite_candidates), [cnid_sqlite_index_changed](cnid_sqlite.c.md#cnid_sqlite_index_changed), [cnid_sqlite_index_create](cnid_sqlite.c.md#cnid_sqlite_index_create), [cnid_sqlite_index_prepare](cnid_sqlite.c.md#cnid_sqlite_index_prepare)

### cnid_sqlite_index_drop

```c
static int cnid_sqlite_index_drop(sqlite3 *con, const char *uuid)
```

Defined at lines 973 to 1015.

Remove the name index of a table in one savepoint.

The full-text table drops on its own statement and may stay: a library without FTS5 cannot drop it, and once the triggers and the log go the table logs nothing. Outside a transaction RELEASE commits; a reader that holds off that commit in rollback-journal mode holds off the fallback's RELEASE too, so there the fallback is ROLLBACK.

Parameters:
* `con`: SQLite connection
* `uuid`: name of the table

Returns: 0, or -1 with errno classified when the index stays or a failure rolled back the transaction

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_empty_table](cnid_sqlite.c.md#cnid_sqlite_empty_table), [cnid_sqlite_index_create](cnid_sqlite.c.md#cnid_sqlite_index_create), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_names_left

```c
static bool cnid_sqlite_names_left(sqlite3 *con, const char *uuid)
```

Defined at lines 1024 to 1045.

Tell whether the full-text table of a stale volume remains.

Parameters:
* `con`: SQLite connection
* `uuid`: name of the volume's table

Returns: true when the table remains or the check fails

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_index_create

```c
static int cnid_sqlite_index_create(const CNID_sqlite_private *db)
```

Defined at lines 1061 to 1132.

Give an empty table its name index, in the caller's write transaction.

The index takes in only rows written after it, so a table with rows keeps its scan and an empty table with a whole index keeps it. Part of an index, as a drop of the table alone leaves, goes first. A library without FTS5 or its trigram tokenizer answers SQLITE_ERROR and leaves the table without an index.

Parameters:
* `db`: SQLite backend state

Returns: 0, or -1 with errno classified when a failure ended the caller's transaction or left an empty table without the index, which it would never get once rows fill it

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_sqlite_count](cnid_sqlite.c.md#cnid_sqlite_count), [cnid_sqlite_index_drop](cnid_sqlite.c.md#cnid_sqlite_index_drop), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Called by: [cnid_sqlite_empty_table](cnid_sqlite.c.md#cnid_sqlite_empty_table), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_index_broken

```c
static bool cnid_sqlite_index_broken(int rc)
```

Defined at lines 1143 to 1146.

Tell whether a failure on the name index says the index is unusable.

An index another connection dropped answers SQLITE_ERROR, one that fails FTS5's own checks SQLITE_CORRUPT.

Parameters:
* `rc`: SQLite result of the prepare, step or move

Returns: true for SQLITE_ERROR and SQLITE_CORRUPT

Called by: [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_index_changed](cnid_sqlite.c.md#cnid_sqlite_index_changed), [cnid_sqlite_index_prepare](cnid_sqlite.c.md#cnid_sqlite_index_prepare)

### cnid_sqlite_index_abandon

```c
static void cnid_sqlite_index_abandon(CNID_sqlite_private *db, const char *reason, const char *remedy)
```

Defined at lines 1162 to 1170.

Stop using the name index on this connection, and log why.

The connection's searches scan the table; the index stays for the connections that can use it.

Parameters:
* `db`: SQLite backend state
* `reason`: why, for the log
* `remedy`: what serves the index again, for the log

Calls: [cnid_sqlite_index_retire](cnid_sqlite.c.md#cnid_sqlite_index_retire)

Called by: [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_index_changed](cnid_sqlite.c.md#cnid_sqlite_index_changed), [cnid_sqlite_index_prepare](cnid_sqlite.c.md#cnid_sqlite_index_prepare)

### cnid_sqlite_index_prepare

```c
static void cnid_sqlite_index_prepare(CNID_sqlite_private *db)
```

Defined at lines 1180 to 1241.

Prepare the name index's searches when this library reads a whole one.

The changes the log holds count as writes of the connection, so a full log that other connections left moves at its next write.

Parameters:
* `db`: SQLite backend state

Calls: [cnid_sqlite_count](cnid_sqlite.c.md#cnid_sqlite_count), [cnid_sqlite_index_abandon](cnid_sqlite.c.md#cnid_sqlite_index_abandon), [cnid_sqlite_index_broken](cnid_sqlite.c.md#cnid_sqlite_index_broken), [cnid_sqlite_index_retire](cnid_sqlite.c.md#cnid_sqlite_index_retire), [cnid_sqlite_index_stmt](cnid_sqlite.c.md#cnid_sqlite_index_stmt), [cnid_sqlite_prepare_one](cnid_sqlite.c.md#cnid_sqlite_prepare_one)

Called by: [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt)

### cnid_sqlite_index_changed

```c
static void cnid_sqlite_index_changed(CNID_sqlite_private *db)
```

Defined at lines 1254 to 1297.

Count a write; at a batch, move the lowest logged Ids into the index.

The changes the log still holds after a move count as writes again, so a larger log moves on the next writes. A move that says the index is broken ends the connection's use of it; after any other failure, a COMMIT's included, the changes stay logged for CNID_SQLITE_INDEX_BATCH more writes. errno is kept.

Parameters:
* `db`: SQLite backend state

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_count](cnid_sqlite.c.md#cnid_sqlite_count), [cnid_sqlite_index_abandon](cnid_sqlite.c.md#cnid_sqlite_index_abandon), [cnid_sqlite_index_broken](cnid_sqlite.c.md#cnid_sqlite_index_broken), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update)

### cnid_sqlite_indexable

```c
static bool cnid_sqlite_indexable(const CNID_sqlite_private *db, const struct sl_name_pattern *seed)
```

Defined at lines 1312 to 1334.

Tell whether the name index narrows a search for a seed.

The trigram index needs 3 characters in a row free of the wildcards of its pattern; without them it reads every name it holds, slower than the table scan. A pattern longer than the connection's limit for a GLOB pattern fails each candidate's check against the index's copy of its name, so it takes the scan too.

Parameters:
* `db`: SQLite backend state
* `seed`: the seed

Returns: true when the index pattern holds such a run within the limit

Calls: [sl_name_pattern_index_glob](../util/sl_names.c.md#sl_name_pattern_index_glob)

Called by: [cnid_sqlite_find_narrows](cnid_sqlite.c.md#cnid_sqlite_find_narrows)

Mentioned in the documentation of: [cnid_sqlite_find_narrows](cnid_sqlite.c.md#cnid_sqlite_find_narrows)

### cnid_sqlite_candidates

```c
static unsigned int cnid_sqlite_candidates(const CNID_sqlite_private *db, const struct sl_name_pattern *const *seeds, int nseeds, unsigned int limit)
```

Defined at lines 1348 to 1374.

Count the seeds' index candidates and the logged Ids, up to a limit.

A candidate of two seeds counts twice and a logged Id whether it matches or not, so the count bounds the walk from above.

Parameters:
* `db`: SQLite backend state
* `seeds`: the seeds
* `nseeds`: number of seeds
* `limit`: the count to stop at, at most CNID_SQLITE_WALK_MAX + 1

Returns: the count, limit when it reaches it or cannot be read

Calls: [cnid_sqlite_count](cnid_sqlite.c.md#cnid_sqlite_count), [sl_name_pattern_index_glob](../util/sl_names.c.md#sl_name_pattern_index_glob)

Called by: [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find)

### cnid_sqlite_find_narrows

```c
static bool cnid_sqlite_find_narrows(struct _cnid_db *cdb, const struct sl_name_pattern *seed, cnid_t scope_did)
```

Defined at lines 1387 to 1394.

cnid_find_narrows: whether the name index narrows a search by a seed

A folder search takes the same seeds as a search of the volume.

Parameters:
* `cdb`: CNID database
* `seed`: the seed
* `scope_did`: unused

Returns: true when the connection reads the index and [cnid_sqlite_indexable()](cnid_sqlite.c.md#cnid_sqlite_indexable) takes the seed

Calls: [cnid_sqlite_indexable](cnid_sqlite.c.md#cnid_sqlite_indexable)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_empty_table

```c
static int cnid_sqlite_empty_table(CNID_sqlite_private *db)
```

Defined at lines 1406 to 1425.

Empty the volume's table and restart its CNIDs at CNID_START.

It runs inside the caller's transaction. The name index goes first, so the delete logs no changes and empties the table at once, and it comes back with the empty table.

Parameters:
* `db`: SQLite backend state

Returns: 0, or -1 with errno classified

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_index_create](cnid_sqlite.c.md#cnid_sqlite_index_create), [cnid_sqlite_index_drop](cnid_sqlite.c.md#cnid_sqlite_index_drop), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_delete

```c
int cnid_sqlite_delete(struct _cnid_db *cdb, const cnid_t id)
```

Defined at lines 1434 to 1476.

Remove a CNID from the SQLite table.

Parameters:
* `cdb`: CNID database
* `id`: CNID in network order, not 0

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_index_changed](cnid_sqlite.c.md#cnid_sqlite_index_changed), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_close

```c
void cnid_sqlite_close(struct _cnid_db *cdb)
```

Defined at lines 1487 to 1523.

Close the SQLite connection of a CNID database and free the database.

The call first folds the WAL back into the database with a TRUNCATE checkpoint and no busy timeout. Readers in other sessions can make the checkpoint skip, and the next close tries again.

Parameters:
* `cdb`: CNID database, and a NULL one logs an error

Calls: [close_prepared_stmt](cnid_sqlite.c.md#close_prepared_stmt)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_update

```c
int cnid_sqlite_update(struct _cnid_db *cdb, cnid_t id, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 1540 to 1700.

Rewrite the entry of a CNID with a new name, parent, and stat.

An UPDATE that matches no row inserts the entry with its CNID. A SQLITE_CONSTRAINT collision retries once inside a transaction, after the call removes the rows that hold the name and parent or the device and inode.

Parameters:
* `cdb`: CNID database
* `id`: CNID to keep
* `st`: stat of the object
* `did`: new parent CNID
* `name`: new name on disk
* `len`: length of name, up to MAXPATHLEN

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_index_changed](cnid_sqlite.c.md#cnid_sqlite_index_changed), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_lookup

```c
cnid_t cnid_sqlite_lookup(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 1722 to 1969.

Find the CNID of an object by name and parent or by device and inode.

Extra matching rows are stale, and the call removes them and reports CNID_ERR_NOTFOUND. A row with an ID or parent outside the CNID range means a corrupted database.

A row with another name or parent means a server-side move or a reused inode. When its CNID equals the hint of the current add, the row takes the new name and parent. Otherwise, and for a row with another device or inode, the call removes the row and reports CNID_ERR_NOTFOUND, so cnid_sqlite_add inserts a new entry.

Parameters:
* `cdb`: CNID database
* `st`: stat of the object
* `did`: CNID of the parent directory, at least 2
* `name`: name on disk
* `len`: length of name, up to MAXPATHLEN

Returns: the CNID, or CNID_INVALID with errno classified

Calls: [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_add

```c
cnid_t cnid_sqlite_add(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len, cnid_t hint)
```

Defined at lines 1992 to 2260.

Find the CNID of an object or add a new entry for it.

A failed lookup leads to an insert only on CNID_ERR_NOTFOUND. The call refuses an empty name, a parent of 0, or an inode of 0. A usable hint goes in as an explicit rowid, unless a depletion emptied the table before. SQLITE_CONSTRAINT from a racing session drops the hint and repeats the lookup, up to CNID_SQLITE_ADD_ATTEMPTS times.

Near CNID_SQLITE_MAX_HINT the call warns once. At UINT32_MAX it empties the table, reseeds the sequence, marks the volume depleted, and fails with CNID_ERR_RESET, so the session ends.

Parameters:
* `cdb`: CNID database
* `st`: stat of the object
* `did`: CNID of the parent directory
* `name`: name on disk
* `len`: length of name, up to MAXPATHLEN
* `hint`: CNID from the metadata of the object, or CNID_INVALID

Returns: the CNID, or CNID_INVALID with errno classified

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_empty_table](cnid_sqlite.c.md#cnid_sqlite_empty_table), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_hint_usable](cnid_sqlite.c.md#cnid_sqlite_hint_usable), [cnid_sqlite_index_changed](cnid_sqlite.c.md#cnid_sqlite_index_changed), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_get

```c
cnid_t cnid_sqlite_get(struct _cnid_db *cdb, cnid_t did, const char *name, size_t len)
```

Defined at lines 2274 to 2360.

Look up the CNID that a directory holds under a name.

A rowid outside the CNID range means a corrupted database.

Parameters:
* `cdb`: CNID database
* `did`: CNID of the directory
* `name`: name on disk
* `len`: length of name, up to MAXPATHLEN

Returns: the CNID, or CNID_INVALID with errno CNID_ERR_NOTFOUND for a missing name or another classified value

Calls: [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_resolve

```c
char * cnid_sqlite_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len)
```

Defined at lines 2376 to 2474.

Resolve a CNID to its parent and name in the SQLite table.

Only a missing row reports CNID_ERR_NOTFOUND, which afp_resolveid answers with the permanent AFPERR_NOID. A parent outside the CNID range or a NULL name means a corrupted database.

Parameters:
* `cdb`: CNID database
* `id`: CNID to resolve, receives the parent CNID or CNID_INVALID
* `buffer`: receives the name, always NUL-terminated
* `len`: size of buffer, not 0

Returns: buffer, or NULL with errno classified

Calls: [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_getstamp

```c
int cnid_sqlite_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
```

Defined at lines 2487 to 2571.

Read the stamp of a volume from the volumes table.

A missing row or a NULL stamp means unusable data, not a failing backend, so both report CNID_ERR_CORRUPT and the session survives.

Parameters:
* `cdb`: CNID database
* `buffer`: receives the stamp, written as a string
* `len`: size of buffer

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_find_bind

```c
static int cnid_sqlite_find_bind(sqlite3_stmt *stmt, bool indexed, const struct sl_name_query *query, const struct sl_name_pattern *const *seeds, int nseeds, cnid_t scope_did, sqlite3_int64 limit)
```

Defined at lines 2585 to 2613.

Bind a name search's parameters.

Parameters:
* `stmt`: the statement
* `indexed`: whether it searches through the index
* `query`: the name comparisons
* `seeds`: the seeds
* `nseeds`: number of seeds
* `scope_did`: CNID of the scope directory, or CNID_INVALID
* `limit`: rows the statement returns at most

Returns: SQLITE_OK, or the result of the bind that failed

Calls: [sl_name_pattern_index_glob](../util/sl_names.c.md#sl_name_pattern_index_glob)

Called by: [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find)

### cnid_sqlite_find

```c
int cnid_sqlite_find(struct _cnid_db *cdb, const struct sl_name_query *query, const struct sl_name_pattern *const *seeds, int nseeds, cnid_t scope_did, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 2641 to 2824.

Find the CNIDs whose names match a query, for cnid_find.

The cnid_find wrapper validates the arguments. The statements decide each name through netatalk_match(), the query bound as a pointer, so the LIMIT counts matches. The statement asks for 1 row more than buffer holds, which sets more_available. A row with an ID outside the CNID range drops out, and the search runs again with room for the rows it skipped. The seeds read the candidates of the name index in one statement; a folder search walks them up its folders for CNID_SQLITE_WALK_MAX candidates at most, the change log's Ids counted with them, and scans the folder otherwise, counting and walking in one transaction. A failure that says the index is unusable ends the connection's use of it and repeats the search as a scan.

Parameters:
* `cdb`: CNID database
* `query`: the name comparisons
* `seeds`: the query's seeds, or NULL for every name
* `nseeds`: number of seeds
* `scope_did`: CNID of the scope directory, or CNID_INVALID for an unscoped search
* `buffer`: receives the CNIDs in network order
* `buflen`: size of buffer
* `more_available`: receives true when more matches exist, or NULL

Returns: the number of CNIDs, or -1 with errno classified

Calls: [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_candidates](cnid_sqlite.c.md#cnid_sqlite_candidates), [cnid_sqlite_find_bind](cnid_sqlite.c.md#cnid_sqlite_find_bind), [cnid_sqlite_index_abandon](cnid_sqlite.c.md#cnid_sqlite_index_abandon), [cnid_sqlite_index_broken](cnid_sqlite.c.md#cnid_sqlite_index_broken), [cnid_sqlite_index_stmt](cnid_sqlite.c.md#cnid_sqlite_index_stmt), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_wipe

```c
int cnid_sqlite_wipe(struct _cnid_db *cdb)
```

Defined at lines 2839 to 2906.

Empty the SQLite table of a volume and restart its CNIDs at CNID_START.

One transaction clears the depletion mark, removes every row, and reseeds the sequence. The call then clears the depletion flags of the connection, so a dbd rebuild restores the CNIDs from the metadata, and prepares the statements again. A failed reprepare leaves every statement closed until the database reopens.

Parameters:
* `cdb`: CNID database

Returns: 0, or -1 with errno classified, CNID_ERR_CLOSE after a failed reprepare

Calls: [asprintf](../compat/misc.c.md#asprintf), [close_prepared_stmt](cnid_sqlite.c.md#close_prepared_stmt), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_empty_table](cnid_sqlite.c.md#cnid_sqlite_empty_table), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_dirlist

```c
static int cnid_sqlite_dirlist(struct _cnid_db *cdb, cnid_t did, size_t max, cnid_dirlist_fn fn, void *ctx)
```

Defined at lines 2918 to 2950.

Hand at most max rows of one folder to fn.

Parameters:
* `cdb`: CNID database
* `did`: the folder
* `max`: the rows to hand over at most
* `fn`: receives each row; nonzero stops the rows
* `ctx`: passed to fn

Returns: 0, or -1 when the query or fn failed

Calls: [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_new

```c
static struct _cnid_db * cnid_sqlite_new(struct vol *vol)
```

Defined at lines 2958 to 2981.

Allocate a CNID database object with the SQLite function table.

Parameters:
* `vol`: volume of the database

Returns: the object, or NULL when calloc fails

Calls: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_close](cnid_sqlite.c.md#cnid_sqlite_close), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_dirlist](cnid_sqlite.c.md#cnid_sqlite_dirlist), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_find_narrows](cnid_sqlite.c.md#cnid_sqlite_find_narrows), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_refused

```c
static bool cnid_sqlite_refused(const struct vol *vol, const char *dir, const struct stat *st)
```

Defined at lines 2993 to 3028.

Tell whether a private volume refuses a CNID directory it found, and log why.

Parameters:
* `vol`: the volume
* `dir`: the directory
* `st`: the directory's lstat or fstat

Returns: true for a symbolic link, the volume's own root, or another account's directory

Calls: [shell_quote](../util/unix.c.md#shell_quote)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_report_orphan

```c
static void cnid_sqlite_report_orphan(const struct vol *vol, const char *dbdir)
```

Defined at lines 3041 to 3075.

Log an unused CNID database named after the volume in its share root.

The call looks for the v_localname file with the .sqlite suffix in the share root, unless the share root is the database directory. Only a regular file with the SQLite header counts, opened without following a link or blocking.

Parameters:
* `vol`: volume
* `dbdir`: database directory in use

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_open

```c
struct _cnid_db * cnid_sqlite_open(struct cnid_open_args *args)
```

Defined at lines 3097 to 3785.

Open the SQLite CNID backend for a volume.

A private volume's database stays its owner's, with mode 0600 in a 0700 directory that the call creates or checks as the calling process, which [cnid_open()](cnid.c.md#cnid_open) makes the owner. A shared volume's is shared, with mode 0666 in a 01777 directory that the call creates or checks as root. The directory is opened O_NOFOLLOW, a private one's path first losing the trailing slashes that make the open follow a final link. It opens the database with SQLITE_OPEN_NOFOLLOW and checks the -wal and -shm files before and after it turns on WAL.

The volumes table maps each volume path to its UUID, stamp, and depletion mark. A UUID that differs only in case takes the spelling on disk, and the call drops the tables of the same path under another UUID. A new CNID table starts its sequence at CNID_START.

Parameters:
* `args`: open arguments with the volume and the flags

Returns: the database, or NULL on error

Calls: [asprintf](../compat/misc.c.md#asprintf), [become_root](../util/unix.c.md#become_root), [close_prepared_stmt](cnid_sqlite.c.md#close_prepared_stmt), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_delete_sequence_row](cnid_sqlite.c.md#cnid_sqlite_delete_sequence_row), [cnid_sqlite_delete_volumes_row](cnid_sqlite.c.md#cnid_sqlite_delete_volumes_row), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_fchmod_companions](cnid_sqlite.c.md#cnid_sqlite_fchmod_companions), [cnid_sqlite_fchmod_regular](cnid_sqlite.c.md#cnid_sqlite_fchmod_regular), [cnid_sqlite_fold](cnid_sqlite.c.md#cnid_sqlite_fold), [cnid_sqlite_index_create](cnid_sqlite.c.md#cnid_sqlite_index_create), [cnid_sqlite_index_drop](cnid_sqlite.c.md#cnid_sqlite_index_drop), [cnid_sqlite_match](cnid_sqlite.c.md#cnid_sqlite_match), [cnid_sqlite_names_left](cnid_sqlite.c.md#cnid_sqlite_names_left), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new), [cnid_sqlite_refused](cnid_sqlite.c.md#cnid_sqlite_refused), [cnid_sqlite_report_orphan](cnid_sqlite.c.md#cnid_sqlite_report_orphan), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [cnid_sqlite_uuid_usable](cnid_sqlite.c.md#cnid_sqlite_uuid_usable), [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt), [shell_quote](../util/unix.c.md#shell_quote), [stripped_slashes_basename](../util/unix.c.md#stripped_slashes_basename), [unbecome_root](../util/unix.c.md#unbecome_root), [uuid_strip_dashes](../util/cnid.c.md#uuid_strip_dashes)

Called through [`_cnid_module::cnid_open`](../../include/atalk/cnid.h.md#struct-_cnid_module) by: [cnid_open](cnid.c.md#cnid_open)

Dispatched via: [cnid_sqlite_module](cnid_sqlite.c.md#cnid_sqlite_module)

# Macros

* Undocumented: `CNID_SQLITE_ADD_ATTEMPTS`, `CNID_SQLITE_BUSY_TIMEOUT`, `CNID_SQLITE_EMPTY_PROBE_TRIES`, `CNID_SQLITE_FIND_ARM_SQL`, `CNID_SQLITE_FIND_INDEX_SQL`, `CNID_SQLITE_FIND_LIMIT`, `CNID_SQLITE_FIND_LOG_SQL`, `CNID_SQLITE_FIND_MATCH`, `CNID_SQLITE_FIND_SCOPE`, `CNID_SQLITE_FIND_SCOPED_INDEX_SQL`, `CNID_SQLITE_HINT_RESERVE`, `CNID_SQLITE_INDEX_BATCH`, `CNID_SQLITE_INDEX_CANDIDATES_SQL`, `CNID_SQLITE_INDEX_DROP_SQL`, `CNID_SQLITE_INDEX_LOGGED_SQL`, `CNID_SQLITE_INDEX_MOVE`, `CNID_SQLITE_INDEX_OBJECTS`, `CNID_SQLITE_INDEX_OBJECTS_SQL`, `CNID_SQLITE_INDEX_REBUILT`, `CNID_SQLITE_INDEX_SQL`, `CNID_SQLITE_INDEX_UPDATE_SQL`, `CNID_SQLITE_MAX_HINT`, `CNID_SQLITE_NAMES`, `CNID_SQLITE_STR`, `CNID_SQLITE_STR_`, `CNID_SQLITE_WALK_LOGGED_SQL`, `CNID_SQLITE_WALK_MAX`
