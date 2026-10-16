---
type: C Source File
title: "libatalk/cnid/sqlite/cnid_sqlite.c"
description: "SQLite CNID backend."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/sqlite/cnid_sqlite.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T06:55:24+02:00 }
---

Part of the [libatalk/cnid](../../cnid.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/adouble.h](../../../include/atalk/adouble.h.md)
* [atalk/cnid_sqlite_private.h](../../../include/atalk/cnid_sqlite_private.h.md)
* [atalk/errchk.h](../../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../../include/atalk/globals.h.md)
* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [atalk/unix.h](../../../include/atalk/unix.h.md)
* [atalk/util.h](../../../include/atalk/util.h.md)
* [atalk/volume.h](../../../include/atalk/volume.h.md)
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

Defined at lines 94 to 141.

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

Defined at lines 153 to 174.

Check and set the mode of the -wal and -shm files of a SQLite database.

The call runs before SQLite opens them, to reject unsafe files, and again after SQLite creates them, to set their mode.

Parameters:
* `dbpath`: path of the database file
* `mode`: permission bits

Returns: 0, or -1 for an unsafe companion or a path that is too long

Calls: [cnid_sqlite_fchmod_regular](cnid_sqlite.c.md#cnid_sqlite_fchmod_regular)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### init_prepared_stmt_one

```c
static int init_prepared_stmt_one(CNID_sqlite_private *db, sqlite3_stmt **stmt, const char *tag, const char *sql_fmt)
```

Defined at lines 189 to 222.

Prepare one statement of the volume, finalizing the previous one.

Preparing loads the schema, so a corrupt or read-only database fails here, and errno gets the same classification as a failed step.

Parameters:
* `db`: SQLite backend state
* `stmt`: statement to finalize and prepare again
* `tag`: statement name for the log
* `sql_fmt`: SQL with s for the table name of the volume, or without one

Returns: 0, or -1 with errno classified

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Called by: [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt)

### init_prepared_stmt

```c
static int init_prepared_stmt(CNID_sqlite_private *db)
```

Defined at lines 234 to 278.

Prepare all 12 statements of the volume, at open and after a wipe.

A failure leaves the later statements NULL, which cnid_sqlite_stmt_ready then refuses. The scoped find walks the subtree with a recursive CTE, which needs SQLite 3.8.3 or later.

Parameters:
* `db`: SQLite backend state

Returns: 0, or -1 with errno classified

Calls: [init_prepared_stmt_one](cnid_sqlite.c.md#init_prepared_stmt_one)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### close_prepared_stmt

```c
static void close_prepared_stmt(CNID_sqlite_private *db)
```

Defined at lines 285 to 311.

Finalize every prepared statement and clear the pointers.

Parameters:
* `db`: SQLite backend state

Called by: [cnid_sqlite_close](cnid_sqlite.c.md#cnid_sqlite_close), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_hint_usable

```c
static bool cnid_sqlite_hint_usable(cnid_t hint)
```

Defined at lines 324 to 328.

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

Defined at lines 340 to 343.

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

Defined at lines 357 to 372.

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

Defined at lines 389 to 423.

Map a SQLite result code onto the CNID_ERR_* contract in errno.

CNID_ERR_BUSY covers states that clear by themselves: contention, a full disk, I/O errors, out of memory, SQLITE_PROTOCOL, and the READONLY recovery states. SQLITE_READONLY maps to EROFS, which afp_deleteid answers with AFPERR_VLOCK. SQLITE_LOCKED stays out of the busy set, because a retry meets the same conflict.

Every other code fails the single operation as CNID_ERR_CORRUPT. CNID_ERR_DB, which ends the session, means an unreachable backend, and a local database file has no such state.

Parameters:
* `sqlite_return`: extended SQLite result code

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_delete_by_uuid](cnid_sqlite.c.md#cnid_sqlite_delete_by_uuid), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [init_prepared_stmt_one](cnid_sqlite.c.md#init_prepared_stmt_one)

### cnid_sqlite_execute

```c
static int cnid_sqlite_execute(sqlite3 *con, const char *sql)
```

Defined at lines 432 to 446.

Run one SQL statement and classify a failure into errno.

Parameters:
* `con`: SQLite connection
* `sql`: SQL text

Returns: 0, or -1 with errno set by cnid_sqlite_set_errno

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_delete_by_uuid

```c
static int cnid_sqlite_delete_by_uuid(sqlite3 *con, const char *sql, const char *uuid)
```

Defined at lines 460 to 488.

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

Defined at lines 497 to 501.

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

Defined at lines 510 to 514.

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

Defined at lines 525 to 530.

Reset a prepared statement and keep errno.

The sqlite3_reset call enters the VFS and can leave errno from a speculative syscall. Cleanup paths run it after errno holds the classification, the only failure signal of the caller.

Parameters:
* `stmt`: prepared statement

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update)

### cnid_sqlite_stmt_ready

```c
static bool cnid_sqlite_stmt_ready(sqlite3_stmt *stmt, const char *caller)
```

Defined at lines 543 to 554.

Tell whether a prepared statement is available to bind and step.

The statements stay NULL when the reprepare of cnid_sqlite_wipe stopped partway. Binding tolerates a NULL statement, but sqlite3_step on one invokes undefined behavior, so every entry point checks the statements it uses.

Parameters:
* `stmt`: prepared statement
* `caller`: function name for the log

Returns: true, or false with errno CNID_ERR_CLOSE

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update)

### cnid_sqlite_begin

```c
static int cnid_sqlite_begin(sqlite3 *con, int *owned)
```

Defined at lines 567 to 575.

Open a transaction and mark it as owned by the calling function.

A failed COMMIT can leave the connection inside a transaction. The owner flag lets a cleanup path roll back only a transaction that its function opened, never one of a caller.

Parameters:
* `con`: SQLite connection
* `owned`: set to 1 once the transaction opens

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_commit

```c
static int cnid_sqlite_commit(sqlite3 *con, int *owned)
```

Defined at lines 587 to 599.

Commit the owned transaction and release ownership on success only.

SQLITE_BUSY on COMMIT leaves the transaction open, and only the cleanup rollback can close it, so ownership survives a failed COMMIT.

Parameters:
* `con`: SQLite connection
* `owned`: cleared after a successful COMMIT

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_rollback

```c
static void cnid_sqlite_rollback(sqlite3 *con, int *owned)
```

Defined at lines 610 to 623.

Roll back an owned transaction and keep errno.

Cleanup paths call it after errno holds the classification, and the ROLLBACK enters the VFS, which could change errno.

Parameters:
* `con`: SQLite connection, or NULL
* `owned`: owner flag, cleared by the call

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_seed_sequence

```c
static int cnid_sqlite_seed_sequence(CNID_sqlite_private *db)
```

Defined at lines 636 to 660.

Reset the AUTOINCREMENT sequence of the volume to CNID_START - 1.

The call updates the sequence row and inserts it only when the UPDATE changed no row, because sqlite_sequence has no UNIQUE constraint for an upsert. It runs inside the transaction of the caller, which also owns the rollback.

Parameters:
* `db`: SQLite backend state

Returns: 0, or -1 with errno classified

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_delete

```c
int cnid_sqlite_delete(struct _cnid_db *cdb, const cnid_t id)
```

Defined at lines 669 to 710.

Remove a CNID from the SQLite table.

Parameters:
* `cdb`: CNID database
* `id`: CNID in network order, not 0

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_close

```c
void cnid_sqlite_close(struct _cnid_db *cdb)
```

Defined at lines 721 to 757.

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

Defined at lines 774 to 933.

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

Calls: [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_lookup

```c
cnid_t cnid_sqlite_lookup(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 955 to 1202.

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

Calls: [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [strlcpy](../../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_add

```c
cnid_t cnid_sqlite_add(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len, cnid_t hint)
```

Defined at lines 1225 to 1499.

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

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_hint_usable](cnid_sqlite.c.md#cnid_sqlite_hint_usable), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_get

```c
cnid_t cnid_sqlite_get(struct _cnid_db *cdb, cnid_t did, const char *name, size_t len)
```

Defined at lines 1513 to 1599.

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

Defined at lines 1615 to 1713.

Resolve a CNID to its parent and name in the SQLite table.

Only a missing row reports CNID_ERR_NOTFOUND, which afp_resolveid answers with the permanent AFPERR_NOID. A parent outside the CNID range or a NULL name means a corrupted database.

Parameters:
* `cdb`: CNID database
* `id`: CNID to resolve, receives the parent CNID or CNID_INVALID
* `buffer`: receives the name, always NUL-terminated
* `len`: size of buffer, not 0

Returns: buffer, or NULL with errno classified

Calls: [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset), [strlcpy](../../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_getstamp

```c
int cnid_sqlite_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
```

Defined at lines 1726 to 1810.

Read the stamp of a volume from the volumes table.

A missing row or a NULL stamp means unusable data, not a failing backend, so both report CNID_ERR_CORRUPT and the session survives.

Parameters:
* `cdb`: CNID database
* `buffer`: receives the stamp, written as a string
* `len`: size of buffer

Returns: 0, or -1 with errno classified

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset), [strlcpy](../../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_find

```c
int cnid_sqlite_find(struct _cnid_db *cdb, const char *name, size_t namelen, cnid_t scope_did, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 1830 to 1968.

Find the CNIDs whose name contains a substring, for cnid_find.

The cnid_find wrapper validates the arguments. The query asks for 1 row more than buffer holds, and that extra row sets more_available. A row with an ID outside the CNID range drops out. A scope directory selects the statement that walks its subtree with a recursive CTE.

Parameters:
* `cdb`: CNID database
* `name`: NUL-terminated UTF-8 substring
* `namelen`: unused, kept for the shared backend signature
* `scope_did`: CNID of the scope directory, or CNID_INVALID for an unscoped search
* `buffer`: receives the CNIDs in network order
* `buflen`: size of buffer
* `more_available`: receives true when more matches exist, or NULL

Returns: the number of CNIDs, or -1 with errno classified

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_wipe

```c
int cnid_sqlite_wipe(struct _cnid_db *cdb)
```

Defined at lines 1983 to 2059.

Empty the SQLite table of a volume and restart its CNIDs at CNID_START.

One transaction clears the depletion mark, removes every row, and reseeds the sequence. The call then clears the depletion flags of the connection, so a dbd rebuild restores the CNIDs from the metadata, and prepares the statements again. A failed reprepare leaves every statement closed until the database reopens.

Parameters:
* `cdb`: CNID database

Returns: 0, or -1 with errno classified, CNID_ERR_CLOSE after a failed reprepare

Calls: [asprintf](../../compat/misc.c.md#asprintf), [close_prepared_stmt](cnid_sqlite.c.md#close_prepared_stmt), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_new

```c
static struct _cnid_db * cnid_sqlite_new(struct vol *vol)
```

Defined at lines 2067 to 2088.

Allocate a CNID database object with the SQLite function table.

Parameters:
* `vol`: volume of the database

Returns: the object, or NULL when calloc fails

Calls: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_close](cnid_sqlite.c.md#cnid_sqlite_close), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_dir_owner_only

```c
static bool cnid_sqlite_dir_owner_only(const char *path)
```

Defined at lines 2099 to 2117.

Tell whether a CNID directory is an owner-only directory of the opener.

The call strips trailing slashes first, so lstat cannot follow a symlink.

Parameters:
* `path`: directory path

Returns: true for a directory that the real uid owns with no group or other permission bits

Calls: [strlcpy](../../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_report_orphan

```c
static void cnid_sqlite_report_orphan(const struct vol *vol, const char *dbdir)
```

Defined at lines 2130 to 2164.

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

Defined at lines 2184 to 2810.

Open the SQLite CNID backend for a volume.

The database stays private, with mode 0600 in a 0700 directory, for a single-user server or an opener that owns its CNID directory. Every other opener shares it, with mode 0666 in a 01777 directory. The call creates or checks the directory as root without following links. It opens the database with SQLITE_OPEN_NOFOLLOW and checks the -wal and -shm files before and after it turns on WAL.

The volumes table maps each volume path to its UUID, stamp, and depletion mark. A UUID that differs only in case takes the spelling on disk, and the call drops the tables of the same path under another UUID. A new CNID table starts its sequence at CNID_START.

Parameters:
* `args`: open arguments with the volume and the flags

Returns: the database, or NULL on error

Calls: [asprintf](../../compat/misc.c.md#asprintf), [become_root](../../util/unix.c.md#become_root), [close_prepared_stmt](cnid_sqlite.c.md#close_prepared_stmt), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_delete_sequence_row](cnid_sqlite.c.md#cnid_sqlite_delete_sequence_row), [cnid_sqlite_delete_volumes_row](cnid_sqlite.c.md#cnid_sqlite_delete_volumes_row), [cnid_sqlite_dir_owner_only](cnid_sqlite.c.md#cnid_sqlite_dir_owner_only), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_fchmod_companions](cnid_sqlite.c.md#cnid_sqlite_fchmod_companions), [cnid_sqlite_fchmod_regular](cnid_sqlite.c.md#cnid_sqlite_fchmod_regular), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new), [cnid_sqlite_report_orphan](cnid_sqlite.c.md#cnid_sqlite_report_orphan), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [cnid_sqlite_uuid_usable](cnid_sqlite.c.md#cnid_sqlite_uuid_usable), [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt), [unbecome_root](../../util/unix.c.md#unbecome_root), [uuid_strip_dashes](../../util/cnid.c.md#uuid_strip_dashes)

Called through [`_cnid_module::cnid_open`](../../../include/atalk/cnid.h.md#struct-_cnid_module) by: [cnid_open](../cnid.c.md#cnid_open)

Dispatched via: [cnid_sqlite_module](cnid_sqlite.c.md#cnid_sqlite_module)

# Macros

* Undocumented: `CNID_SQLITE_ADD_ATTEMPTS`, `CNID_SQLITE_BUSY_TIMEOUT`, `CNID_SQLITE_EMPTY_PROBE_TRIES`, `CNID_SQLITE_HINT_RESERVE`, `CNID_SQLITE_MAX_HINT`
