---
type: C Source File
title: "libatalk/cnid/mysql/cnid_mysql.c"
description: "MySQL CNID backend."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/mysql/cnid_mysql.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/cnid](../../cnid.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/adouble.h](../../../include/atalk/adouble.h.md)
* [atalk/cnid_mysql_private.h](../../../include/atalk/cnid_mysql_private.h.md)
* [atalk/errchk.h](../../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../../include/atalk/globals.h.md)
* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [atalk/unix.h](../../../include/atalk/unix.h.md)
* [atalk/util.h](../../../include/atalk/util.h.md)
* [atalk/volume.h](../../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `errmsg.h`, `errno.h`, `mysql.h`, `mysqld_error.h`, `net/if.h`, `netdb.h`, `netinet/in.h`, `netinet/tcp.h`, `stdlib.h`, `sys/param.h`, `sys/socket.h`, `sys/stat.h`, `sys/time.h`, `sys/uio.h`, `sys/un.h`, `time.h`

# Function tables

### cnid_mysql_module

Initialized at line 2096 as `struct _cnid_module`. Assigns:

* `cnid_open`: [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open)

# Functions

### cnid_mysql_hint_usable

```c
static bool cnid_mysql_hint_usable(cnid_t hint)
```

Defined at lines 108 to 112.

Tell whether a CNID hint from AppleDouble or EA metadata is safe to bind.

The call rejects the reserved range under CNID_START. It also rejects any value above CNID_MYSQL_MAX_HINT, which as an explicit Id would push the AUTO_INCREMENT high-water mark toward the depletion reset.

Parameters:
* `hint`: CNID in network order

Returns: true when CNID_START <= hint <= CNID_MYSQL_MAX_HINT in host order

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add)

### init_prepared_stmt_lookup

```c
static int init_prepared_stmt_lookup(CNID_mysql_private *db)
```

Defined at lines 123 to 170.

Prepare the lookup statement of a MySQL CNID table.

The statement selects the rows that match the name and parent, or the device and inode, of an object.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `lookup_param`, `lookup_result`, `lookup_result_dev`, `lookup_result_did`, `lookup_result_id`, `lookup_result_ino`, `lookup_result_name`, `lookup_result_name_len`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_add

```c
static int init_prepared_stmt_add(CNID_mysql_private *db)
```

Defined at lines 179 to 209.

Prepare the insert statement for a new entry, numbered by AUTO_INCREMENT.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `add_param`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_put

```c
static int init_prepared_stmt_put(CNID_mysql_private *db)
```

Defined at lines 217 to 250.

Prepare the insert statement for an entry with an explicit CNID.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `put_param`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_get

```c
static int init_prepared_stmt_get(CNID_mysql_private *db)
```

Defined at lines 258 to 284.

Prepare the statement that finds the CNID of a name in a directory.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `get_param`, `get_result`, `get_result_id`, `stmt_param_did`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_delete

```c
static int init_prepared_stmt_delete(CNID_mysql_private *db)
```

Defined at lines 292 to 311.

Prepare the statement that removes an entry by CNID.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `delete_param`, `stmt_param_id`

### init_prepared_stmt_resolve

```c
static int init_prepared_stmt_resolve(CNID_mysql_private *db)
```

Defined at lines 319 to 345.

Prepare the statement that finds the parent and the name of a CNID.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `resolve_param`, `resolve_result`, `resolve_result_did`, `resolve_result_name`, `resolve_result_name_len`, `stmt_param_id`

### init_prepared_stmt_purge

```c
static int init_prepared_stmt_purge(CNID_mysql_private *db)
```

Defined at lines 356 to 390.

Prepare the statement that removes every entry clashing with an update.

The statement removes the rows that match the CNID, the name and parent, or the device and inode.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `purge_param`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt

```c
static int init_prepared_stmt(CNID_mysql_private *db)
```

Defined at lines 398 to 410.

Prepare all 7 statements of a MySQL CNID table.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when one fails

