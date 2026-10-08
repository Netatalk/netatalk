---
type: C Source File
title: "etc/afpd/extattrs.c"
description: "5 functions, includes 17 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/extattrs.c"
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

Defined at lines 57 to 251.

Note: we're being called twice. Firstly the client only want the size of all EA names, secondly it wants these names. In order to avoid scanning EAs twice we cache them in a static buffer.

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_loaded](../../include/atalk/adouble.h.md#ad_meta_loaded), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cname](directory.c.md#cname), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_findname](ofork.c.md#of_findname), [of_statdir](ofork.c.md#of_statdir), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_list`](../../include/atalk/vfs.h.md#struct-vfs_ops): [list_eas](../../libatalk/vfs/ea_ad.c.md#list_eas), [sys_list_eas](../../libatalk/vfs/ea_sys.c.md#sys_list_eas), [vfs_ea_list](../../libatalk/vfs/vfs.c.md#vfs_ea_list)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `ea_finderinfo`, `ea_resourcefork`

### to_stringz

```c
static char * to_stringz(char *ibuf, uint16_t len)
```

Defined at lines 253 to 266.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setextattr](extattrs.c.md#afp_setextattr)

### afp_getextattr

```c
int afp_getextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 268 to 371.

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cname](directory.c.md#cname), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_findname](ofork.c.md#of_findname), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [to_stringz](extattrs.c.md#to_stringz)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_getcontent`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_eacontent](../../libatalk/vfs/ea_ad.c.md#get_eacontent), [sys_get_eacontent](../../libatalk/vfs/ea_sys.c.md#sys_get_eacontent), [vfs_ea_getcontent](../../libatalk/vfs/vfs.c.md#vfs_ea_getcontent)

Calls through [`vfs_ops::vfs_ea_getsize`](../../include/atalk/vfs.h.md#struct-vfs_ops): [get_easize](../../libatalk/vfs/ea_ad.c.md#get_easize), [sys_get_easize](../../libatalk/vfs/ea_sys.c.md#sys_get_easize), [vfs_ea_getsize](../../libatalk/vfs/vfs.c.md#vfs_ea_getsize)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### afp_setextattr

```c
int afp_setextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 373 to 513.

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cname](directory.c.md#cname), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_findname](ofork.c.md#of_findname), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [to_stringz](extattrs.c.md#to_stringz)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_set`](../../include/atalk/vfs.h.md#struct-vfs_ops): [set_ea](../../libatalk/vfs/ea_ad.c.md#set_ea), [sys_set_ea](../../libatalk/vfs/ea_sys.c.md#sys_set_ea), [vfs_ea_set](../../libatalk/vfs/vfs.c.md#vfs_ea_set)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_remextattr

```c
int afp_remextattr(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 515 to 630.

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [cname](directory.c.md#cname), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_findname](ofork.c.md#of_findname), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [to_stringz](extattrs.c.md#to_stringz)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Calls through [`vfs_ops::vfs_ea_remove`](../../include/atalk/vfs.h.md#struct-vfs_ops): [remove_ea](../../libatalk/vfs/ea_ad.c.md#remove_ea), [sys_remove_ea](../../libatalk/vfs/ea_sys.c.md#sys_remove_ea), [vfs_ea_remove](../../libatalk/vfs/vfs.c.md#vfs_ea_remove)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

# File-scope variables

`ea_finderinfo`, `ea_resourcefork`
