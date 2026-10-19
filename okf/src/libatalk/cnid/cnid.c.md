---
type: C Source File
title: "libatalk/cnid/cnid.c"
description: "The CNID database front end and backend registry."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/cnid.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-20T00:03:30+11:00 }
---

Part of the [libatalk/cnid](../cnid.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `string.h`, `strings.h`, `sys/param.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### cnid_module_named

```c
static const cnid_module * cnid_module_named(const char *name)
```

Defined at lines 49 to 58.

Find a registered CNID backend by name, ignoring case.

Parameters:
* `name`: backend name

Returns: the backend module, or NULL

Called by: [cnid_open](cnid.c.md#cnid_open), [cnid_register](cnid.c.md#cnid_register), [cnid_scheme_registered](cnid.c.md#cnid_scheme_registered)

Uses file-scope variables: `modules`

### cnid_scheme_registered

```c
bool cnid_scheme_registered(const char *name)
```

Defined at lines 66 to 69.

Tell whether cnid_register added a CNID backend of a name.

Parameters:
* `name`: backend name, compared without case

Returns: true when cnid_module_named finds it

Calls: [cnid_module_named](cnid.c.md#cnid_module_named)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional), [resolve_metadata_volume](../../bin/nad/megatron.c.md#resolve_metadata_volume)

### cnid_register

```c
void cnid_register(struct _cnid_module *module)
```

Defined at lines 80 to 98.

Add a CNID backend to the list of registered backends.

A second backend with the same name, compared without case, only logs an error. A backend cannot leave the registry again and must remain alive for the lifetime of the process.

Parameters:
* `module`: backend to add, kept by pointer

Calls: [cnid_module_named](cnid.c.md#cnid_module_named)

Called by: [cnid_init](cnid_init.c.md#cnid_init)

Uses file-scope variables: `modules`

### cnid_open

```c
struct _cnid_db * cnid_open(struct vol *vol, char *type, int flags)
```

Defined at lines 115 to 178.

Open the CNID database of a volume with the backend that a scheme names.

An unknown scheme falls back to DEFAULT_CNID_SCHEME with a warning. A build without MySQL refuses the mysql scheme instead. CNID_FLAG_NODEV in flags also lands in the database flags. A caller at euid 0 opens a private volume as its owner, through [become_owner()](../util/unix.c.md#become_owner), and a failed switch fails the open.

Parameters:
* `vol`: volume of the database
* `type`: CNID scheme name
* `flags`: CNID_FLAG_* open flags

Returns: the database, or NULL when no backend fits or the open fails

Calls: [become_owner](../util/unix.c.md#become_owner), [cnid_module_named](cnid.c.md#cnid_module_named), [unbecome_owner](../util/unix.c.md#unbecome_owner)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional), [resolve_metadata_volume](../../bin/nad/megatron.c.md#resolve_metadata_volume), [volume_openDB](../../etc/afpd/volume.c.md#volume_opendb)

Calls through [`_cnid_module::cnid_open`](../../include/atalk/cnid.h.md#struct-_cnid_module): [cnid_mysql_open](cnid_mysql.c.md#cnid_mysql_open), [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open)

Mentioned in the documentation of: [cnid_sqlite_open](cnid_sqlite.c.md#cnid_sqlite_open), [create_appledesktop_folder](../../etc/afpd/desktop.c.md#create_appledesktop_folder)

### valide

```c
static cnid_t valide(cnid_t id)
```

Defined at lines 192 to 214.

Reject a CNID from a backend that falls in the reserved range.

An ID under CNID_START means a corrupted database. The first such ID logs an error, and each one sets errno to CNID_ERR_CORRUPT. CNID_INVALID passes with the errno of the backend. Callers read the CNID_ERR_* value to choose between a permanent reply and a retry, and CNID_ERR_DB ends the session. The logger keeps errno, so the value stays until a later call replaces it.

Parameters:
* `id`: CNID in network order

Returns: id, or CNID_INVALID

Called by: [cnid_add](cnid.c.md#cnid_add), [cnid_get](cnid.c.md#cnid_get), [cnid_lookup](cnid.c.md#cnid_lookup)

### cnid_close

```c
void cnid_close(struct _cnid_db *db)
```

Defined at lines 221 to 229.

Close a CNID database through its backend.

Parameters:
* `db`: database, and a NULL one logs an error

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [close_metadata_volume](../../bin/nad/megatron.c.md#close_metadata_volume), [closevol](../../bin/nad/nad_util.c.md#closevol), [closevol](../../etc/afpd/volume.c.md#closevol), [main](../../bin/dbd/cmd_dbd.c.md#main)

Calls through [`_cnid_db::cnid_close`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_add

```c
cnid_t cnid_add(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, const char *name, const size_t len, cnid_t hint)
```

Defined at lines 246 to 261.

Add an object to the CNID database or find its existing CNID.

The call clears errno, so errno reports the classification of this call only. The result passes through valide.

Parameters:
* `cdb`: CNID database
* `st`: stat of the object, for the device and inode
* `did`: CNID of the parent directory
* `name`: name on disk
* `len`: length of name, where 0 fails with CNID_ERR_PARAM
* `hint`: CNID that the metadata of the object carries, or CNID_INVALID

Returns: the CNID, or CNID_INVALID with errno set to a CNID_ERR_* value or 0

Calls: [valide](cnid.c.md#valide)

Called by: [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [cnid_for_path](../util/cnid.c.md#cnid_for_path), [cnid_for_paths_parent](../../bin/nad/nad_util.c.md#cnid_for_paths_parent), [get_id](../../etc/afpd/file.c.md#get_id), [reenumerate_loop](../../etc/afpd/file.c.md#reenumerate_loop)

Calls through [`_cnid_db::cnid_add`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

Mentioned in the documentation of: [cnid_for_path](../util/cnid.c.md#cnid_for_path)

### cnid_delete

```c
int cnid_delete(struct _cnid_db *cdb, cnid_t id)
```

Defined at lines 270 to 276.

Remove a CNID from the database.

Parameters:
* `cdb`: CNID database
* `id`: CNID to remove

Returns: the backend result, 0 on success

Called by: [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_deleteid](../../etc/afpd/file.c.md#afp_deleteid), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [deletefile](../../etc/afpd/file.c.md#deletefile), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [rm](../../bin/nad/nad_rm.c.md#rm), [rmdir_with_cnid](../../bin/nad/nad_rmdir.c.md#rmdir_with_cnid)

Calls through [`_cnid_db::cnid_delete`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_get

```c
cnid_t cnid_get(struct _cnid_db *cdb, const cnid_t did, char *name, const size_t len)
```

Defined at lines 288 to 295.

Look up the CNID of a name in a directory.

Parameters:
* `cdb`: CNID database
* `did`: CNID of the directory
* `name`: name on disk
* `len`: length of name

Returns: the CNID after valide, or CNID_INVALID

Calls: [valide](cnid.c.md#valide)

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [deletefile](../../etc/afpd/file.c.md#deletefile), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [of_hint_siblings](../../etc/afpd/ofork.c.md#of_hint_siblings), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

Calls through [`_cnid_db::cnid_get`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_getstamp

```c
int cnid_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
```

Defined at lines 309 to 330.

Read the stamp of a CNID database.

A backend without stamps gets the current time instead, which makes every cached CNID stale.

Parameters:
* `cdb`: CNID database
* `buffer`: receives the stamp
* `len`: size of buffer

Returns: the backend result, or for a backend without stamps 0, or -1 when buffer cannot hold a time_t

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [cmd_dbd_scanvol](../../bin/dbd/cmd_dbd_scanvol.c.md#cmd_dbd_scanvol), [openvol_optional](../../bin/nad/nad_util.c.md#openvol_optional), [resolve_metadata_volume](../../bin/nad/megatron.c.md#resolve_metadata_volume)

Calls through [`_cnid_db::cnid_getstamp`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_lookup

```c
cnid_t cnid_lookup(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, char *name, const size_t len)
```

Defined at lines 343 to 351.

Look up the CNID of an object by name and parent or by device and inode.

Parameters:
* `cdb`: CNID database
* `st`: stat of the object
* `did`: CNID of the parent directory
* `name`: name on disk
* `len`: length of name

Returns: the CNID after valide, or CNID_INVALID

Calls: [valide](cnid.c.md#valide)

Called by: [afp_createid](../../etc/afpd/file.c.md#afp_createid), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [dir_modify](../../etc/afpd/directory.c.md#dir_modify), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [update_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#update_cnid)

Calls through [`_cnid_db::cnid_lookup`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_find

```c
int cnid_find(struct _cnid_db *cdb, const char *name, size_t namelen, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 376 to 381.

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

Mentioned in the documentation of: [cnid_find_scoped](cnid.c.md#cnid_find_scoped)

### cnid_find_scoped

```c
int cnid_find_scoped(struct _cnid_db *cdb, const char *name, size_t namelen, cnid_t scope_did, void *buffer, size_t buflen, bool *more_available)
```

Defined at lines 403 to 441.

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

Defined at lines 456 to 472.

Resolve a CNID to its name and parent directory.

A stored name of ".." means a corrupted database, and the call then fails with CNID_ERR_CORRUPT.

Parameters:
* `cdb`: CNID database
* `id`: CNID to resolve, receives the parent CNID or CNID_INVALID
* `buffer`: work buffer for the name
* `len`: size of buffer

Returns: the name inside buffer, or NULL

Called by: [afp_deleteid](../../etc/afpd/file.c.md#afp_deleteid), [afp_resolveid](../../etc/afpd/file.c.md#afp_resolveid), [catsearch_db](../../etc/afpd/catsearch.c.md#catsearch_db), [dirlookup_internal](../../etc/afpd/directory.c.md#dirlookup_internal), [nad_find](../../bin/nad/nad_find.c.md#nad_find), [private_demangle](../../etc/afpd/mangle.c.md#private_demangle), [sl_cnid_to_path](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_to_path), [sl_rpc_fetchAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_storeAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_storeattributesforoidarray)

Calls through [`_cnid_db::cnid_resolve`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

Mentioned in the documentation of: [sl_cnid_to_path](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_to_path)

### cnid_update

```c
int cnid_update(struct _cnid_db *cdb, const cnid_t id, const struct stat *st, const cnid_t did, char *name, const size_t len)
```

Defined at lines 485 to 492.

Update the name, parent, and stat of a CNID entry.

Parameters:
* `cdb`: CNID database
* `id`: CNID to update
* `st`: new stat of the object
* `did`: new parent CNID
* `name`: new name on disk
* `len`: length of name

Returns: the backend result, 0 on success

Called by: [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [do_move](../../bin/nad/nad_mv.c.md#do_move), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [update_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#update_cnid)

Calls through [`_cnid_db::cnid_update`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_wipe

```c
int cnid_wipe(struct _cnid_db *cdb)
```

Defined at lines 503 to 513.

Empty a CNID database.

A backend without a wipe function leaves the database unchanged and reports success.

Parameters:
* `cdb`: CNID database

Returns: the backend result, 0 on success

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main)

Calls through [`_cnid_db::cnid_wipe`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

# File-scope variables

`modules`
