---
type: C Source File
title: "etc/afpd/extattrs.c"
description: "5 functions, includes 17 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/extattrs.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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
* [fork.h](fork.h.md)
* [volume.h](volume.h.md)
* System headers: `errno.h`, `stdlib.h`, `string.h`, `unistd.h`

# Functions

### afp_listextattr

```c
int afp_listextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 68 to 262.

FPListExtAttrs: list the extended attribute names of a file or directory.

The client calls twice, first for the size of all names and then for the names themselves. A static buffer keeps the names between the two calls, so one scan of the extended attributes serves both.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or an AFPERR_* code

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_loaded](../../include/atalk/adouble.h.md#ad_meta_loaded), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cname](directory.c.md#cname), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_findname](ofork.c.md#of_findname), [of_statdir](ofork.c.md#of_statdir), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_list`](../../include/atalk/vfs.h.md#struct-vfs_ops): [list_eas](../../libatalk/vfs/ea_ad.c.md#list_eas), [sys_list_eas](../../libatalk/vfs/ea_sys.c.md#sys_list_eas), [vfs_ea_list](../../libatalk/vfs/vfs.c.md#vfs_ea_list)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `ea_finderinfo`, `ea_resourcefork`

### to_stringz

```c
static char * to_stringz(char *ibuf, uint16_t len)
```

Defined at lines 272 to 285.

Terminate a counted extended attribute name in a static buffer for log lines.

Parameters:
* `ibuf`: name from the request
* `len`: length of the name, capped at 255

Returns: the static buffer

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setextattr](extattrs.c.md#afp_setextattr)

### afp_getextattr

```c
int afp_getextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 304 to 407.

FPGetExtAttr: return the size or the content of an extended attribute.

A MaxReplySize of 0 asks for the size only. The name arrives in UTF-8 and converts to the unix charset. An open fork of the file lends its metadata descriptor to the VFS call. kXAttrNoFollow keeps the call off symlink targets.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the VFS result, AFPERR_PARAM for an unknown volume, AFPERR_MISC for an empty converted name, or the error of the directory or path lookup

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cname](directory.c.md#cname), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_findname](ofork.c.md#of_findname), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [to_stringz](extattrs.c.md#to_stringz)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_getcontent`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_eacontent](../../libatalk/vfs/ea_ad.c.md#get_eacontent), [sys_get_eacontent](../../libatalk/vfs/ea_sys.c.md#sys_get_eacontent), [vfs_ea_getcontent](../../libatalk/vfs/vfs.c.md#vfs_ea_getcontent)

Calls through [`vfs_ops::vfs_ea_getsize`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_easize](../../libatalk/vfs/ea_ad.c.md#get_easize), [sys_get_easize](../../libatalk/vfs/ea_sys.c.md#sys_get_easize), [vfs_ea_getsize](../../libatalk/vfs/vfs.c.md#vfs_ea_getsize)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### afp_setextattr

```c
int afp_setextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 425 to 565.

FPSetExtAttr: create or replace an extended attribute.

kXAttrCreate and kXAttrReplace map to O_CREAT and O_TRUNC for the VFS call. A name over 255 bytes or a value over MAX_EA_SIZE gets AFPERR_PARAM. After a write the dircache entry takes the new stat, and the other sessions get a refresh hint.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the VFS result, AFPERR_PARAM, AFPERR_MISC for an empty converted name, or the error of the directory or path lookup

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cname](directory.c.md#cname), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_findname](ofork.c.md#of_findname), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [to_stringz](extattrs.c.md#to_stringz)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_set`](../../include/atalk/vfs.h.md#struct-vfs_ops): [set_ea](../../libatalk/vfs/ea_ad.c.md#set_ea), [sys_set_ea](../../libatalk/vfs/ea_sys.c.md#sys_set_ea), [vfs_ea_set](../../libatalk/vfs/vfs.c.md#vfs_ea_set)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_remextattr

```c
int afp_remextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 581 to 696.

FPRemoveExtAttr: remove an extended attribute.

A name over 255 bytes gets AFPERR_PARAM. After a removal the dircache entry takes the new stat, and the other sessions get a refresh hint.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the VFS result, AFPERR_PARAM, AFPERR_MISC for an empty converted name, or the error of the directory or path lookup

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cname](directory.c.md#cname), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_findname](ofork.c.md#of_findname), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [to_stringz](extattrs.c.md#to_stringz)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_remove`](../../include/atalk/vfs.h.md#struct-vfs_ops): [remove_ea](../../libatalk/vfs/ea_ad.c.md#remove_ea), [sys_remove_ea](../../libatalk/vfs/ea_sys.c.md#sys_remove_ea), [vfs_ea_remove](../../libatalk/vfs/vfs.c.md#vfs_ea_remove)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

# File-scope variables

`ea_finderinfo`, `ea_resourcefork`
