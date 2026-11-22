---
type: C Source File
title: "libatalk/cnid/cnid_mysql.c"
description: "MySQL CNID backend."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/cnid_mysql.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-11-22T14:39:48+01:00 }
---

Part of the [libatalk/cnid](../cnid.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid_mysql_private.h](../../include/atalk/cnid_mysql_private.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/sl_names.h](../../include/atalk/sl_names.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `errmsg.h`, `errno.h`, `mysql.h`, `mysqld_error.h`, `net/if.h`, `netdb.h`, `netinet/in.h`, `netinet/tcp.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/stat.h`, `sys/time.h`, `sys/uio.h`, `sys/un.h`, `time.h`

# Function tables

### cnid_mysql_module

Initialized at line 2820 as `struct _cnid_module`. Assigns:

* `cnid_open`: [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open)

# Functions

### cnid_mysql_hint_usable

```c
static bool cnid_mysql_hint_usable(cnid_t hint)
```

Defined at lines 132 to 136.

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

Defined at lines 147 to 194.

Prepare the lookup statement of a MySQL CNID table.

The statement selects the rows that match the name and parent, or the device and inode, of an object.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `lookup_param`, `lookup_result`, `lookup_result_dev`, `lookup_result_did`, `lookup_result_id`, `lookup_result_ino`, `lookup_result_name`, `lookup_result_name_len`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_folded_param

```c
static void cnid_mysql_folded_param(MYSQL_BIND *param)
```

Defined at lines 203 to 209.

Point a statement parameter at the folded name.

NULLIF() in the statements stores the 0 bytes of no fold as NULL.

Parameters:
* `param`: the parameter

Called by: [init_prepared_stmt_add](cnid_mysql.c.md#init_prepared_stmt_add), [init_prepared_stmt_put](cnid_mysql.c.md#init_prepared_stmt_put)

Uses file-scope variables: `stmt_param_folded`, `stmt_param_folded_len`

### cnid_mysql_fold_param

```c
static void cnid_mysql_fold_param(const char *name, size_t len, charset_t charset)
```

Defined at lines 221 to 234.

Fold a name into the statement parameter of the folded column.

A fold longer than the column, or none for want of memory, leaves the column NULL, which every search takes as a candidate.

Parameters:
* `name`: the name
* `len`: bytes of name
* `charset`: the name's charset, the volume's

Calls: [sl_name_fold_in](../util/sl_names.c.md#sl_name_fold_in)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

Uses file-scope variables: `stmt_param_folded`, `stmt_param_folded_len`

### init_prepared_stmt_add

```c
static int init_prepared_stmt_add(CNID_mysql_private *db)
```

Defined at lines 246 to 296.

Prepare the insert statement for a new entry, numbered by AUTO_INCREMENT.

The prepare tells whether the table has the column of folded names: a table an older netatalk made has none.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_mysql_folded_param](cnid_mysql.c.md#cnid_mysql_folded_param)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `add_param`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

Mentioned in the documentation of: [init_prepared_stmt_put](cnid_mysql.c.md#init_prepared_stmt_put)

### init_prepared_stmt_put

```c
static int init_prepared_stmt_put(CNID_mysql_private *db)
```

Defined at lines 307 to 350.

Prepare the insert statement for an entry with an explicit CNID.

The statement writes the folded name where [init_prepared_stmt_add()](cnid_mysql.c.md#init_prepared_stmt_add) found the column.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_mysql_folded_param](cnid_mysql.c.md#cnid_mysql_folded_param)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `put_param`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_get

```c
static int init_prepared_stmt_get(CNID_mysql_private *db)
```

Defined at lines 358 to 384.

Prepare the statement that finds the CNID of a name in a directory.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `get_param`, `get_result`, `get_result_id`, `stmt_param_did`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_delete

```c
static int init_prepared_stmt_delete(CNID_mysql_private *db)
```

Defined at lines 392 to 411.

Prepare the statement that removes an entry by CNID.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `delete_param`, `stmt_param_id`

### init_prepared_stmt_resolve

```c
static int init_prepared_stmt_resolve(CNID_mysql_private *db)
```

Defined at lines 419 to 445.

Prepare the statement that finds the parent and the name of a CNID.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `resolve_param`, `resolve_result`, `resolve_result_did`, `resolve_result_name`, `resolve_result_name_len`, `stmt_param_id`

### init_prepared_stmt_purge

```c
static int init_prepared_stmt_purge(CNID_mysql_private *db)
```

Defined at lines 456 to 490.

Prepare the statement that removes every entry clashing with an update.

The statement removes the rows that match the CNID, the name and parent, or the device and inode.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `purge_param`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_dirlist

```c
static int init_prepared_stmt_dirlist(CNID_mysql_private *db)
```

Defined at lines 498 to 535.

Prepare the statement that lists the rows of one folder.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 when preparing or binding fails

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `dirlist_param`, `dirlist_param_max`, `dirlist_result`, `dirlist_result_dev`, `dirlist_result_id`, `dirlist_result_ino`, `dirlist_result_name`, `dirlist_result_name_len`, `stmt_param_did`

### init_prepared_stmt

```c
static int init_prepared_stmt(CNID_mysql_private *db)
```

Defined at lines 550 to 569.

Prepare all 8 statements of a MySQL CNID table.

A failure closes every statement and sets CNID_ERR_DB, which ends the session. The lookup or purge before an add's or an update's insert then finds its statement NULL in [cnid_mysql_stmt_execute()](cnid_mysql.c.md#cnid_mysql_stmt_execute), which prepares them all again.

Parameters:
* `db`: MySQL backend state with the connection and the table name

Returns: 0, or -1 with errno CNID_ERR_DB when one fails

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [init_prepared_stmt_add](cnid_mysql.c.md#init_prepared_stmt_add), [init_prepared_stmt_delete](cnid_mysql.c.md#init_prepared_stmt_delete), [init_prepared_stmt_dirlist](cnid_mysql.c.md#init_prepared_stmt_dirlist), [init_prepared_stmt_get](cnid_mysql.c.md#init_prepared_stmt_get), [init_prepared_stmt_lookup](cnid_mysql.c.md#init_prepared_stmt_lookup), [init_prepared_stmt_purge](cnid_mysql.c.md#init_prepared_stmt_purge), [init_prepared_stmt_put](cnid_mysql.c.md#init_prepared_stmt_put), [init_prepared_stmt_resolve](cnid_mysql.c.md#init_prepared_stmt_resolve)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### close_prepared_stmt

```c
static void close_prepared_stmt(CNID_mysql_private *db)
```

Defined at lines 580 to 594.

Close and clear the backend's prepared MySQL statements.

NULL-safe, and NULLs each statement after closing it. A failed recovery leaves a mix of closed and live statements, and a later close or re-init must neither double-close nor leak the survivors.

Parameters:
* `db`: backend private data holding the statement pointers

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_close](cnid_mysql.c.md#cnid_mysql_close), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

### cnid_mysql_set_errno

```c
static void cnid_mysql_set_errno(unsigned int mysql_error_code)
```

Defined at lines 607 to 624.

Classify a MySQL error code as a CNID_ERR_* errno.

Lock wait timeouts and deadlocks become CNID_ERR_BUSY, because the lock frees again. InnoDB reports a deadlock at once, and concurrent sessions that insert into one CNID table hit both codes routinely. Crashed tables become CNID_ERR_CORRUPT. Any other code becomes CNID_ERR_DB, which ends the session.

Parameters:
* `mysql_error_code`: MySQL error number, or 0 for a failure without one

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_drain_results](cnid_mysql.c.md#cnid_mysql_drain_results), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_get](cnid_mysql.c.md#cnid_mysql_get), [cnid_mysql_getstamp](cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open), [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### cnid_mysql_drain_results

```c
static int cnid_mysql_drain_results(MYSQL *con)
```

Defined at lines 636 to 656.

Drain the remaining results of a multi-statement batch.

The cnid_mysql_execute call reports only the first statement of a batch. A positive mysql_next_result means a later statement failed, so a wipe whose TRUNCATE or ALTER failed still fails here.

Parameters:
* `con`: MySQL connection

Returns: 0 when every statement succeeded, -1 with errno classified otherwise

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno)

Called by: [cnid_mysql_empty](cnid_mysql.c.md#cnid_mysql_empty)

### cnid_mysql_empty

```c
static int cnid_mysql_empty(const CNID_mysql_private *db, int depleted)
```

Defined at lines 670 to 691.

Empty the volume's table, restart its CNIDs at 17 and write Depleted.

One batch runs it, but TRUNCATE and ALTER commit on their own, so a TRUNCATE that fails leaves the new mark and the rows in place.

Parameters:
* `db`: MySQL backend state
* `depleted`: the mark, 1 or 0

Returns: 0, or -1 with errno classified when a statement failed

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_mysql_drain_results](cnid_mysql.c.md#cnid_mysql_drain_results), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_wipe](cnid_mysql.c.md#cnid_mysql_wipe)

### cnid_mysql_stmt_execute

```c
static int cnid_mysql_stmt_execute(CNID_mysql_private *db, MYSQL_STMT **stmtp)
```

Defined at lines 706 to 755.

Execute a prepared statement and reprepare after CR_SERVER_LOST.

The stmtp argument must point at the field in db, because a reprepare replaces every statement and the retry runs the new one. A reprepare also drops the stored result of any other statement. CR_SERVER_LOST on CNID_MYSQL_RECONNECT_ATTEMPTS executes in a row ends the retries. A NULL statement from an earlier failed reprepare gets prepared first.

Parameters:
* `db`: MySQL backend state
* `stmtp`: address of the statement field in db

Returns: 0, or -1 with errno classified

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Called by: [cnid_mysql_delete](cnid_mysql.c.md#cnid_mysql_delete), [cnid_mysql_dirlist](cnid_mysql.c.md#cnid_mysql_dirlist), [cnid_mysql_get](cnid_mysql.c.md#cnid_mysql_get), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

Mentioned in the documentation of: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

### cnid_mysql_execute

```c
static int cnid_mysql_execute(MYSQL *con, const char *sql)
```

Defined at lines 767 to 779.

Run an SQL statement or batch on a connection.

The call returns -1 for any failure of mysql_query, so EC_NEG1 call sites see it.

Parameters:
* `con`: MySQL connection
* `sql`: SQL text

Returns: 0, or -1 with errno classified

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno)

Called by: [cnid_mysql_empty](cnid_mysql.c.md#cnid_mysql_empty), [cnid_mysql_getstamp](cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open)

### cnid_mysql_escape

```c
static char * cnid_mysql_escape(MYSQL *con, const char *s)
```

Defined at lines 788 to 812.

Escape a string for a quoted literal of SQL text.

Parameters:
* `con`: MySQL connection, whose charset the escaping follows
* `s`: the string

Returns: the escaped string, to free(), or NULL when out of memory

Called by: [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open)

### cnid_mysql_delete

```c
int cnid_mysql_delete(struct _cnid_db *cdb, const cnid_t id)
```

Defined at lines 821 to 839.

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

Defined at lines 846 to 871.

Close the MySQL connection of a CNID database and free the database.

Parameters:
* `cdb`: CNID database, and a NULL one logs an error

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open)

### cnid_mysql_update

```c
int cnid_mysql_update(struct _cnid_db *cdb, cnid_t id, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 890 to 974.

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

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [cnid_mysql_fold_param](cnid_mysql.c.md#cnid_mysql_fold_param), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Called by: [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_lookup

```c
cnid_t cnid_mysql_lookup(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 996 to 1194.

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

Defined at lines 1216 to 1408.

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

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [cnid_mysql_empty](cnid_mysql.c.md#cnid_mysql_empty), [cnid_mysql_fold_param](cnid_mysql.c.md#cnid_mysql_fold_param), [cnid_mysql_hint_usable](cnid_mysql.c.md#cnid_mysql_hint_usable), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_get

```c
cnid_t cnid_mysql_get(struct _cnid_db *cdb, cnid_t did, const char *name, size_t len)
```

Defined at lines 1423 to 1490.

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

Defined at lines 1505 to 1569.

Resolve a CNID to its parent and name in the MySQL table.

A contended database reports a classified errno, not a missing CNID, which afp_resolveid would turn into a permanent AFPERR_NOID.

Parameters:
* `cdb`: CNID database
* `id`: CNID to resolve, receives the parent CNID or CNID_INVALID
* `buffer`: receives the name
* `len`: size of buffer

Returns: buffer, or NULL with errno classified

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new), [cnid_mysql_under](cnid_mysql.c.md#cnid_mysql_under)

Uses file-scope variables: `resolve_result`, `resolve_result_did`, `resolve_result_name`, `resolve_result_name_len`, `stmt_param_id`

### cnid_mysql_getstamp

```c
int cnid_mysql_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
```

Defined at lines 1581 to 1644.

Read the stamp of a volume from the volumes table.

The stamp is the time the volume first entered the table.

Parameters:
* `cdb`: CNID database
* `buffer`: receives len bytes of the stamp
* `len`: bytes to read

Returns: 0, or -1 on error

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_like

```c
static const char * cnid_mysql_like(const struct sl_name_pattern *seed, bool folded)
```

Defined at lines 1660 to 1665.

The LIKE pattern a name search sends the server for a seed.

Folded1 compares bytes, so it takes the seed's pattern over folded names; Name takes the pattern that holds under any collation. Every name the seed matches satisfies the pattern of either column.

Parameters:
* `seed`: the seed
* `folded`: whether the pattern compares the folded names

Returns: the pattern, which the seed holds

Calls: [sl_name_pattern_index_like](../util/sl_names.c.md#sl_name_pattern_index_like), [sl_name_pattern_like](../util/sl_names.c.md#sl_name_pattern_like)

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find), [cnid_mysql_find_narrows](cnid_mysql.c.md#cnid_mysql_find_narrows)

### cnid_mysql_find_folded

```c
static bool cnid_mysql_find_folded(const CNID_mysql_private *db, cnid_t scope_did)
```

Defined at lines 1678 to 1683.

Tell whether a name search compares the folded names.

A folder search compares Name, which its walk reads from the DidName index, where the folded names would cost a read of each row of the folder.

Parameters:
* `db`: MySQL backend state
* `scope_did`: the search's folder, or CNID_INVALID for the volume

Returns: true when the table holds folded names and the search covers the volume, as every search does on a server without recursive CTEs

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find), [cnid_mysql_find_narrows](cnid_mysql.c.md#cnid_mysql_find_narrows)

Mentioned in the documentation of: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

### cnid_mysql_find_narrows

```c
static bool cnid_mysql_find_narrows(struct _cnid_db *cdb, const struct sl_name_pattern *seed, cnid_t scope_did)
```

Defined at lines 1693 to 1701.

Tell whether a seed's LIKE pattern narrows a name search.

Parameters:
* `cdb`: CNID database
* `seed`: the seed
* `scope_did`: the search's folder, or CNID_INVALID for the volume

Returns: true when the pattern holds a character other than ''

Calls: [cnid_mysql_find_folded](cnid_mysql.c.md#cnid_mysql_find_folded), [cnid_mysql_like](cnid_mysql.c.md#cnid_mysql_like)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_folded_narrows

```c
static bool cnid_mysql_folded_narrows(const struct sl_name_pattern *seed, void *ctx)
```

Defined at lines 1712 to 1718.

Tell whether a seed's folded-name pattern narrows a volume search.

For the volume search a folder search falls back to.

Parameters:
* `seed`: the seed
* `ctx`: unused

Returns: true when the pattern holds a character other than ''

Calls: [sl_name_pattern_index_like](../util/sl_names.c.md#sl_name_pattern_index_like)

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

### cnid_mysql_find_cond

```c
static bool cnid_mysql_find_cond(bool folded, int npat, char *cond, size_t size)
```

Defined at lines 1732 to 1755.

Write the WHERE condition of a name search over its LIKE patterns.

The condition keeps the rows whose names one pattern keeps, every row for none, and a row without a folded name as a candidate for the matcher.

Parameters:
* `folded`: whether the patterns compare the folded names
* `npat`: LIKE patterns, 0 for every row
* `cond`: receives the condition
* `size`: room in cond

Returns: true when the condition fits

Called by: [cnid_mysql_find_prepare](cnid_mysql.c.md#cnid_mysql_find_prepare)

### cnid_mysql_find_prepare

```c
static int cnid_mysql_find_prepare(CNID_mysql_private *db, MYSQL_STMT *stmt, int npat, bool folded, bool *scoped)
```

Defined at lines 1773 to 1848.

Prepare a name search: a batch of the volume, or a folder's subtree.

A volume search takes the rows after an Id whose names a LIKE pattern keeps. A folder search walks its subtree with a recursive CTE once and streams the rows unordered: a batch after the last Id would walk and sort the whole subtree again for every batch. A server that cannot parse the CTE latches the unscoped search for the connection.

Parameters:
* `db`: MySQL backend state
* `stmt`: the statement
* `npat`: LIKE patterns, 0 for every name
* `folded`: whether the patterns compare the folded names
* `scoped`: whether the search is scoped, cleared on a fallback

Returns: 0, or -1 when the prepare fails

Calls: [asprintf](../compat/misc.c.md#asprintf), [cnid_mysql_find_cond](cnid_mysql.c.md#cnid_mysql_find_cond)

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

### cnid_mysql_cut_short

```c
static bool cnid_mysql_cut_short(MYSQL *con)
```

Defined at lines 1862 to 1891.

Tell whether a folder search's walk stopped short of its subtree.

MariaDB ends a recursion past max_recursive_iterations with the rows so far and warning 1931, which the client library does not count for a prepared statement, so SHOW WARNINGS tells. A server that keeps fewer warnings than it counts, under max_error_count, may have dropped it.

Parameters:
* `con`: MySQL connection

Returns: true when the walk stopped short or may have, or its warnings do not read

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

### cnid_mysql_find_bind

```c
static int cnid_mysql_find_bind(MYSQL_STMT *stmt, bool scoped, unsigned long long *scope, bool binary, const char *const *likes, unsigned long *lens, int npat, unsigned long long *after, unsigned long long *batch, MYSQL_BIND *result)
```

Defined at lines 1908 to 1949.

Bind a name search's parameters and its result columns.

Parameters:
* `stmt`: the prepared search
* `scoped`: whether the search takes a scope, and no batches
* `scope`: CNID of the scope directory in host order
* `binary`: whether the patterns compare bytes of folded names
* `likes`: the LIKE patterns
* `lens`: bytes of each pattern
* `npat`: number of patterns
* `after`: Id the rows of a batch follow
* `batch`: rows one batch fetches
* `result`: the Id and the name of a row

Returns: 0, or -1 when a bind fails

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

### cnid_mysql_id_ok

```c
static bool cnid_mysql_id_ok(unsigned long long id)
```

Defined at lines 1959 to 1968.

Tell whether a row's Id is a CNID, logging one that is not.

A corrupt row's match drops out of the search.

Parameters:
* `id`: the Id

Returns: true for 1 to UINT32_MAX

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

### cnid_mysql_lowest

```c
static bool cnid_mysql_lowest(uint32_t *heap, int *n, int cap, uint32_t id)
```

Defined at lines 1980 to 2017.

Keep the lowest Ids of an unordered stream of matches, in a max-heap.

Parameters:
* `heap`: the Ids kept, the largest first
* `n`: Ids in heap
* `cap`: room in heap
* `id`: the Id of a match

Returns: true when heap was full, so the search holds more matches than it keeps

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

### cnid_mysql_id_order

```c
static int cnid_mysql_id_order(const void *a, const void *b)
```

Defined at lines 2026 to 2031.

Order two host-order Ids.

Parameters:
* `a`: an Id
* `b`: another

Returns: -1, 0 or 1

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

### cnid_mysql_under

```c
static int cnid_mysql_under(struct _cnid_db *cdb, struct cnid_mysql_scope *sc, cnid_t id)
```

Defined at lines 2064 to 2101.

Tell whether an entry lies under a folder search's folder.

The walk ends at the folder, at the root, at a folder whose verdict a walk kept, or past MAXPATHLEN / 2 folders, as many as a path of MAXPATHLEN bytes holds, which only a loop in the table makes and which leaves the entry outside. Each folder it passed then keeps the verdict, so a loop costs one walk a search. An entry whose walk meets a missing row is outside and keeps no verdict, as a rename deletes a row before it adds it.

Parameters:
* `cdb`: CNID database
* `sc`: the folder and the verdicts
* `id`: CNID of the entry, in network order

Returns: 1 when the entry lies under the folder, the folder itself not, 0 when it lies outside, -1 when a resolve fails otherwise than for a missing row, errno classified

Calls: [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve)

Called by: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

Mentioned in the documentation of: [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find)

### cnid_mysql_find

```c
int cnid_mysql_find(struct _cnid_db *cdb, const struct sl_name_query *query, const struct sl_name_pattern *const *seeds, int nseeds, cnid_t scope_did, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 2133 to 2343.

Find the CNIDs whose names match a query, for cnid_find.

The cnid_find wrapper validates the arguments. One statement takes the rows whose names one of the seeds' LIKE patterns keeps, every row without seeds, and the query decides each name; the patterns bind as statement parameters, never as SQL text. A volume search reads the rows after the last Id the previous batch read, the first batch 1 more than buffer holds and each next eight times larger, CNID_MYSQL_FIND_BATCH at most, until a match past buffer sets more_available. A folder search walks its subtree with a recursive CTE once and streams the rows unordered, keeping the lowest Ids that match, the matches a volume search finds; it compares Name, as [cnid_mysql_find_folded()](cnid_mysql.c.md#cnid_mysql_find_folded) says. A server without CTE support, a failed scoped execute, or a walk cut short with a warning, as MariaDB's past max_recursive_iterations, makes it a volume search that keeps the matches [cnid_mysql_under()](cnid_mysql.c.md#cnid_mysql_under) places under the folder, before they take room in buffer. That search compares the folded names where the table holds them, except on the search that finds the server without CTEs.

Parameters:
* `cdb`: CNID database
* `query`: the name comparisons
* `seeds`: the query's seeds, or NULL for every name
* `nseeds`: number of seeds
* `scope_did`: CNID of the scope directory, or CNID_INVALID for all of the volume
* `buffer`: receives the CNIDs in network order
* `buflen`: size of buffer
* `more_available`: receives true when more matches exist, or NULL

Returns: the number of CNIDs, or -1 on error

Calls: [cnid_mysql_cut_short](cnid_mysql.c.md#cnid_mysql_cut_short), [cnid_mysql_find_bind](cnid_mysql.c.md#cnid_mysql_find_bind), [cnid_mysql_find_folded](cnid_mysql.c.md#cnid_mysql_find_folded), [cnid_mysql_find_prepare](cnid_mysql.c.md#cnid_mysql_find_prepare), [cnid_mysql_folded_narrows](cnid_mysql.c.md#cnid_mysql_folded_narrows), [cnid_mysql_id_ok](cnid_mysql.c.md#cnid_mysql_id_ok), [cnid_mysql_id_order](cnid_mysql.c.md#cnid_mysql_id_order), [cnid_mysql_like](cnid_mysql.c.md#cnid_mysql_like), [cnid_mysql_lowest](cnid_mysql.c.md#cnid_mysql_lowest), [cnid_mysql_under](cnid_mysql.c.md#cnid_mysql_under), [sl_name_pattern_index_like](../util/sl_names.c.md#sl_name_pattern_index_like), [sl_name_query_match](../util/sl_names.c.md#sl_name_query_match), [sl_name_query_seeds](../util/sl_names.c.md#sl_name_query_seeds)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_wipe

```c
int cnid_mysql_wipe(struct _cnid_db *cdb)
```

Defined at lines 2354 to 2378.

Empty the MySQL table of a volume and restart its CNIDs at 17.

The call also clears the depletion marks of the volume and of the connection. A dbd rebuild then restores the CNIDs from the metadata again.

Parameters:
* `cdb`: CNID database

Returns: 0, or -1 with errno classified

Calls: [cnid_mysql_empty](cnid_mysql.c.md#cnid_mysql_empty)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_dirlist

```c
static int cnid_mysql_dirlist(struct _cnid_db *cdb, cnid_t did, size_t max, cnid_dirlist_fn fn, void *ctx)
```

Defined at lines 2393 to 2424.

Hand at most max rows of one folder to fn.

Every row is fetched, so the connection is free for the next statement when fn stops early.

Parameters:
* `cdb`: CNID database
* `did`: the folder
* `max`: the rows to hand over at most
* `fn`: receives each row; nonzero stops the rows
* `ctx`: passed to fn

Returns: 0, or -1 when the query or fn failed

Calls: [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `dirlist_param_max`, `dirlist_result`, `dirlist_result_dev`, `dirlist_result_id`, `dirlist_result_ino`, `dirlist_result_name`, `dirlist_result_name_len`, `stmt_param_did`

### cnid_mysql_new

```c
static struct _cnid_db * cnid_mysql_new(struct vol *vol)
```

Defined at lines 2432 to 2455.

Allocate a CNID database object with the MySQL function table.

Parameters:
* `vol`: volume of the database

Returns: the object, or NULL when calloc fails

Calls: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_close](cnid_mysql.c.md#cnid_mysql_close), [cnid_mysql_delete](cnid_mysql.c.md#cnid_mysql_delete), [cnid_mysql_dirlist](cnid_mysql.c.md#cnid_mysql_dirlist), [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find), [cnid_mysql_find_narrows](cnid_mysql.c.md#cnid_mysql_find_narrows), [cnid_mysql_get](cnid_mysql.c.md#cnid_mysql_get), [cnid_mysql_getstamp](cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update), [cnid_mysql_wipe](cnid_mysql.c.md#cnid_mysql_wipe)

Called by: [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open)

### cnid_mysql_open

```c
struct _cnid_db * cnid_mysql_open(struct cnid_open_args *args)
```

Defined at lines 2470 to 2818.

Open the MySQL CNID backend for a volume.

The call creates the state directory of the volume as root, connects to the server, and creates the database when missing. It then creates the volumes table and the CNID table of the volume, named by the volume UUID. It drops the tables and rows of the same path under another UUID. The volume row gets its stamp on first use, and a depleted flag carries into the connection.

Parameters:
* `args`: open arguments with the volume and the flags

Returns: the database, or NULL on error

Calls: [asprintf](../compat/misc.c.md#asprintf), [become_root](../util/unix.c.md#become_root), [cnid_mysql_close](cnid_mysql.c.md#cnid_mysql_close), [cnid_mysql_escape](cnid_mysql.c.md#cnid_mysql_escape), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt), [unbecome_root](../util/unix.c.md#unbecome_root), [uuid_strip_dashes](../util/cnid.c.md#uuid_strip_dashes)

Called through [`_cnid_module::cnid_open`](../../include/atalk/cnid.h.md#struct-_cnid_module) by: [cnid_open](cnid.c.md#cnid_open)

Dispatched via: [cnid_mysql_module](cnid_mysql.c.md#cnid_mysql_module)

# Types

### struct cnid_mysql_scope

Defined at line 2039.
* `cnid_t scope`
* `cnid_t did`
* `bool under`
* `struct cnid_mysql_scope seen`

# Macros

* Undocumented: `CNID_MYSQL_ADD_ATTEMPTS`, `CNID_MYSQL_FIND_BATCH`, `CNID_MYSQL_FOLDED`, `CNID_MYSQL_FOLDED_MAX`, `CNID_MYSQL_FOLDED_NAME`, `CNID_MYSQL_HINT_RESERVE`, `CNID_MYSQL_MAX_HINT`, `CNID_MYSQL_RECONNECT_ATTEMPTS`, `CNID_MYSQL_SCOPE_CACHE`

# File-scope variables

`add_param`, `delete_param`, `dirlist_param`, `dirlist_param_max`, `dirlist_result`, `dirlist_result_dev`, `dirlist_result_id`, `dirlist_result_ino`, `dirlist_result_name`, `dirlist_result_name_len`, `get_param`, `get_result`, `get_result_id`, `lookup_param`, `lookup_result`, `lookup_result_dev`, `lookup_result_did`, `lookup_result_id`, `lookup_result_ino`, `lookup_result_name`, `lookup_result_name_len`, `purge_param`, `put_param`, `resolve_param`, `resolve_result`, `resolve_result_did`, `resolve_result_name`, `resolve_result_name_len`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_folded`, `stmt_param_folded_len`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`
