---
type: C Source File
title: "libatalk/cnid/mysql/cnid_mysql.c"
description: "26 functions, includes 8 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/mysql/cnid_mysql.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Initialized at line 1873 as `struct _cnid_module`. Assigns:

* `cnid_open`: [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open)

# Functions

### cnid_mysql_hint_usable

```c
static bool cnid_mysql_hint_usable(cnid_t hint)
```

Defined at lines 99 to 103.

Whether a CNID hint from AppleDouble/EA metadata is safe to bind.

Rejects the reserved range below CNID_START and anything above CNID_MYSQL_MAX_HINT, which as an explicit Id would raise the AUTO_INCREMENT high-water mark towards the depletion reset.

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add)

### init_prepared_stmt_lookup

```c
static int init_prepared_stmt_lookup(CNID_mysql_private *db)
```

Defined at lines 105 to 152.

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `lookup_param`, `lookup_result`, `lookup_result_dev`, `lookup_result_did`, `lookup_result_id`, `lookup_result_ino`, `lookup_result_name`, `lookup_result_name_len`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_add

```c
static int init_prepared_stmt_add(CNID_mysql_private *db)
```

Defined at lines 154 to 184.

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `add_param`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_put

```c
static int init_prepared_stmt_put(CNID_mysql_private *db)
```

Defined at lines 186 to 219.

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `put_param`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_get

```c
static int init_prepared_stmt_get(CNID_mysql_private *db)
```

Defined at lines 221 to 247.

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `get_param`, `get_result`, `get_result_id`, `stmt_param_did`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt_delete

```c
static int init_prepared_stmt_delete(CNID_mysql_private *db)
```

Defined at lines 249 to 268.

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `delete_param`, `stmt_param_id`

### init_prepared_stmt_resolve

```c
static int init_prepared_stmt_resolve(CNID_mysql_private *db)
```

Defined at lines 270 to 296.

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `resolve_param`, `resolve_result`, `resolve_result_did`, `resolve_result_name`, `resolve_result_name_len`, `stmt_param_id`

### init_prepared_stmt_purge

```c
static int init_prepared_stmt_purge(CNID_mysql_private *db)
```

Defined at lines 298 to 332.

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Uses file-scope variables: `purge_param`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### init_prepared_stmt

```c
static int init_prepared_stmt(CNID_mysql_private *db)
```

Defined at lines 334 to 346.

