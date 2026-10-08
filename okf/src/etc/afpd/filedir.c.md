---
type: C Source File
title: "etc/afpd/filedir.c"
description: "13 functions, includes 24 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/filedir.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/acl.h](../../include/atalk/acl.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
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
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [filedir.h](filedir.h.md)
* [fork.h](fork.h.md)
* [unix.h](unix.h.md)
* [virtual_icon.h](virtual_icon.h.md)
* [volume.h](volume.h.md)
* System headers: `bstrlib.h`, `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### virtual_icon_reply

```c
static int virtual_icon_reply(const AFPObj *obj, struct vol *vol, uint16_t fbitmap, uint16_t dbitmap, char *rbuf, size_t *rbuflen)
```

Defined at lines 47 to 70.

Calls: [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Called by: [afp_getfildirparams](filedir.c.md#afp_getfildirparams)

### afp_getfildirparams

```c
int afp_getfildirparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 72 to 195.

Calls: [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_statdir](ofork.c.md#of_statdir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled), [virtual_icon_reply](filedir.c.md#virtual_icon_reply)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_setfildirparams

```c
int afp_setfildirparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 197 to 275.

Calls: [cname](directory.c.md#cname), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_statdir](ofork.c.md#of_statdir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setdirparams](directory.c.md#setdirparams), [setfilparams](file.c.md#setfilparams), [setvoltime](volume.c.md#setvoltime), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### check_name

```c
int check_name(const struct vol *vol, char *name)
```

Defined at lines 280 to 293.

Factorise some checks on a pathname

Calls: [veto_file](filedir.c.md#veto_file)

Called by: [cname](directory.c.md#cname), [moveandrename](filedir.c.md#moveandrename)

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](../../libatalk/vfs/vfs.c.md#validupath_adouble), [validupath_ea](../../libatalk/vfs/vfs.c.md#validupath_ea), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath)

### moveandrename

```c
static int moveandrename(const AFPObj *obj, struct vol *vol, struct dir *sdir, int sdir_fd, char *oldname, char *newname, int isdir)
```

Defined at lines 299 to 628.

move and rename sdir:oldname to curdir:newname in volume vol

Note: special care is needed for lock

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [check_name](filedir.c.md#check_name), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [ctoupath](filedir.c.md#ctoupath), [dir_event_path](directory.c.md#dir_event_path), [dir_modify](directory.c.md#dir_modify), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [mtoupath](desktop.c.md#mtoupath), [of_findname](ofork.c.md#of_findname), [of_findnameat](ofork.c.md#of_findnameat), [of_rename](ofork.c.md#of_rename), [renamedir](directory.c.md#renamedir), [renamefile](file.c.md#renamefile), [sl_index_event](spotlight.c.md#sl_index_event), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_rename](filedir.c.md#afp_rename)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_rename

```c
int afp_rename(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 631 to 721.

Calls: [cname](directory.c.md#cname), [copy_path_name](file.c.md#copy_path_name), [dirlookup](directory.c.md#dirlookup), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [moveandrename](filedir.c.md#moveandrename), [movecwd](directory.c.md#movecwd), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setvoltime](volume.c.md#setvoltime), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### delete_vetoed_files_at

```c
static int delete_vetoed_files_at(struct vol *vol, int parent_fd, const char *name, bool in_vetodir)
```

Defined at lines 737 to 832.

Recursively delete vetoed files and directories.

Recursively scans the directory for vetoed files and tries to delete them, then tries to delete the directory. That may fail if the directory contains normal files that aren't vetoed.

Parameters:
* `vol`: volume handle
* `parent_fd`: file descriptor of the parent directory
* `name`: directory name relative to parent_fd
* `in_vetodir`: true if we are already in a vetoed directory

Returns: 0 if the directory was deleted, otherwise a negative error code.

Calls: [netatalk_unlinkat](../../libatalk/vfs/unix.c.md#netatalk_unlinkat), [veto_file](filedir.c.md#veto_file)

Called by: [delete_vetoed_files](filedir.c.md#delete_vetoed_files)

### delete_vetoed_files

```c
int delete_vetoed_files(struct vol *vol, const char *upath, bool in_vetodir)
```

Defined at lines 834 to 852.

Calls: [delete_vetoed_files_at](filedir.c.md#delete_vetoed_files_at)

Called by: [afp_delete](filedir.c.md#afp_delete), [deletecurdir](directory.c.md#deletecurdir)

### afp_delete

```c
int afp_delete(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 855 to 1137.

Calls: [cname](directory.c.md#cname), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [delete_vetoed_files](filedir.c.md#delete_vetoed_files), [deletecurdir](directory.c.md#deletecurdir), [deletefile](file.c.md#deletefile), [dir_event_path](directory.c.md#dir_event_path), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_close_inode_forks](ofork.c.md#of_close_inode_forks), [of_delete_blocked](ofork.c.md#of_delete_blocked), [of_findname](ofork.c.md#of_findname), [of_statdir](ofork.c.md#of_statdir), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setvoltime](volume.c.md#setvoltime), [sl_index_event](spotlight.c.md#sl_index_event), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### absupath

```c
char * absupath(const struct vol *vol, struct dir *dir, char *u)
```

Defined at lines 1140 to 1162.

Calls: [fullpath_join](directory.c.md#fullpath_join)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [ctoupath](filedir.c.md#ctoupath)

### ctoupath

```c
char * ctoupath(const struct vol *vol, struct dir *dir, char *name)
```

Defined at lines 1164 to 1172.

Calls: [absupath](filedir.c.md#absupath), [mtoupath](desktop.c.md#mtoupath)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [moveandrename](filedir.c.md#moveandrename)

### afp_moveandrename

```c
int afp_moveandrename(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1175 to 1304.

Calls: [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [cname](directory.c.md#cname), [copy_path_name](file.c.md#copy_path_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [moveandrename](filedir.c.md#moveandrename), [mtoupath](desktop.c.md#mtoupath), [path_error](directory.c.md#path_error), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setfilmode](../../libatalk/vfs/unix.c.md#setfilmode), [setvoltime](volume.c.md#setvoltime), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Calls through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setfilmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](../../libatalk/vfs/vfs.c.md#rf_setfilmode_ea), [ea_chmod_file](../../libatalk/vfs/ea_ad.c.md#ea_chmod_file), [vfs_setfilmode](../../libatalk/vfs/vfs.c.md#vfs_setfilmode)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### veto_file

```c
int veto_file(const char *veto_str, const char *path)
```

Defined at lines 1312 to 1345.

given a veto_str like "abc/zxc/" and path "abc", return 1

Note: veto_str should be '/' delimited

Returns: if path matches any one of the veto_str elements exactly, then 1 is returned otherwise, 0 is returned.

Called by: [check_dirent](enumerate.c.md#check_dirent), [check_name](filedir.c.md#check_name), [delete_vetoed_files_at](filedir.c.md#delete_vetoed_files_at)
