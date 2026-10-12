---
type: C Source File
title: "etc/afpd/filedir.c"
description: "AFP commands for both files and directories: FPGetFileDirParms, FPSetFileDirParms, FPRename, FPDelete, and FPMoveAndRename."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/filedir.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 68 to 91.

Reply to FPGetFileDirParms for the virtual Icon file of the volume root.

The reply carries both bitmaps, the file flag, and the synthesized file parameters of virtual_icon_getfilparams.

Parameters:
* `obj`: session object
* `vol`: volume with the virtual icon
* `fbitmap`: AFP file bitmap
* `dbitmap`: AFP directory bitmap, echoed only
* `rbuf`: reply block
* `rbuflen`: receives the reply length

Returns: AFP_OK, or the error of virtual_icon_getfilparams

Calls: [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Called by: [afp_getfildirparams](filedir.c.md#afp_getfildirparams)

### afp_getfildirparams

```c
int afp_getfildirparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 111 to 234.

FPGetFileDirParms: return the parameters of a file or directory.

The reply carries the file and directory bitmaps, a flag that tells files from directories, and the parameters of getfilparams or getdirparams. The virtual Icon file of the volume root answers through virtual_icon_reply, by its CNID and by its name. A directory that the user cannot enter still reports its parameters.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_NOOBJ, AFPERR_PARAM for an unknown volume, or another AFPERR_* code

Calls: [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_statdir](ofork.c.md#of_statdir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled), [virtual_icon_reply](filedir.c.md#virtual_icon_reply)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_setfildirparams

```c
int afp_setfildirparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 253 to 331.

FPSetFileDirParms: set the parameters of a file or directory.

The call dispatches to setdirparams or setfilparams by the type of the object. The virtual Icon file of the volume root refuses changes with AFPERR_OLOCK. A directory that the user cannot enter still accepts changes. The parameters start at the next even offset of the request, and success updates the volume modification time.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_VLOCK for a read-only volume, AFPERR_OLOCK, AFPERR_NOOBJ, or another AFPERR_* code

Calls: [cname](directory.c.md#cname), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_statdir](ofork.c.md#of_statdir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setdirparams](directory.c.md#setdirparams), [setfilparams](file.c.md#setfilparams), [setvoltime](volume.c.md#setvoltime), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### check_name

```c
int check_name(const struct vol *vol, char *name)
```

Defined at lines 341 to 354.

Reject a name that the AppleDouble layout reserves or that the volume vetoes.

Parameters:
* `vol`: volume
* `name`: Unix name of a file or directory

Returns: 0 for an acceptable name, AFPERR_EXIST otherwise

Calls: [veto_file](filedir.c.md#veto_file)

Called by: [cname](directory.c.md#cname), [moveandrename](filedir.c.md#moveandrename)

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](../../libatalk/vfs/vfs.c.md#validupath_adouble), [validupath_ea](../../libatalk/vfs/vfs.c.md#validupath_ea), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath)

### moveandrename

```c
static int moveandrename(const AFPObj *obj, struct vol *vol, struct dir *sdir, int sdir_fd, char *oldname, char *newname, int isdir)
```

Defined at lines 383 to 712.

Move and rename sdir/oldname to newname in the working directory.

A file takes its CNID from the database, and a directory passes itself as sdir and supplies the DID. The rename-inhibit attribute blocks the call unless the volume ignores it, and check_name screens the new name. An unchanged name succeeds at once. Inside one directory a name that another file holds fails with AFPERR_EXIST, and across directories any existing destination fails. So does a destination that this session holds open.

After renamefile or renamedir succeeds, the dircache entry and its children follow. FCE and Spotlight get a move event. The AppleDouble metadata and the CNID record get the new parent and name, and sibling afpd processes get hints for the object and both parents.

Parameters:
* `obj`: session object
* `vol`: volume of both directories
* `sdir`: source directory, or the directory itself when isdir is nonzero
* `sdir_fd`: descriptor of the source directory, or -1 for the working directory
* `oldname`: current Mac name
* `newname`: new Mac name
* `isdir`: nonzero when the object is a directory

Returns: AFP_OK, AFPERR_OLOCK, AFPERR_EXIST, AFPERR_PARAM, AFPERR_MISC, or the error of renamefile or renamedir

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [check_name](filedir.c.md#check_name), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [ctoupath](filedir.c.md#ctoupath), [dir_event_path](directory.c.md#dir_event_path), [dir_modify](directory.c.md#dir_modify), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [mtoupath](desktop.c.md#mtoupath), [of_findname](ofork.c.md#of_findname), [of_findnameat](ofork.c.md#of_findnameat), [of_rename](ofork.c.md#of_rename), [renamedir](directory.c.md#renamedir), [renamefile](file.c.md#renamefile), [sl_index_event](spotlight.c.md#sl_index_event), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_rename](filedir.c.md#afp_rename)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_rename

```c
int afp_rename(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 730 to 820.

FPRename: rename a file or directory inside its parent directory.

The volume root fails with AFPERR_NORENAME, and the virtual Icon file of the volume root with AFPERR_OLOCK. An empty new name succeeds without work. The moveandrename call does the rest, and success updates the volume modification time.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_NORENAME, AFPERR_OLOCK, AFPERR_VLOCK, AFPERR_PARAM, or the error of moveandrename

Calls: [cname](directory.c.md#cname), [copy_path_name](file.c.md#copy_path_name), [dirlookup](directory.c.md#dirlookup), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [moveandrename](filedir.c.md#moveandrename), [movecwd](directory.c.md#movecwd), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setvoltime](volume.c.md#setvoltime), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### delete_vetoed_files_at

```c
static int delete_vetoed_files_at(struct vol *vol, int parent_fd, const char *name, bool in_vetodir)
```

Defined at lines 836 to 931.

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

Defined at lines 946 to 964.

Delete the vetoed files in directory upath and then the directory itself.

Only a volume with AFPVOL_DELVETO allows it. The work happens in delete_vetoed_files_at, relative to the working directory. A directory that still holds files outside the veto list survives.

Parameters:
* `vol`: volume of the directory
* `upath`: directory name, relative to the working directory
* `in_vetodir`: true when upath itself lies inside a vetoed directory

Returns: 0 when the directory went away, a negative value otherwise

Calls: [delete_vetoed_files_at](filedir.c.md#delete_vetoed_files_at)

Called by: [afp_delete](filedir.c.md#afp_delete), [deletecurdir](directory.c.md#deletecurdir)

### afp_delete

```c
int afp_delete(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 988 to 1270.

FPDelete: delete a file or an empty directory.

Deleting the virtual Icon file of the volume root fails with AFPERR_OLOCK, and the volume root itself fails with AFPERR_ACCESS. A directory that cname entered goes through deletecurdir. A directory that the user cannot enter goes through a direct rmdir, except on version 2 AppleDouble volumes. A file that this session holds open fails with AFPERR_BUSY. A file whose dircache inode no longer matches fails with AFPERR_NOOBJ.

On a volume with AFPVOL_DELVETO a directory that holds only vetoed files still goes away. Success sends FCE and Spotlight events and tells sibling afpd processes.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_BUSY, AFPERR_DIRNEMPT, AFPERR_ACCESS, AFPERR_OLOCK, AFPERR_NOOBJ, AFPERR_VLOCK, or another AFPERR_* code

Calls: [cname](directory.c.md#cname), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [delete_vetoed_files](filedir.c.md#delete_vetoed_files), [deletecurdir](directory.c.md#deletecurdir), [deletefile](file.c.md#deletefile), [dir_event_path](directory.c.md#dir_event_path), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_close_inode_forks](ofork.c.md#of_close_inode_forks), [of_delete_blocked](ofork.c.md#of_delete_blocked), [of_findname](ofork.c.md#of_findname), [of_statdir](ofork.c.md#of_statdir), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setvoltime](volume.c.md#setvoltime), [sl_index_event](spotlight.c.md#sl_index_event), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### absupath

```c
char * absupath(const struct vol *vol, struct dir *dir, char *u)
```

Defined at lines 1283 to 1305.

Full path of the Unix name u inside dir.

The result lives in a static buffer that the next call overwrites.

Parameters:
* `vol`: volume
* `dir`: directory that holds u
* `u`: Unix name

Returns: the full path, or NULL for a NULL argument or a path beyond MAXPATHLEN

Calls: [fullpath_join](directory.c.md#fullpath_join)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [ctoupath](filedir.c.md#ctoupath)

### ctoupath

```c
char * ctoupath(const struct vol *vol, struct dir *dir, char *name)
```

Defined at lines 1315 to 1323.

Full Unix path of a Mac name inside dir.

Parameters:
* `vol`: volume
* `dir`: directory that holds the name
* `name`: Mac name

Returns: the full path in the static buffer of absupath, or NULL on failure

Calls: [absupath](filedir.c.md#absupath), [mtoupath](desktop.c.md#mtoupath)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [moveandrename](filedir.c.md#moveandrename)

### afp_moveandrename

```c
int afp_moveandrename(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1343 to 1472.

FPMoveAndRename: move a file or directory into another directory, with a new name if given.

The request names the source by DID and path and the destination directory by DID and path, followed by the new name. An empty new name keeps the old one. The virtual Icon file of the volume root cannot move. Without UNIX privileges a moved file takes the mode that its new directory implies, plus the volume file permissions. Success updates the volume modification time.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_OLOCK, AFPERR_VLOCK, AFPERR_NOOBJ, AFPERR_PARAM, or the error of moveandrename

Calls: [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [cname](directory.c.md#cname), [copy_path_name](file.c.md#copy_path_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [moveandrename](filedir.c.md#moveandrename), [mtoupath](desktop.c.md#mtoupath), [path_error](directory.c.md#path_error), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setfilmode](../../libatalk/vfs/unix.c.md#setfilmode), [setvoltime](volume.c.md#setvoltime), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Calls through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_setfilmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](../../libatalk/vfs/vfs.c.md#rf_setfilmode_ea), [ea_chmod_file](../../libatalk/vfs/ea_ad.c.md#ea_chmod_file), [vfs_setfilmode](../../libatalk/vfs/vfs.c.md#vfs_setfilmode)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### veto_file

```c
int veto_file(const char *veto_str, const char *path)
```

Defined at lines 1484 to 1517.

Whether path matches one entry of a slash-separated veto list.

The list has the form "abc/zxc/", and an entry must equal path exactly, byte for byte.

Parameters:
* `veto_str`: veto list with a slash after every entry, or NULL
* `path`: name to test, or NULL

Returns: 1 for a match, 0 otherwise

Called by: [check_dirent](enumerate.c.md#check_dirent), [check_name](filedir.c.md#check_name), [delete_vetoed_files_at](filedir.c.md#delete_vetoed_files_at)
