---
type: C Source File
title: "etc/afpd/enumerate.c"
description: "Directory listing: FPEnumerate, FPEnumerateExt, and FPEnumerateExt2."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/enumerate.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T15:29:27+11:00 }
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

### enumerate_stats_get

```c
void enumerate_stats_get(struct enumerate_stats *out)
```

Defined at lines 84 to 87.

Copy the session's directory listing counts.

Parameters:
* `out`: the counts

Uses file-scope variables: `enumerate_stat`

### enumerate_log_stats

```c
void enumerate_log_stats(void)
```

Defined at lines 92 to 98.

Log the session's directory listing counts.

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

Uses file-scope variables: `enumerate_stat`

### sd_drop

```c
static void sd_drop(struct savedir *sd)
```

Defined at lines 105 to 117.

Empty a slot, freeing its names.

Parameters:
* `sd`: the slot

Calls: [cnid_dir_forget](../../libatalk/cnid/cnid.c.md#cnid_dir_forget), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Called by: [enumerate](enumerate.c.md#enumerate), [sd_select](enumerate.c.md#sd_select)

### sd_select

```c
static void sd_select(struct savedir *sd, uint16_t vid, uint32_t did)
```

Defined at lines 130 to 163.

Bring a directory's saved listing into sd, or an empty slot.

The listing sd held is parked first. A directory without a saved listing keeps sd when sd is empty, else takes a parked slot, an empty one before the least recently used, whose names are freed.

Parameters:
* `sd`: [enumerate()](enumerate.c.md#enumerate)'s listing in use
* `vid`: the directory's volume id
* `did`: the directory's id

Calls: [sd_drop](enumerate.c.md#sd_drop)

Called by: [enumerate](enumerate.c.md#enumerate)

Uses file-scope variables: `sd_parked`

### sd_kind

```c
static char sd_kind(const struct dirent *de)
```

Defined at lines 172 to 189.

A saved name's kind as readdir() tells it.

Parameters:
* `de`: the name's directory entry

Returns: SD_DIR or SD_FILE by d_type, SD_UNKNOWN for a symlink, an unknown type, or where readdir() gives no d_type

Called by: [enumerate_loop](enumerate.c.md#enumerate_loop)

### enumerate_loop

```c
static int enumerate_loop(struct dirent *de, char *mname, void *data)
```

Defined at lines 202 to 233.

Append a directory entry name to the enumeration name buffer.

Each record is a length byte, the name, a NUL and the name's kind. The buffer doubles when the record does not fit.

Parameters:
* `de`: directory entry
* `mname`: unused
* `data`: struct savedir that holds the buffer

Returns: 0, or -1 with ENOMEM when the buffer cannot grow

Calls: [sd_kind](enumerate.c.md#sd_kind), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [enumerate](enumerate.c.md#enumerate)

### sd_leave

```c
static void sd_leave(char *name, int len)
```

Defined at lines 242 to 248.

Leave a saved name out of the listing and have the folder's offspring counted again.

Parameters:
* `name`: the saved name, its kind byte at name + len + 1
* `len`: the record's length byte

Called by: [enumerate](enumerate.c.md#enumerate), [sd_stat](enumerate.c.md#sd_stat)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### sd_stat

```c
static int sd_stat(const struct vol *vol, char *name, int len, struct path *s_path)
```

Defined at lines 265 to 309.

Stat a saved name, from its dircache entry when it has one.

Records the name's kind. A name whose stat fails is marked SD_SKIP, so that no later batch stats it again, and has the folder's offspring counted again.

Parameters:
* `vol`: the listing's volume
* `name`: the saved name, its kind byte at name + len + 1
* `len`: the record's length byte, longer than the name once a conversion has shortened it
* `s_path`: the name, its stat and its dircache entry

Returns: 0 with s_path filled and its d_cached the name's entry or NULL, -1 for a name whose stat failed

Calls: [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [of_stat](ofork.c.md#of_stat), [sd_leave](enumerate.c.md#sd_leave)

Called by: [enumerate](enumerate.c.md#enumerate)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### check_dirent

```c
char * check_dirent(const struct vol *vol, char *name)
```

Defined at lines 327 to 348.

Filter a directory entry for enumeration.

Drops ".", "..", names longer than 255 bytes, the names the volume's VFS modules reserve for their metadata, and vetoed names.

Bug: A dangling symlink defeats the filter. Move a folder that holds one into the trash and empty the trash. afp_enumerate then returns an empty listing while afp_getdirparams still reports offspring, so the client never calls afp_delete.

See also: https://sourceforge.net/p/netatalk/bugs/97/

Parameters:
* `vol`: volume the entry belongs to
* `name`: entry name as read from the directory

Returns: name when the client may see the entry, NULL otherwise

Calls: [strnlen](../../libatalk/compat/misc.c.md#strnlen), [veto_file](filedir.c.md#veto_file)

Called by: [catsearch](catsearch.c.md#catsearch), [enumerate](enumerate.c.md#enumerate), [for_each_dirent](enumerate.c.md#for_each_dirent)

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](../../libatalk/vfs/vfs.c.md#validupath_adouble), [validupath_ea](../../libatalk/vfs/vfs.c.md#validupath_ea), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath)

### for_each_dirent

```c
int for_each_dirent(const struct vol *vol, char *name, dir_loop fn, void *data)
```

Defined at lines 364 to 397.

Call fn for every entry of directory name that clients may see.

The check_dirent call filters the entries and supplies the Mac name. The count covers every visible entry, so a NULL fn only counts.

Parameters:
* `vol`: volume of the directory
* `name`: directory to walk
* `fn`: callback, or NULL to count only
* `data`: callback data

Returns: the number of visible entries, or -1 when opendir, readdir or fn failed

Calls: [check_dirent](enumerate.c.md#check_dirent)

Called by: [enumerate](enumerate.c.md#enumerate), [offcnt_serve](directory.c.md#offcnt_serve), [reenumerate_id](file.c.md#reenumerate_id)

### enumerate

```c
static int enumerate(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int ext)
```

Defined at lines 433 to 958.

Shared body of FPEnumerate, FPEnumerateExt, and FPEnumerateExt2.

The request names a directory, a file bitmap, a directory bitmap, the requested count, a start index from 1, and the reply size limit. FPEnumerateExt2 widens the start index and the limit to 32 bits. The first request for a directory reads every visible name into a saved listing, and later requests for it continue from that listing; the session keeps the listings of eight directories. Entries in the dircache skip the stat.

Each entry carries its length, a file or directory flag, and the parameters of getfilparams or getdirparams, padded to an even length. An entry type with a zero bitmap stays out of the reply. The volume root ends with the virtual Icon file when the volume provides one. An index past the last entry fails with AFPERR_NOOBJ.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return
* `ext`: 0 for FPEnumerate, 1 for FPEnumerateExt, 2 for FPEnumerateExt2

Returns: AFP_OK, AFPERR_NOOBJ at the end of the directory, AFPERR_NODIR, AFPERR_BITMAP, AFPERR_PARAM, or another AFPERR_* code

Calls: [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [check_dirent](enumerate.c.md#check_dirent), [cname](directory.c.md#cname), [cnid_dir_arm](../../libatalk/cnid/cnid.c.md#cnid_dir_arm), [cnid_dir_serve](../../libatalk/cnid/cnid.c.md#cnid_dir_serve), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [dir_add](directory.c.md#dir_add), [dir_remove](directory.c.md#dir_remove), [dirlookup](directory.c.md#dirlookup), [enumerate_loop](enumerate.c.md#enumerate_loop), [for_each_dirent](enumerate.c.md#for_each_dirent), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [get_afp_errno](directory.c.md#get_afp_errno), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_stat](ofork.c.md#of_stat), [ostat](../../libatalk/util/unix.c.md#ostat), [path_error](directory.c.md#path_error), [real_icon_exists](virtual_icon.c.md#real_icon_exists), [sd_drop](enumerate.c.md#sd_drop), [sd_leave](enumerate.c.md#sd_leave), [sd_select](enumerate.c.md#sd_select), [sd_stat](enumerate.c.md#sd_stat), [setdiroffcnt](directory.c.md#setdiroffcnt), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Called by: [afp_enumerate](enumerate.c.md#afp_enumerate), [afp_enumerate_ext](enumerate.c.md#afp_enumerate_ext), [afp_enumerate_ext2](enumerate.c.md#afp_enumerate_ext2)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `enumerate_stat`

Mentioned in the documentation of: [sd_select](enumerate.c.md#sd_select)

### afp_enumerate

```c
int afp_enumerate(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 971 to 975.

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

Defined at lines 988 to 992.

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

Defined at lines 1006 to 1010.

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

Defined at line 49.
* `u_short sd_vid`
* `uint32_t sd_did`
* `size_t sd_buflen`
* `char * sd_buf`
* `char * sd_last`
* `unsigned int sd_sindex`
* `int sd_kinds`
* `unsigned int sd_end`

# Macros

* `REPLY_PARAM_MAXLEN`: This is the maximal length of a single entry for a file/dir in the reply block if all bits in the file/dir bitmap are set: header(4) + [params(104)](../atalkd/config.c.md#params) + macnamelength(1) + macname(31) + utf8(4) + utf8namelen(2) + utf8name(255) + oddpadding(1).
* Undocumented: `SDBUFBRK`, `SD_DIR`, `SD_FILE`, `SD_SKIP`, `SD_SLOTS`, `SD_UNKNOWN`, `min`

# File-scope variables

`enumerate_stat`, `sd_parked`
