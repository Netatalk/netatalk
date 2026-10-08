---
type: C Source File
title: "etc/afpd/directory.c"
description: "42 functions, includes 29 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/directory.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/afp_util.h](../../include/atalk/afp_util.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/spotlight.h](../../include/atalk/spotlight.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [filedir.h](filedir.h.md)
* [fork.h](fork.h.md)
* [hash.h](hash.h.md)
* [idle_worker.h](idle_worker.h.md)
* [mangle.h](mangle.h.md)
* [pfd_cache.h](pfd_cache.h.md)
* [unix.h](unix.h.md)
* [virtual_icon.h](virtual_icon.h.md)
* [volume.h](volume.h.md)
* System headers: `assert.h`, `bstrlib.h`, `errno.h`, `grp.h`, `pwd.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `utime.h`

# Functions

### netatalk_mkdir

```c
static int netatalk_mkdir(const struct vol *vol, const char *name)
```

Defined at lines 121 to 165.

Calls: [ad_mkdir](../../libatalk/adouble/ad_open.c.md#ad_mkdir)

Called by: [afp_createdir](directory.c.md#afp_createdir), [copydir](directory.c.md#copydir)

### deletedir

```c
static int deletedir(const struct vol *vol, int dirfd, char *dir)
```

Defined at lines 168 to 224.

Calls: [netatalk_rmdir](../../libatalk/vfs/unix.c.md#netatalk_rmdir), [netatalk_unlinkat](../../libatalk/vfs/unix.c.md#netatalk_unlinkat), [opendirat](../../libatalk/vfs/unix.c.md#opendirat), [ostatat](../../libatalk/util/unix.c.md#ostatat)

Called by: [renamedir](directory.c.md#renamedir)

### copydir

```c
static int copydir(struct vol *vol, struct dir *ddir, int dirfd, char *src, char *dst)
```

Defined at lines 227 to 306.

do a recursive copy.

Calls: [copyfile](file.c.md#copyfile), [netatalk_mkdir](directory.c.md#netatalk_mkdir), [opendirat](../../libatalk/vfs/unix.c.md#opendirat), [ostatat](../../libatalk/util/unix.c.md#ostatat)

Called by: [renamedir](directory.c.md#renamedir)

### diroffcnt

```c
static int diroffcnt(struct dir *dir, struct stat *st)
```

Defined at lines 311 to 314.

is our cached offspring count valid?

Called by: [getdirparams](directory.c.md#getdirparams)

### invisible_dots

```c
static int invisible_dots(const struct vol *vol, const char *name)
```

Defined at lines 317 to 321.

Called by: [getdirparams](directory.c.md#getdirparams)

### set_dir_errors

```c
static int set_dir_errors(struct path *path, const char *where, int err)
```

Defined at lines 324 to 338.

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname)

Called by: [setdirparams](directory.c.md#setdirparams)

### cname_mtouname

```c
static int cname_mtouname(const struct vol *vol, struct dir *dir, struct path *ret, int toUTF8)
```

Defined at lines 350 to 418.

Convert name in client encoding to server encoding.

Convert ret->m_name to ret->u_name from client encoding to server encoding. This only gets called from [cname()](directory.c.md#cname).

Returns: 0 on success, -1 on error

Note: If the passed ret->m_name is mangled, we'll demangle it

Calls: [demangle_osx](mangle.c.md#demangle_osx), [movecwd](directory.c.md#movecwd), [mtoUTF8](file.c.md#mtoutf8), [mtoupath](desktop.c.md#mtoupath), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [ucs2_to_charset](../../libatalk/unicode/charcnv.c.md#ucs2_to_charset), [utompath](desktop.c.md#utompath)

Called by: [cname](directory.c.md#cname)

Uses file-scope variables: `afp_errno`, `curdir`

### path_from_dir

```c
static struct path * path_from_dir(struct vol *vol, struct dir *dir, struct path *ret)
```

Defined at lines 432 to 487.

Build struct path from struct dir.

The final movecwd in cname failed, possibly with EPERM or ENOENT. We:

1. move cwd into parent dir (we're often already there, but not always)
1. set struct path to the dirname
1. in case of AFPERR_ACCESS: the dir is there, we just can't chdir into it AFPERR_NOOBJ: the dir was there when we stated it in cname, so we have a race 1. indicate there's no dir for this path
1. remove the dir

Calls: [dir_remove](directory.c.md#dir_remove), [dirlookup](directory.c.md#dirlookup), [movecwd](directory.c.md#movecwd)

Called by: [cname](directory.c.md#cname)

Uses file-scope variables: `afp_errno`, `curdir`

Mentioned in the documentation of: [cname](directory.c.md#cname)

### get_afp_errno

```c
int get_afp_errno(const int param)
```

Defined at lines 494 to 501.

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_addcomment](desktop.c.md#afp_addcomment), [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_createid](file.c.md#afp_createid), [afp_delete](filedir.c.md#afp_delete), [afp_exchangefiles](file.c.md#afp_exchangefiles), [afp_getcomment](desktop.c.md#afp_getcomment), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_opendir](directory.c.md#afp_opendir), [afp_openfork](fork.c.md#afp_openfork), [afp_rename](filedir.c.md#afp_rename), [afp_rmvappl](appl.c.md#afp_rmvappl), [afp_rmvcomment](desktop.c.md#afp_rmvcomment), [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams), [enumerate](enumerate.c.md#enumerate), [getforkparams](fork.c.md#getforkparams)

Uses file-scope variables: `afp_errno`

### dirlookup_internal_retry

```c
static struct dir * dirlookup_internal_retry(const struct vol *vol, cnid_t did, int strict, bstring *fullpath_ptr, char **upath_ptr)
```

Defined at lines 522 to 561.

Retry helper for dirlookup_internal on ENOENT.

When stat() fails with ENOENT, this function cleans stale cache entries and retries the lookup via CNID database. This handles race conditions where directories are renamed/moved by external processes between cache and stat.

Parameters:
* `vol`: Volume
* `did`: Target DID that failed stat
* `strict`: Validation mode (0=initial target, 1=parent recursion)
* `fullpath_ptr`: Pointer to fullpath to cleanup
* `upath_ptr`: Pointer to upath to cleanup

Returns: Retried struct dir from CNID, or NULL on failure

Calls: [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dirlookup_internal](directory.c.md#dirlookup_internal)

Called by: [dirlookup_internal](directory.c.md#dirlookup_internal)

### dirlookup_internal

```c
static struct dir * dirlookup_internal(const struct vol *vol, cnid_t did, int retry, int strict)
```

Defined at lines 590 to 788.

Internal CNID (Directory ID) resolution with retry control.

Resolve a CNID (Directory ID), allocate a struct dir for it.

Algorithm:

1. Check for special DIDs 0 (invalid), 1 (root parent), and 2 (root).
1. Search dircache: * If found and valid, return cached entry
* If strict mode, validate inode before use
1. On cache miss, query CNID database for path component
1. Recurse to build parent chain (terminates at root or cache hit)
1. Build fullpath and stat to verify existence
1. Create new struct dir and populate
1. Add new entry to cache

On ENOENT during stat (step 5), if retry=1:

* Clean stale cache entries
* Retry once via CNID (prevents TOCTOU issues)

Parameters:
* `vol`: pointer to struct vol
* `did`: CNID to resolve
* `retry`: 1 = allow one retry on ENOENT, 0 = no retry
* `strict`: 1 = strict dircache lookup (validate with stat+inode), 0 = optimistic

Returns: pointer to struct dir, or NULL with afp_errno set

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [dir_free](directory.c.md#dir_free), [dir_new](directory.c.md#dir_new), [dir_remove](directory.c.md#dir_remove), [dircache_add](dircache.c.md#dircache_add), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dirlookup_internal_retry](directory.c.md#dirlookup_internal_retry), [fullpath_join](directory.c.md#fullpath_join), [ostat](../../libatalk/util/unix.c.md#ostat), [utompath](desktop.c.md#utompath)

Called by: [dirlookup](directory.c.md#dirlookup), [dirlookup_internal_retry](directory.c.md#dirlookup_internal_retry)

Uses file-scope variables: `afp_errno`, `rootParent`

### dirlookup

```c
struct dir * dirlookup(const struct vol *vol, cnid_t did)
```

Defined at lines 803 to 810.

Optimistic CNID resolution for read/safe code paths.

Resolves a CNID to its cached entry using probabilistic validation. Safe for read operations (enumerate, getparams, opendir, openfork) because stale entries are detected on use and recovered via [movecwd()](directory.c.md#movecwd).

Parameters:
* `vol`: pointer to struct vol
* `did`: DID to resolve

Returns: pointer to struct dir (may have DIRF_ISFILE flag for files)

Calls: [dirlookup_internal](directory.c.md#dirlookup_internal)

Called by: [afp_access](acls.c.md#afp_access), [afp_addappl](appl.c.md#afp_addappl), [afp_addcomment](desktop.c.md#afp_addcomment), [afp_copyfile](file.c.md#afp_copyfile), [afp_createid](file.c.md#afp_createid), [afp_deleteid](file.c.md#afp_deleteid), [afp_getacl](acls.c.md#afp_getacl), [afp_getcomment](desktop.c.md#afp_getcomment), [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_opendir](directory.c.md#afp_opendir), [afp_openfork](fork.c.md#afp_openfork), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_rename](filedir.c.md#afp_rename), [afp_resolveid](file.c.md#afp_resolveid), [afp_rmvappl](appl.c.md#afp_rmvappl), [afp_rmvcomment](desktop.c.md#afp_rmvcomment), [afp_setextattr](extattrs.c.md#afp_setextattr), [afp_setforkparams](fork.c.md#afp_setforkparams), [afp_syncdir](directory.c.md#afp_syncdir), [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db), [cname](directory.c.md#cname), [deletecurdir](directory.c.md#deletecurdir), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children](dircache.c.md#dircache_remove_children), [dirlookup_strict](directory.c.md#dirlookup_strict), [enumerate](enumerate.c.md#enumerate), [getforkparams](fork.c.md#getforkparams), [makemacpath](appl.c.md#makemacpath), [of_closefork](ofork.c.md#of_closefork), [of_statdir](ofork.c.md#of_statdir), [path_from_dir](directory.c.md#path_from_dir), [private_demangle](mangle.c.md#private_demangle), [read_fork](fork.c.md#read_fork), [rfork_invalidate_for_ofork](fork.c.md#rfork_invalidate_for_ofork), [setdirparams](directory.c.md#setdirparams), [sl_rpc_fetchAttributesForOIDArray](spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_storeAttributesForOIDArray](spotlight.c.md#sl_rpc_storeattributesforoidarray)

Mentioned in the documentation of: [dir_remove_and_free](directory.c.md#dir_remove_and_free), [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [dircache_remove_children](dircache.c.md#dircache_remove_children), [dirlookup_strict](directory.c.md#dirlookup_strict)

### dirlookup_strict

```c
struct dir * dirlookup_strict(const struct vol *vol, cnid_t did)
```

Defined at lines 825 to 878.

Validated DID resolution for write/change code paths.

Like [dirlookup()](directory.c.md#dirlookup), but performs stat()+inode validation to ensure the cached entry matches the current filesystem state. Gurantees operating on valid entry. Use for all write/change operations: delete, setdirparams, setfilparams, setfildirparams, setacl, copyfile (dest), exchangefiles, createdir, createfile

Parameters:
* `vol`: pointer to struct vol
* `did`: DID to resolve

Returns: pointer to validated struct dir, or NULL with afp_errno set

Calls: [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dirlookup](directory.c.md#dirlookup), [ostat](../../libatalk/util/unix.c.md#ostat)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [afp_exchangefiles](file.c.md#afp_exchangefiles), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_openfork](fork.c.md#afp_openfork), [afp_rename](filedir.c.md#afp_rename), [afp_setacl](acls.c.md#afp_setacl), [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams), [dir_remove](directory.c.md#dir_remove), [moveandrename](filedir.c.md#moveandrename), [movecwd](directory.c.md#movecwd)

### dir_new

```c
struct dir * dir_new(const char *m_name, const char *u_name, const struct vol *vol, cnid_t pdid, cnid_t did, bstring path, struct stat *st)
```

Defined at lines 897 to 962.

Construct struct dir.

Construct struct dir from parameters.

Parameters:
* `m_name`: directory name in UTF8-dec
* `u_name`: directory name in server side encoding
* `vol`: pointer to struct vol
* `pdid`: Parent CNID
* `did`: CNID
* `path`: Full unix path to object
* `st`: struct stat of object

Returns: pointer to new struct dir or NULL on error

Note: Most of the time mac name and unix name are the same.

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [afp_createfile](file.c.md#afp_createfile), [afp_openvol](volume.c.md#afp_openvol), [dir_add](directory.c.md#dir_add), [dirlookup_internal](directory.c.md#dirlookup_internal), [getmetadata](file.c.md#getmetadata)

### fullpath_join_blk

```c
bstring fullpath_join_blk(const bstring parent, const char *name, int nlen)
```

Defined at lines 978 to 991.

Build "parent/name" as a new bstring from a length-known leaf.

One sized allocation, no growth reallocs: bcatblk's grow check (mlen <= slen + len) stays false at the sized fit, and the extra byte keeps bconchar's balloc(slen + 2) below mlen even for nlen == 0.

Parameters:
* `parent`: parent directory fullpath (required)
* `name`: leaf name in server side encoding (required)
* `nlen`: strlen of name

Returns: new bstring (caller owns), NULL on allocation failure — never a partial join

Called by: [fullpath_join](directory.c.md#fullpath_join), [pfd_repair_path](pfd_cache.c.md#pfd_repair_path)

### fullpath_join

```c
bstring fullpath_join(const bstring parent, const char *name)
```

Defined at lines 1002 to 1005.

Build "parent/name" as a new bstring.

Parameters:
* `parent`: parent directory fullpath (required)
* `name`: leaf name in server side encoding (required)

Returns: new bstring (caller owns), NULL on allocation failure — never a partial join

Calls: [fullpath_join_blk](directory.c.md#fullpath_join_blk)

Called by: [absupath](filedir.c.md#absupath), [afp_createfile](file.c.md#afp_createfile), [dir_add](directory.c.md#dir_add), [dir_modify](directory.c.md#dir_modify), [dirlookup_internal](directory.c.md#dirlookup_internal), [getmetadata](file.c.md#getmetadata)

### dir_free

```c
void dir_free(struct dir *dir)
```

Defined at lines 1012 to 1044.

Free a struct dir and all its members.

Parameters:
* `dir`: (rw) pointer to struct dir

Calls: [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [afp_createfile](file.c.md#afp_createfile), [afp_openvol](volume.c.md#afp_openvol), [closevol](volume.c.md#closevol), [dir_add](directory.c.md#dir_add), [dir_free_invalid_q](directory.c.md#dir_free_invalid_q), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [dirlookup_internal](directory.c.md#dirlookup_internal), [getmetadata](file.c.md#getmetadata)

### dir_modify

```c
int dir_modify(const struct vol *vol, struct dir *dir, const struct dir_modify_args *args)
```

Defined at lines 1071 to 1422.

Update a cached entry in-place with selective field updates.

Modifies specified field groups on an existing cache entry while maintaining hash table consistency. The DCMOD_* bitmask in args->flags controls which fields are updated — unset groups are not touched.

When DCMOD_PATH is set and new_uname or new_pdid differs from current values, the DID/name index is automatically reindexed. The CNID index (d_vid, d_did) is NEVER changed.

Always promotes the entry in ARC (signals recency to eviction algorithm).

Common call patterns: Rename: DCMOD_PATH | DCMOD_STAT After setfilparams: DCMOD_STAT | DCMOD_AD (Phase 2) Stat-only refresh: DCMOD_STAT

Parameters:
* `vol`: Volume (required)
* `dir`: Cache entry to update (required)
* `args`: Parameter struct with DCMOD_* flags and field values

Returns: 0 on success, -1 on error (hash re-insert failure)

Calls: [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [dir_remove](directory.c.md#dir_remove), [dircache_add](dircache.c.md#dircache_add), [dircache_promote](dircache.c.md#dircache_promote), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [dircache_remove](dircache.c.md#dircache_remove), [fullpath_join](directory.c.md#fullpath_join), [pfd_purge](pfd_cache.c.md#pfd_purge), [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [ad_addcomment](desktop.c.md#ad_addcomment), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_rmvcomment](desktop.c.md#ad_rmvcomment), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setacl](acls.c.md#afp_setacl), [afp_setextattr](extattrs.c.md#afp_setextattr), [moveandrename](filedir.c.md#moveandrename), [of_closefork](ofork.c.md#of_closefork), [process_cache_hints](dircache.c.md#process_cache_hints), [rfork_invalidate_for_ofork](fork.c.md#rfork_invalidate_for_ofork), [setdirparams](directory.c.md#setdirparams), [setfilparams](file.c.md#setfilparams), [validation_refresh_and_hint](dircache.c.md#validation_refresh_and_hint)

Uses file-scope variables: `curdir`, `rfork_stat_invalidated` in [etc/afpd/dircache.c](dircache.c.md)

Mentioned in the documentation of: [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [process_cache_hints](dircache.c.md#process_cache_hints)

### dir_add

```c
struct dir * dir_add(struct vol *vol, const struct dir *dir, struct path *path, int len)
```

Defined at lines 1443 to 1558.

Create struct dir from struct path.

Create a new struct dir from struct path. Then add it to the cache.

1. Open adouble file, get CNID from it.
1. Search the database, hinting with the CNID from (1).
1. Build fullpath and create struct dir.
1. Add it to the cache.

Parameters:
* `vol`: pointer to struct vol, possibly modified in callee
* `dir`: pointer to parent directory
* `path`: pointer to struct path with valid path->u_name
* `len`: strlen of path->u_name

Returns: Pointer to new struct dir or NULL on error.

Note: Function also assigns path->m_name from path->u_name.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [dir_free](directory.c.md#dir_free), [dir_new](directory.c.md#dir_new), [dir_remove](directory.c.md#dir_remove), [dircache_add](dircache.c.md#dircache_add), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [fullpath_join](directory.c.md#fullpath_join), [get_id](file.c.md#get_id), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [utompath](desktop.c.md#utompath)

Called by: [afp_createdir](directory.c.md#afp_createdir), [catsearch](catsearch.c.md#catsearch), [cname](directory.c.md#cname), [enumerate](enumerate.c.md#enumerate)

### dir_free_invalid_q

```c
void dir_free_invalid_q(void)
```

Defined at lines 1565 to 1572.

Free the queue with invalid struct dirs.

Note: This gets called at the end of every AFP func.

Calls: [dequeue](../../libatalk/util/queue.c.md#dequeue), [dir_free](directory.c.md#dir_free)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp), [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `invalid_dircache_entries`

### dir_remove_and_free

```c
void dir_remove_and_free(const struct vol *vol, struct dir *dir)
```

Defined at lines 1587 to 1603.

Remove a cache entry and free it immediately.

Unlike [dir_remove()](directory.c.md#dir_remove), this function:

* Does NOT enqueue into invalid_dircache_entries (frees immediately)
* Does NOT check or modify curdir (caller must ensure entry != curdir)
* Does NOT attempt curdir recovery via [dirlookup()](directory.c.md#dirlookup)

DIRCACHE_NOSHRINK prevents hash table shrink during worker iteration.

Used by idle worker during temporal separation AND by [dircache_flush_deferred_for_vol()](dircache.c.md#dircache_flush_deferred_for_vol) during synchronous volume close.

Calls: [dir_free](directory.c.md#dir_free), [dircache_remove](dircache.c.md#dircache_remove)

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `curdir`

### dir_remove

```c
int dir_remove(const struct vol *vol, struct dir *dir, int report_invalid)
```

Defined at lines 1627 to 1727.

Remove a file/directory from dircache with automatic curdir recovery.

This function centralizes global curdir safety for all callers. When removing a cache entry that is curdir, it attempts curdir recovery via CNID database and falls back to volume root if recovery fails.

1. Check the dir
1. Detect if removing curdir and save DID
1. Remove from cache and queue for deallocation
1. Set curdir=NULL if removing curdir (safer than dangling pointer)
1. Attempt recovery via dirlookup(saved_did)
1. If recovery fails, fallback to vol->v_root or rootParent
1. Mark entry invalid

Parameters:
* `vol`: volume pointer
* `dir`: directory/file entry to remove from cache
* `report_invalid`: 1 = report as invalid_on_use (entry was used and found stale), 0 = don't report (proactive cleanup, user deletion, etc.)

Returns: 0 on success, -1 if curdir was removed and recovery failed (curdir guaranteed non-NULL on return)

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [dircache_remove](dircache.c.md#dircache_remove), [dircache_report_invalid_entry](dircache.c.md#dircache_report_invalid_entry), [dirlookup](directory.c.md#dirlookup), [dirlookup_strict](directory.c.md#dirlookup_strict), [enqueue](../../libatalk/util/queue.c.md#enqueue), [iw_note_work](idle_worker.c.md#iw_note_work)

Called by: [afp_delete](filedir.c.md#afp_delete), [afp_exchangefiles](file.c.md#afp_exchangefiles), [afp_openfork](fork.c.md#afp_openfork), [afp_setfilparams](file.c.md#afp_setfilparams), [cname](directory.c.md#cname), [deletecurdir](directory.c.md#deletecurdir), [deletefile](file.c.md#deletefile), [dir_add](directory.c.md#dir_add), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dirlookup_internal](directory.c.md#dirlookup_internal), [dirlookup_internal_retry](directory.c.md#dirlookup_internal_retry), [dirlookup_strict](directory.c.md#dirlookup_strict), [enumerate](enumerate.c.md#enumerate), [getmetadata](file.c.md#getmetadata), [movecwd](directory.c.md#movecwd), [of_closefork](ofork.c.md#of_closefork), [path_from_dir](directory.c.md#path_from_dir), [process_cache_hints](dircache.c.md#process_cache_hints), [setfilparams](file.c.md#setfilparams), [validation_expunge_and_hint](dircache.c.md#validation_expunge_and_hint)

Uses file-scope variables: `afp_errno`, `curdir`, `invalid_dircache_entries`

Mentioned in the documentation of: [dir_remove_and_free](directory.c.md#dir_remove_and_free), [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [dircache_remove_children](dircache.c.md#dircache_remove_children), [process_cache_hints](dircache.c.md#process_cache_hints), [should_validate_cache_entry](dircache.c.md#should_validate_cache_entry)

### cname

```c
struct path * cname(struct vol *vol, struct dir *dir, char **cpath)
```

Defined at lines 1755 to 2072.

Resolve a catalog node name path.

1. Evaluate path type
1. Move to start dir, if we can't, it might be e.g. because of EACCES, build path from dirname, so e.g. getdirparams has sth it can chew on. curdir is dir parent then. All this is done in [path_from_dir()](directory.c.md#path_from_dir).
1. Parse next cnode name in path, cases:
1. single "\0" -> do nothing
1. two or more consecutive "\0" -> chdir("..") one or more times
1. cnode name -> copy it to [path.m_name](../../include/atalk/directory.h.md#struct-path)
1. Get unix name from mac name
1. Special handling of request with did 1
1. stat the cnode name
1. If it's not there, it's probably an afp_createfile|dir, return with curdir = dir parent, struct path = dirname
1. If it's there and it's a file, it must should be the last element of the requested path. Return with curdir = cnode name parent dir, struct path = filename
1. Treat symlinks like files, don't follow them
1. If it's a dir:
1. Search the dircache for it
1. If it's not in the cache, create a struct dir for it and add it to the cache
1. chdir into the dir and
1. set m_name to the mac equivalent of "."
1. goto 3

Calls: [check_name](filedir.c.md#check_name), [cname_mtouname](directory.c.md#cname_mtouname), [dir_add](directory.c.md#dir_add), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [movecwd](directory.c.md#movecwd), [mtoupath](desktop.c.md#mtoupath), [of_stat](ofork.c.md#of_stat), [path_from_dir](directory.c.md#path_from_dir), [setmessage](messages.c.md#setmessage), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_access](acls.c.md#afp_access), [afp_addappl](appl.c.md#afp_addappl), [afp_addcomment](desktop.c.md#afp_addcomment), [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_createid](file.c.md#afp_createid), [afp_delete](filedir.c.md#afp_delete), [afp_exchangefiles](file.c.md#afp_exchangefiles), [afp_getacl](acls.c.md#afp_getacl), [afp_getappl](appl.c.md#afp_getappl), [afp_getcomment](desktop.c.md#afp_getcomment), [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_opendir](directory.c.md#afp_opendir), [afp_openfork](fork.c.md#afp_openfork), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_rename](filedir.c.md#afp_rename), [afp_rmvappl](appl.c.md#afp_rmvappl), [afp_rmvcomment](desktop.c.md#afp_rmvcomment), [afp_setacl](acls.c.md#afp_setacl), [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setextattr](extattrs.c.md#afp_setextattr), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams), [enumerate](enumerate.c.md#enumerate)

Uses file-scope variables: `afp_errno`, `curdir`

Mentioned in the documentation of: [cname_mtouname](directory.c.md#cname_mtouname)

### ochdir_vol

```c
static int ochdir_vol(const char *dir, const struct vol *vol)
```

Defined at lines 2082 to 2103.

[ochdir()](../../libatalk/util/unix.c.md#ochdir) wrapper that rejects chdir across filesystem device boundaries

Parameters:
* `dir`: path to chdir to
* `vol`: volume the path must reside on

Returns: 0 on success, 1 on symlink or device violation, -1 on syserror

Calls: [ochdir](../../libatalk/util/unix.c.md#ochdir)

Called by: [movecwd](directory.c.md#movecwd)

### movecwd

```c
int movecwd(const struct vol *vol, struct dir *dir)
```

Defined at lines 2113 to 2237.

chdir() to dir

Parameters:
* `vol`: pointer to struct vol
* `dir`: pointer to struct dir

Returns: 0 on success, -1 on error with afp_errno set appropriately

Calls: [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dirlookup_strict](directory.c.md#dirlookup_strict), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [ochdir_vol](directory.c.md#ochdir_vol)

Called by: [afp_createdir](directory.c.md#afp_createdir), [afp_deleteid](file.c.md#afp_deleteid), [afp_rename](filedir.c.md#afp_rename), [afp_resolveid](file.c.md#afp_resolveid), [afp_setforkparams](fork.c.md#afp_setforkparams), [afp_syncdir](directory.c.md#afp_syncdir), [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db), [cname](directory.c.md#cname), [cname_mtouname](directory.c.md#cname_mtouname), [deletecurdir](directory.c.md#deletecurdir), [getforkparams](fork.c.md#getforkparams), [of_statdir](ofork.c.md#of_statdir), [path_from_dir](directory.c.md#path_from_dir), [setdirparams](directory.c.md#setdirparams), [sl_rpc_storeAttributesForOIDArray](spotlight.c.md#sl_rpc_storeattributesforoidarray)

Uses file-scope variables: `afp_errno`, `curdir`, `rootParent`

Mentioned in the documentation of: [dirlookup](directory.c.md#dirlookup)

### check_access

```c
int check_access(const AFPObj *obj, struct vol *vol, char *path, int mode)
```

Defined at lines 2244 to 2265.

We can't use unix file's perm to support Apple's inherited protection modes. If we aren't the file's owner we can't change its perms when moving it and smb nfs,... don't even try.

Calls: [accessmode](unix.c.md#accessmode), [ad_dir](../../libatalk/adouble/ad_open.c.md#ad_dir)

Called by: [ad_addcomment](desktop.c.md#ad_addcomment), [ad_rmvcomment](desktop.c.md#ad_rmvcomment), [afp_openfork](fork.c.md#afp_openfork), [setfilparams](file.c.md#setfilparams)

Uses file-scope variables: `curdir`

### file_access

```c
int file_access(const AFPObj *obj, struct vol *vol, struct path *path, int mode)
```

Defined at lines 2268 to 2288.

Calls: [accessmode](unix.c.md#accessmode)

Called by: [afp_openfork](fork.c.md#afp_openfork), [find_adouble](file.c.md#find_adouble)

Uses file-scope variables: `curdir`

### setdiroffcnt

```c
void setdiroffcnt(struct dir *dir, struct stat *st, uint32_t count)
```

Defined at lines 2291 to 2296.

Called by: [enumerate](enumerate.c.md#enumerate), [getdirparams](directory.c.md#getdirparams), [reenumerate_id](file.c.md#reenumerate_id)

### dirreenumerate

```c
int dirreenumerate(struct dir *dir, struct stat *st)
```

Defined at lines 2302 to 2305.

Called by: [reenumerate_id](file.c.md#reenumerate_id)

### getdirparams

```c
int getdirparams(const AFPObj *obj, const struct vol *vol, uint16_t bitmap, struct path *s_path, struct dir *dir, char *buf, size_t *buflen)
```

Defined at lines 2312 to 2582.

Calls: [accessmode](unix.c.md#accessmode), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [diroffcnt](directory.c.md#diroffcnt), [for_each_dirent](enumerate.c.md#for_each_dirent), [invisible_dots](directory.c.md#invisible_dots), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [set_name](file.c.md#set_name), [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset), [setdiroffcnt](directory.c.md#setdiroffcnt), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Called by: [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [enumerate](enumerate.c.md#enumerate), [rslt_add](catsearch.c.md#rslt_add)

### path_error

```c
int path_error(struct path *path, int error)
```

Defined at lines 2585 to 2600.

Calls: [path_isadir](../../include/atalk/directory.h.md#path_isadir)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_opendir](directory.c.md#afp_opendir), [afp_setdirparams](directory.c.md#afp_setdirparams), [enumerate](enumerate.c.md#enumerate)

Uses file-scope variables: `afp_errno`

### afp_setdirparams

```c
int afp_setdirparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2603 to 2661.

