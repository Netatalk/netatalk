---
type: C Source File
title: "etc/afpd/enumerate.c"
description: "Directory listing: FPEnumerate, FPEnumerateExt, and FPEnumerateExt2."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/enumerate.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-31T16:21:52+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
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
* System headers: `bstrlib.h`, `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/file.h`, `sys/param.h`, `time.h`

# Functions

### enumerate_stats_get

```c
void enumerate_stats_get(struct enumerate_stats *out)
```

Defined at lines 92 to 95.

Copy the session's directory listing counts.

Parameters:
* `out`: the counts

Uses file-scope variables: `enumerate_stat`

### enumerate_log_stats

```c
void enumerate_log_stats(void)
```

Defined at lines 100 to 107.

Log the session's directory listing counts.

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close)

Uses file-scope variables: `enumerate_stat`

### sd_drop

```c
static void sd_drop(struct savedir *sd)
```

Defined at lines 114 to 126.

Empty a listing, freeing its names and its folder's CNID table.

Parameters:
* `sd`: the listing

Calls: [cnid_dir_forget](../../libatalk/cnid/cnid.c.md#cnid_dir_forget), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Called by: [enumerate_in](enumerate.c.md#enumerate_in), [read_failed](enumerate.c.md#read_failed), [sd_select](enumerate.c.md#sd_select)

### read_failed

```c
static int read_failed(struct savedir *sd, const char *what)
```

Defined at lines 137 to 159.

Drop a listing whose stat or read failed, and map the errno to the AFP result.

Parameters:
* `sd`: the listing
* `what`: the call that failed, for the log

Returns: AFPERR_ACCESS, AFPERR_BADTYPE, AFPERR_MISC for a descriptor or memory shortage, else AFPERR_NODIR

Calls: [sd_drop](enumerate.c.md#sd_drop)

Called by: [enumerate_in](enumerate.c.md#enumerate_in)

### sd_select

```c
static void sd_select(struct savedir *sd, uint16_t vid, uint32_t did)
```

Defined at lines 172 to 205.

Bring a directory's saved listing into sd, or an empty slot.

The listing sd held is parked first. A directory without a saved listing keeps sd when sd is empty, else takes a parked slot, an empty one before the least recently used, whose names are freed.

Parameters:
* `sd`: [enumerate_in()](enumerate.c.md#enumerate_in)'s listing in use
* `vid`: the directory's volume id
* `did`: the directory's id

Calls: [sd_drop](enumerate.c.md#sd_drop)

Called by: [enumerate_in](enumerate.c.md#enumerate_in)

Uses file-scope variables: `sd_parked`

Mentioned in the documentation of: [sd_fetch](enumerate.c.md#sd_fetch)

### sd_kind

```c
static char sd_kind(const struct dirent *de)
```

Defined at lines 214 to 231.

A saved name's kind as readdir() tells it.

Parameters:
* `de`: the name's directory entry

Returns: SD_DIR or SD_FILE by d_type, SD_UNKNOWN for a symlink, an unknown type, or where readdir() gives no d_type

Called by: [enumerate_loop](enumerate.c.md#enumerate_loop)

### sd_append

```c
static int sd_append(struct savedir *sd, const char *name, int len, char kind)
```

Defined at lines 247 to 274.

Append a name to a listing's buffer.

Each record is a length byte, the name, a NUL and the name's kind. The buffer doubles before a record that does not fit, so a record is written whole or not at all; sd_last is the write position and keeps its place across the move.

Parameters:
* `sd`: the listing
* `name`: the name, len bytes of it saved
* `len`: the name's length, 1 to UTF8FILELEN_EARLY
* `kind`: the record's kind byte

Returns: 0, or -1 with ENOMEM when the buffer cannot grow

Called by: [enumerate_loop](enumerate.c.md#enumerate_loop), [sd_fetch](enumerate.c.md#sd_fetch)

### enumerate_loop

```c
static int enumerate_loop(struct dirent *de, char *mname, void *data)
```

Defined at lines 284 to 289.

Append a directory entry name to the enumeration name buffer.

Parameters:
* `de`: directory entry
* `mname`: unused
* `data`: struct savedir that holds the buffer

Returns: 0, or -1 with ENOMEM when the buffer cannot grow

Calls: [sd_append](enumerate.c.md#sd_append), [sd_kind](enumerate.c.md#sd_kind), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [sd_fetch](enumerate.c.md#sd_fetch)

### sd_leave

```c
static void sd_leave(struct savedir *sd, char *name, int len)
```

Defined at lines 299 to 306.

Leave a saved name out of the listing and have the folder's offspring counted again.

Parameters:
* `sd`: the listing
* `name`: the saved name, its kind byte at name + len + 1
* `len`: the record's length byte

Called by: [enumerate_in](enumerate.c.md#enumerate_in), [sd_stat](enumerate.c.md#sd_stat)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### sd_stat

```c
static int sd_stat(const struct vol *vol, struct savedir *sd, char *name, int len, struct path *s_path)
```

Defined at lines 325 to 376.

Stat a saved name, from its dircache entry when it has one.

Records the name's kind, which the walk to a batch's index counts by. An entry of the other kind than readdir() gave is stale, so the name is stat'ed instead. A name whose stat fails is marked SD_SKIP, so that no later batch of the read stats it again, and has the folder's offspring counted again.

Parameters:
* `vol`: the listing's volume
* `sd`: the listing
* `name`: the saved name, its kind byte at name + len + 1
* `len`: the record's length byte, the name's length
* `s_path`: the name, its stat and its dircache entry

Returns: 0 with s_path filled and its d_cached the name's entry or NULL, -1 for a name whose stat failed

Calls: [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [of_stat](ofork.c.md#of_stat), [sd_leave](enumerate.c.md#sd_leave)

Called by: [enumerate_in](enumerate.c.md#enumerate_in)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### enumerate_ctime_recent

```c
bool enumerate_ctime_recent(const struct timespec *ctime, const struct timespec *now)
```

Defined at lines 393 to 408.

Tell whether a change at a ctime can leave a later change's ctime as it is, for a read that begins at now.

The granule of a change is a second where the filesystem keeps no nanoseconds, else the tick of the clock it stamps with, which trails afpd's clock by up to a tick.

Parameters:
* `ctime`: the folder's ctime
* `now`: the time the read began, after the folder's stat

Returns: true for a ctime within two seconds before now, or within two ticks of a 100 Hz clock where it keeps nanoseconds, or up to two seconds ahead of now; a ctime further ahead is a stored date, as Linux gives a folder Mac OS 9 made on HFS+, not a recent change

Called by: [sd_fetch](enumerate.c.md#sd_fetch)

### sd_fetch

```c
static int sd_fetch(struct savedir *sd, const struct vol *vol, const struct stat *st, uint32_t sindex)
```

Defined at lines 430 to 503.

The working folder's listing: the saved one while the folder keeps the listing's stamp, else the folder read from disk and saved.

Any batch serves the saved listing while the folder keeps a stamp other than the zero one, which a read in the granule of the folder's last change takes, so a folder whose stat shows no ctime is read at every batch. A new pass of a listing that left a name out reads the folder again too, so the name's stat runs again. The read leaves sd_last at the first record; a zero length byte ends the records.

Parameters:
* `sd`: the folder's slot, from [sd_select()](enumerate.c.md#sd_select), or the volume root's listing, whose DID [enumerate()](enumerate.c.md#enumerate) clears at each request
* `vol`: the folder's volume
* `st`: the folder's stat, taken before the read
* `sindex`: the batch's start index

Returns: 1 after a read, 0 for the saved listing, or -1 with errno from the read, or ENOMEM

Calls: [atalk_stat_ctime_timespec](../../include/atalk/compat.h.md#atalk_stat_ctime_timespec), [atalk_timespec_eq](../../include/atalk/compat.h.md#atalk_timespec_eq), [enumerate_ctime_recent](enumerate.c.md#enumerate_ctime_recent), [enumerate_loop](enumerate.c.md#enumerate_loop), [for_each_dirent](enumerate.c.md#for_each_dirent), [is_virtual_icon](virtual_icon.c.md#is_virtual_icon), [sd_append](enumerate.c.md#sd_append), [setdiroffcnt](directory.c.md#setdiroffcnt)

Called by: [enumerate_in](enumerate.c.md#enumerate_in)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md), `enumerate_stat`

### check_dirent

```c
char * check_dirent(const struct vol *vol, char *name)
```

Defined at lines 522 to 543.

Filter a directory entry for enumeration.

Drops ".", "..", names longer than 255 bytes, the names the volume's VFS modules reserve for their metadata, and vetoed names. A ._ file this session holds open is read through the fork's own descriptor.

Bug: A dangling symlink defeats the filter. Move a folder that holds one into the trash and empty the trash. afp_enumerate then returns an empty listing while afp_getdirparams still reports offspring, so the client never calls afp_delete.

See also: https://sourceforge.net/p/netatalk/bugs/97/

Parameters:
* `vol`: volume the entry belongs to
* `name`: entry name as read from the directory

Returns: name when the client may see the entry, NULL otherwise

Calls: [of_sidecar_fd](ofork.c.md#of_sidecar_fd), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [veto_file](filedir.c.md#veto_file)

Called by: [catsearch](catsearch.c.md#catsearch), [enumerate_in](enumerate.c.md#enumerate_in), [for_each_dirent](enumerate.c.md#for_each_dirent)

Calls through [`vfs_ops::vfs_validupath`](../../include/atalk/vfs.h.md#struct-vfs_ops): [validupath_adouble](../../libatalk/vfs/vfs.c.md#validupath_adouble), [validupath_ea](../../libatalk/vfs/vfs.c.md#validupath_ea), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath)

### for_each_dirent

```c
int for_each_dirent(const struct vol *vol, char *name, dir_loop fn, void *data)
```

Defined at lines 559 to 592.

Call fn for every entry of directory name that clients may see.

The check_dirent call filters the entries and supplies the Mac name. The count covers every visible entry, so a NULL fn only counts.

Parameters:
* `vol`: volume of the directory
* `name`: directory to walk
* `fn`: callback, or NULL to count only
* `data`: callback data

Returns: the number of visible entries, or -1 when opendir, readdir or fn failed

Calls: [check_dirent](enumerate.c.md#check_dirent)

Called by: [offcnt_serve](directory.c.md#offcnt_serve), [reenumerate_id](file.c.md#reenumerate_id), [sd_fetch](enumerate.c.md#sd_fetch)

### enumerate_in

```c
static int enumerate_in(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int ext, struct savedir *own)
```

Defined at lines 641 to 1076.

Shared body of FPEnumerate, FPEnumerateExt, and FPEnumerateExt2.

The request names a directory, a file bitmap, a directory bitmap, the requested count, a start index from 1, and the reply size limit. FPEnumerateExt2 widens the start index and the limit to 32 bits. The first request for a directory reads every visible name into a saved listing. Later requests for it, from any index, use that listing while the directory keeps the inode and ctime the read saw, unless the read came within the granule of the directory's last change or the directory shows no ctime; the session keeps the listings of eight directories. A request after a change reads the whole directory again and goes on at its index in the new read, and so does a request at index 1 of a listing that left a name out. Any session's write on the volume moves the root's ctime, so a saved listing of the root would seldom serve: every request for the root reads it again into a listing of its own, which no slot keeps. A name's dircache entry of the kind readdir() gives stands in for its stat. A directory whose entry does not then hold the directory's stat under its DID answers AFPERR_NODIR, or AFPERR_MISC where the database could not answer for the folder at the path.

Each entry carries its length, a file or directory flag, and the parameters of getfilparams or getdirparams, padded to an even length. An entry type with a zero bitmap stays out of the reply. The volume root ends with the virtual Icon file when the volume provides one. An index past the last entry fails with AFPERR_NOOBJ.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return
* `ext`: 0 for FPEnumerate, 1 for FPEnumerateExt, 2 for FPEnumerateExt2
* `own`: the volume root's listing, its DID cleared

Returns: AFP_OK, AFPERR_NOOBJ at the end of the directory, AFPERR_NODIR for a directory that is gone or replaced under its name, AFPERR_BITMAP, AFPERR_PARAM, or another AFPERR_* code

Calls: [ad_convert](../../libatalk/adouble/ad_conv.c.md#ad_convert), [check_dirent](enumerate.c.md#check_dirent), [cname](directory.c.md#cname), [cnid_dir_arm](../../libatalk/cnid/cnid.c.md#cnid_dir_arm), [cnid_dir_forget](../../libatalk/cnid/cnid.c.md#cnid_dir_forget), [cnid_dir_serve](../../libatalk/cnid/cnid.c.md#cnid_dir_serve), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [dir_add_fill](directory.c.md#dir_add_fill), [dir_remove_tree](directory.c.md#dir_remove_tree), [dir_stat_refresh](directory.c.md#dir_stat_refresh), [dirlookup](directory.c.md#dirlookup), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [get_afp_errno](directory.c.md#get_afp_errno), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath), [getdirparams](directory.c.md#getdirparams), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_stat](ofork.c.md#of_stat), [ostat](../../libatalk/util/unix.c.md#ostat), [path_error](directory.c.md#path_error), [read_failed](enumerate.c.md#read_failed), [sd_drop](enumerate.c.md#sd_drop), [sd_fetch](enumerate.c.md#sd_fetch), [sd_leave](enumerate.c.md#sd_leave), [sd_select](enumerate.c.md#sd_select), [sd_stat](enumerate.c.md#sd_stat), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Called by: [enumerate](enumerate.c.md#enumerate)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Mentioned in the documentation of: [enumerate](enumerate.c.md#enumerate), [sd_select](enumerate.c.md#sd_select)

### enumerate

```c
static int enumerate(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int ext)
```

Defined at lines 1093 to 1099.

[enumerate_in()](enumerate.c.md#enumerate_in) with the volume root's listing, whose buffer stays from request to request while each request for the root reads the root into it again

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return
* `ext`: 0 for FPEnumerate, 1 for FPEnumerateExt, 2 for FPEnumerateExt2

Returns: [enumerate_in()](enumerate.c.md#enumerate_in)'s result

Calls: [enumerate_in](enumerate.c.md#enumerate_in)

Called by: [afp_enumerate](enumerate.c.md#afp_enumerate), [afp_enumerate_ext](enumerate.c.md#afp_enumerate_ext), [afp_enumerate_ext2](enumerate.c.md#afp_enumerate_ext2)

Mentioned in the documentation of: [sd_fetch](enumerate.c.md#sd_fetch)

### afp_enumerate

```c
int afp_enumerate(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1112 to 1116.

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

Defined at lines 1129 to 1133.

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

Defined at lines 1147 to 1151.

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

Defined at line 52.
* `u_short sd_vid`
* `uint32_t sd_did`
* `size_t sd_buflen`
* `char * sd_buf`
* `char * sd_last`
* `unsigned int sd_sindex`
* `int sd_kinds`
* `ino_t sd_ino`
* `struct timespec sd_ctime`
* `bool sd_left`

# Macros

* `REPLY_PARAM_MAXLEN`: This is the maximal length of a single entry for a file/dir in the reply block if all bits in the file/dir bitmap are set: header(4) + [params(104)](../atalkd/config.c.md#params) + macnamelength(1) + macname(31) + utf8(4) + utf8namelen(2) + utf8name(255) + oddpadding(1).
* Undocumented: `SDBUFBRK`, `SD_DIR`, `SD_FILE`, `SD_ICON`, `SD_SKIP`, `SD_SLOTS`, `SD_UNKNOWN`, `min`

# File-scope variables

`enumerate_stat`, `sd_parked`
