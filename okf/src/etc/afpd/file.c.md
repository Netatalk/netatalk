---
type: C Source File
title: "etc/afpd/file.c"
description: "28 functions, 1 type, includes 23 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/file.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-09T22:45:51+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/afp_util.h](../../include/atalk/afp_util.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/spotlight.h](../../include/atalk/spotlight.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [filedir.h](filedir.h.md)
* [fork.h](fork.h.md)
* [unix.h](unix.h.md)
* [volume.h](volume.h.md)
* System headers: `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `utime.h`

# Functions

### path_resolve_cached_file

```c
static struct dir * path_resolve_cached_file(const struct vol *vol, const struct dir *dir, struct path *path)
```

Defined at lines 83 to 96.

Calls: [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [path_cached_file](../../include/atalk/directory.h.md#path_cached_file), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [getfilparams](file.c.md#getfilparams), [getmetadata](file.c.md#getmetadata)

### has_dotdot_component

```c
static int has_dotdot_component(const char *path)
```

Defined at lines 98 to 119.

Called by: [symlink_target_safe](file.c.md#symlink_target_safe)

### path_is_inside_volume

```c
static int path_is_inside_volume(const struct vol *vol, const char *path)
```

Defined at lines 121 to 134.

Called by: [symlink_target_safe](file.c.md#symlink_target_safe)

### symlink_target_safe

```c
static int symlink_target_safe(const struct vol *vol, const char *link_path, const char *target)
```

Defined at lines 136 to 186.

Calls: [has_dotdot_component](file.c.md#has_dotdot_component), [path_is_inside_volume](file.c.md#path_is_inside_volume), [realpath_safe](../../libatalk/util/unix.c.md#realpath_safe), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [setfilparams](file.c.md#setfilparams)

### check_delete_inhibit

```c
static int check_delete_inhibit(const struct vol *vol, int dirfd, char *file)
```

Defined at lines 204 to 241.

Check whether AFP DeleteInhibit forbids a destructive operation.

Read the in-core header when a fork with loaded metadata is held; otherwise use a private [ad_metadata()](../../libatalk/adouble/ad_open.c.md#ad_metadata). Never ad_open(HF) onto of->of_ad: with v2 the sidecar shares the resource-fork inode and with EA the metadata fd is the data fd, so a transient metadata open and close could strand a held lock. Metadata read errors fail closed.

Parameters:
* `vol`: volume containing the file
* `dirfd`: parent directory fd, or -1 for the current directory
* `file`: file name to check

Returns: AFP_OK when replacement is allowed, AFPERR_OLOCK when DeleteInhibit is set, or AFPERR_ACCESS when the metadata cannot be read safely

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_metadataat](../../libatalk/adouble/ad_open.c.md#ad_metadataat), [of_findname](ofork.c.md#of_findname), [of_findnameat](ofork.c.md#of_findnameat)

Called by: [deletefile](file.c.md#deletefile), [setfilparams](file.c.md#setfilparams)

### replace_with_symlink

```c
static int replace_with_symlink(const char *path, const char *target)
```

Defined at lines 256 to 292.

Atomically replace a file with a symbolic link.

Stage the symlink beside its destination and rename it over the file. symlink() supplies O_EXCL-like collision handling, so an existing temporary name is never overwritten. If staging or rename fails, the original file remains in place and the temporary symlink is removed when possible.

Parameters:
* `path`: path of the file to replace
* `target`: symbolic link target

Returns: 0 on success, -1 on error with errno set

Calls: [netatalk_unlink](../../libatalk/vfs/unix.c.md#netatalk_unlink)

Called by: [setfilparams](file.c.md#setfilparams)

### default_type

```c
static int default_type(void *finder)
```

Defined at lines 296 to 303.

Called by: [get_finderinfo](file.c.md#get_finderinfo), [setfilparams](file.c.md#setfilparams)

Uses file-scope variables: `old_ufinderi`, `ufinderi`

### get_finderinfo

```c
void * get_finderinfo(const struct vol *vol, const char *upath, struct adouble *adp, void *data, int islink)
```

Defined at lines 306 to 369.

Calls: [default_type](file.c.md#default_type), [getextmap](../../libatalk/util/netatalk_conf.c.md#getextmap)

Called by: [getmetadata](file.c.md#getmetadata), [unpack_finderinfo](catsearch.c.md#unpack_finderinfo)

Uses file-scope variables: `ufinderi`

### set_name

```c
char * set_name(const struct vol *vol, char *data, cnid_t pid, char *name, cnid_t id, bool utf8)
```

Defined at lines 373 to 430.

Calls: [mtoupath](desktop.c.md#mtoupath), [utompath](desktop.c.md#utompath)

Called by: [getdirparams](directory.c.md#getdirparams), [getmetadata](file.c.md#getmetadata), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

### get_id

```c
uint32_t get_id(struct vol *vol, struct adouble *adp, const struct stat *st, const cnid_t did, const char *upath, const int len)
```

Defined at lines 460 to 551.

Get CNID for did/upath args both from database and adouble file.

1. Get the objects CNID as stored in its adouble file
1. Get the objects CNID from the database
1. Store resource fork data

Parameters:
* `vol`: volume
* `adp`: adouble struct of object upath, might be NULL
* `st`: stat of upath, must NOT be NULL
* `did`: parent CNID of upath
* `upath`: name of object
* `len`: strlen of upath

Calls: [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getid](../../libatalk/adouble/ad_attr.c.md#ad_getid), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_add](../../libatalk/cnid/cnid.c.md#cnid_add), [cnid_volume_reset](volume.c.md#cnid_volume_reset), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath)

Called by: [afp_createfile](file.c.md#afp_createfile), [afp_createid](file.c.md#afp_createid), [afp_openfork](fork.c.md#afp_openfork), [copyfile](file.c.md#copyfile), [crit_check](catsearch.c.md#crit_check), [dir_add](directory.c.md#dir_add), [getmetadata](file.c.md#getmetadata), [setfilparams](file.c.md#setfilparams)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Mentioned in the documentation of: [cnid_sqlite_set_errno](../../libatalk/cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_set_errno)

### getmetadata

```c
int getmetadata(const AFPObj *obj, struct vol *vol, uint16_t bitmap, struct path *path, struct dir *dir, char *buf, size_t *buflen, struct adouble *adp)
```

Defined at lines 554 to 1069.

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [accessmode](unix.c.md#accessmode), [ad_forcegetid](../../libatalk/adouble/ad_attr.c.md#ad_forcegetid), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_getid](../../libatalk/adouble/ad_attr.c.md#ad_getid), [ad_reso_size](../../libatalk/adouble/ad_open.c.md#ad_reso_size), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_free](directory.c.md#dir_free), [dir_new](directory.c.md#dir_new), [dir_remove](directory.c.md#dir_remove), [dircache_add](dircache.c.md#dircache_add), [fullpath_join](directory.c.md#fullpath_join), [get_finderinfo](file.c.md#get_finderinfo), [get_id](file.c.md#get_id), [ostat](../../libatalk/util/unix.c.md#ostat), [path_cached_file](../../include/atalk/directory.h.md#path_cached_file), [path_resolve_cached_file](file.c.md#path_resolve_cached_file), [set_name](file.c.md#set_name), [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [utompath](desktop.c.md#utompath)

Called by: [getfilparams](file.c.md#getfilparams), [getforkparams](fork.c.md#getforkparams)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### getfilparams

```c
int getfilparams(const AFPObj *obj, struct vol *vol, uint16_t bitmap, struct path *path, struct dir *dir, char *buf, size_t *buflen, int skip_fork_check)
```

Defined at lines 1072 to 1139.

Calls: [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_loaded](../../include/atalk/adouble.h.md#ad_meta_loaded), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [getmetadata](file.c.md#getmetadata), [of_ad](ofork.c.md#of_ad), [path_resolve_cached_file](file.c.md#path_resolve_cached_file)

Called by: [afp_getappl](appl.c.md#afp_getappl), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_resolveid](file.c.md#afp_resolveid), [enumerate](enumerate.c.md#enumerate), [rslt_add](catsearch.c.md#rslt_add)

### afp_createfile

```c
int afp_createfile(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1142 to 1335.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cname](directory.c.md#cname), [dir_event_path](directory.c.md#dir_event_path), [dir_free](directory.c.md#dir_free), [dir_new](directory.c.md#dir_new), [dircache_add](dircache.c.md#dircache_add), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [fullpath_join](directory.c.md#fullpath_join), [get_afp_errno](directory.c.md#get_afp_errno), [get_id](file.c.md#get_id), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [netatalk_unlink](../../libatalk/vfs/unix.c.md#netatalk_unlink), [of_findname](ofork.c.md#of_findname), [setvoltime](volume.c.md#setvoltime), [sl_index_event](spotlight.c.md#sl_index_event), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_setfilparams

```c
int afp_setfilparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1337 to 1412.

Calls: [cname](directory.c.md#cname), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [setfilparams](file.c.md#setfilparams), [setvoltime](volume.c.md#setvoltime), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### setfilparams

```c
int setfilparams(const AFPObj *obj, struct vol *vol, struct path *path, uint16_t f_bitmap, char *buf)
```

Defined at lines 1420 to 1874.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setattr](../../libatalk/adouble/ad_attr.c.md#ad_setattr), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [check_access](directory.c.md#check_access), [check_delete_inhibit](file.c.md#check_delete_inhibit), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [default_type](file.c.md#default_type), [dir_modify](directory.c.md#dir_modify), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [get_id](file.c.md#get_id), [getdefextmap](../../libatalk/util/netatalk_conf.c.md#getdefextmap), [getextmap](../../libatalk/util/netatalk_conf.c.md#getextmap), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_ad](ofork.c.md#of_ad), [of_stat](ofork.c.md#of_stat), [ostat](../../libatalk/util/unix.c.md#ostat), [replace_with_symlink](file.c.md#replace_with_symlink), [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset), [setdirparams](directory.c.md#setdirparams), [setfilowner](unix.c.md#setfilowner), [setfilunixmode](unix.c.md#setfilunixmode), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [symlink_target_safe](file.c.md#symlink_target_safe)

Called by: [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams)

Uses file-scope variables: `Cur_Path` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `ufinderi`

### renamefile

```c
int renamefile(struct vol *vol, struct dir *ddir, int sdir_fd, char *src, char *dst, char *newname, struct adouble *adp)
```

Defined at lines 1892 to 1972.

Rename a file, including its resource fork and mac name.

renamefile and copyfile take the old and new unix pathnames and the new mac name.

Parameters:
* `vol`: volume structure
* `ddir`: dest dir structure
* `sdir_fd`: source dir fd to which src path is relative (for openat et al semantics) passing -1 means this is not used, src path is a full path
* `src`: the source path
* `dst`: the dest filename in current dir
* `newname`: the dest mac name
* `adp`: adouble struct of src file, if open, or & zeroed one

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [copyfile](file.c.md#copyfile), [deletefile](file.c.md#deletefile), [unix_rename](../../libatalk/vfs/unix.c.md#unix_rename)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [moveandrename](filedir.c.md#moveandrename)

Calls through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_renamefile_adouble](../../libatalk/vfs/vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](../../libatalk/vfs/vfs.c.md#rf_renamefile_ea), [ea_renamefile](../../libatalk/vfs/ea_ad.c.md#ea_renamefile), [vfs_renamefile](../../libatalk/vfs/vfs.c.md#vfs_renamefile)

### mtoUTF8

```c
size_t mtoUTF8(const struct vol *vol, const char *src, size_t srclen, char *dest, size_t destlen)
```

Defined at lines 1977 to 1988.

convert a Mac long name to an utf8 name

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string)

Called by: [cname_mtouname](directory.c.md#cname_mtouname), [copy_path_name](file.c.md#copy_path_name)

### copy_path_name

```c
int copy_path_name(const struct vol *vol, char *newname, char *ibuf)
```

Defined at lines 1991 to 2052.

Calls: [mtoUTF8](file.c.md#mtoutf8)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_rename](filedir.c.md#afp_rename)

### afp_copyfile

```c
int afp_copyfile(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2056 to 2241.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_testlock](../../libatalk/adouble/ad_lock.c.md#ad_testlock), [cname](directory.c.md#cname), [copy_path_name](file.c.md#copy_path_name), [copyfile](file.c.md#copyfile), [ctoupath](filedir.c.md#ctoupath), [dir_event_path](directory.c.md#dir_event_path), [dirlookup](directory.c.md#dirlookup), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [mtoupath](desktop.c.md#mtoupath), [of_ad](ofork.c.md#of_ad), [of_findname](ofork.c.md#of_findname), [path_error](directory.c.md#path_error), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [setvoltime](volume.c.md#setvoltime), [sl_index_event](spotlight.c.md#sl_index_event), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### copyfile

```c
int copyfile(struct vol *s_vol, struct vol *d_vol, struct dir *d_dir, int sfd, char *src, char *dst, char *newname, struct adouble *adp, int held)
```

Defined at lines 2248 to 2436.

Note: if newname is NULL (from [directory.c](directory.c.md)) we don't want to copy the resource fork. because we are doing it elsewhere. currently if newname is NULL then adp is NULL.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_copy_header](../../libatalk/adouble/ad_flush.c.md#ad_copy_header), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_openat](../../libatalk/adouble/ad_open.c.md#ad_openat), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [copy_fork](../../libatalk/adouble/ad_write.c.md#copy_fork), [deletefile](file.c.md#deletefile), [get_id](file.c.md#get_id)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [copydir](directory.c.md#copydir), [renamefile](file.c.md#renamefile)

Calls through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_copyfile_adouble](../../libatalk/vfs/vfs.c.md#rf_copyfile_adouble), [RF_copyfile_ea](../../libatalk/vfs/vfs.c.md#rf_copyfile_ea), [ea_copyfile](../../libatalk/vfs/ea_ad.c.md#ea_copyfile), [sys_ea_copyfile](../../libatalk/vfs/ea_sys.c.md#sys_ea_copyfile), [vfs_copyfile](../../libatalk/vfs/vfs.c.md#vfs_copyfile)

### deletefile

```c
int deletefile(const struct vol *vol, int dirfd, char *file, int checkAttrib, cnid_t *idp)
```

Defined at lines 2458 to 2567.

Note: dirfd can be used for unlinkat semantics

Calls: [check_delete_inhibit](file.c.md#check_delete_inhibit), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [netatalk_unlinkat](../../libatalk/vfs/unix.c.md#netatalk_unlinkat), [of_get_locks](ofork.c.md#of_get_locks), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_delete](filedir.c.md#afp_delete), [copyfile](file.c.md#copyfile), [renamefile](file.c.md#renamefile)

Calls through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletefile_adouble](../../libatalk/vfs/vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](../../libatalk/vfs/vfs.c.md#rf_deletefile_ea), [ea_deletefile](../../libatalk/vfs/ea_ad.c.md#ea_deletefile), [vfs_deletefile](../../libatalk/vfs/vfs.c.md#vfs_deletefile)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

Mentioned in the documentation of: [of_delete_blocked](ofork.c.md#of_delete_blocked)

### afp_createid

```c
int afp_createid(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2570 to 2650.

Returns: a file id

Calls: [cname](directory.c.md#cname), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [get_id](file.c.md#get_id), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### reenumerate_loop

```c
static int reenumerate_loop(struct dirent *de, char *mname, void *data)
```

Defined at lines 2658 to 2686.

Calls: [cnid_add](../../libatalk/cnid/cnid.c.md#cnid_add), [cnid_volume_reset](volume.c.md#cnid_volume_reset), [ostat](../../libatalk/util/unix.c.md#ostat)

Called by: [reenumerate_id](file.c.md#reenumerate_id)

### reenumerate_id

```c
static int reenumerate_id(struct vol *vol, char *name, struct dir *dir)
```

Defined at lines 2693 to 2722.

Calls: [dirreenumerate](directory.c.md#dirreenumerate), [for_each_dirent](enumerate.c.md#for_each_dirent), [ostat](../../libatalk/util/unix.c.md#ostat), [reenumerate_loop](file.c.md#reenumerate_loop), [setdiroffcnt](directory.c.md#setdiroffcnt)

Called by: [afp_resolveid](file.c.md#afp_resolveid)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_resolveid

```c
int afp_resolveid(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2726 to 2867.

resolve a file id

Calls: [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [dirlookup](directory.c.md#dirlookup), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [movecwd](directory.c.md#movecwd), [of_stat](ofork.c.md#of_stat), [reenumerate_id](file.c.md#reenumerate_id), [utompath](desktop.c.md#utompath)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_deleteid

```c
int afp_deleteid(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2870 to 2986.

Calls: [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [movecwd](directory.c.md#movecwd), [ostat](../../libatalk/util/unix.c.md#ostat)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### find_adouble

```c
static struct adouble * find_adouble(const AFPObj *obj, struct vol *vol, struct path *path, struct ofork **of, struct adouble *adp)
```

Defined at lines 2989 to 3040.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [file_access](directory.c.md#file_access), [of_findname](ofork.c.md#of_findname)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### afp_exchangefiles

```c
int afp_exchangefiles(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3044 to 3497.

Calls: [absupath](filedir.c.md#absupath), [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_copy_header](../../libatalk/adouble/ad_flush.c.md#ad_copy_header), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_init_offsets](../../libatalk/adouble/ad_open.c.md#ad_init_offsets), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [become_root](../../libatalk/util/unix.c.md#become_root), [cname](directory.c.md#cname), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [find_adouble](file.c.md#find_adouble), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_rename](ofork.c.md#of_rename), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [renamefile](file.c.md#renamefile), [setfilowner](unix.c.md#setfilowner), [setfilunixmode](unix.c.md#setfilunixmode), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Types

### struct reenum

Defined at line 2653.
* `struct vol * vol`
* `cnid_t did`

# Macros

* Undocumented: `APPLETEMP`, `PARAM_NEED_ADP`

# File-scope variables

`old_ufinderi`, `ufinderi`