Calls: [init_prepared_stmt_add](cnid_mysql.c.md#init_prepared_stmt_add), [init_prepared_stmt_delete](cnid_mysql.c.md#init_prepared_stmt_delete), [init_prepared_stmt_get](cnid_mysql.c.md#init_prepared_stmt_get), [init_prepared_stmt_lookup](cnid_mysql.c.md#init_prepared_stmt_lookup), [init_prepared_stmt_purge](cnid_mysql.c.md#init_prepared_stmt_purge), [init_prepared_stmt_put](cnid_mysql.c.md#init_prepared_stmt_put), [init_prepared_stmt_resolve](cnid_mysql.c.md#init_prepared_stmt_resolve)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### close_prepared_stmt

```c
static void close_prepared_stmt(CNID_mysql_private *db)
```

Defined at lines 353 to 367.

NULL-safe and NULLs each handle: a failed recovery leaves a mix of closed and live handles, and a later close or re-init must neither double-close nor leak the survivors.

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_close](cnid_mysql.c.md#cnid_mysql_close), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### cnid_mysql_set_errno

```c
static void cnid_mysql_set_errno(unsigned int mysql_error_code)
```

Defined at lines 375 to 392.

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_drain_results](cnid_mysql.c.md#cnid_mysql_drain_results), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_get](cnid_mysql.c.md#cnid_mysql_get), [cnid_mysql_getstamp](cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open), [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### cnid_mysql_drain_results

```c
static int cnid_mysql_drain_results(MYSQL *con)
```

Defined at lines 404 to 424.

Drain the remaining results of a multi-statement batch.

[cnid_mysql_execute()](cnid_mysql.c.md#cnid_mysql_execute) reports only the batch's first statement. mysql_next_result() returns >0 when a later one failed, which a plain "== 0" loop reads as the end of the batch — so a wipe whose TRUNCATE or ALTER failed would be taken for a completed one.

Returns: 0 when every statement succeeded, -1 otherwise

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_wipe](cnid_mysql.c.md#cnid_mysql_wipe)

### cnid_mysql_stmt_execute

```c
static int cnid_mysql_stmt_execute(CNID_mysql_private *db, MYSQL_STMT **stmtp)
```

Defined at lines 432 to 481.

stmtp must point at the handle field in [CNID_mysql_private](../../../include/atalk/cnid_mysql_private.h.md#struct-cnid_mysql_private): CR_SERVER_LOST recovery reallocates every handle, and the retry must execute the fresh one. Recovery also invalidates any other statement's in-flight stored result.

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Called by: [cnid_mysql_delete](cnid_mysql.c.md#cnid_mysql_delete), [cnid_mysql_get](cnid_mysql.c.md#cnid_mysql_get), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

### cnid_mysql_execute

```c
static int cnid_mysql_execute(MYSQL *con, const char *sql)
```

Defined at lines 485 to 497.

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_getstamp](cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open), [cnid_mysql_wipe](cnid_mysql.c.md#cnid_mysql_wipe)

Mentioned in the documentation of: [cnid_mysql_drain_results](cnid_mysql.c.md#cnid_mysql_drain_results)

### cnid_mysql_delete

```c
int cnid_mysql_delete(struct _cnid_db *cdb, const cnid_t id)
```

Defined at lines 499 to 517.

Calls: [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute)

Called by: [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `stmt_param_id`

### cnid_mysql_close

```c
void cnid_mysql_close(struct _cnid_db *cdb)
```

Defined at lines 519 to 543.

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_update

```c
int cnid_mysql_update(struct _cnid_db *cdb, cnid_t id, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 545 to 623.

Calls: [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Called by: [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_lookup

```c
cnid_t cnid_mysql_lookup(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len)
```

Defined at lines 625 to 821.

Calls: [cnid_mysql_delete](cnid_mysql.c.md#cnid_mysql_delete), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update)

Called by: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `lookup_result`, `lookup_result_dev`, `lookup_result_did`, `lookup_result_id`, `lookup_result_ino`, `lookup_result_name`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_add

```c
cnid_t cnid_mysql_add(struct _cnid_db *cdb, const struct stat *st, cnid_t did, const char *name, size_t len, cnid_t hint)
```

Defined at lines 823 to 1027.

Calls: [asprintf](../../compat/misc.c.md#asprintf), [close_prepared_stmt](cnid_mysql.c.md#close_prepared_stmt), [cnid_mysql_drain_results](cnid_mysql.c.md#cnid_mysql_drain_results), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_hint_usable](cnid_mysql.c.md#cnid_mysql_hint_usable), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_get

```c
cnid_t cnid_mysql_get(struct _cnid_db *cdb, cnid_t did, const char *name, size_t len)
```

Defined at lines 1029 to 1096.

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `get_result`, `get_result_id`, `stmt_param_did`, `stmt_param_name`, `stmt_param_name_len`

### cnid_mysql_resolve

```c
char * cnid_mysql_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len)
```

Defined at lines 1098 to 1162.

Calls: [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [cnid_mysql_stmt_execute](cnid_mysql.c.md#cnid_mysql_stmt_execute)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

Uses file-scope variables: `resolve_result`, `resolve_result_did`, `resolve_result_name`, `resolve_result_name_len`, `stmt_param_id`

### cnid_mysql_getstamp

```c
int cnid_mysql_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
```

Defined at lines 1167 to 1228.

Caller passes buffer where we will store the db stamp

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_find

```c
int cnid_mysql_find(struct _cnid_db *cdb, const char *name, size_t namelen, cnid_t scope_did, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 1243 to 1461.

Backend implementation of [cnid_find()](../cnid.c.md#cnid_find) for the mysql backend.

Parameters are pre-validated by the [libatalk/cnid/cnid.c](../cnid.c.md) wrapper, so cdb, name, namelen and buflen are all sane on entry. Detects truncation by building "LIMIT max_results + 1" into the SQL and observing whether the extra row was produced; reports it via `more_available`.

The Name LIKE parameter is bound via mysql_stmt_bind_param, never inlined into the SQL string, to prevent a filename SQL-injection vector. The LIMIT literal is not user-controlled (derived from caller's buflen) and is safely interpolated into the SQL string.

Calls: [asprintf](../../compat/misc.c.md#asprintf)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_wipe

```c
int cnid_mysql_wipe(struct _cnid_db *cdb)
```

Defined at lines 1463 to 1504.

Calls: [asprintf](../../compat/misc.c.md#asprintf), [cnid_mysql_drain_results](cnid_mysql.c.md#cnid_mysql_drain_results), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute)

Called by: [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new)

### cnid_mysql_new

```c
static struct _cnid_db * cnid_mysql_new(struct vol *vol)
```

Defined at lines 1506 to 1527.

Calls: [cnid_mysql_add](cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_close](cnid_mysql.c.md#cnid_mysql_close), [cnid_mysql_delete](cnid_mysql.c.md#cnid_mysql_delete), [cnid_mysql_find](cnid_mysql.c.md#cnid_mysql_find), [cnid_mysql_get](cnid_mysql.c.md#cnid_mysql_get), [cnid_mysql_getstamp](cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_lookup](cnid_mysql.c.md#cnid_mysql_lookup), [cnid_mysql_resolve](cnid_mysql.c.md#cnid_mysql_resolve), [cnid_mysql_update](cnid_mysql.c.md#cnid_mysql_update), [cnid_mysql_wipe](cnid_mysql.c.md#cnid_mysql_wipe)

Called by: [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open)

### cnid_mysql_open

```c
struct _cnid_db * cnid_mysql_open(struct cnid_open_args *args)
```

Defined at lines 1530 to 1871.

Calls: [asprintf](../../compat/misc.c.md#asprintf), [become_root](../../util/unix.c.md#become_root), [cnid_mysql_execute](cnid_mysql.c.md#cnid_mysql_execute), [cnid_mysql_new](cnid_mysql.c.md#cnid_mysql_new), [cnid_mysql_set_errno](cnid_mysql.c.md#cnid_mysql_set_errno), [init_prepared_stmt](cnid_mysql.c.md#init_prepared_stmt), [unbecome_root](../../util/unix.c.md#unbecome_root), [uuid_strip_dashes](../../util/cnid.c.md#uuid_strip_dashes)

Called through [`_cnid_module::cnid_open`](../../../include/atalk/cnid.h.md#struct-_cnid_module) by: [cnid_open](../cnid.c.md#cnid_open)

Dispatched via: [cnid_mysql_module](cnid_mysql.c.md#cnid_mysql_module)

# Macros

* Undocumented: `CNID_MYSQL_ADD_ATTEMPTS`, `CNID_MYSQL_HINT_RESERVE`, `CNID_MYSQL_MAX_HINT`, `CNID_MYSQL_RECONNECT_ATTEMPTS`

# File-scope variables

`add_param`, `delete_param`, `get_param`, `get_result`, `get_result_id`, `lookup_param`, `lookup_result`, `lookup_result_dev`, `lookup_result_did`, `lookup_result_id`, `lookup_result_ino`, `lookup_result_name`, `lookup_result_name_len`, `purge_param`, `put_param`, `resolve_param`, `resolve_result`, `resolve_result_did`, `resolve_result_name`, `resolve_result_name_len`, `stmt_param_dev`, `stmt_param_did`, `stmt_param_id`, `stmt_param_ino`, `stmt_param_name`, `stmt_param_name_len`
