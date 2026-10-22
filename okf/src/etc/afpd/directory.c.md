---
type: C Source File
title: "etc/afpd/directory.c"
description: "Directory resolution by CNID and path, the AFP directory commands, FPMapID, and FPMapName."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/directory.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-22T19:27:15+11:00 }
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

Defined at lines 135 to 179.

Create a directory and map the mkdir error to an AFP result code.

With AFPVOL_UNIX_PRIV the mode is 0777 minus the volume umask. Otherwise ad_mkdir derives the mode from the parent directory and sets the owner.

Parameters:
* `vol`: volume of the directory
* `name`: directory to create, relative to the working directory

Returns: AFP_OK, AFPERR_MISC, AFPERR_NOOBJ, AFPERR_VLOCK, AFPERR_ACCESS, AFPERR_EXIST, AFPERR_DFULL, or AFPERR_PARAM

Calls: [ad_mkdir](../../libatalk/adouble/ad_open.c.md#ad_mkdir)

Called by: [afp_createdir](directory.c.md#afp_createdir), [copydir](directory.c.md#copydir)

### deletedir

```c
static int deletedir(const struct vol *vol, int dirfd, char *dir)
```

Defined at lines 196 to 252.

Delete directory dir and everything below it.

The cross-device rename path uses it after copydir, and to undo a failed copydir. The walk deletes files with netatalk_unlinkat, recurses into subdirectories, and stops at the first error. It skips entries whose stat fails, and a directory that fails to open counts as already gone.

Parameters:
* `vol`: volume of the directory
* `dirfd`: directory that dir is relative to, or -1 for the working directory
* `dir`: directory to delete

Returns: AFP_OK, AFPERR_PARAM for a path beyond MAXPATHLEN, or the AFP error of the first failed delete

Calls: [netatalk_rmdir](../../libatalk/vfs/unix.c.md#netatalk_rmdir), [netatalk_unlinkat](../../libatalk/vfs/unix.c.md#netatalk_unlinkat), [opendirat](../../libatalk/vfs/unix.c.md#opendirat), [ostatat](../../libatalk/util/unix.c.md#ostatat)

Called by: [renamedir](directory.c.md#renamedir)

### copydir

```c
static int copydir(struct vol *vol, struct dir *ddir, int dirfd, char *src, char *dst)
```

Defined at lines 271 to 350.

Duplicate the directory tree src as dst for a cross-device rename.

The netatalk_mkdir call creates dst. Subdirectories recurse, files go through copyfile, and each new file and directory keeps the modification time of its source. The walk skips entries whose stat fails and stops at the first error.

Parameters:
* `vol`: volume of both trees
* `ddir`: destination parent directory, passed to copyfile
* `dirfd`: directory that src is relative to, or -1 for the working directory
* `src`: source directory
* `dst`: destination directory, relative to the working directory

Returns: AFP_OK, AFPERR_PARAM for a missing source or a path beyond MAXPATHLEN, or the AFP error of the first failure

Calls: [copyfile](file.c.md#copyfile), [netatalk_mkdir](directory.c.md#netatalk_mkdir), [opendirat](../../libatalk/vfs/unix.c.md#opendirat), [ostatat](../../libatalk/util/unix.c.md#ostatat)

Called by: [renamedir](directory.c.md#renamedir)

### diroffcnt

```c
static int diroffcnt(struct dir *dir, struct stat *st)
```

Defined at lines 359 to 362.

Whether the cached offspring count of dir is still valid.

Parameters:
* `dir`: directory with a cached offspring count
* `st`: fresh stat of the directory

Returns: 1 when the ctime matches the cached one, 0 otherwise

Called by: [offcnt_serve](directory.c.md#offcnt_serve)

### invisible_dots

```c
static int invisible_dots(const struct vol *vol, const char *name)
```

Defined at lines 373 to 377.

Whether name is a dot file that the volume flags as invisible to the Finder.

Parameters:
* `vol`: volume
* `name`: directory entry name

Returns: 1 for a dot name other than . and .. on a volume with AFPVOL_INV_DOTS, 0 otherwise

Called by: [getdirparams](directory.c.md#getdirparams)

### set_dir_errors

```c
static int set_dir_errors(struct path *path, const char *where, int err)
```

Defined at lines 390 to 404.

Map an errno from a directory parameter change to an AFP result code.

EPERM and EACCES map to AFPERR_ACCESS, and EROFS maps to AFPERR_VLOCK. Any other error logs the failing step and maps to AFPERR_PARAM.

Parameters:
* `path`: directory whose parameters changed
* `where`: name of the failing step, for the log line
* `err`: errno value

Returns: AFPERR_ACCESS, AFPERR_VLOCK, or AFPERR_PARAM

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname)

Called by: [setdirparams](directory.c.md#setdirparams)

### cname_mtouname

```c
static int cname_mtouname(const struct vol *vol, struct dir *dir, struct path *ret, int toUTF8)
```

Defined at lines 422 to 484.

Convert the client name in ret->m_name to the server name in ret->u_name.

Only cname calls it. From AFP 3.0 on, a type 2 path name converts to UTF-8 first. Below DID 1 a match with the legacy volume name becomes the UTF-8 volume name. A name that demangle_osx recognizes as mangled resolves to the real name, and m_name receives the client view of it. Any other name goes through mtoupath.

Parameters:
* `vol`: volume of the name
* `dir`: directory that holds the name
* `ret`: path whose m_name to convert and whose u_name to set
* `toUTF8`: nonzero for a type 2 path on AFP 3.0 or later

Returns: 0, or -1 with afp_errno set

Calls: [demangle_osx](mangle.c.md#demangle_osx), [mtoUTF8](file.c.md#mtoutf8), [mtoupath](desktop.c.md#mtoupath), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [ucs2_to_charset](../../libatalk/unicode/charcnv.c.md#ucs2_to_charset), [utompath](desktop.c.md#utompath), [vol_macname](volume.c.md#vol_macname)

Called by: [cname](directory.c.md#cname)

Uses file-scope variables: `afp_errno`

### path_from_dir

```c
static struct path * path_from_dir(struct vol *vol, struct dir *dir, struct path *ret)
```

Defined at lines 502 to 557.

Build a path result from a dircache entry after cname failed to change into it.

The call changes into the parent of dir and fills ret with the names of dir. With AFPERR_ACCESS the directory exists but denies entry, and ret keeps dir. With AFPERR_NOOBJ the directory vanished after cname looked at it, so ret gets no directory and the call removes dir from the cache. The volume root and its parent get no result.

Parameters:
* `vol`: volume of dir
* `dir`: directory that cname could not enter
* `ret`: path result to fill

Returns: ret, or NULL for the volume root or its parent, for any other afp_errno, or when the parent change failed

Calls: [dir_remove](directory.c.md#dir_remove), [dirlookup](directory.c.md#dirlookup), [movecwd](directory.c.md#movecwd)

Called by: [cname](directory.c.md#cname)

Uses file-scope variables: `afp_errno`, `curdir`

### get_afp_errno

```c
int get_afp_errno(const int param)
```

Defined at lines 571 to 578.

AFP error that the last path resolution left, with AFPERR_DID1 replaced by param.

Parameters:
* `param`: AFP error to report when the resolution ended at DID 1

Returns: afp_errno, or param when afp_errno is AFPERR_DID1

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_addcomment](desktop.c.md#afp_addcomment), [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_createid](file.c.md#afp_createid), [afp_delete](filedir.c.md#afp_delete), [afp_exchangefiles](file.c.md#afp_exchangefiles), [afp_getcomment](desktop.c.md#afp_getcomment), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_opendir](directory.c.md#afp_opendir), [afp_openfork](fork.c.md#afp_openfork), [afp_rename](filedir.c.md#afp_rename), [afp_rmvappl](appl.c.md#afp_rmvappl), [afp_rmvcomment](desktop.c.md#afp_rmvcomment), [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams), [enumerate](enumerate.c.md#enumerate), [getforkparams](fork.c.md#getforkparams)

Uses file-scope variables: `afp_errno`

### dirlookup_internal_retry

```c
static struct dir * dirlookup_internal_retry(const struct vol *vol, cnid_t did, int strict, bstring *fullpath_ptr, char **upath_ptr)
```

Defined at lines 599 to 638.

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

Defined at lines 667 to 865.

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

Defined at lines 880 to 887.

Optimistic CNID resolution for read/safe code paths.

Resolves a CNID to its cached entry using probabilistic validation. Safe for read operations (enumerate, getparams, opendir, openfork) because stale entries are detected on use and recovered via [movecwd()](directory.c.md#movecwd).

Parameters:
* `vol`: pointer to struct vol
* `did`: DID to resolve

Returns: pointer to struct dir (may have DIRF_ISFILE flag for files)

Calls: [dirlookup_internal](directory.c.md#dirlookup_internal)

Called by: [afp_access](acls.c.md#afp_access), [afp_addappl](appl.c.md#afp_addappl), [afp_addcomment](desktop.c.md#afp_addcomment), [afp_copyfile](file.c.md#afp_copyfile), [afp_createid](file.c.md#afp_createid), [afp_deleteid](file.c.md#afp_deleteid), [afp_getacl](acls.c.md#afp_getacl), [afp_getcomment](desktop.c.md#afp_getcomment), [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_opendir](directory.c.md#afp_opendir), [afp_openfork](fork.c.md#afp_openfork), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_rename](filedir.c.md#afp_rename), [afp_resolveid](file.c.md#afp_resolveid), [afp_rmvappl](appl.c.md#afp_rmvappl), [afp_rmvcomment](desktop.c.md#afp_rmvcomment), [afp_setextattr](extattrs.c.md#afp_setextattr), [afp_setforkparams](fork.c.md#afp_setforkparams), [afp_syncdir](directory.c.md#afp_syncdir), [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db), [cname](directory.c.md#cname), [deletecurdir](directory.c.md#deletecurdir), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children](dircache.c.md#dircache_remove_children), [dirlookup_strict](directory.c.md#dirlookup_strict), [enumerate](enumerate.c.md#enumerate), [getforkparams](fork.c.md#getforkparams), [makemacpath](appl.c.md#makemacpath), [of_closefork](ofork.c.md#of_closefork), [of_dircache_entry](ofork.c.md#of_dircache_entry), [of_hint_siblings](ofork.c.md#of_hint_siblings), [of_statdir](ofork.c.md#of_statdir), [path_from_dir](directory.c.md#path_from_dir), [setdirparams](directory.c.md#setdirparams), [sl_rpc_fetchAttributesForOIDArray](spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_storeAttributesForOIDArray](spotlight.c.md#sl_rpc_storeattributesforoidarray)

Mentioned in the documentation of: [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [dircache_remove_children](dircache.c.md#dircache_remove_children), [dirlookup_strict](directory.c.md#dirlookup_strict)

### dirlookup_strict

```c
struct dir * dirlookup_strict(const struct vol *vol, cnid_t did)
```

Defined at lines 902 to 955.

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

Defined at lines 974 to 1028.

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

Called by: [afp_createfile](file.c.md#afp_createfile), [afp_openvol](volume.c.md#afp_openvol), [dir_add](directory.c.md#dir_add), [dirlookup_internal](directory.c.md#dirlookup_internal), [getmetadata](file.c.md#getmetadata)

### fullpath_join_blk

```c
bstring fullpath_join_blk(const bstring parent, const char *name, int nlen)
```

Defined at lines 1044 to 1057.

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

Defined at lines 1068 to 1071.

Build "parent/name" as a new bstring.

Parameters:
* `parent`: parent directory fullpath (required)
* `name`: leaf name in server side encoding (required)

Returns: new bstring (caller owns), NULL on allocation failure — never a partial join

Calls: [fullpath_join_blk](directory.c.md#fullpath_join_blk)

Called by: [absupath](filedir.c.md#absupath), [afp_createfile](file.c.md#afp_createfile), [dir_add](directory.c.md#dir_add), [dir_modify](directory.c.md#dir_modify), [dirlookup_internal](directory.c.md#dirlookup_internal), [getmetadata](file.c.md#getmetadata), [of_closefork](ofork.c.md#of_closefork), [of_name_holds](ofork.c.md#of_name_holds)

### dir_free

```c
void dir_free(struct dir *dir)
```

Defined at lines 1078 to 1108.

Free a struct dir and all its members.

Parameters:
* `dir`: (rw) pointer to struct dir

Calls: [ea_cache_free](ad_cache.c.md#ea_cache_free), [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [afp_createfile](file.c.md#afp_createfile), [afp_openvol](volume.c.md#afp_openvol), [closevol](volume.c.md#closevol), [dir_add](directory.c.md#dir_add), [dir_free_invalid_q](directory.c.md#dir_free_invalid_q), [dir_remove_and_free](directory.c.md#dir_remove_and_free), [dirlookup_internal](directory.c.md#dirlookup_internal), [getmetadata](file.c.md#getmetadata), [iw_main](idle_worker.c.md#iw_main)

### dir_modify

```c
int dir_modify(const struct vol *vol, struct dir *dir, const struct dir_modify_args *args)
```

Defined at lines 1135 to 1472.

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

Calls: [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [dir_remove](directory.c.md#dir_remove), [dircache_add](dircache.c.md#dircache_add), [dircache_promote](dircache.c.md#dircache_promote), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [dircache_remove](dircache.c.md#dircache_remove), [ea_cache_free](ad_cache.c.md#ea_cache_free), [fullpath_join](directory.c.md#fullpath_join), [pfd_purge](pfd_cache.c.md#pfd_purge), [rfork_cache_free](ad_cache.c.md#rfork_cache_free)

Called by: [ad_addcomment](desktop.c.md#ad_addcomment), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_rmvcomment](desktop.c.md#ad_rmvcomment), [afp_setacl](acls.c.md#afp_setacl), [dir_stat_refresh](directory.c.md#dir_stat_refresh), [ea_written](extattrs.c.md#ea_written), [moveandrename](filedir.c.md#moveandrename), [of_closefork](ofork.c.md#of_closefork), [of_rfork_changed](ofork.c.md#of_rfork_changed), [process_cache_hints](dircache.c.md#process_cache_hints), [read_fork](fork.c.md#read_fork), [setdirparams](directory.c.md#setdirparams), [setfilparams](file.c.md#setfilparams), [validation_refresh_and_hint](dircache.c.md#validation_refresh_and_hint)

Uses file-scope variables: `curdir`, `rfork_stat_invalidated` in [etc/afpd/dircache.c](dircache.c.md)

Mentioned in the documentation of: [dir_stat_refresh](directory.c.md#dir_stat_refresh), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [dircache_search_by_did](dircache.c.md#dircache_search_by_did)

### dir_add

```c
struct dir * dir_add(struct vol *vol, const struct dir *dir, struct path *path, int len)
```

Defined at lines 1493 to 1608.

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

Defined at lines 1615 to 1622.

Free the queue with invalid struct dirs.

Note: This gets called at the end of every AFP func.

Calls: [dequeue](../../libatalk/util/queue.c.md#dequeue), [dir_free](directory.c.md#dir_free)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp), [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `invalid_dircache_entries`

### dir_remove_and_free

```c
void dir_remove_and_free(const struct vol *vol, struct dir *dir)
```

Defined at lines 1639 to 1655.

Remove a dircache entry and free it at once.

Unlike dir_remove the call frees the entry immediately instead of deferring the free. It never touches curdir or recovers it through dirlookup, and the caller guarantees that dir is not curdir. DIRCACHE_NOSHRINK keeps the hash tables at their size while a scan runs.

The idle worker uses it, and so does dircache_flush_deferred_for_vol while a volume closes. The volume root, its parent, and entries that are already invalid stay untouched.

Parameters:
* `vol`: volume of the entry
* `dir`: entry to free, invalid afterwards

Calls: [dir_free](directory.c.md#dir_free), [dircache_remove](dircache.c.md#dircache_remove)

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

Uses file-scope variables: `curdir`

### dir_remove

```c
int dir_remove(const struct vol *vol, struct dir *dir, int report_invalid)
```

Defined at lines 1679 to 1779.

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

Called by: [afp_delete](filedir.c.md#afp_delete), [afp_exchangefiles](file.c.md#afp_exchangefiles), [afp_openfork](fork.c.md#afp_openfork), [afp_setfilparams](file.c.md#afp_setfilparams), [cname](directory.c.md#cname), [deletecurdir](directory.c.md#deletecurdir), [dir_add](directory.c.md#dir_add), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dirlookup_internal](directory.c.md#dirlookup_internal), [dirlookup_internal_retry](directory.c.md#dirlookup_internal_retry), [dirlookup_strict](directory.c.md#dirlookup_strict), [enumerate](enumerate.c.md#enumerate), [getmetadata](file.c.md#getmetadata), [movecwd](directory.c.md#movecwd), [of_closefork](ofork.c.md#of_closefork), [path_from_dir](directory.c.md#path_from_dir), [process_cache_hints](dircache.c.md#process_cache_hints), [setfilparams](file.c.md#setfilparams), [validation_expunge_and_hint](dircache.c.md#validation_expunge_and_hint)

Uses file-scope variables: `afp_errno`, `curdir`, `invalid_dircache_entries`

Mentioned in the documentation of: [dircache_expunge_duplicate](dircache.c.md#dircache_expunge_duplicate), [dircache_remove_children](dircache.c.md#dircache_remove_children), [should_validate_cache_entry](dircache.c.md#should_validate_cache_entry)

### cname

```c
struct path * cname(struct vol *vol, struct dir *dir, char **cpath)
```

Defined at lines 1823 to 2141.

Resolve an AFP path relative to a directory and change into the directory that holds it.

The path is a type byte followed by a length and the names. Type 2 carries a Pascal string in the client encoding. Type 3 carries a hint and a UTF-8 name and needs AFP 3.0 or later. The numbered steps match the markers in the body.

1. Read the path type and length.
1. Change into dir. When that fails for an empty path, path_from_dir builds the result from the name of dir, so callers such as getdirparams still get a path.
1. Walk the path one name at a time.
1. A single NUL separator does nothing.
1. Each further NUL moves up to the parent directory, never above the volume root.
1. Collect the next name, at most 255 bytes, into the static path buffer.
1. Convert the client name to the server name with cname_mtouname.
1. Below DID 1 the only valid name is the volume name.
1. Stat the name.
1. A missing name must be the last element, as in a create request. The result then names it in the working directory.
1. A file must be the last element. The result names it in its parent.
1. A symlink counts as a file and stays unresolved.
1. A directory continues the walk.
1. Look the directory up in the dircache.
1. Add a dircache entry when the lookup misses.
1. Change into the directory.
1. Clear m_name, so the end of the walk knows that the last element was a directory.

A path that ends in a directory yields "." in that directory, and a walk that ends at DID 1 fails with AFPERR_DID1. Names that check_name rejects, such as .AppleDouble, fail with AFPERR_PARAM and can send a veto message to the client.

Parameters:
* `vol`: volume of the path
* `dir`: directory that the path starts from
* `cpath`: request cursor at the path, advanced past it

Returns: the static path result, or NULL with afp_errno set

Calls: [check_name](filedir.c.md#check_name), [cname_mtouname](directory.c.md#cname_mtouname), [dir_add](directory.c.md#dir_add), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [movecwd](directory.c.md#movecwd), [mtoupath](desktop.c.md#mtoupath), [of_stat](ofork.c.md#of_stat), [path_from_dir](directory.c.md#path_from_dir), [setmessage](messages.c.md#setmessage), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_access](acls.c.md#afp_access), [afp_addappl](appl.c.md#afp_addappl), [afp_addcomment](desktop.c.md#afp_addcomment), [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_createid](file.c.md#afp_createid), [afp_delete](filedir.c.md#afp_delete), [afp_exchangefiles](file.c.md#afp_exchangefiles), [afp_getacl](acls.c.md#afp_getacl), [afp_getappl](appl.c.md#afp_getappl), [afp_getcomment](desktop.c.md#afp_getcomment), [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_opendir](directory.c.md#afp_opendir), [afp_openfork](fork.c.md#afp_openfork), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_rename](filedir.c.md#afp_rename), [afp_rmvappl](appl.c.md#afp_rmvappl), [afp_rmvcomment](desktop.c.md#afp_rmvcomment), [afp_setacl](acls.c.md#afp_setacl), [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setextattr](extattrs.c.md#afp_setextattr), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams), [enumerate](enumerate.c.md#enumerate)

Uses file-scope variables: `afp_errno`, `curdir`

Mentioned in the documentation of: [ea_cache_entry](ad_cache.c.md#ea_cache_entry), [ea_written](extattrs.c.md#ea_written)

### ochdir_vol

```c
static int ochdir_vol(const char *dir, const struct vol *vol)
```

Defined at lines 2151 to 2172.

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

Defined at lines 2182 to 2308.

chdir() to dir

Parameters:
* `vol`: pointer to struct vol
* `dir`: pointer to struct dir

Returns: 0 on success, -1 on error with afp_errno set appropriately

Calls: [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dirlookup_strict](directory.c.md#dirlookup_strict), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [ochdir_vol](directory.c.md#ochdir_vol)

Called by: [afp_createdir](directory.c.md#afp_createdir), [afp_deleteid](file.c.md#afp_deleteid), [afp_rename](filedir.c.md#afp_rename), [afp_resolveid](file.c.md#afp_resolveid), [afp_setforkparams](fork.c.md#afp_setforkparams), [afp_syncdir](directory.c.md#afp_syncdir), [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db), [cname](directory.c.md#cname), [deletecurdir](directory.c.md#deletecurdir), [getforkparams](fork.c.md#getforkparams), [of_statdir](ofork.c.md#of_statdir), [path_from_dir](directory.c.md#path_from_dir), [setdirparams](directory.c.md#setdirparams), [sl_rpc_storeAttributesForOIDArray](spotlight.c.md#sl_rpc_storeattributesforoidarray)

Uses file-scope variables: `afp_errno`, `curdir`, `rootParent`

Mentioned in the documentation of: [dirlookup](directory.c.md#dirlookup)

### check_access

```c
int check_access(const AFPObj *obj, struct vol *vol, char *path, int mode)
```

Defined at lines 2324 to 2345.

Whether the user may read or write in the parent directory of path.

The test uses the AFP access rights of the parent directory instead of the file permissions. Unix permissions cannot express the inherited protection modes of AFP, and a user other than the owner cannot change them while moving the file.

Parameters:
* `obj`: session object
* `vol`: volume of path
* `path`: file whose parent directory to test
* `mode`: OPENACC_RD, OPENACC_WR, or both

Returns: 0 when the rights allow mode, -1 otherwise

Calls: [accessmode](unix.c.md#accessmode), [ad_dir](../../libatalk/adouble/ad_open.c.md#ad_dir)

Called by: [ad_addcomment](desktop.c.md#ad_addcomment), [ad_rmvcomment](desktop.c.md#ad_rmvcomment), [afp_openfork](fork.c.md#afp_openfork), [setfilparams](file.c.md#setfilparams)

Uses file-scope variables: `curdir`

### file_access

```c
int file_access(const AFPObj *obj, struct vol *vol, struct path *path, int mode)
```

Defined at lines 2356 to 2376.

Whether the user may read or write path, from its AFP access rights.

Parameters:
* `obj`: session object
* `vol`: volume of path
* `path`: file with its stat filled in
* `mode`: OPENACC_RD, OPENACC_WR, or both

Returns: 0 when the rights allow mode, -1 otherwise

Calls: [accessmode](unix.c.md#accessmode)

Called by: [afp_openfork](fork.c.md#afp_openfork), [find_adouble](file.c.md#find_adouble)

Uses file-scope variables: `curdir`

### offcnt_stats_get

```c
void offcnt_stats_get(struct offcnt_stats *out)
```

Defined at lines 2385 to 2388.

Copy the session's offspring count statistics.

Parameters:
* `out`: receives the counters

Uses file-scope variables: `offcnt_stat`

### offcnt_log_stats

```c
void offcnt_log_stats(const char *username)
```

Defined at lines 2395 to 2406.

Log the session's offspring count statistics.

Parameters:
* `username`: the session's user

Uses file-scope variables: `offcnt_stat`

### dir_stat_refresh

```c
void dir_stat_refresh(const struct vol *vol, struct dir *dir, struct stat *st)
```

Defined at lines 2418 to 2428.

Bring a dircache entry's stat up to a stat taken from disk.

Same inode only: an inode change may make [dir_modify()](directory.c.md#dir_modify) re-key or drop the entry, which is left to validation and the AppleDouble lookup.

Parameters:
* `vol`: the volume
* `dir`: the entry
* `st`: the stat taken from disk for the entry's object

Calls: [dir_modify](directory.c.md#dir_modify)

Called by: [afp_createfile](file.c.md#afp_createfile), [check_acl_access](acls.c.md#check_acl_access), [ea_cache_entry](ad_cache.c.md#ea_cache_entry), [getdirparams](directory.c.md#getdirparams), [setdiroffcnt](directory.c.md#setdiroffcnt)

Mentioned in the documentation of: [ea_cache_entry](ad_cache.c.md#ea_cache_entry)

### setdiroffcnt

```c
void setdiroffcnt(const struct vol *vol, struct dir *dir, struct stat *st, uint32_t count)
```

Defined at lines 2443 to 2450.

Cache the offspring count of a directory together with its ctime.

On the same inode the entry is brought to the stat first, so the count and the entry agree on the ctime. The cached count stays valid while the directory ctime matches. The call also clears DIRF_CNID, because a plain count does not prove that the CNID database holds every entry.

Parameters:
* `vol`: the volume
* `dir`: directory to update
* `st`: fresh stat of the directory
* `count`: number of visible entries

Calls: [dir_stat_refresh](directory.c.md#dir_stat_refresh)

Called by: [enumerate](enumerate.c.md#enumerate), [offcnt_serve](directory.c.md#offcnt_serve), [reenumerate_id](file.c.md#reenumerate_id)

### offcnt_serve

```c
static uint16_t offcnt_serve(const struct vol *vol, struct dir *dir, struct stat *st, char *upath)
```

Defined at lines 2461 to 2491.

Serve a folder's offspring count, walking it when the count is stale.

Parameters:
* `vol`: the volume
* `dir`: the folder's dircache entry, holding the count
* `st`: the stat the request holds for the folder
* `upath`: the folder's path relative to the working directory

Returns: the count capped at 0xffff, or 0 when the folder cannot be read

Calls: [diroffcnt](directory.c.md#diroffcnt), [for_each_dirent](enumerate.c.md#for_each_dirent), [setdiroffcnt](directory.c.md#setdiroffcnt)

Called by: [getdirparams](directory.c.md#getdirparams)

Uses file-scope variables: `offcnt_stat`

### dirreenumerate

```c
int dirreenumerate(struct dir *dir, struct stat *st)
```

Defined at lines 2506 to 2509.

Whether the last CNID reenumeration of dir is still current.

The file ID resolver registers every entry of a directory with the CNID database and then sets DIRF_CNID. That work stays current while the directory ctime matches.

Parameters:
* `dir`: directory with a cached offspring count
* `st`: fresh stat of the directory

Returns: nonzero when the ctime matches and dir carries DIRF_CNID, 0 otherwise

Called by: [reenumerate_id](file.c.md#reenumerate_id)

### getdirparams

```c
int getdirparams(const AFPObj *obj, const struct vol *vol, uint16_t bitmap, struct path *s_path, struct dir *dir, char *buf, size_t *buflen)
```

Defined at lines 2534 to 2811.

Pack the directory parameters that bitmap selects into buf.

Callers pass "." with curdir, or, from enumerate, the name of dir inside curdir. The parameters follow the bit order of the AFP directory bitmap. Attributes, dates, and Finder info come from the AppleDouble metadata when it opens. Dot files can get the invisible flag, and the volume root can get the custom icon flag of the virtual icon.

The offspring count reuses the cached value while the ctime matches and scans the directory otherwise. Name fields carry offsets, and the names follow the fixed fields. A request with DIRPBIT_PDINFO before AFP 3.0 comes from a ProDOS client and gets dates in local time.

Parameters:
* `obj`: session object
* `vol`: volume of the directory
* `bitmap`: AFP directory bitmap, DIRPBIT_* bits
* `s_path`: path of the directory, with its stat
* `dir`: dircache entry of the directory
* `buf`: reply buffer receiving the parameters
* `buflen`: receives the bytes written

Returns: AFP_OK, or AFPERR_BITMAP for an unknown bit

Calls: [accessmode](unix.c.md#accessmode), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [dir_stat_refresh](directory.c.md#dir_stat_refresh), [invisible_dots](directory.c.md#invisible_dots), [offcnt_serve](directory.c.md#offcnt_serve), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [set_name](file.c.md#set_name), [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset), [set_vol_macname](volume.c.md#set_vol_macname), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Called by: [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [enumerate](enumerate.c.md#enumerate), [rslt_add](catsearch.c.md#rslt_add)

### path_error

```c
int path_error(struct path *path, int error)
```

Defined at lines 2825 to 2840.

AFP error for a path that cname could not resolve to the expected directory.

A directory yields afp_errno, the reason cname could not enter it. A path whose stat failed yields error. A path that names a file yields AFPERR_BADTYPE.

Parameters:
* `path`: path result from cname
* `error`: AFP error for a missing object

Returns: afp_errno, error, or AFPERR_BADTYPE

Calls: [path_isadir](../../include/atalk/directory.h.md#path_isadir)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_opendir](directory.c.md#afp_opendir), [afp_setdirparams](directory.c.md#afp_setdirparams), [enumerate](enumerate.c.md#enumerate)

Uses file-scope variables: `afp_errno`

### afp_setdirparams

```c
int afp_setdirparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2859 to 2917.

FPSetDirParms: set the parameters of a directory.

A path that cname could not enter fails with the path_error result. Only AFPERR_ACCESS lets the call proceed, so a client can restore the permissions of a directory it cannot enter. The parameters start at the next even offset of the request. The setdirparams call applies them, and success updates the volume modification time.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_VLOCK for a read-only volume, AFPERR_PARAM, or another AFPERR_* code

Calls: [cname](directory.c.md#cname), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [path_error](directory.c.md#path_error), [setdirparams](directory.c.md#setdirparams), [setvoltime](volume.c.md#setvoltime)

Uses file-scope variables: `afp_errno`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### setdirparams

```c
int setdirparams(struct vol *vol, struct path *path, uint16_t d_bitmap, char *buf)
```

Defined at lines 2931 to 3380.

Set a directory's parameters from an FPSetDirParms bitmap and buffer.

The directory arrives in canonical form: path->u_name is empty and path->d_dir names it.

Parameters:
* `vol`: volume the directory belongs to
* `path`: directory in canonical form
* `d_bitmap`: FPSetDirParms bitmap of the parameters present in buf
* `buf`: parameter values, in bitmap order

Returns: AFP_OK, or the AFPERR_* code of the parameter that failed

Calls: [accessmode](unix.c.md#accessmode), [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setattr](../../libatalk/adouble/ad_attr.c.md#ad_setattr), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [dir_modify](directory.c.md#dir_modify), [dir_rx_set](../../libatalk/vfs/unix.c.md#dir_rx_set), [dirlookup](directory.c.md#dirlookup), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [movecwd](directory.c.md#movecwd), [mtoumode](unix.c.md#mtoumode), [ostat](../../libatalk/util/unix.c.md#ostat), [set_dir_errors](directory.c.md#set_dir_errors), [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset), [setdeskmode](desktop.c.md#setdeskmode), [setdeskowner](desktop.c.md#setdeskowner), [setdirowner](unix.c.md#setdirowner), [setdirunixmode](unix.c.md#setdirunixmode), [setvoltime](volume.c.md#setvoltime)

Called by: [afp_setdirparams](directory.c.md#afp_setdirparams), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [setfilparams](file.c.md#setfilparams)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `Cur_Path`, `curdir`

### afp_syncdir

```c
int afp_syncdir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3398 to 3496.

FPSyncDir: flush a directory and its AppleDouble metadata to stable storage.

The volume root, DID 2, triggers a full sync, because clients send the command to flush an entire volume. Any other directory gets an fsync of itself where dirfd exists, and of the metadata file that vol->ad_path names for it. A missing metadata file fails with AFPERR_NOOBJ.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_NOOBJ, AFPERR_ACCESS, or AFPERR_PARAM

Calls: [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [movecwd](directory.c.md#movecwd)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

Uses file-scope variables: `afp_errno`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_createdir

```c
int afp_createdir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3517 to 3623.

FPCreateDir: create a directory and return its DID.

The request names the new directory relative to a parent DID. The call creates it with netatalk_mkdir, adds it to the dircache, and writes its AppleDouble metadata with the Mac name and the CNID. FCE and Spotlight learn about the new directory, and sibling afpd processes get a refresh hint for the parent.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, receives the new DID
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_EXIST, AFPERR_VLOCK for a read-only volume, AFPERR_ACCESS when the metadata cannot open, or another AFPERR_* code

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cname](directory.c.md#cname), [dir_add](directory.c.md#dir_add), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [movecwd](directory.c.md#movecwd), [netatalk_mkdir](directory.c.md#netatalk_mkdir), [of_stat](ofork.c.md#of_stat), [setvoltime_entry](volume.c.md#setvoltime_entry), [sl_index_event](spotlight.c.md#sl_index_event), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno`, `curdir`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### renamedir

```c
int renamedir(struct vol *vol, int dirfd, char *src, char *dst, struct dir *newparent, char *newname)
```

Defined at lines 3646 to 3705.

Rename a directory, or transfer its tree when the rename crosses devices.

The caller validates the source and cleans the cache. EXDEV makes the call duplicate the tree with copydir and delete the source with deletedir. The VFS modules then follow the rename, and the AppleDouble metadata gets the new Mac name.

Parameters:
* `vol`: volume of the directory
* `dirfd`: directory that src is relative to, or -1 for the working directory
* `src`: current Unix name, not a path
* `dst`: new Unix name, not a path, relative to the working directory
* `newparent`: destination parent directory, the working directory
* `newname`: new Mac name

Returns: AFP_OK, AFPERR_NOOBJ, AFPERR_ACCESS, AFPERR_VLOCK, AFPERR_CANTMOVE for a move into a subdirectory of itself, AFPERR_PARAM, or the error of copydir or deletedir

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [copydir](directory.c.md#copydir), [deletedir](directory.c.md#deletedir), [unix_rename](../../libatalk/vfs/unix.c.md#unix_rename)

Called by: [moveandrename](filedir.c.md#moveandrename)

Calls through [`vfs_ops::vfs_renamedir`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_renamedir_adouble](../../libatalk/vfs/vfs.c.md#rf_renamedir_adouble), [RF_renamedir_ea](../../libatalk/vfs/vfs.c.md#rf_renamedir_ea), [vfs_renamedir](../../libatalk/vfs/vfs.c.md#vfs_renamedir)

### dir_event_path

```c
const char * dir_event_path(char *buf, size_t buflen, const struct dir *parent, const char *name)
```

Defined at lines 3720 to 3739.

Full path of name inside parent, for FCE and Spotlight events.

The path comes from the cached full path of parent. Without one, or when the result would not fit buf, fullpathname builds the path from the working directory, so an event path never ends clipped.

Parameters:
* `buf`: buffer for the path
* `buflen`: size of buf
* `parent`: directory that holds name, or NULL
* `name`: entry name

Returns: buf, or the static buffer of fullpathname

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [moveandrename](filedir.c.md#moveandrename)

### deletecurdir

```c
int deletecurdir(struct vol *vol)
```

Defined at lines 3756 to 3836.

Delete the working directory, which must be empty.

The AppleDouble NODELETE attribute blocks the delete unless the volume ignores it. The VFS modules first delete their metadata, and then the call moves up to the parent and deletes the directory. A directory that holds only vetoed files goes through delete_vetoed_files, and one that is already gone counts as deleted. The call then deletes the CNID, drops the dircache entries of the directory and its children, and tells the sibling afpd processes.

Parameters:
* `vol`: volume of the working directory

Returns: AFP_OK, AFPERR_OLOCK for a NODELETE directory, AFPERR_ACCESS, AFPERR_DIRNEMPT, or another AFP error

Calls: [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [delete_vetoed_files](filedir.c.md#delete_vetoed_files), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dirlookup](directory.c.md#dirlookup), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [movecwd](directory.c.md#movecwd), [netatalk_rmdir_all_errors](../../libatalk/vfs/unix.c.md#netatalk_rmdir_all_errors)

Called by: [afp_delete](filedir.c.md#afp_delete)

Calls through [`vfs_ops::vfs_deletecurdir`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletecurdir_adouble](../../libatalk/vfs/vfs.c.md#rf_deletecurdir_adouble), [RF_deletecurdir_ea](../../libatalk/vfs/vfs.c.md#rf_deletecurdir_ea), [vfs_deletecurdir](../../libatalk/vfs/vfs.c.md#vfs_deletecurdir)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `afp_errno`, `curdir`

### afp_mapid

```c
int afp_mapid(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3856 to 3998.

FPMapID: map a user ID, group ID, or UUID to a name.

Subfunctions 1 and 2 map a user or group ID to a name in the Mac charset. Subfunctions 3 and 4 map them to a UTF-8 name and need AFP 3.0. ID 0 maps to an empty name. Subfunctions 5 and 6 map a UUID to a user or group name and need AFP 3.2 and the OPTION_UUID server option. Their reply carries the UUID type and the Unix ID ahead of the name.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, receives the name
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_NOITEM for an unknown ID, AFPERR_PARAM, or AFPERR_MISC

Calls: [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [getnamefromuuid](../../libatalk/acl/uuid.c.md#getnamefromuuid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_mapname

```c
int afp_mapname(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 4016 to 4123.

FPMapName: map a user or group name to a Unix ID or a UUID.

Subfunctions 1 and 2 take a UTF-8 user or group name and need AFP 3.0. Subfunctions 3 and 4 take a user or group name as a Pascal string. All four return the Unix ID. Subfunctions 5 and 6 return the UUID of a user or group and need AFP 3.2 and the OPTION_UUID server option.

Parameters:
* `obj`: session object
* `ibuf`: request block, receives a NUL after the name
* `ibuflen`: request block length
* `rbuf`: reply block, receives the ID or UUID
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_NOITEM for an unknown name, or AFPERR_PARAM

Calls: [getuuidfromname](../../libatalk/acl/uuid.c.md#getuuidfromname)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_closedir

```c
int afp_closedir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 4138 to 4152.

FPCloseDir: accept the close of a variable DID directory.

DIDs stay fixed for the life of the process, so the call only looks up the volume.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, or AFPERR_PARAM for an unknown volume

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_opendir

```c
int afp_opendir(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 4171 to 4214.

FPOpenDir: return the DID of a directory.

DIDs never change, so opening only resolves the path. The cname call creates the dircache entry and its DID on the way, and the reply carries the DID of the directory that cname entered. A path that names a file or a missing object fails.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, receives the DID
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_NOOBJ, AFPERR_PARAM, or another AFPERR_* code from the path lookup

Calls: [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_stat](ofork.c.md#of_stat), [path_error](directory.c.md#path_error)

Uses file-scope variables: `afp_errno`, `curdir`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Macros

* Undocumented: `DIRLOOKUP_LOG_FMT`

# File-scope variables

`Cur_Path`, `afp_errno`, `curdir`, `invalid_dircache_entries`, `offcnt_stat`, `rootParent`
