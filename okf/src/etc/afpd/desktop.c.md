---
type: C Source File
title: "etc/afpd/desktop.c"
description: "Manage the AppleDesktop folder and its contents."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/desktop.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [fork.h](fork.h.md)
* [mangle.h](mangle.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `bstrlib.h`, `ctype.h`, `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/stat.h`, `sys/uio.h`

# Functions

### setdeskmode

```c
int setdeskmode(const struct vol *vol, const mode_t mode)
```

Defined at lines 69 to 176.

Apply a volume root mode change to the desktop database folder.

The call changes .AppleDesktop, its one- and two-character subdirectories, and their entries. Directories get DIRBITS and mode, files mode without the execute bits. The call refuses a mode that takes read or search access from the owner. It ignores EPERM, and other chmod failures only log.

Parameters:
* `vol`: volume of the desktop database
* `mode`: new mode of the volume root

Returns: 0, or -1 for a refused mode or when getcwd, chdir, or opendir fails

Calls: [dir_rx_set](../../libatalk/vfs/unix.c.md#dir_rx_set), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [ochmod](../../libatalk/util/unix.c.md#ochmod)

Called by: [setdirparams](directory.c.md#setdirparams)

### setdeskowner

```c
int setdeskowner(const struct vol *vol, uid_t uid, gid_t gid)
```

Defined at lines 189 to 263.

Apply a volume root owner change to the desktop database folder.

The call walks .AppleDesktop like setdeskmode and changes the owner and group of every entry. It ignores EPERM, and other chown failures only log.

Parameters:
* `vol`: volume of the desktop database
* `uid`: new owner, or -1 to keep it
* `gid`: new group, or -1 to keep it

Returns: 0, or -1 when getcwd, chdir, or opendir fails

Calls: [fullpathname](../../libatalk/util/unix.c.md#fullpathname)

Called by: [setdirparams](directory.c.md#setdirparams)

### create_appledesktop_folder

```c
static void create_appledesktop_folder(const struct vol *vol)
```

Defined at lines 274 to 305.

Make sure the desktop database folder of a volume exists.

When the database path lacks .AppleDesktop, the call moves the folder from the volume root as root. Without either folder, or when the move fails, it creates a new one with mode 0777 under the umask.

Parameters:
* `vol`: volume of the desktop database

Calls: [become_root](../../libatalk/util/unix.c.md#become_root), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Called by: [afp_opendt](desktop.c.md#afp_opendt)

### afp_opendt

```c
int afp_opendt(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 320 to 337.

FPOpenDT: open the desktop database of a volume.

The call creates the desktop folder when needed and answers with the volume ID as the desktop reference.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK, or AFPERR_PARAM for an unknown volume

Calls: [create_appledesktop_folder](desktop.c.md#create_appledesktop_folder), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_closedt

```c
int afp_closedt(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 351 to 364.

FPCloseDT: close the desktop database of a volume.

The desktop reference is the volume ID, and the call only checks it.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: AFP_OK, or AFPERR_PARAM for an unknown reference

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### icon_dtfile

```c
static char * icon_dtfile(struct vol *vol, uint8_t creator[4])
```

Defined at lines 375 to 378.

Return the path of the icon file of a creator in the desktop database.

Parameters:
* `vol`: volume of the desktop database
* `creator`: creator code

Returns: the path in the static buffer of dtfile, or NULL

Calls: [dtfile](desktop.c.md#dtfile)

Called by: [afp_addicon](desktop.c.md#afp_addicon), [afp_geticon](desktop.c.md#afp_geticon), [afp_geticoninfo](desktop.c.md#afp_geticoninfo), [iconopen](desktop.c.md#iconopen)

### iconopen

```c
static int iconopen(struct vol *vol, uint8_t creator[4], int flags, int mode)
```

Defined at lines 394 to 441.

Open the icon file of a creator in the desktop database.

One descriptor serves the icon file of the last creator and volume, and an open of the same file reuses it with its position. Any other open closes it first. With O_CREAT, a missing file also gets its two parent directories.

Parameters:
* `vol`: volume of the desktop database
* `creator`: creator code
* `flags`: open flags
* `mode`: creation mode, masked with the parent directory permissions

Returns: 0, or -1 when open fails

Calls: [ad_mkdir](../../libatalk/adouble/ad_open.c.md#ad_mkdir), [ad_mode](../../libatalk/adouble/ad_open.c.md#ad_mode), [icon_dtfile](desktop.c.md#icon_dtfile)

Called by: [afp_addicon](desktop.c.md#afp_addicon), [afp_geticon](desktop.c.md#afp_geticon), [afp_geticoninfo](desktop.c.md#afp_geticoninfo)

Uses file-scope variables: `si`

### afp_addicon

```c
int afp_addicon(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 462 to 666.

FPAddIcon: store an icon bitmap in the icon file of a creator.

Each record of the icon file holds a 12-byte header and the bitmap. The header carries the tag, the file type, the icon type, and the bitmap size. A record with the same tag, file type, and icon type gets the new bitmap in place, and a new icon goes at the end. The bitmap arrives as the write data of the request.

Over [DSI](../../include/atalk/dsi.h.md#struct-dsi) an error drains that data first, so the session stays in step.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: receive buffer for the bitmap
* `rbuflen`: buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_NOITEM when iconopen fails, AFPERR_ITYPE for a size change, or AFPERR_PARAM

Calls: [asp_wrtcont](../../libatalk/asp/asp_write.c.md#asp_wrtcont), [dsi_write](../../libatalk/dsi/dsi_write.c.md#dsi_write), [dsi_writeflush](../../libatalk/dsi/dsi_write.c.md#dsi_writeflush), [dsi_writeinit](../../libatalk/dsi/dsi_write.c.md#dsi_writeinit), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [icon_dtfile](desktop.c.md#icon_dtfile), [iconopen](desktop.c.md#iconopen)

Uses file-scope variables: `si`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_geticoninfo

```c
int afp_geticoninfo(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 691 to 766.

FPGetIconInfo: look up an icon of a creator by index and return its record header.

The index counts from 1, and the open icon file keeps its position between calls. A creator of 4 zero bytes gets one fixed entry with icon type 1. The reply holds the 12-byte record header.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: AFP_OK, AFPERR_NOITEM past the last icon or without an icon file, or AFPERR_PARAM

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [icon_dtfile](desktop.c.md#icon_dtfile), [iconopen](desktop.c.md#iconopen)

Uses file-scope variables: `si`, `ucreator`, `usize`, `utag`, `utype`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_geticon

```c
int afp_geticon(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 785 to 935.

FPGetIcon: return the bitmap of an icon from the icon file of a creator.

The call looks for the first record with the requested file type and icon type and returns up to the requested size of its bitmap. Over [DSI](../../include/atalk/dsi.h.md#struct-dsi) a bitmap larger than the reply buffer streams through dsi_read, or through sendfile in a WITH_SENDFILE build. A failure during streaming ends the session.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_NOITEM without a matching icon, or AFPERR_PARAM

Calls: [dsi_read](../../libatalk/dsi/dsi_read.c.md#dsi_read), [dsi_readdone](../../libatalk/dsi/dsi_read.c.md#dsi_readdone), [dsi_readinit](../../libatalk/dsi/dsi_read.c.md#dsi_readinit), [dsi_stream_read_file](../../libatalk/dsi/dsi_stream.c.md#dsi_stream_read_file), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [icon_dtfile](desktop.c.md#icon_dtfile), [iconopen](desktop.c.md#iconopen)

Uses file-scope variables: `si`

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### dtfile

```c
char * dtfile(const struct vol *vol, uint8_t creator[], char *ext)
```

Defined at lines 952 to 993.

Build the path of a desktop database file for a creator.

The path is v_dbpath/.AppleDesktop, then a directory named by the first creator byte, then the 4 creator bytes and ext. A byte that is not printable, or a slash, becomes 2 hex digits.

Parameters:
* `vol`: volume of the desktop database
* `creator`: 4-byte creator code
* `ext`: file name suffix, such as ".appl" or ".icon"

Returns: the path in a static buffer, or NULL without a database path or for a path that is too long

Calls: [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat)

Called by: [afp_addappl](appl.c.md#afp_addappl), [afp_rmvappl](appl.c.md#afp_rmvappl), [applopen](appl.c.md#applopen), [icon_dtfile](desktop.c.md#icon_dtfile)

### mtoupath

```c
char * mtoupath(const struct vol *vol, char *mpath, cnid_t did, int utf8)
```

Defined at lines 1008 to 1044.

Convert an AFP name into the name on disk.

An empty name becomes ".". A mangled name resolves through demangle and the CNID database. Any other name converts from UTF8-MAC or the Mac charset to the volume charset.

Parameters:
* `vol`: volume of the name
* `mpath`: AFP name of one path element
* `did`: CNID of the parent directory
* `utf8`: nonzero for a UTF8-MAC name of AFP 3

Returns: the name in a static buffer, or NULL when the conversion fails

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [demangle](mangle.c.md#demangle), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_setforkparams](fork.c.md#afp_setforkparams), [catsearch_afp](catsearch.c.md#catsearch_afp), [cname](directory.c.md#cname), [cname_mtouname](directory.c.md#cname_mtouname), [ctoupath](filedir.c.md#ctoupath), [getforkparams](fork.c.md#getforkparams), [moveandrename](filedir.c.md#moveandrename), [of_closefork](ofork.c.md#of_closefork), [set_name](file.c.md#set_name)

### utompath

```c
char * utompath(const struct vol *vol, char *upath, cnid_t id, int utf8)
```

Defined at lines 1061 to 1096. Declared in [bin/nad/nad.h](../../bin/nad/nad.h.md).

Convert a name on disk into the AFP name a client sees.

The name converts from the volume charset to UTF8-MAC or the Mac charset. A name too long for the client, or with characters it cannot map, goes through mangle, which needs the CNID. A failed conversion mangles "???" instead.

Parameters:
* `vol`: volume of the name
* `upath`: name on disk of one path element
* `id`: CNID of the file, or 0
* `utf8`: nonzero for a UTF8-MAC name of AFP 3

Returns: the name in a static buffer, or NULL when the name needs mangling and id is 0

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [mangle](mangle.c.md#mangle)

Called by: [afp_resolveid](file.c.md#afp_resolveid), [catsearch_db](catsearch.c.md#catsearch_db), [cname_mtouname](directory.c.md#cname_mtouname), [crit_check](catsearch.c.md#crit_check), [dir_add](directory.c.md#dir_add), [dirlookup_internal](directory.c.md#dirlookup_internal), [getmetadata](file.c.md#getmetadata), [nad_header_read](../../bin/nad/nad_adouble.c.md#nad_header_read), [private_demangle](mangle.c.md#private_demangle), [set_name](file.c.md#set_name)

### ad_addcomment

```c
static int ad_addcomment(const AFPObj *obj, struct vol *vol, struct path *path, char *ibuf)
```

Defined at lines 1114 to 1192.

Store a Finder comment in the AppleDouble metadata of a file or directory.

The comment keeps at most 199 bytes and needs write access. An open fork lends its adouble. Any other object opens its metadata and creates it when missing. Metadata without a comment entry stores nothing. After the write the dircache entry takes the new stat and metadata, and the other sessions get a refresh hint.

Parameters:
* `obj`: session object
* `vol`: volume of the object
* `path`: resolved object
* `ibuf`: comment as a Pascal string

Returns: AFP_OK, or AFPERR_ACCESS without write access or metadata

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getentryoff](../../libatalk/adouble/ad_open.c.md#ad_getentryoff), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [check_access](directory.c.md#check_access), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_findname](ofork.c.md#of_findname), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_addcomment](desktop.c.md#afp_addcomment)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_addcomment

```c
int afp_addcomment(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1208 to 1241.

FPAddComment: store a Finder comment for a file or directory.

The request carries the volume, the directory, the path, and the comment as a Pascal string at an even offset. The comment goes to ad_addcomment.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the result of ad_addcomment, AFPERR_PARAM for an unknown volume, or the error of the directory or path lookup

Calls: [ad_addcomment](desktop.c.md#ad_addcomment), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### ad_getcomment

```c
static int ad_getcomment(struct vol *vol, struct path *path, char *rbuf, size_t *rbuflen)
```

Defined at lines 1259 to 1323.

Read the Finder comment of a file or directory from its AppleDouble metadata.

With no open fork, a dircache entry that recorded no metadata, with a matching ctime and inode, answers AFPERR_NOITEM without a disk read. A read also fills a dircache entry that held no metadata. A stored comment of 0 or over 199 bytes counts as missing. The reply holds at most 128 bytes, because macOS uses 128 and longer comments break Adobe CS2.

Parameters:
* `vol`: volume of the object
* `path`: resolved object
* `rbuf`: receives the comment as a Pascal string
* `rbuflen`: reply length on return

Returns: AFP_OK, or AFPERR_NOITEM without metadata or comment

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_getentryoff](../../libatalk/adouble/ad_open.c.md#ad_getentryoff), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_metadata](../../libatalk/adouble/ad_open.c.md#ad_metadata), [ad_rlen_meta_absent](../../include/atalk/directory.h.md#ad_rlen_meta_absent), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [of_findname](ofork.c.md#of_findname), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_getcomment](desktop.c.md#afp_getcomment)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_getcomment

```c
int afp_getcomment(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1336 to 1365.

FPGetComment: return the Finder comment of a file or directory.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply length on return

Returns: the result of ad_getcomment, AFPERR_PARAM for an unknown volume, or the error of the directory or path lookup

Calls: [ad_getcomment](desktop.c.md#ad_getcomment), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### ad_rmvcomment

```c
static int ad_rmvcomment(const AFPObj *obj, struct vol *vol, struct path *path)
```

Defined at lines 1381 to 1452.

Remove the Finder comment from the AppleDouble metadata of a file or directory.

The call needs write access and never creates metadata. It sets the comment length to 0, then refreshes the dircache entry and sends the other sessions a refresh hint.

Parameters:
* `obj`: session object
* `vol`: volume of the object
* `path`: resolved object

Returns: AFP_OK, AFPERR_ACCESS, AFPERR_NOITEM without metadata, or AFPERR_PARAM

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getentryoff](../../libatalk/adouble/ad_open.c.md#ad_getentryoff), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [check_access](directory.c.md#check_access), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_findname](ofork.c.md#of_findname), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_rmvcomment](desktop.c.md#afp_rmvcomment)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### afp_rmvcomment

```c
int afp_rmvcomment(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1465 to 1494.

FPRemoveComment: remove the Finder comment of a file or directory.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: 0 on return

Returns: the result of ad_rmvcomment, AFPERR_PARAM for an unknown volume, or the error of the directory or path lookup

Calls: [ad_rmvcomment](desktop.c.md#ad_rmvcomment), [cname](directory.c.md#cname), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Macros

* Undocumented: `EXEC_MODE`, `min`

# File-scope variables

`hexdig`, `si`, `ucreator`, `usize`, `utag`, `utype`
