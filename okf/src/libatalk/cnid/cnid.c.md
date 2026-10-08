---
type: C Source File
title: "libatalk/cnid/cnid.c"
description: "16 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/cnid.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/cnid](../cnid.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/list.h](../../include/atalk/list.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `string.h`, `strings.h`, `sys/param.h`, `sys/types.h`, `time.h`

# Functions

### cnid_module_named

```c
static const cnid_module * cnid_module_named(const char *name)
```

Defined at lines 40 to 49.

Registered backend module named `name`, or NULL.

Called by: [cnid_open](cnid.c.md#cnid_open), [cnid_register](cnid.c.md#cnid_register), [cnid_scheme_registered](cnid.c.md#cnid_scheme_registered)

Uses file-scope variables: `modules`

### cnid_scheme_registered

```c
bool cnid_scheme_registered(const char *name)
```

Defined at lines 51 to 54.

Calls: [cnid_module_named](cnid.c.md#cnid_module_named)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional), [resolve_metadata_volume](../../bin/nad/megatron.c.md#resolve_metadata_volume)

### cnid_register

```c
void cnid_register(struct _cnid_module *module)
```

Defined at lines 59 to 69.

Once module has been registered, it cannot be unregistered.

Calls: [cnid_module_named](cnid.c.md#cnid_module_named)

Called by: [cnid_init](cnid_init.c.md#cnid_init)

Uses file-scope variables: `modules`

### cnid_open

```c
struct _cnid_db * cnid_open(struct vol *vol, char *type, int flags)
```

Defined at lines 72 to 118.

Opens CNID database using particular back-end

Calls: [cnid_module_named](cnid.c.md#cnid_module_named)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional), [resolve_metadata_volume](../../bin/nad/megatron.c.md#resolve_metadata_volume), [volume_openDB](../../etc/afpd/volume.c.md#volume_opendb)

Calls through [`_cnid_module::cnid_open`](../../include/atalk/cnid.h.md#struct-_cnid_module): [cnid_mysql_open](mysql/cnid_mysql.c.md#cnid_mysql_open), [cnid_sqlite_open](sqlite/cnid_sqlite.c.md#cnid_sqlite_open)

### valide

```c
static cnid_t valide(cnid_t id)
```

Defined at lines 129 to 151.

protect against bogus value from the DB. adddir really doesn't like 2

Every CNID_INVALID returned from this file carries its own errno: callers read the CNID_ERR_* values to choose between a permanent reply and a retryable one, and CNID_ERR_DB ends the session. No syscall overwrites those values and the logger preserves errno, so one persists until deliberately replaced.

Called by: [cnid_add](cnid.c.md#cnid_add), [cnid_get](cnid.c.md#cnid_get), [cnid_lookup](cnid.c.md#cnid_lookup)

### cnid_close

```c
void cnid_close(struct _cnid_db *db)
```

Defined at lines 154 to 162.

Closes CNID database. Currently it's just a wrapper around db->[cnid_close()](cnid.c.md#cnid_close).

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [close_metadata_volume](../../bin/nad/megatron.c.md#close_metadata_volume), [closevol](../../bin/nad/nad_util.c.md#closevol), [closevol](../../etc/afpd/volume.c.md#closevol), [main](../../bin/dbd/cmd_dbd.c.md#main)

Calls through [`_cnid_db::cnid_close`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_add

```c
cnid_t cnid_add(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, const char *name, const size_t len, cnid_t hint)
```

Defined at lines 165 to 180.

Calls: [valide](cnid.c.md#valide)

Called by: [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [cnid_for_path](../util/cnid.c.md#cnid_for_path), [cnid_for_paths_parent](../../bin/nad/nad_util.c.md#cnid_for_paths_parent), [get_id](../../etc/afpd/file.c.md#get_id), [reenumerate_loop](../../etc/afpd/file.c.md#reenumerate_loop)

Calls through [`_cnid_db::cnid_add`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

Mentioned in the documentation of: [cnid_for_path](../util/cnid.c.md#cnid_for_path)

### cnid_delete

```c
int cnid_delete(struct _cnid_db *cdb, cnid_t id)
```

Defined at lines 183 to 189.

Called by: [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_deleteid](../../etc/afpd/file.c.md#afp_deleteid), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [deletefile](../../etc/afpd/file.c.md#deletefile), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [rm](../../bin/nad/nad_rm.c.md#rm), [rmdir_with_cnid](../../bin/nad/nad_rmdir.c.md#rmdir_with_cnid)

Calls through [`_cnid_db::cnid_delete`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_get

```c
cnid_t cnid_get(struct _cnid_db *cdb, const cnid_t did, char *name, const size_t len)
```

Defined at lines 193 to 200.

Calls: [valide](cnid.c.md#valide)

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [deletefile](../../etc/afpd/file.c.md#deletefile), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

Calls through [`_cnid_db::cnid_get`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_getstamp

```c
int cnid_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
```

Defined at lines 203 to 224.

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [cmd_dbd_scanvol](../../bin/dbd/cmd_dbd_scanvol.c.md#cmd_dbd_scanvol), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional), [resolve_metadata_volume](../../bin/nad/megatron.c.md#resolve_metadata_volume)

Calls through [`_cnid_db::cnid_getstamp`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_lookup

```c
cnid_t cnid_lookup(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, char *name, const size_t len)
```

Defined at lines 227 to 235.

Calls: [valide](cnid.c.md#valide)

Called by: [afp_createid](../../etc/afpd/file.c.md#afp_createid), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [dir_modify](../../etc/afpd/directory.c.md#dir_modify), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [update_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#update_cnid)

Calls through [`_cnid_db::cnid_lookup`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_find

```c
int cnid_find(struct _cnid_db *cdb, const char *name, size_t namelen, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 260 to 265.

Search the CNID database for entries whose name contains a substring.

Centralises parameter validation so all CNID backends behave identically against bad input.

The optional `more_available` out-parameter, when non-NULL, is written unconditionally on entry to false and again by the backend: false on error, true iff the result set was truncated on success (more matches exist than fit in `buffer`).

Parameters:
* `cdb`: CNID database handle
* `name`: UTF-8 substring to search for, NUL-terminated
* `namelen`: bytes in `name`, range 1..MAXPATHLEN
* `buffer`: caller-provided buffer for matching CNIDs in network byte order
* `buflen`: capacity of `buffer` in bytes, must be >= CNID_FIND_MIN_BUFLEN
* `more_available`: set to true iff result set was truncated, NULL to opt out

Returns: number of CNIDs written to `buffer` on success, -1 on failure (errno = CNID_ERR_PARAM for invalid arguments, otherwise the backend's CNID_ERR_* classification, or 0 where it makes none)

Calls: [cnid_find_scoped](cnid.c.md#cnid_find_scoped)

Called by: [catsearch_db](../../etc/afpd/catsearch.c.md#catsearch_db), [nad_find](../../bin/nad/nad_find.c.md#nad_find)

Mentioned in the documentation of: [cnid_find_scoped](cnid.c.md#cnid_find_scoped), [cnid_mysql_find](mysql/cnid_mysql.c.md#cnid_mysql_find), [cnid_sqlite_find](sqlite/cnid_sqlite.c.md#cnid_sqlite_find), [sl_cnid_cap_for](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_cap_for), [sl_cnid_collect](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_collect)

### cnid_find_scoped

```c
int cnid_find_scoped(struct _cnid_db *cdb, const char *name, size_t namelen, cnid_t scope_did, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 287 to 325.

[cnid_find()](cnid.c.md#cnid_find) restricted to the subtree of a directory

Identical contract to [cnid_find()](cnid.c.md#cnid_find); additionally, when `scope_did` is not CNID_INVALID only entries lying underneath that directory match. The scope directory itself is not a result.

Parameters:
* `cdb`: CNID database handle
* `name`: UTF-8 substring to search for, NUL-terminated
* `namelen`: bytes in `name`, range 1..MAXPATHLEN
* `scope_did`: CNID of the scope directory in network byte order, or CNID_INVALID for the whole volume
* `buffer`: caller-provided buffer for matching CNIDs in network byte order
* `buflen`: capacity of `buffer` in bytes, must be >= CNID_FIND_MIN_BUFLEN
* `more_available`: set to true iff result set was truncated, NULL to opt out

Returns: number of CNIDs written to `buffer` on success, -1 on failure (errno = CNID_ERR_PARAM for invalid arguments, otherwise the backend's CNID_ERR_* classification, or 0 where it makes none)

Called by: [cnid_find](cnid.c.md#cnid_find), [sl_cnid_collect](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_collect)

Calls through [`_cnid_db::cnid_find`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_resolve

```c
char * cnid_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len)
```

Defined at lines 328 to 344.

Called by: [afp_deleteid](../../etc/afpd/file.c.md#afp_deleteid), [afp_resolveid](../../etc/afpd/file.c.md#afp_resolveid), [catsearch_db](../../etc/afpd/catsearch.c.md#catsearch_db), [dirlookup_internal](../../etc/afpd/directory.c.md#dirlookup_internal), [nad_find](../../bin/nad/nad_find.c.md#nad_find), [private_demangle](../../etc/afpd/mangle.c.md#private_demangle), [sl_cnid_to_path](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_to_path), [sl_rpc_fetchAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_storeAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_storeattributesforoidarray)

Calls through [`_cnid_db::cnid_resolve`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

Mentioned in the documentation of: [sl_cnid_to_path](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_to_path)

### cnid_update

```c
int cnid_update(struct _cnid_db *cdb, const cnid_t id, const struct stat *st, const cnid_t did, char *name, const size_t len)
```

Defined at lines 347 to 354.

Called by: [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [do_move](../../bin/nad/nad_mv.c.md#do_move), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [update_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#update_cnid)

Calls through [`_cnid_db::cnid_update`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_wipe

```c
int cnid_wipe(struct _cnid_db *cdb)
```

Defined at lines 357 to 367.

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main)

Calls through [`_cnid_db::cnid_wipe`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

# Macros

* Undocumented: `USE_LIST`

# File-scope variables

`modules`
