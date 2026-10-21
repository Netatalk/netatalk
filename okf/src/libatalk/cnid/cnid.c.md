---
type: C Source File
title: "libatalk/cnid/cnid.c"
description: "The CNID database front end and backend registry."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/cnid/cnid.c"
tags: ["libatalk/cnid"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T20:46:18+11:00 }
---

Part of the [libatalk/cnid](../cnid.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `stdlib.h`, `string.h`, `strings.h`, `sys/param.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### cnid_module_named

```c
static const cnid_module * cnid_module_named(const char *name)
```

Defined at lines 50 to 59.

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

Defined at lines 67 to 70.

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

Defined at lines 81 to 99.

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

Defined at lines 116 to 179.

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

Defined at lines 193 to 215.

Reject a CNID from a backend that falls in the reserved range.

An ID under CNID_START means a corrupted database. The first such ID logs an error, and each one sets errno to CNID_ERR_CORRUPT. CNID_INVALID passes with the errno of the backend. Callers read the CNID_ERR_* value to choose between a permanent reply and a retry, and CNID_ERR_DB ends the session. The logger keeps errno, so the value stays until a later call replaces it.

Parameters:
* `id`: CNID in network order

Returns: id, or CNID_INVALID

Called by: [cnid_add](cnid.c.md#cnid_add), [cnid_get](cnid.c.md#cnid_get), [cnid_lookup](cnid.c.md#cnid_lookup)

### dirtab_hash

```c
static uint64_t dirtab_hash(const char *name, size_t len)
```

Defined at lines 253 to 262.

FNV-1a hash of a name.

Parameters:
* `name`: the name
* `len`: its length

Returns: the hash

Called by: [dirtab_lookup](cnid.c.md#dirtab_lookup), [dirtab_take](cnid.c.md#dirtab_take)

### dirtab_empty

```c
static void dirtab_empty(struct cnid_dirtab *t)
```

Defined at lines 269 to 274.

Free a table's rows and mark it free.

Parameters:
* `t`: the table

Called by: [cnid_close](cnid.c.md#cnid_close), [cnid_dir_arm](cnid.c.md#cnid_dir_arm), [cnid_dir_forget](cnid.c.md#cnid_dir_forget)

### dirtab_of

```c
static struct cnid_dirtab * dirtab_of(const struct _cnid_db *cdb, cnid_t did)
```

Defined at lines 283 to 293.

The table a folder holds, or NULL.

Parameters:
* `cdb`: CNID database
* `did`: the folder

Returns: the table, or NULL

Called by: [cnid_add](cnid.c.md#cnid_add), [cnid_dir_arm](cnid.c.md#cnid_dir_arm), [cnid_dir_forget](cnid.c.md#cnid_dir_forget)

### dirtab_take

```c
static int dirtab_take(void *ctx, uint64_t id, const char *name, size_t len, uint64_t dev, uint64_t ino)
```

Defined at lines 306 to 335.

Take one row of a folder into its table.

Parameters:
* `ctx`: the table
* `id`: the row's CNID
* `name`: the row's name
* `len`: the name's length
* `dev`: the row's device
* `ino`: the row's inode

Returns: 0, or -1 when the rows cannot grow

Calls: [dirtab_hash](cnid.c.md#dirtab_hash)

Called by: [dirtab_load](cnid.c.md#dirtab_load)

### dirtab_load

```c
static void dirtab_load(struct _cnid_db *cdb, struct cnid_dirtab *t)
```

Defined at lines 347 to 385.

Load a folder's rows from one backend query.

A failed query or allocation turns the table off, and the folder's names take the backend; so does a row that finds no free slot within CNID_DIRTAB_PROBES of its own, which names chosen to collide make.

Parameters:
* `cdb`: CNID database
* `t`: the folder's table

Calls: [dirtab_take](cnid.c.md#dirtab_take)

Called by: [cnid_add](cnid.c.md#cnid_add)

Calls through [`_cnid_db::cnid_dirlist`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### dirtab_lookup

```c
static cnid_t dirtab_lookup(const struct cnid_dirtab *t, const char *name, size_t len, uint64_t dev, uint64_t ino)
```

Defined at lines 398 to 418.

A loaded table's CNID for a name whose row holds the object's device and inode.

Parameters:
* `t`: the folder's table
* `name`: the name
* `len`: its length
* `dev`: the object's device, as the backend stores it
* `ino`: the object's inode

Returns: the CNID, or CNID_INVALID

Calls: [dirtab_hash](cnid.c.md#dirtab_hash)

Called by: [cnid_add](cnid.c.md#cnid_add)

### dirtab_off

```c
static void dirtab_off(const struct _cnid_db *cdb)
```

Defined at lines 425 to 432.

Turn the loaded tables off: a CNID write may have moved a row.

Parameters:
* `cdb`: CNID database

Called by: [cnid_delete](cnid.c.md#cnid_delete), [cnid_update](cnid.c.md#cnid_update), [cnid_wipe](cnid.c.md#cnid_wipe)

### cnid_dir_arm

```c
void cnid_dir_arm(struct _cnid_db *cdb, cnid_t did)
```

Defined at lines 445 to 473.

Let one backend query answer a folder's cnid_add calls.

The rows load at the folder's (CNID_DIRTAB_MISSES + 1)-th call. Arming a folder again drops its rows, and the least recently armed table makes room.

Parameters:
* `cdb`: CNID database, where a backend without cnid_dirlist arms nothing
* `did`: the folder

Calls: [dirtab_empty](cnid.c.md#dirtab_empty), [dirtab_of](cnid.c.md#dirtab_of)

Called by: [enumerate](../../etc/afpd/enumerate.c.md#enumerate)

Uses file-scope variables: `dirtab_clock`

### cnid_dir_forget

```c
void cnid_dir_forget(struct _cnid_db *cdb, cnid_t did)
```

Defined at lines 481 to 488.

Free a folder's table.

Parameters:
* `cdb`: CNID database, or NULL
* `did`: the folder

Calls: [dirtab_empty](cnid.c.md#dirtab_empty), [dirtab_of](cnid.c.md#dirtab_of)

Called by: [sd_drop](../../etc/afpd/enumerate.c.md#sd_drop)

### cnid_dir_serve

```c
void cnid_dir_serve(struct _cnid_db *cdb, cnid_t did)
```

Defined at lines 497 to 502.

Let a folder's table answer [cnid_add()](cnid.c.md#cnid_add) while its listing's batch runs, and no table outside one.

Parameters:
* `cdb`: CNID database, or NULL
* `did`: the folder, CNID_INVALID when the batch ends

Called by: [enumerate](../../etc/afpd/enumerate.c.md#enumerate)

### cnid_close

```c
void cnid_close(struct _cnid_db *db)
```

Defined at lines 509 to 522.

Close a CNID database through its backend.

Parameters:
* `db`: database, and a NULL one logs an error

Calls: [dirtab_empty](cnid.c.md#dirtab_empty)

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [close_metadata_volume](../../bin/nad/megatron.c.md#close_metadata_volume), [closevol](../../bin/nad/nad_util.c.md#closevol), [closevol](../../etc/afpd/volume.c.md#closevol), [main](../../bin/dbd/cmd_dbd.c.md#main)

Calls through [`_cnid_db::cnid_close`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_add

```c
cnid_t cnid_add(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, const char *name, const size_t len, cnid_t hint)
```

Defined at lines 542 to 575.

Add an object to the CNID database or find its existing CNID.

The call clears errno before it asks the backend, so errno reports the classification of this call only. The result passes through valide. While the folder's listing batch runs, its table answers for an object last changed before the second preceding the table's load, whose row holds the object's device and inode, unless hint names another CNID.

Parameters:
* `cdb`: CNID database
* `st`: stat of the object, for the device and inode
* `did`: CNID of the parent directory
* `name`: name on disk
* `len`: length of name, where 0 fails with CNID_ERR_PARAM
* `hint`: CNID that the metadata of the object carries, or CNID_INVALID

Returns: the CNID, or CNID_INVALID with errno set to a CNID_ERR_* value or 0

Calls: [dirtab_load](cnid.c.md#dirtab_load), [dirtab_lookup](cnid.c.md#dirtab_lookup), [dirtab_of](cnid.c.md#dirtab_of), [valide](cnid.c.md#valide)

Called by: [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [cnid_for_path](../util/cnid.c.md#cnid_for_path), [cnid_for_paths_parent](../../bin/nad/nad_util.c.md#cnid_for_paths_parent), [get_id](../../etc/afpd/file.c.md#get_id), [reenumerate_loop](../../etc/afpd/file.c.md#reenumerate_loop)

Calls through [`_cnid_db::cnid_add`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

Mentioned in the documentation of: [cnid_dir_serve](cnid.c.md#cnid_dir_serve), [cnid_for_path](../util/cnid.c.md#cnid_for_path)

### cnid_delete

```c
int cnid_delete(struct _cnid_db *cdb, cnid_t id)
```

Defined at lines 584 to 591.

Remove a CNID from the database.

Parameters:
* `cdb`: CNID database
* `id`: CNID to remove

Returns: the backend result, 0 on success

Calls: [dirtab_off](cnid.c.md#dirtab_off)

Called by: [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_deleteid](../../etc/afpd/file.c.md#afp_deleteid), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [deletefile](../../etc/afpd/file.c.md#deletefile), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [rm](../../bin/nad/nad_rm.c.md#rm), [rmdir_with_cnid](../../bin/nad/nad_rmdir.c.md#rmdir_with_cnid)

Calls through [`_cnid_db::cnid_delete`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_get

```c
cnid_t cnid_get(struct _cnid_db *cdb, const cnid_t did, char *name, const size_t len)
```

Defined at lines 603 to 610.

Look up the CNID of a name in a directory.

Parameters:
* `cdb`: CNID database
* `did`: CNID of the directory
* `name`: name on disk
* `len`: length of name

Returns: the CNID after valide, or CNID_INVALID

Calls: [valide](cnid.c.md#valide)

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [deletefile](../../etc/afpd/file.c.md#deletefile), [ea_written](../../etc/afpd/extattrs.c.md#ea_written), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [of_hint_siblings](../../etc/afpd/ofork.c.md#of_hint_siblings), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

Calls through [`_cnid_db::cnid_get`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_getstamp

```c
int cnid_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
```

Defined at lines 624 to 645.

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

Defined at lines 658 to 666.

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

Defined at lines 691 to 696.

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

Defined at lines 718 to 756.

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

Defined at lines 771 to 788.

Resolve a CNID to its name and parent directory.

A stored name of "." or ".." means a corrupted database, and the call then fails with CNID_ERR_CORRUPT.

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

Defined at lines 801 to 809.

Update the name, parent, and stat of a CNID entry.

Parameters:
* `cdb`: CNID database
* `id`: CNID to update
* `st`: new stat of the object
* `did`: new parent CNID
* `name`: new name on disk
* `len`: length of name

Returns: the backend result, 0 on success

Calls: [dirtab_off](cnid.c.md#dirtab_off)

Called by: [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [do_move](../../bin/nad/nad_mv.c.md#do_move), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [update_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#update_cnid)

Calls through [`_cnid_db::cnid_update`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

### cnid_wipe

```c
int cnid_wipe(struct _cnid_db *cdb)
```

Defined at lines 820 to 831.

Empty a CNID database.

A backend without a wipe function leaves the database unchanged and reports success.

Parameters:
* `cdb`: CNID database

Returns: the backend result, 0 on success

Calls: [dirtab_off](cnid.c.md#dirtab_off)

Called by: [main](../../bin/dbd/cmd_dbd.c.md#main)

Calls through [`_cnid_db::cnid_wipe`](../../include/atalk/cnid.h.md#struct-_cnid_db): no table assigns this field

# Types

### struct cnid_dirrow

a folder's row, its name kept as a hash

Defined at line 223.
* `uint64_t hash`
* `uint64_t dev`
* `uint64_t ino`
* `cnid_t id`

### struct cnid_dirtab

the rows of one folder, indexed by name hash

Defined at line 231.
* `cnid_t did`
* `enum dirtab_state state`
* `unsigned long armed`: arm order; the oldest makes room
* `unsigned int calls`: cnid_add calls while armed
* `time_t loaded`: the second before the load began
* `uint32_t count`
* `uint32_t cap`
* `struct cnid_dirrow * rows`
* `uint32_t * index`: row number + 1, 0 for a free slot
* `uint32_t mask`

# Typedefs and enums

* `enum dirtab_state`: `DIRTAB_FREE`, `DIRTAB_ARMED`, `DIRTAB_READY`, `DIRTAB_OFF`

# Macros

* Undocumented: `DIRTABS`

# File-scope variables

`dirtab_clock`, `modules`
