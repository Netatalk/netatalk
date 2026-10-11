---
type: C Source File
title: "etc/afpd/enumerate.c"
description: "7 functions, 1 type, includes 16 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/enumerate.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Defined at lines 64 to 98.

Append a directory entry name to the enumeration name buffer.

Each record is a length byte, the name, and a NUL. The buffer grows in SDBUFBRK steps when the record does not fit.

Parameters:
* `de`: directory entry
* `mname`: unused
* `data`: struct savedir that holds the buffer

Returns: 0, or -1 with ENOMEM when the buffer cannot grow

Calls: [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [enumerate](enumerate.c.md#enumerate)

### check_dirent

```c
char * check_dirent(const struct vol *vol, char *name)
```

Defined at lines 116 to 132.

Filter a directory entry for enumeration.

Drops ".", "..", the names the volume's VFS modules reserve for their metadata, and vetoed names.

Bug: A dangling symlink defeats the filter. Move a folder that holds one into the trash and empty the trash. afp_enumerate then returns an empty listing while afp_getdirparams still reports offspring, so the client never calls afp_delete.

See also: https://sourceforge.net/p/netatalk/bugs/97/

Parameters:
* `vol`: volume the entry belongs to
* `name`: entry name as read from the directory

Returns: name when the client may see the entry, NULL otherwise

Calls: [veto_file](filedir.c.md#veto_file)

Called by: [catsearch](catsearch.c.md#catsearch), [for_each_dirent](enumerate.c.md#for_each_dirent)

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](../../libatalk/vfs/vfs.c.md#validupath_adouble), [validupath_ea](../../libatalk/vfs/vfs.c.md#validupath_ea), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath)

### for_each_dirent

```c
int for_each_dirent(const struct vol *vol, char *name, dir_loop fn, void *data)
```

Defined at lines 147 to 175.

Call fn for every entry of directory name that clients may see.

The check_dirent call filters the entries and supplies the Mac name. The count covers every visible entry, so a NULL fn only counts.

Parameters:
* `vol`: volume of the directory
* `name`: directory to walk
* `fn`: callback, or NULL to count only
* `data`: callback data

Returns: the number of visible entries, or -1 when opendir or fn failed

Calls: [check_dirent](enumerate.c.md#check_dirent)

Called by: [enumerate](enumerate.c.md#enumerate), [getdirparams](directory.c.md#getdirparams), [reenumerate_id](file.c.md#reenumerate_id)

### enumerate

```c
static int enumerate(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int ext)
```

Defined at lines 211 to 688.

Shared body of FPEnumerate, FPEnumerateExt, and FPEnumerateExt2.

The request names a directory, a file bitmap, a directory bitmap, the requested count, a start index from 1, and the reply size limit. FPEnumerateExt2 widens the start index and the limit to 32 bits. The first request for a directory reads every visible name into a static buffer, and later requests for the same directory continue from it. Entries in the dircache skip the stat.

Each entry carries its length, a file or directory flag, and the parameters of getfilparams or getdirparams, padded to an even length. An entry type with a zero bitmap stays out of the reply. The volume root ends with the virtual Icon file when the volume provides one. An index past the last entry fails with AFPERR_NOOBJ.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return
* `ext`: 0 for FPEnumerate, 1 for FPEnumerateExt, 2 for FPEnumerateExt2

Returns: AFP_OK, AFPERR_NOOBJ at the end of the directory, AFPERR_NODIR, AFPERR_BITMAP, AFPERR_PARAM, or another AFPERR_* code

Calls: [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [cname](directory.c.md#cname), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [dir_add](directory.c.md#dir_add), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [enumerate_loop](enumerate.c.md#enumerate_loop), [for_each_dirent](enumerate.c.md#for_each_dirent), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [get_afp_errno](directory.c.md#get_afp_errno), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_stat](ofork.c.md#of_stat), [ostat](../../libatalk/util/unix.c.md#ostat), [path_error](directory.c.md#path_error), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [setdiroffcnt](directory.c.md#setdiroffcnt), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Called by: [afp_enumerate](enumerate.c.md#afp_enumerate), [afp_enumerate_ext](enumerate.c.md#afp_enumerate_ext), [afp_enumerate_ext2](enumerate.c.md#afp_enumerate_ext2)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_enumerate

```c
int afp_enumerate(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 701 to 705.

FPEnumerate: list directory entries with one-byte entry lengths.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or the AFPERR_* code of enumerate

Calls: [enumerate](enumerate.c.md#enumerate)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_enumerate_ext

```c
int afp_enumerate_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 718 to 722.

FPEnumerateExt: list directory entries with two-byte entry lengths.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or the AFPERR_* code of enumerate

Calls: [enumerate](enumerate.c.md#enumerate)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_enumerate_ext2

```c
int afp_enumerate_ext2(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 736 to 740.

FPEnumerateExt2: list directory entries with a 32-bit start index and reply size limit.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or the AFPERR_* code of enumerate

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

* `REPLY_PARAM_MAXLEN`: This is the maximal length of a single entry for a file/dir in the reply block if all bits in the file/dir bitmap are set: header(4) + [params(104)](../atalkd/config.c.md#params) + macnamelength(1) + macname(31) + utf8(4) + utf8namelen(2) + utf8name(255) + oddpadding(1).
* Undocumented: `SDBUFBRK`, `min`
