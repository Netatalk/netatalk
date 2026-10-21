---
type: C Source File
title: "etc/afpd/extattrs.c"
description: "The AFP extended attribute commands: FPListExtAttrs, FPGetExtAttr, FPSetExtAttr, and FPRemoveExtAttr."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/extattrs.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T20:46:18+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/afp_util.h](../../include/atalk/afp_util.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [extattrs.h](extattrs.h.md)
* [file.h](file.h.md)
* [fork.h](fork.h.md)
* [volume.h](volume.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `unistd.h`

# Functions

### afp_listextattr

```c
int afp_listextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 76 to 287.

FPListExtAttrs: list the extended attribute names of a file or directory.

The client calls twice, first for the size of all names and then for the names themselves. The dircache entry of an object the cache serves keeps the names from the first read, so the second call and every later list of the object read nothing until the entry sees a change.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or an AFPERR_* code

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_loaded](../../include/atalk/adouble.h.md#ad_meta_loaded), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [ea_cache_entry](ad_cache.c.md#ea_cache_entry), [ea_cache_names](ad_cache.c.md#ea_cache_names), [ea_cache_store_names](ad_cache.c.md#ea_cache_store_names), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_findname](ofork.c.md#of_findname), [of_statdir](ofork.c.md#of_statdir), [path_entry](file.c.md#path_entry), [path_isadir](../../include/atalk/directory.h.md#path_isadir)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_list`](../../include/atalk/vfs.h.md#struct-vfs_ops): [list_eas](../../libatalk/vfs/ea_ad.c.md#list_eas), [sys_list_eas](../../libatalk/vfs/ea_sys.c.md#sys_list_eas), [vfs_ea_list](../../libatalk/vfs/vfs.c.md#vfs_ea_list)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `ea_finderinfo`, `ea_resourcefork`

### ea_uname

```c
static int ea_uname(const struct vol *vol, char **ibuf, const char *end, char *uname, size_t size)
```

Defined at lines 308 to 342.

Read the EA name field of a request and convert the name for the volume's EA backend.

The field is a 16-bit length and the name, after a pad byte that brings the cursor to an even address. Native EAs take the name in the volume's charset and AppleDouble EA files in UTF-8, the charsets each backend lists names from.

Parameters:
* `vol`: volume
* `ibuf`: cursor at the field, past it on AFP_OK
* `end`: end of the request
* `uname`: the converted name, terminated
* `size`: bytes available at uname

Returns: AFP_OK, or AFPERR_PARAM for a name over 255 bytes or past the request's end, and for one that does not convert, converts to nothing or holds a NUL byte

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string)

Called by: [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setextattr](extattrs.c.md#afp_setextattr)

Mentioned in the documentation of: [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setextattr](extattrs.c.md#afp_setextattr)

### afp_getextattr

```c
int afp_getextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 366 to 505.

FPGetExtAttr: return the size or the content of an extended attribute.

A MaxReplySize of 0 asks for the size only. The name arrives in UTF-8 and converts for the volume's EA backend. For an object the dircache serves, the whole value is read once and kept in the entry, a name the cached list lacks is missing without a read, and the reply is built here: the size word alone for a probe or for a value over MaxReplySize, which is AFPERR_PARAM, the value otherwise. An open fork of the file lends its metadata descriptor to the VFS call. kXAttrNoFollow keeps the call off symlink targets.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the VFS result, AFPERR_PARAM for an unknown volume or a name [ea_uname()](extattrs.c.md#ea_uname) refuses, AFPERR_MISC on a volume without EA support, AFPERR_NOOBJ for a path that names no object, or the error of the directory or path lookup

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [ea_absent](../../libatalk/vfs/ea_ad.c.md#ea_absent), [ea_cache_absent](ad_cache.c.md#ea_cache_absent), [ea_cache_entry](ad_cache.c.md#ea_cache_entry), [ea_cache_store_value](ad_cache.c.md#ea_cache_store_value), [ea_cache_value](ad_cache.c.md#ea_cache_value), [ea_uname](extattrs.c.md#ea_uname), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_findname](ofork.c.md#of_findname), [path_isadir](../../include/atalk/directory.h.md#path_isadir)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_getcontent`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_eacontent](../../libatalk/vfs/ea_ad.c.md#get_eacontent), [sys_get_eacontent](../../libatalk/vfs/ea_sys.c.md#sys_get_eacontent), [vfs_ea_getcontent](../../libatalk/vfs/vfs.c.md#vfs_ea_getcontent)

Calls through [`vfs_ops::vfs_ea_getsize`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_easize](../../libatalk/vfs/ea_ad.c.md#get_easize), [sys_get_easize](../../libatalk/vfs/ea_sys.c.md#sys_get_easize), [vfs_ea_getsize](../../libatalk/vfs/vfs.c.md#vfs_ea_getsize)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### ea_written

```c
static void ea_written(AFPObj *obj, struct vol *vol, struct path *s_path, int ret)
```

Defined at lines 519 to 553.

Settle the volume and the dircache after an EA write: the entry's cached names and values go whatever the result, since ea=ad rewrites its container header on a failed write too; a successful write stamps the volume's modification time, refreshes the entry's stat and hints the siblings.

Parameters:
* `obj`: session
* `vol`: volume of the object
* `s_path`: the request's path as [cname()](directory.c.md#cname) left it
* `ret`: the backend's result

Calls: [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_modify](directory.c.md#dir_modify), [ea_cache_free](ad_cache.c.md#ea_cache_free), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [ostat](../../libatalk/util/unix.c.md#ostat), [path_entry](file.c.md#path_entry), [setvoltime](volume.c.md#setvoltime), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setextattr](extattrs.c.md#afp_setextattr)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_setextattr

```c
int afp_setextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 574 to 680.

FPSetExtAttr: create or replace an extended attribute.

kXAttrCreate and kXAttrReplace map to O_CREAT and O_TRUNC for the VFS call. A value over MAX_EA_SIZE or past the request's end gets AFPERR_PARAM. The dircache entry drops its cached EA names and values whatever the write's result; after a successful write the entry takes the new stat and the other sessions get a refresh hint.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the VFS result, AFPERR_PARAM, also for a name [ea_uname()](extattrs.c.md#ea_uname) refuses, AFPERR_VLOCK on a read-only volume, AFPERR_MISC on a volume without EA support, AFPERR_NOOBJ for a path that names no object, or the error of the directory or path lookup

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [ea_uname](extattrs.c.md#ea_uname), [ea_written](extattrs.c.md#ea_written), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_findname](ofork.c.md#of_findname), [path_isadir](../../include/atalk/directory.h.md#path_isadir)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_set`](../../include/atalk/vfs.h.md#struct-vfs_ops): [set_ea](../../libatalk/vfs/ea_ad.c.md#set_ea), [sys_set_ea](../../libatalk/vfs/ea_sys.c.md#sys_set_ea), [vfs_ea_set](../../libatalk/vfs/vfs.c.md#vfs_ea_set)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### afp_remextattr

```c
int afp_remextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 699 to 781.

FPRemoveExtAttr: remove an extended attribute.

The dircache entry drops its cached EA names and values whatever the removal's result; after a successful removal the entry takes the new stat and the other sessions get a refresh hint.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the VFS result, AFPERR_PARAM, also for a name [ea_uname()](extattrs.c.md#ea_uname) refuses, AFPERR_VLOCK on a read-only volume, AFPERR_MISC on a volume without EA support, AFPERR_NOOBJ for a path that names no object, or the error of the directory or path lookup

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [ea_uname](extattrs.c.md#ea_uname), [ea_written](extattrs.c.md#ea_written), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_findname](ofork.c.md#of_findname), [path_isadir](../../include/atalk/directory.h.md#path_isadir)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_remove`](../../include/atalk/vfs.h.md#struct-vfs_ops): [remove_ea](../../libatalk/vfs/ea_ad.c.md#remove_ea), [sys_remove_ea](../../libatalk/vfs/ea_sys.c.md#sys_remove_ea), [vfs_ea_remove](../../libatalk/vfs/vfs.c.md#vfs_ea_remove)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

# File-scope variables

`ea_finderinfo`, `ea_resourcefork`
