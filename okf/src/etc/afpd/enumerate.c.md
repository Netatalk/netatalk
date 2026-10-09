---
type: C Source File
title: "etc/afpd/enumerate.c"
description: "7 functions, 1 type, includes 16 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/enumerate.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-09T22:45:51+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [filedir.h](filedir.h.md)
* [fork.h](fork.h.md)
* [virtual_icon.h](virtual_icon.h.md)
* [volume.h](volume.h.md)
* System headers: `bstrlib.h`, `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/file.h`, `sys/param.h`

# Functions

### enumerate_loop

```c
static int enumerate_loop(struct dirent *de, char *mname, void *data)
```

Defined at lines 53 to 87.

Calls: [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [enumerate](enumerate.c.md#enumerate)

### check_dirent

```c
char * check_dirent(const struct vol *vol, char *name)
```

Defined at lines 102 to 118.

Bug: Doesn't work with dangling symlink i.e.: * Move a folder with a dangling symlink in the trash
* empty the trash

afp_enumerate return an empty listing but offspring count != 0 in afp_getdirparams and the client doesn't try to call afp_delete!

See also: https://sourceforge.net/p/netatalk/bugs/97/

Calls: [veto_file](filedir.c.md#veto_file)

Called by: [catsearch](catsearch.c.md#catsearch), [for_each_dirent](enumerate.c.md#for_each_dirent)

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](../../libatalk/vfs/vfs.c.md#validupath_adouble), [validupath_ea](../../libatalk/vfs/vfs.c.md#validupath_ea), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath)

### for_each_dirent

```c
int for_each_dirent(const struct vol *vol, char *name, dir_loop fn, void *data)
```

Defined at lines 122 to 150.

Calls: [check_dirent](enumerate.c.md#check_dirent)

Called by: [enumerate](enumerate.c.md#enumerate), [getdirparams](directory.c.md#getdirparams), [reenumerate_id](file.c.md#reenumerate_id)

### enumerate

```c
static int enumerate(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int ext)
```

Defined at lines 160 to 637.

Calls: [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [cname](directory.c.md#cname), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [dir_add](directory.c.md#dir_add), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [enumerate_loop](enumerate.c.md#enumerate_loop), [for_each_dirent](enumerate.c.md#for_each_dirent), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [get_afp_errno](directory.c.md#get_afp_errno), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_stat](ofork.c.md#of_stat), [ostat](../../libatalk/util/unix.c.md#ostat), [path_error](directory.c.md#path_error), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setdiroffcnt](directory.c.md#setdiroffcnt), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Called by: [afp_enumerate](enumerate.c.md#afp_enumerate), [afp_enumerate_ext](enumerate.c.md#afp_enumerate_ext), [afp_enumerate_ext2](enumerate.c.md#afp_enumerate_ext2)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_enumerate

```c
int afp_enumerate(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 640 to 644.

Calls: [enumerate](enumerate.c.md#enumerate)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_enumerate_ext

```c
int afp_enumerate_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 647 to 651.

Calls: [enumerate](enumerate.c.md#enumerate)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_enumerate_ext2

```c
int afp_enumerate_ext2(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 654 to 658.

Calls: [enumerate](enumerate.c.md#enumerate)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

# Types

### struct savedir

Struct to save directory reading context in.

Defined at line 43.
* `u_short sd_vid`
* `uint32_t sd_did`
* `int sd_buflen`
* `char * sd_buf`
* `char * sd_last`
* `unsigned int sd_sindex`

# Macros

* Undocumented: `REPLY_PARAM_MAXLEN`, `SDBUFBRK`, `min`