Calls: [cname](directory.c.md#cname), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [path_error](directory.c.md#path_error), [setdirparams](directory.c.md#setdirparams), [setvoltime](volume.c.md#setvoltime)

Uses file-scope variables: `afp_errno`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### setdirparams

```c
int setdirparams(struct vol *vol, struct path *path, uint16_t d_bitmap, char *buf)
```

Defined at lines 2666 to 3111.

Note: assume path == '\0' e.g. it's a directory in canonical form

Calls: [accessmode](unix.c.md#accessmode), [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setattr](../../libatalk/adouble/ad_attr.c.md#ad_setattr), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [dir_modify](directory.c.md#dir_modify), [dir_rx_set](../../libatalk/vfs/unix.c.md#dir_rx_set), [dirlookup](directory.c.md#dirlookup), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [movecwd](directory.c.md#movecwd), [mtoumode](unix.c.md#mtoumode), [ostat](../../libatalk/util/unix.c.md#ostat), [set_dir_errors](directory.c.md#set_dir_errors), [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset), [setdeskmode](desktop.c.md#setdeskmode), [setdeskowner](desktop.c.md#setdeskowner), [setdirowner](unix.c.md#setdirowner), [setdirunixmode](unix.c.md#setdirunixmode)

Called by: [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [setfilparams](file.c.md#setfilparams)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `Cur_Path`, `curdir`

### afp_syncdir

```c
int afp_syncdir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3113 to 3211.

Calls: [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [movecwd](directory.c.md#movecwd)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Uses file-scope variables: `afp_errno`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_createdir

```c
int afp_createdir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3213 to 3317.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cname](directory.c.md#cname), [dir_add](directory.c.md#dir_add), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [movecwd](directory.c.md#movecwd), [netatalk_mkdir](directory.c.md#netatalk_mkdir), [of_stat](ofork.c.md#of_stat), [setvoltime](volume.c.md#setvoltime), [sl_index_event](spotlight.c.md#sl_index_event), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno`, `curdir`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### renamedir

```c
int renamedir(struct vol *vol, int dirfd, char *src, char *dst, struct dir *newparent, char *newname)
```

Defined at lines 3328 to 3387.

Rename a directory.

Parameters:
* `vol`: volume
* `dirfd`: -1 means ignore dirfd (or use AT_FDCWD), otherwise src is relative to dirfd
* `src`: old unix filename (not a pathname)
* `dst`: new unix filename (not a pathname)
* `newparent`: curdir
* `newname`: new mac name

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [copydir](directory.c.md#copydir), [deletedir](directory.c.md#deletedir), [unix_rename](../../libatalk/vfs/unix.c.md#unix_rename)

Called by: [moveandrename](filedir.c.md#moveandrename)

Calls through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_renamedir_adouble](../../libatalk/vfs/vfs.c.md#rf_renamedir_adouble), [RF_renamedir_ea](../../libatalk/vfs/vfs.c.md#rf_renamedir_ea), [vfs_renamedir](../../libatalk/vfs/vfs.c.md#vfs_renamedir)

### dir_event_path

```c
const char * dir_event_path(char *buf, size_t buflen, const struct dir *parent, const char *name)
```

Defined at lines 3389 to 3408.

Absolute path of a child of parent for FCE/Spotlight event strings, built from the cached d_fullpath — no getcwd. Falls back to [fullpathname()](../../libatalk/util/unix.c.md#fullpathname) when the parent or its path is unavailable. Returns buf, or [fullpathname()](../../libatalk/util/unix.c.md#fullpathname)'s static buffer on the fallback path.

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [moveandrename](filedir.c.md#moveandrename)

### deletecurdir

```c
int deletecurdir(struct vol *vol)
```

Defined at lines 3411 to 3491.

delete an empty directory

Calls: [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [delete_vetoed_files](filedir.c.md#delete_vetoed_files), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dirlookup](directory.c.md#dirlookup), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [movecwd](directory.c.md#movecwd), [netatalk_rmdir_all_errors](../../libatalk/vfs/unix.c.md#netatalk_rmdir_all_errors)

Called by: [afp_delete](filedir.c.md#afp_delete)

Calls through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletecurdir_adouble](../../libatalk/vfs/vfs.c.md#rf_deletecurdir_adouble), [RF_deletecurdir_ea](../../libatalk/vfs/vfs.c.md#rf_deletecurdir_ea), [vfs_deletecurdir](../../libatalk/vfs/vfs.c.md#vfs_deletecurdir)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `afp_errno`, `curdir`

### afp_mapid

```c
int afp_mapid(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3493 to 3635.

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [getnamefromuuid](../../libatalk/acl/uuid.c.md#getnamefromuuid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_mapname

```c
int afp_mapname(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3637 to 3744.

Calls: [getuuidfromname](../../libatalk/acl/uuid.c.md#getuuidfromname)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_closedir

```c
int afp_closedir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3749 to 3763.

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_opendir

```c
int afp_opendir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3768 to 3811.

Calls: [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_stat](ofork.c.md#of_stat), [path_error](directory.c.md#path_error)

Uses file-scope variables: `afp_errno`, `curdir`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Macros

* Undocumented: `DIRLOOKUP_LOG_FMT`

# File-scope variables

`Cur_Path`, `afp_errno`, `curdir`, `invalid_dircache_entries`, `rootParent`