Calls: [init_prepared_stmt_add](cnid_mysql.c.md#init_prepared_stmt_add), [init_prepared_stmt_delete](cnid_mysql.c.md#init_prepared_stmt_delete), [init_prepared_stmt_get](cnid_mysql.c.md#init_prepared_stmt_get), [init_prepared_stmt_lookup](cnid_mysql.c.md#init_prepared_stmt_lookup), [init_prepared_stmt_purge](cnid_mysql.c.md#init_prepared_stmt_purge), [init_prepared_stmt_put](cnid_mysql.c.md#init_prepared_stmt_put), [init_prepared_stmt_resolve](cnid_mysql.c.md#init_prepared_stmt_resolve)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### close_prepared_stmt

```c
static void close_prepared_stmt(CNID_mysql_private *db)
```

Defined at lines 421 to 435.

Close and clear the backend's prepared MySQL statements.

NULL-safe, and NULLs each statement after closing it. A failed recovery leaves a mix of closed and live statements, and a later close or re-init must neither double-close nor leak the survivors.

Parameters:
* `db`: backend private data holding the statement pointers

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_close](cnid_mysql.c.md#cnid_mysql_close), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### cnid_mysql_set_errno

```c
static void cnid_mysql_set_errno(unsigned int mysql_error_code)
```

Defined at lines 448 to 465.

Classify a MySQL error code as a CNID_ERR_* errno.

Lock wait timeouts and deadlocks become CNID_ERR_BUSY, because the lock frees again. InnoDB reports a deadlock at once, and concurrent sessions that insert into one CNID table hit both codes routinely. Crashed tables become CNID_ERR_CORRUPT. Any other code becomes CNID_ERR_DB, which ends the session.

Parameters:
* `mysql_error_code`: MySQL error number, or 0 for a failure without one

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_drain_results](cnid_mysql.c.md#cnid_mysql_drain_results), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_get](cnid_mysql.c.md#cnid_mysql_get), [cnid_mysql_getstamp](cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open), [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### cnid_mysql_drain_results

```c
static int cnid_mysql_drain_results(MYSQL *con)
```

Defined at lines 477 to 497.

Drain the remaining results of a multi-statement batch.

The cnid_mysql_execute call reports only the first statement of a batch. A positive mysql_next_result means a later statement failed, so a wipe whose TRUNCATE or ALTER failed still fails here.

Parameters:
* `con`: MySQL connection

Returns: 0 when every statement succeeded, -1 with errno classified otherwise

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_wipe](cnid_mysql.c.md#cnid_mysql_wipe)

### cnid_mysql_stmt_execute

```c
static int cnid_mysql_stmt_execute(CNID_mysql_private *db, MYSQL_STMT **stmtp)
```

Defined at lines 512 to 561.

Execute a prepared statement and reprepare after CR_SERVER_LOST.

The stmtp argument must point at the field in db, because a reprepare replaces every statement and the retry runs the new one. A reprepare also drops the stored result of any other statement. CR_SERVER_LOST on CNID_MYSQL_RECONNECT_ATTEMPTS executes in a row ends the retries. A NULL statement from an earlier failed reprepare gets prepared first.

Parameters:
* `db`: MySQL backend state
* `stmtp`: address of the statement field in db

Returns: 0, or -1 with errno classified

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Called by: [cnid_mysql_delete](cnid_mysql.c.md#cnid_mysql_delete), [cnid_mysql_get](cnid_mysql.c.md#cnid_mysql_get), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### cnid_mysql_execute

```c
static int cnid_mysql_execute(MYSQL *con, const char *sql)
```

Defined at lines 573 to 585.

Run an SQL statement or batch on a connection.

The call returns -1 for any failure of mysql_query, so EC_NEG1 call sites see it.

Parameters:
* `con`: MySQL connection
* `sql`: SQL text

Returns: 0, or -1 with errno classified

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_getstamp](cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open), [cnid_mysql_wipe](cnid_mysql.c.md#cnid_mysql_wipe)

### cnid_mysql_delete

```c
int cnid_mysql_delete(struct _cnid_db *cdb, const cnid_t id)
```

Defined at lines 594 to 612.

Remove a CNID from the MySQL table.

Parameters:
* `cdb`: CNID database
* `id`: CNID in network order, not 0

Returns: 0, or -1 with errno classified

Calls: [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute)

Called by: [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `stmt_param_id`

### cnid_mysql_close

```c
void cnid_mysql_close(struct _cnid_db *cdb)
```

Defined at lines 619 to 643.

Close the MySQL connection of a CNID database and free the database.

Parameters:
* `cdb`: CNID database, and a NULL one logs an error

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_update

```c
int cnid_mysql_update(struct _cnid_db *cdb, cnid_t id, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 662 to 740.

Rewrite the entry of a CNID with a new name, parent, and stat.

The call removes every row that clashes on the CNID, the name and parent, or the device and inode. It then inserts the entry with its CNID. ER_DUP_ENTRY or CR_SERVER_LOST repeats both steps, up to CNID_MYSQL_ADD_ATTEMPTS times.

Parameters:
* `cdb`: CNID database
* `id`: CNID to keep
* `st`: stat of the object
* `did`: new parent CNID
* `name`: new name on disk
* `len`: length of name, up to MAXPATHLEN

Returns: 0, or -1 with errno classified, CNID_ERR_CORRUPT after the last attempt

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Called by: [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_lookup

```c
cnid_t cnid_mysql_lookup(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 762 to 958.

Find the CNID of an object by name and parent or by device and inode.

A single matching row is the answer. Two rows, one per index, are stale, and the call removes both.

A row with another name or parent means a server-side move or a reused inode. When its CNID equals the hint of the current add, the row takes the new name and parent. Otherwise, and for a row with another device or inode, the call removes the row. Each removal reports CNID_ERR_NOTFOUND, so cnid_mysql_add inserts a new entry.

Parameters:
* `cdb`: CNID database
* `st`: stat of the object
* `did`: CNID of the parent directory
* `name`: name on disk
* `len`: length of name, up to MAXPATHLEN

Returns: the CNID, or CNID_INVALID with errno classified, CNID_ERR_NOTFOUND for a missing entry

Calls: [cnid_mysql_delete](cnid_mysql.c.md#cnid_mysql_delete), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `lookup_result`, `lookup_result_dev`, `lookup_result_did`, `lookup_result_id`, `lookup_result_ino`, `lookup_result_name`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_add

```c
cnid_t cnid_mysql_add(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len, cnid_t hint)
```

Defined at lines 980 to 1184.

Find the CNID of an object or add a new entry for it.

A failed lookup leads to an insert only on CNID_ERR_NOTFOUND. A usable hint goes in as an explicit Id, unless a depletion emptied the table before. ER_DUP_ENTRY from a racing session repeats the lookup, up to CNID_MYSQL_ADD_ATTEMPTS times.

Near CNID_MYSQL_MAX_HINT the call warns once. Past UINT32_MAX it empties the table, marks the volume depleted, and fails with CNID_ERR_RESET, so the session ends.

Parameters:
* `cdb`: CNID database
* `st`: stat of the object
* `did`: CNID of the parent directory
* `name`: name on disk
* `len`: length of name, up to MAXPATHLEN
* `hint`: CNID from the metadata of the object, or CNID_INVALID

Returns: the CNID, or CNID_INVALID with errno classified

Calls: [asprintf](../../compat/misc.c.md#asprintf), [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [cnid_mysql_drain_results](cnid_mysql.c.md#cnid_mysql_drain_results), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_hint_usable](cnid_mysql.c.md#cnid_mysql_hint_usable), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_get

```c
cnid_t cnid_mysql_get(struct _cnid_db *cdb, cnid_t did, const char *name, size_t len)
```

Defined at lines 1199 to 1266.

Look up the CNID that a directory holds under a name.

A contended database reports a classified errno, not a missing name, which the caller would turn into a permanent AFPERR_NOOBJ.

Parameters:
* `cdb`: CNID database
* `did`: CNID of the directory
* `name`: name on disk
* `len`: length of name, up to MAXPATHLEN

Returns: the CNID, or CNID_INVALID with errno CNID_ERR_NOTFOUND for a missing name

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `get_result`, `get_result_id`, `stmt_param_did`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_resolve

```c
char * cnid_mysql_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len)
```

Defined at lines 1281 to 1345.

Resolve a CNID to its parent and name in the MySQL table.

A contended database reports a classified errno, not a missing CNID, which afp_resolveid would turn into a permanent AFPERR_NOID.

Parameters:
* `cdb`: CNID database
* `id`: CNID to resolve, receives the parent CNID or CNID_INVALID
* `buffer`: receives the name
* `len`: size of buffer

Returns: buffer, or NULL with errno classified

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `resolve_result`, `resolve_result_did`, `resolve_result_name`, `resolve_result_name_len`, `stmt_param_id`

### cnid_mysql_getstamp

```c
int cnid_mysql_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
```

Defined at lines 1357 to 1418.

Read the stamp of a volume from the volumes table.

The stamp is the time the volume first entered the table.

Parameters:
* `cdb`: CNID database
* `buffer`: receives len bytes of the stamp
* `len`: bytes to read

Returns: 0, or -1 on error

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_find

```c
int cnid_mysql_find(struct _cnid_db *cdb, const char *name, size_t namelen, cnid_t scope_did, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 1439 to 1657.

Find the CNIDs whose name contains a substring, for cnid_find.

The cnid_find wrapper validates the arguments. The query asks for 1 row more than buffer holds, and that extra row sets more_available. The name pattern binds as a statement parameter, never as SQL text. A scoped search walks the subtree with a recursive CTE. A server without CTE support falls back to an unscoped search.

Parameters:
* `cdb`: CNID database
* `name`: UTF-8 substring
* `namelen`: length of name
* `scope_did`: CNID of the scope directory, or CNID_INVALID for all of the volume
* `buffer`: receives the CNIDs in network order
* `buflen`: size of buffer
* `more_available`: receives true when more matches exist, or NULL

Returns: the number of CNIDs, or -1 on error

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_wipe

```c
int cnid_mysql_wipe(struct _cnid_db *cdb)
```

Defined at lines 1668 to 1709.

Empty the MySQL table of a volume and restart its CNIDs at 17.

The call also clears the depletion marks of the volume and of the connection. A dbd rebuild then restores the CNIDs from the metadata again.

Parameters:
* `cdb`: CNID database

Returns: 0, or -1 with errno classified

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_mysql_drain_results](cnid_mysql.c.md#cnid_mysql_drain_results), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_new

```c
static struct _cnid_db * cnid_mysql_new(struct vol *vol)
```

Defined at lines 1717 to 1738.

Allocate a CNID database object with the MySQL function table.

Parameters:
* `vol`: volume of the database

Returns: the object, or NULL when calloc fails

Calls: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_close](cnid_mysql.c.md#cnid_mysql_close), [cnid_mysql_delete](cnid_mysql.c.md#cnid_mysql_delete), [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find), [cnid_mysql_get](cnid_mysql.c.md#cnid_mysql_get), [cnid_mysql_getstamp](cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update), [cnid_mysql_wipe](cnid_mysql.c.md#cnid_mysql_wipe)

Called by: [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open)

### cnid_mysql_open

```c
struct _cnid_db * cnid_mysql_open(struct cnid_open_args *args)
```

Defined at lines 1753 to 2094.

Open the MySQL CNID backend for a volume.

The call creates the state directory of the volume as root, connects to the server, and creates the database when missing. It then creates the volumes table and the CNID table of the volume, named by the volume UUID. It drops the tables and rows of the same path under another UUID. The volume row gets its stamp on first use, and a depleted flag carries into the connection.

Parameters:
* `args`: open arguments with the volume and the flags

Returns: the database, or NULL on error

Calls: [asprintf](../../compat/misc.c.md#asprintf), [become_root](../../util/unix.c.md#become_root), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt), [unbecome_root](../../util/unix.c.md#unbecome_root), [uuid_strip_dashes](../../util/cnid.c.md#uuid_strip_dashes)

Called through [`_cnid_module::cnid_open`](../../../include/atalk/cnid.h.md#struct-_cnid_module) by: [cnid_open](../cnid.c.md#cnid_open)

Dispatched via: [cnid_mysql_module](cnid_mysql.c.md#cnid_mysql_module)

# Macros

* Undocumented: `CNID_MYSQL_ADD_ATTEMPTS`, `CNID_MYSQL_HINT_RESERVE`, `CNID_MYSQL_MAX_HINT`, `CNID_MYSQL_RECONNECT_ATTEMPTS`

# File-scope variables

`add_param`, `delete_param`, `get_param`, `get_result`, `get_result_id`, `lookup_param`, `lookup_result`, `lookup_result_dev`, `lookup_result_did`, `lookup_result_id`, `lookup_result_ino`, `lookup_result_name`, `lookup_result_name_len`, `purge_param`, `put_param`, `resolve_param`, `resolve_result`, `resolve_result_did`, `resolve_result_name`, `resolve_result_name_len`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`
