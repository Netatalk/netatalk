---
type: C Source File
title: "libatalk/cnid/sqlite/cnid_sqlite.c"
description: "33 functions, includes 8 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/sqlite/cnid_sqlite.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Initialized at line 39 as `struct _cnid_module`. Assigns:

* `cnid_open`: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

# Functions

### cnid_sqlite_fchmod_regular

```c
static int cnid_sqlite_fchmod_regular(const char *path, mode_t mode, bool missing_ok)
```

Defined at lines 84 to 131.

Called by: [cnid_sqlite_fchmod_companions](cnid_sqlite.c.md#cnid_sqlite_fchmod_companions), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_fchmod_companions

```c
static int cnid_sqlite_fchmod_companions(const char *dbpath, mode_t mode)
```

Defined at lines 135 to 156.

Calls: [cnid_sqlite_fchmod_regular](cnid_sqlite.c.md#cnid_sqlite_fchmod_regular)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### init_prepared_stmt_one

```c
static int init_prepared_stmt_one(CNID_sqlite_private *db, sqlite3_stmt **stmt, const char *tag, const char *sql_fmt)
```

Defined at lines 166 to 199.

Prepare one per-volume statement, replacing any previous handle.

Parameters:
* `db`: backend private data
* `stmt`: statement handle to finalize and re-prepare
* `tag`: statement name, for the debug log only
* `sql_fmt`: SQL with one s for the volume's table name, or none

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Called by: [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt)

### init_prepared_stmt

```c
static int init_prepared_stmt(CNID_sqlite_private *db)
```

Defined at lines 207 to 251.

(Re-)prepare every per-volume statement, at open and after a wipe

A failure leaves the handles past that point NULL; [cnid_sqlite_stmt_ready()](cnid_sqlite.c.md#cnid_sqlite_stmt_ready) rejects those.

Calls: [init_prepared_stmt_one](cnid_sqlite.c.md#init_prepared_stmt_one)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### close_prepared_stmt

```c
static void close_prepared_stmt(CNID_sqlite_private *db)
```

Defined at lines 256 to 282.

Finalize every prepared statement and clear the handles.

Called by: [cnid_sqlite_close](cnid_sqlite.c.md#cnid_sqlite_close), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_hint_usable

```c
static bool cnid_sqlite_hint_usable(cnid_t hint)
```

Defined at lines 291 to 295.

Whether a CNID hint from AppleDouble/EA metadata is safe to bind.

Rejects the reserved range below CNID_START and anything above CNID_SQLITE_MAX_HINT, which as an explicit rowid would raise the AUTOINCREMENT high-water mark towards the depletion reset.

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add)

### cnid_sqlite_rowid_ok

```c
static bool cnid_sqlite_rowid_ok(uint64_t rowid)
```

Defined at lines 304 to 307.

Whether a rowid read from the database still fits a CNID.

The Id column is a 64-bit sqlite integer while a CNID is 32 bits. Narrowing one that does not fit would produce the id of an unrelated live row, which the caller would then hand to a client, or delete.

Called by: [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve)

### cnid_sqlite_uuid_usable

```c
static bool cnid_sqlite_uuid_usable(const char *uuid)
```

Defined at lines 317 to 332.

Whether a volume UUID is safe to interpolate as a table name.

A table name cannot be a bound parameter, so it is built into the SQL text. [uuid_strip_dashes()](../../util/cnid.c.md#uuid_strip_dashes) yields exactly 32 hex digits; the UUIDs read back out of the volumes table are only as trustworthy as the world-writable database file, so anything else is refused rather than quoted and hoped for.

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_set_errno

```c
static void cnid_sqlite_set_errno(int sqlite_return)
```

Defined at lines 348 to 382.

Map a sqlite3 result code onto the CNID error contract in errno.

CNID_ERR_BUSY covers what clears on its own: contention, disk full, I/O error, out of memory, and SQLITE_PROTOCOL, which is WAL's bounded locking handshake giving up. SQLITE_LOCKED is not in that set — it reports a same-connection or shared-cache conflict the busy handler never waits on, so the state survives a retry.

CNID_ERR_DB, which [get_id()](../../../etc/afpd/file.c.md#get_id) in [etc/afpd/file.c](../../../etc/afpd/file.c.md) answers by ending the session, means the backend itself is unreachable. A local database file has no such state: the connection outlives whatever one statement returns, so an unrecognised code fails the single operation as CNID_ERR_CORRUPT.

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_delete_by_uuid](cnid_sqlite.c.md#cnid_sqlite_delete_by_uuid), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [init_prepared_stmt_one](cnid_sqlite.c.md#init_prepared_stmt_one)

Mentioned in the documentation of: [cnid_sqlite_delete_by_uuid](cnid_sqlite.c.md#cnid_sqlite_delete_by_uuid), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

### cnid_sqlite_execute

```c
static int cnid_sqlite_execute(sqlite3 *con, const char *sql)
```

Defined at lines 389 to 403.

Run one SQL statement, classifying a failure into errno.

Returns: 0 on success, -1 with errno set by [cnid_sqlite_set_errno()](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_delete_by_uuid

```c
static int cnid_sqlite_delete_by_uuid(sqlite3 *con, const char *sql, const char *uuid)
```

Defined at lines 415 to 443.

Delete one row identified by a UUID, through a bound parameter.

The UUID is data here, not an identifier, so it never enters the SQL text: the stale entries this removes are read back out of the world-writable database file, where a value chosen to break out of a quoted string would otherwise run as SQL under the [become_root()](../../util/unix.c.md#become_root) the cleanup holds.

Returns: 0 on success, -1 with errno set by [cnid_sqlite_set_errno()](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno)

Called by: [cnid_sqlite_delete_sequence_row](cnid_sqlite.c.md#cnid_sqlite_delete_sequence_row), [cnid_sqlite_delete_volumes_row](cnid_sqlite.c.md#cnid_sqlite_delete_volumes_row)

### cnid_sqlite_delete_volumes_row

```c
static int cnid_sqlite_delete_volumes_row(sqlite3 *con, const char *uuid)
```

Defined at lines 448 to 452.

Remove a stale volume's row from the volumes table.

Calls: [cnid_sqlite_delete_by_uuid](cnid_sqlite.c.md#cnid_sqlite_delete_by_uuid)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_delete_sequence_row

```c
static int cnid_sqlite_delete_sequence_row(sqlite3 *con, const char *uuid)
```

Defined at lines 457 to 461.

Remove a stale volume's AUTOINCREMENT high-water mark.

Calls: [cnid_sqlite_delete_by_uuid](cnid_sqlite.c.md#cnid_sqlite_delete_by_uuid)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_stmt_reset

```c
static void cnid_sqlite_stmt_reset(sqlite3_stmt *stmt)
```

Defined at lines 470 to 475.

Reset a prepared statement without disturbing the classification.

sqlite3_reset() enters the VFS and can leave errno set by a speculative syscall. It runs in every cleanup path after errno has been classified, and that classification is the caller's only signal for what failed.

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update)

### cnid_sqlite_stmt_ready

```c
static bool cnid_sqlite_stmt_ready(sqlite3_stmt *stmt, const char *caller)
```

Defined at lines 484 to 495.

Whether a prepared statement is available to bind and step.

The handles are NULL when [cnid_sqlite_wipe()](cnid_sqlite.c.md#cnid_sqlite_wipe)'s re-preparation stopped partway. sqlite3_bind_*() tolerates a NULL statement; sqlite3_step() on one is undefined, so every entry point checks the handles it uses.

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update)

Mentioned in the documentation of: [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt)

### cnid_sqlite_begin

```c
static int cnid_sqlite_begin(sqlite3 *con, int *owned)
```

Defined at lines 504 to 512.

Open a transaction and mark it owned by the calling function.

An exec'd COMMIT can fail and error-jump to cleanup, leaving the connection inside a transaction. Ownership is tracked so a cleanup handler can only abandon a transaction its own function started, never a caller's.

Calls: [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_commit

```c
static int cnid_sqlite_commit(sqlite3 *con, int *owned)
```

Defined at lines 517 to 529.

Commit the owned transaction, releasing ownership only on success.

Calls: [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_rollback

```c
static void cnid_sqlite_rollback(sqlite3 *con, int *owned)
```

Defined at lines 537 to 550.

Abandon an owned transaction without disturbing the classification.

Runs from cleanup paths after errno has been classified; the ROLLBACK enters the VFS and could leave errno set by a speculative syscall.

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_seed_sequence

```c
static int cnid_sqlite_seed_sequence(CNID_sqlite_private *db)
```

Defined at lines 559 to 583.

Reseed the volume's AUTOINCREMENT sequence to the reserved floor.

UPDATE first, then INSERT only where the UPDATE changed no row: sqlite_sequence has no UNIQUE constraint to upsert against. Runs inside the caller's transaction; on failure the caller owns the rollback.

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

### cnid_sqlite_delete

```c
int cnid_sqlite_delete(struct _cnid_db *cdb, const cnid_t id)
```

Defined at lines 585 to 626.

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_close

```c
void cnid_sqlite_close(struct _cnid_db *cdb)
```

Defined at lines 628 to 664.

Calls: [close_prepared_stmt](cnid_sqlite.c.md#close_prepared_stmt)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_update

```c
int cnid_sqlite_update(struct _cnid_db *cdb, cnid_t id, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 666 to 825.

Calls: [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_lookup

```c
cnid_t cnid_sqlite_lookup(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 827 to 1074.

Calls: [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [strlcpy](../../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_add

```c
cnid_t cnid_sqlite_add(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len, cnid_t hint)
```

Defined at lines 1076 to 1350.

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_hint_usable](cnid_sqlite.c.md#cnid_sqlite_hint_usable), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_get

```c
cnid_t cnid_sqlite_get(struct _cnid_db *cdb, cnid_t did, const char *name, size_t len)
```

Defined at lines 1352 to 1438.

Calls: [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_resolve

```c
char * cnid_sqlite_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len)
```

Defined at lines 1440 to 1538.

Calls: [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset), [strlcpy](../../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_getstamp

```c
int cnid_sqlite_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
```

Defined at lines 1543 to 1627.

Caller passes buffer where we will store the db stamp

Calls: [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset), [strlcpy](../../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_find

```c
int cnid_sqlite_find(struct _cnid_db *cdb, const char *name, size_t namelen, cnid_t scope_did, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 1645 to 1783.

Backend implementation of [cnid_find()](../cnid.c.md#cnid_find) for the sqlite backend.

Parameters are pre-validated by the [libatalk/cnid/cnid.c](../cnid.c.md) wrapper, so cdb, name, namelen and buflen are all sane on entry. Detects truncation by binding LIMIT max_results+1 and observing whether the extra row was produced; reports it via `more_available`.

`namelen` is unused: the SQLite backend builds the LIKE pattern via asprintf("%%%s%%", name), which already requires a NUL-terminated `name`. The parameter is kept to satisfy the cnid_db function-pointer signature shared with the mysql backend.

`scope_did` selects the statement: CNID_INVALID searches the whole volume, anything else only the subtree rooted at that directory.

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_sqlite_rowid_ok](cnid_sqlite.c.md#cnid_sqlite_rowid_ok), [cnid_sqlite_set_errno](cnid_sqlite.c.md#cnid_sqlite_set_errno), [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready), [cnid_sqlite_stmt_reset](cnid_sqlite.c.md#cnid_sqlite_stmt_reset)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

### cnid_sqlite_wipe

```c
int cnid_sqlite_wipe(struct _cnid_db *cdb)
```

Defined at lines 1785 to 1861.

Calls: [asprintf](../../compat/misc.c.md#asprintf), [close_prepared_stmt](cnid_sqlite.c.md#close_prepared_stmt), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt)

Called by: [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new)

Mentioned in the documentation of: [cnid_sqlite_stmt_ready](cnid_sqlite.c.md#cnid_sqlite_stmt_ready)

### cnid_sqlite_new

```c
static struct _cnid_db * cnid_sqlite_new(struct vol *vol)
```

Defined at lines 1863 to 1884.

Calls: [cnid_sqlite_add](cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_close](cnid_sqlite.c.md#cnid_sqlite_close), [cnid_sqlite_delete](cnid_sqlite.c.md#cnid_sqlite_delete), [cnid_sqlite_find](cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_get](cnid_sqlite.c.md#cnid_sqlite_get), [cnid_sqlite_getstamp](cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](cnid_sqlite.c.md#cnid_sqlite_resolve), [cnid_sqlite_update](cnid_sqlite.c.md#cnid_sqlite_update), [cnid_sqlite_wipe](cnid_sqlite.c.md#cnid_sqlite_wipe)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_dir_owner_only

```c
static bool cnid_sqlite_dir_owner_only(const char *path)
```

Defined at lines 1889 to 1907.

Whether a CNID directory is the opener's own owner-only directory.

Calls: [strlcpy](../../compat/strlcpy.c.md#strlcpy)

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_report_orphan

```c
static void cnid_sqlite_report_orphan(const struct vol *vol, const char *dbdir)
```

Defined at lines 1917 to 1951.

Log a database named after the volume in its share root, unused unless the share root is the database directory.

Only a regular file with the SQLite header counts, opened without following a link or blocking.

Called by: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

### cnid_sqlite_open

```c
struct _cnid_db * cnid_sqlite_open(struct cnid_open_args *args)
```

Defined at lines 1953 to 2579.

Calls: [asprintf](../../compat/misc.c.md#asprintf), [become_root](../../util/unix.c.md#become_root), [close_prepared_stmt](cnid_sqlite.c.md#close_prepared_stmt), [cnid_sqlite_begin](cnid_sqlite.c.md#cnid_sqlite_begin), [cnid_sqlite_commit](cnid_sqlite.c.md#cnid_sqlite_commit), [cnid_sqlite_delete_sequence_row](cnid_sqlite.c.md#cnid_sqlite_delete_sequence_row), [cnid_sqlite_delete_volumes_row](cnid_sqlite.c.md#cnid_sqlite_delete_volumes_row), [cnid_sqlite_dir_owner_only](cnid_sqlite.c.md#cnid_sqlite_dir_owner_only), [cnid_sqlite_execute](cnid_sqlite.c.md#cnid_sqlite_execute), [cnid_sqlite_fchmod_companions](cnid_sqlite.c.md#cnid_sqlite_fchmod_companions), [cnid_sqlite_fchmod_regular](cnid_sqlite.c.md#cnid_sqlite_fchmod_regular), [cnid_sqlite_new](cnid_sqlite.c.md#cnid_sqlite_new), [cnid_sqlite_report_orphan](cnid_sqlite.c.md#cnid_sqlite_report_orphan), [cnid_sqlite_rollback](cnid_sqlite.c.md#cnid_sqlite_rollback), [cnid_sqlite_seed_sequence](cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [cnid_sqlite_uuid_usable](cnid_sqlite.c.md#cnid_sqlite_uuid_usable), [init_prepared_stmt](cnid_sqlite.c.md#init_prepared_stmt), [unbecome_root](../../util/unix.c.md#unbecome_root), [uuid_strip_dashes](../../util/cnid.c.md#uuid_strip_dashes)

Called through [`_cnid_module::cnid_open`](../../../include/atalk/cnid.h.md#struct-_cnid_module) by: [cnid_open](../cnid.c.md#cnid_open)

Dispatched via: [cnid_sqlite_module](cnid_sqlite.c.md#cnid_sqlite_module)

# Macros

* Undocumented: `CNID_SQLITE_ADD_ATTEMPTS`, `CNID_SQLITE_BUSY_TIMEOUT`, `CNID_SQLITE_EMPTY_PROBE_TRIES`, `CNID_SQLITE_HINT_RESERVE`, `CNID_SQLITE_MAX_HINT`
