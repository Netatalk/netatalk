---
type: C Source File
title: "etc/afpd/file.c"
description: "File parameters and the AFP file commands: FPCreateFile, FPCopyFile, FPExchangeFiles, and the file ID commands."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/file.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-22T19:27:15+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/afp_util.h](../../include/atalk/afp_util.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
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
* [volume.h](volume.h.md)
* System headers: `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `utime.h`

# Functions

### path_resolve_cached_file

```c
struct dir * path_resolve_cached_file(const struct vol *vol, const struct dir *dir, struct path *path)
```

Defined at lines 98 to 111.

Dircache entry of a file path, looked up once per request.

A hit in path->d_cached skips the name length scan and the hash walk. Otherwise dircache_search_by_name runs, and its result lands in d_cached for later consumers of the same request.

Parameters:
* `vol`: volume of the file
* `dir`: directory that holds the file
* `path`: path of the file, its d_cached memo updated

Returns: the dircache entry, or NULL when the file has none

Calls: [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [path_cached_file](../../include/atalk/directory.h.md#path_cached_file), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [adl_lkup](catsearch.c.md#adl_lkup), [crit_mac_name](catsearch.c.md#crit_mac_name), [getfilparams](file.c.md#getfilparams), [getmetadata](file.c.md#getmetadata), [path_entry](file.c.md#path_entry)

Mentioned in the documentation of: [path_entry](file.c.md#path_entry)

### path_entry

```c
struct dir * path_entry(const struct vol *vol, const struct dir *dir, struct path *path)
```

Defined at lines 125 to 130.

The dircache entry a request's path names.

A directory's entry is path->d_dir. A file's comes from the memoized lookup of [path_resolve_cached_file()](file.c.md#path_resolve_cached_file).

Parameters:
* `vol`: volume of the object
* `dir`: directory that holds a file
* `path`: path of the object, its d_cached memo updated for a file

Returns: the dircache entry, or NULL when a file has none

Calls: [path_isadir](../../include/atalk/directory.h.md#path_isadir), [path_resolve_cached_file](file.c.md#path_resolve_cached_file)

Called by: [afp_listextattr](extattrs.c.md#afp_listextattr), [ea_cache_entry](ad_cache.c.md#ea_cache_entry), [ea_written](extattrs.c.md#ea_written)

### symlink_target_safe

```c
static int symlink_target_safe(const struct vol *vol, const char *link_path, const char *target)
```

Defined at lines 145 to 195.

Whether a client-supplied symlink target stays inside the volume.

The setfilparams call asks it before it turns a file into a symlink. Absolute targets and targets with a .. component fail at once. The target counts from the directory of link_path. It must resolve through realpath to a path inside the volume, on the device of the volume root.

Parameters:
* `vol`: volume of the link
* `link_path`: path of the file that becomes the symlink
* `target`: requested link target

Returns: 1 when the target is safe, 0 otherwise

Calls: [path_at_or_below](../../libatalk/util/unix.c.md#path_at_or_below), [path_has_dot_component](../../libatalk/util/unix.c.md#path_has_dot_component), [realpath_safe](../../libatalk/util/unix.c.md#realpath_safe), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [setfilparams](file.c.md#setfilparams)

### check_delete_inhibit

```c
int check_delete_inhibit(const struct vol *vol, int dirfd, struct path *path)
```

Defined at lines 213 to 260.

Check whether AFP DeleteInhibit forbids a destructive operation.

Read the in-core header when a fork with loaded metadata is held. A held fork with a data descriptor reads the metadata through its own adouble, whose descriptors serve the read, because closing a second descriptor of the file would drop the fork's locks. Otherwise use a private [ad_metadata()](../../libatalk/adouble/ad_open.c.md#ad_metadata). Metadata read errors fail closed.

Parameters:
* `vol`: volume containing the file
* `dirfd`: parent directory fd, or -1 for the current directory
* `path`: file to check, its stat filled when missing

Returns: AFP_OK when replacement is allowed, AFPERR_OLOCK when DeleteInhibit is set, or AFPERR_ACCESS when the metadata cannot be read safely

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_metadataat](../../libatalk/adouble/ad_open.c.md#ad_metadataat), [of_findnameat](ofork.c.md#of_findnameat)

Called by: [deletefile](file.c.md#deletefile), [of_closefork](ofork.c.md#of_closefork), [setfilparams](file.c.md#setfilparams)

### replace_with_symlink

```c
static int replace_with_symlink(const char *path, const char *target)
```

Defined at lines 275 to 311.

Atomically replace a file with a symbolic link.

Stage the symlink beside its destination and rename it over the file. symlink() supplies O_EXCL-like collision handling, so an existing temporary name is never overwritten. If staging or rename fails, the original file remains in place and the temporary symlink is removed when possible.

Parameters:
* `path`: path of the file to replace
* `target`: symbolic link target

Returns: 0 on success, -1 on error with errno set

Calls: [netatalk_unlink](../../libatalk/vfs/unix.c.md#netatalk_unlink)

Called by: [setfilparams](file.c.md#setfilparams)

### default_type

```c
static int default_type(void *finder)
```

Defined at lines 320 to 327.

Whether Finder info carries no real type and creator.

Parameters:
* `finder`: Finder info

Returns: 1 when type and creator are all zero or the old TEXTUNIX default, 0 otherwise

Called by: [get_finderinfo](file.c.md#get_finderinfo), [setfilparams](file.c.md#setfilparams)

Uses file-scope variables: `old_ufinderi`, `ufinderi`

### get_finderinfo

```c
void * get_finderinfo(const struct vol *vol, const char *upath, struct adouble *adp, void *data, int islink)
```

Defined at lines 346 to 409.

Fill data with the Finder info of a file.

The AppleDouble FinderInfo entry serves first. Without one, data starts as the default Finder info, and a dot file on a volume with AFPVOL_INV_DOTS gets the invisible flag. On macOS an ea volume then consults the native Finder info EA. When the caller passes islink and the volume follows symlinks, the info marks an alias of type slnk and creator rhap. When the type stays the default, the extension map supplies type and creator.

Parameters:
* `vol`: volume of the file
* `upath`: Unix name of the file
* `adp`: AppleDouble with metadata, or NULL
* `data`: receives ADEDLEN_FINDERI bytes
* `islink`: nonzero when the file is a symlink

Returns: data

Calls: [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [default_type](file.c.md#default_type), [getextmap](../../libatalk/util/netatalk_conf.c.md#getextmap)

Called by: [getmetadata](file.c.md#getmetadata), [unpack_finderinfo](catsearch.c.md#unpack_finderinfo)

Uses file-scope variables: `ufinderi`

### set_name

```c
char * set_name(const struct vol *vol, char *data, char *name, char *uname, cnid_t id, bool utf8)
```

Defined at lines 427 to 471.

Append a name in AFP wire format to data.

A Mac name is a Pascal string of up to 31 bytes, the MACFILELEN limit. A Mac name request on a session with UTF-8 names converts the Unix name to the Mac charset. A UTF-8 name carries the text encoding hint of the volume and a two-byte length, limited to 255 bytes.

Parameters:
* `vol`: volume of the name
* `data`: buffer position to append at
* `name`: name to append, in the session's encoding
* `uname`: Unix name of the object
* `id`: CNID, for the name conversion
* `utf8`: true for the UTF-8 format, false for a Mac name

Returns: the buffer position after the name

Calls: [utompath](desktop.c.md#utompath)

Called by: [getdirparams](directory.c.md#getdirparams), [getmetadata](file.c.md#getmetadata), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

### get_id

```c
uint32_t get_id(struct vol *vol, struct adouble *adp, const struct stat *st, const cnid_t did, const char *upath, const int len)
```

Defined at lines 506 to 597.

CNID of a file or directory, from the CNID database and the AppleDouble header.

The AppleDouble header supplies the CNID it remembers, and cnid_add looks up or creates the database record with that hint. When the two differ, the header gets the database CNID. A failed cnid_add sets afp_errno, and a table reset also triggers cnid_volume_reset. A broken database connection ends the process.

Parameters:
* `vol`: volume of the object
* `adp`: AppleDouble of the object, or NULL
* `st`: stat of the object
* `did`: parent DID
* `upath`: Unix name of the object
* `len`: length of upath

Returns: the CNID in network byte order, or CNID_INVALID without a CNID database or on failure

Calls: [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getid](../../libatalk/adouble/ad_attr.c.md#ad_getid), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_add](../../libatalk/cnid/cnid.c.md#cnid_add), [cnid_volume_reset](volume.c.md#cnid_volume_reset), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [getcwdpath](../../libatalk/util/unix.c.md#getcwdpath)

Called by: [afp_createfile](file.c.md#afp_createfile), [afp_createid](file.c.md#afp_createid), [afp_openfork](fork.c.md#afp_openfork), [copyfile](file.c.md#copyfile), [crit_mac_name](catsearch.c.md#crit_mac_name), [dir_add](directory.c.md#dir_add), [getmetadata](file.c.md#getmetadata), [setfilparams](file.c.md#setfilparams)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### getmetadata

```c
int getmetadata(const AFPObj *obj, struct vol *vol, uint16_t bitmap, struct path *path, struct dir *dir, char *buf, size_t *buflen, struct adouble *adp)
```

Defined at lines 625 to 1140.

Pack the file parameters that bitmap selects into buf.

The parameters follow the bit order of the AFP file bitmap. A request for the file number or a name first settles the CNID. A dircache entry whose inode matches provides it. Otherwise get_id assigns one, and a new dircache entry records the file. With a stat from an open fork, the identity follows the held object, even when its name now holds another file.

Attributes, dates, Finder info, and the resource fork length come from adp when the caller passes one. Before AFP 2.0 the no-rename and no-delete attributes also set the no-write attribute. A request with FILPBIT_PDINFO before AFP 3.0 comes from a ProDOS client and gets dates in local time.

Parameters:
* `obj`: session object
* `vol`: volume of the file
* `bitmap`: AFP file bitmap, FILPBIT_* bits
* `path`: path of the file with its stat, its id and d_cached updated
* `dir`: directory that holds the file
* `buf`: reply buffer receiving the parameters
* `buflen`: receives the bytes written
* `adp`: AppleDouble with the metadata of the file, or NULL

Returns: AFP_OK, AFPERR_BITMAP for an unknown bit, AFPERR_NOOBJ, AFPERR_MISC, or the afp_errno of get_id

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [accessmode](unix.c.md#accessmode), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_forcegetid](../../libatalk/adouble/ad_attr.c.md#ad_forcegetid), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_getdate](../../libatalk/adouble/ad_date.c.md#ad_getdate), [ad_getid](../../libatalk/adouble/ad_attr.c.md#ad_getid), [ad_reso_size](../../libatalk/adouble/ad_open.c.md#ad_reso_size), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_free](directory.c.md#dir_free), [dir_new](directory.c.md#dir_new), [dir_remove](directory.c.md#dir_remove), [dircache_add](dircache.c.md#dircache_add), [fullpath_join](directory.c.md#fullpath_join), [get_finderinfo](file.c.md#get_finderinfo), [get_id](file.c.md#get_id), [ostat](../../libatalk/util/unix.c.md#ostat), [path_cached_file](../../include/atalk/directory.h.md#path_cached_file), [path_resolve_cached_file](file.c.md#path_resolve_cached_file), [set_name](file.c.md#set_name), [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [utompath](desktop.c.md#utompath)

Called by: [getfilparams](file.c.md#getfilparams), [getforkparams](fork.c.md#getforkparams)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### getfilparams

```c
int getfilparams(const AFPObj *obj, struct vol *vol, uint16_t bitmap, struct path *path, struct dir *dir, char *buf, size_t *buflen, int skip_fork_check)
```

Defined at lines 1165 to 1232.

Pack the file parameters that bitmap selects and open the metadata they need.

Bits that need the AppleDouble use the metadata of an open fork when there is one, and the dircache-backed ad_metadata_cached otherwise. Attribute requests also look for open forks of other processes, unless skip_fork_check spares those syscalls during an enumeration. A metadata EACCES fails the call, and any other metadata error leaves the file without AppleDouble data. The getmetadata call then packs the parameters.

Parameters:
* `obj`: session object
* `vol`: volume of the file
* `bitmap`: AFP file bitmap, FILPBIT_* bits
* `path`: path of the file with its stat
* `dir`: directory that holds the file
* `buf`: reply buffer receiving the parameters
* `buflen`: receives the bytes written
* `skip_fork_check`: nonzero to skip the open fork test, as enumerate does

Returns: AFP_OK, AFPERR_ACCESS when the metadata is unreadable, or the result of getmetadata

Calls: [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_loaded](../../include/atalk/adouble.h.md#ad_meta_loaded), [ad_metadata_cached](ad_cache.c.md#ad_metadata_cached), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [getmetadata](file.c.md#getmetadata), [of_ad](ofork.c.md#of_ad), [path_resolve_cached_file](file.c.md#path_resolve_cached_file)

Called by: [afp_getappl](appl.c.md#afp_getappl), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_resolveid](file.c.md#afp_resolveid), [enumerate](enumerate.c.md#enumerate), [rslt_add](catsearch.c.md#rslt_add)

Mentioned in the documentation of: [crit_mac_name](catsearch.c.md#crit_mac_name)

### link_target_in

```c
static bool link_target_in(const char *upath, const struct stat *folder)
```

Defined at lines 1241 to 1265.

Tell whether the file a link names is an entry of a folder.

Parameters:
* `upath`: name of a link in the current folder
* `folder`: stat of the folder

Returns: true when the link's target sits in the folder

Calls: [realpath_safe](../../libatalk/util/unix.c.md#realpath_safe)

Called by: [afp_createfile](file.c.md#afp_createfile)

### afp_createfile

```c
int afp_createfile(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1286 to 1494.

FPCreateFile: create a file with its AppleDouble metadata.

A hard create, any nonzero flag byte, truncates an existing file. A soft create fails with AFPERR_EXIST when the name exists. A file that this session holds open fails with AFPERR_BUSY for a hard create and AFPERR_EXIST for a soft one. The call writes the Mac name and the CNID into the metadata and records the file in the dircache. FCE and Spotlight learn about the new file, and sibling afpd processes get a refresh hint for the parent.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_EXIST, AFPERR_BUSY, AFPERR_VLOCK, AFPERR_ACCESS, AFPERR_DFULL, AFPERR_BADTYPE for a directory path, or another AFPERR_* code

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [ad_store_to_cache](ad_cache.c.md#ad_store_to_cache), [cname](directory.c.md#cname), [dir_event_path](directory.c.md#dir_event_path), [dir_free](directory.c.md#dir_free), [dir_new](directory.c.md#dir_new), [dir_stat_refresh](directory.c.md#dir_stat_refresh), [dircache_add](dircache.c.md#dircache_add), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [fullpath_join](directory.c.md#fullpath_join), [get_afp_errno](directory.c.md#get_afp_errno), [get_id](file.c.md#get_id), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [link_target_in](file.c.md#link_target_in), [netatalk_unlink](../../libatalk/vfs/unix.c.md#netatalk_unlink), [of_findname](ofork.c.md#of_findname), [setvoltime_entry](volume.c.md#setvoltime_entry), [sl_index_event](spotlight.c.md#sl_index_event), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_setfilparams

```c
int afp_setfilparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1513 to 1588.

FPSetFileParms: set the parameters of a file.

A directory path fails with AFPERR_BADTYPE and a missing file with AFPERR_NOOBJ. A dircache entry whose inode no longer matches means that something outside this afpd replaced the file. The call then drops the entry and fails with AFPERR_NOOBJ. The parameters start at the next even offset of the request, and success updates the volume modification time.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_VLOCK for a read-only volume, or another AFPERR_* code

Calls: [cname](directory.c.md#cname), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [setfilparams](file.c.md#setfilparams), [setvoltime](volume.c.md#setvoltime), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### setfilparams

```c
int setfilparams(const AFPObj *obj, struct vol *vol, struct path *path, uint16_t f_bitmap, char *buf)
```

Defined at lines 1619 to 2073.

Apply the file parameters that f_bitmap selects from buf.

Without UNIX privileges the user needs write access to the parent directory. A first pass applies the UNIX privileges and owner and converts a file into a symlink, all without the AppleDouble. A second pass with the metadata open sets attributes, dates, Finder info, and ProDOS info.

On a volume that does not follow symlinks, Finder info of type slnk and creator rhap turns a regular file into a symlink. The link target is the path that the file contains, and symlink_target_safe guards it. Date and Finder info changes update the file modification time. Privilege changes and a changed invisible attribute also update the parent. Sibling afpd processes get a refresh hint.

Parameters:
* `obj`: session object
* `vol`: volume of the file
* `path`: file, restated after a symlink conversion
* `f_bitmap`: AFP file bitmap, FILPBIT_* bits
* `buf`: parameter block in bitmap order

Returns: AFP_OK, AFPERR_ACCESS, AFPERR_BITMAP, AFPERR_MISC, or another AFPERR_* code

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setattr](../../libatalk/adouble/ad_attr.c.md#ad_setattr), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [check_access](directory.c.md#check_access), [check_delete_inhibit](file.c.md#check_delete_inhibit), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [default_type](file.c.md#default_type), [dir_modify](directory.c.md#dir_modify), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [get_id](file.c.md#get_id), [getdefextmap](../../libatalk/util/netatalk_conf.c.md#getdefextmap), [getextmap](../../libatalk/util/netatalk_conf.c.md#getextmap), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_ad](ofork.c.md#of_ad), [of_stat](ofork.c.md#of_stat), [ostat](../../libatalk/util/unix.c.md#ostat), [replace_with_symlink](file.c.md#replace_with_symlink), [set_utc_offset](../../libatalk/adouble/ad_date.c.md#set_utc_offset), [setdirparams](directory.c.md#setdirparams), [setfilowner](unix.c.md#setfilowner), [setfilunixmode](unix.c.md#setfilunixmode), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [symlink_target_safe](file.c.md#symlink_target_safe)

Called by: [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [afp_setfilparams](file.c.md#afp_setfilparams)

Uses file-scope variables: `Cur_Path` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md), `ufinderi`

### renamefile

```c
int renamefile(struct vol *vol, struct dir *ddir, int sdir_fd, char *src, char *dst, char *newname, struct adouble *adp)
```

Defined at lines 2096 to 2176.

Rename a file together with its metadata and Mac name.

The caller cleans the cache. EXDEV makes the call duplicate the file with copyfile and delete the source. A file with open forks refuses that path with AFPERR_OLOCK. When the VFS modules fail to follow the rename, the data fork rename goes back. The metadata then gets the new Mac name, and a metadata file that fails to open needs nothing.

Parameters:
* `vol`: volume of the file
* `ddir`: destination directory
* `sdir_fd`: directory that src is relative to, or -1 when src is a full path
* `src`: current Unix path
* `dst`: new Unix name in the working directory
* `newname`: new Mac name
* `adp`: AppleDouble of the source from its open fork, or a zeroed one

Returns: AFP_OK, AFPERR_NOOBJ, AFPERR_ACCESS, AFPERR_VLOCK, AFPERR_OLOCK, AFPERR_PARAM, or the error of copyfile or deletefile

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [copyfile](file.c.md#copyfile), [deletefile](file.c.md#deletefile), [unix_rename](../../libatalk/vfs/unix.c.md#unix_rename)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [moveandrename](filedir.c.md#moveandrename)

Calls through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_renamefile_adouble](../../libatalk/vfs/vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](../../libatalk/vfs/vfs.c.md#rf_renamefile_ea), [ea_renamefile](../../libatalk/vfs/ea_ad.c.md#ea_renamefile), [vfs_renamefile](../../libatalk/vfs/vfs.c.md#vfs_renamefile)

### mtoUTF8

```c
size_t mtoUTF8(const struct vol *vol, const char *src, size_t srclen, char *dest, size_t destlen)
```

Defined at lines 2191 to 2202.

Convert a Mac charset name to decomposed UTF-8.

The Mac charset is the one that the volume configures.

Parameters:
* `vol`: volume supplying the Mac charset
* `src`: name in the Mac charset
* `srclen`: bytes in src
* `dest`: receives the UTF-8 name
* `destlen`: capacity of dest

Returns: the length of the UTF-8 name, or (size_t)-1 when the conversion failed

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string)

Called by: [cname_mtouname](directory.c.md#cname_mtouname), [copy_path_name](file.c.md#copy_path_name)

### copy_path_name

```c
int copy_path_name(const struct vol *vol, char *newname, char *ibuf)
```

Defined at lines 2216 to 2277.

Extract a new file name from an AFP path field.

Type 2 carries a Pascal string in the Mac charset, converted to UTF-8 from AFP 3.0 on. Type 3 carries a hint and a UTF-8 name and needs AFP 3.0. A name with an embedded NUL is a path, not a file name, and fails.

Parameters:
* `vol`: volume of the request
* `newname`: receives the name, with room for AFPOBJ_TMPSIZ bytes
* `ibuf`: request cursor at the path type byte

Returns: the name length, 0 for an empty name, or -1 on failure

Calls: [mtoUTF8](file.c.md#mtoutf8)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_moveandrename](filedir.c.md#afp_moveandrename), [afp_rename](filedir.c.md#afp_rename)

### afp_copyfile

```c
int afp_copyfile(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2297 to 2482.

FPCopyFile: duplicate a file, possibly onto another volume.

The request names the source by volume, DID, and path, then the destination directory by volume, DID, and path, and the new name. A source open with a deny-read mode, or one this session cannot open read-only with share modes, fails with AFPERR_DENYCONF. A source fork that this session holds keeps its descriptors, so its locks survive. The copyfile call creates the new file. FCE, Spotlight, and sibling afpd processes then learn about it.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_DENYCONF, AFPERR_VLOCK for a read-only destination volume, AFPERR_PARAM, or another AFPERR_* code

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_testlock](../../libatalk/adouble/ad_lock.c.md#ad_testlock), [cname](directory.c.md#cname), [copy_path_name](file.c.md#copy_path_name), [copyfile](file.c.md#copyfile), [ctoupath](filedir.c.md#ctoupath), [dir_event_path](directory.c.md#dir_event_path), [dirlookup](directory.c.md#dirlookup), [dirlookup_strict](directory.c.md#dirlookup_strict), [fce_register](fce_api.c.md#fce_register), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [mtoupath](desktop.c.md#mtoupath), [of_ad](ofork.c.md#of_ad), [of_findname](ofork.c.md#of_findname), [path_error](directory.c.md#path_error), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [setvoltime_entry](volume.c.md#setvoltime_entry), [sl_index_event](spotlight.c.md#sl_index_event), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### copyfile

```c
int copyfile(struct vol *s_vol, struct vol *d_vol, struct dir *d_dir, int sfd, char *src, char *dst, char *newname, struct adouble *adp, int held)
```

Defined at lines 2504 to 2692.

Duplicate a file with its metadata and resource fork at a new path.

The AppleDouble layer transfers the data fork. The destination volume's VFS transfers the resource fork and the extended attributes. With newname NULL, as called from [directory.c](directory.c.md), the caller transfers the resource fork and adp is NULL.

Parameters:
* `s_vol`: source volume
* `d_vol`: destination volume
* `d_dir`: destination directory
* `sfd`: directory descriptor src is relative to, or -1
* `src`: source path
* `dst`: destination path
* `newname`: Mac name of the new file, or NULL to leave the resource fork alone
* `adp`: open AppleDouble of the source, or NULL to open it here
* `held`: adp belongs to an open fork and must stay open afterwards

Returns: AFP_OK, AFPERR_EXIST when dst exists, or another AFPERR_* code

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_copy_header](../../libatalk/adouble/ad_flush.c.md#ad_copy_header), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_openat](../../libatalk/adouble/ad_open.c.md#ad_openat), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [copy_fork](../../libatalk/adouble/ad_write.c.md#copy_fork), [deletefile](file.c.md#deletefile), [get_id](file.c.md#get_id)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [copydir](directory.c.md#copydir), [renamefile](file.c.md#renamefile)

Calls through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_copyfile_adouble](../../libatalk/vfs/vfs.c.md#rf_copyfile_adouble), [RF_copyfile_ea](../../libatalk/vfs/vfs.c.md#rf_copyfile_ea), [ea_copyfile](../../libatalk/vfs/ea_ad.c.md#ea_copyfile), [sys_ea_copyfile](../../libatalk/vfs/ea_sys.c.md#sys_ea_copyfile), [vfs_copyfile](../../libatalk/vfs/vfs.c.md#vfs_copyfile)

### deletefile

```c
int deletefile(const struct vol *vol, int dirfd, char *file, int checkAttrib, cnid_t *idp)
```

Defined at lines 2730 to 2839.

Delete a file and its AppleDouble metadata.

Refuses while another process holds a deny mode or a byte-range lock on the file. The data file goes first, so a refused unlink leaves the AppleDouble file in place; a failed AppleDouble removal after it is only logged. With checkAttrib set, deletefile honors the delete-inhibit attribute and deletes the CNID of the file afterwards.

Parameters:
* `vol`: volume the file belongs to
* `dirfd`: directory descriptor file is relative to, or -1
* `file`: file name
* `checkAttrib`: honor kFPDeleteInhibitBit and delete the CNID, as afp_delete does
* `idp`: known CNID of the file or CNID_INVALID, receives the CNID deleted, may be NULL

Returns: AFP_OK, AFPERR_NOOBJ, AFPERR_BUSY, AFPERR_ACCESS, or another AFPERR_* code

Calls: [check_delete_inhibit](file.c.md#check_delete_inhibit), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [netatalk_unlinkat](../../libatalk/vfs/unix.c.md#netatalk_unlinkat), [of_get_locks](ofork.c.md#of_get_locks)

Called by: [afp_delete](filedir.c.md#afp_delete), [copyfile](file.c.md#copyfile), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [of_closefork](ofork.c.md#of_closefork), [renamefile](file.c.md#renamefile)

Calls through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletefile_adouble](../../libatalk/vfs/vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](../../libatalk/vfs/vfs.c.md#rf_deletefile_ea), [ea_deletefile](../../libatalk/vfs/ea_ad.c.md#ea_deletefile), [vfs_deletefile](../../libatalk/vfs/vfs.c.md#vfs_deletefile)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

Mentioned in the documentation of: [of_delete_blocked](ofork.c.md#of_delete_blocked)

### afp_createid

```c
int afp_createid(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2853 to 2940.

FPCreateID: return a file's CNID as its file ID.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, receives the file ID
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK with the new file ID in rbuf, AFPERR_EXISTID with the existing ID in rbuf, or another AFPERR_* code

Calls: [cname](directory.c.md#cname), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [get_id](file.c.md#get_id), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_stat](ofork.c.md#of_stat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### reenumerate_loop

```c
static int reenumerate_loop(struct dirent *de, char *mname, void *data)
```

Defined at lines 2960 to 2988.

Register one directory entry with the CNID database.

Errors of cnid_add pass silently, except a table reset. A reset invalidates every CNID of the session, so the call triggers cnid_volume_reset and stops the walk.

Parameters:
* `de`: directory entry
* `mname`: unused
* `data`: struct reenum with the volume and the directory DID

Returns: 0, or -1 after a table reset

Calls: [cnid_add](../../libatalk/cnid/cnid.c.md#cnid_add), [cnid_volume_reset](volume.c.md#cnid_volume_reset), [ostat](../../libatalk/util/unix.c.md#ostat)

Called by: [reenumerate_id](file.c.md#reenumerate_id)

### reenumerate_id

```c
static int reenumerate_id(struct vol *vol, char *name, struct dir *dir)
```

Defined at lines 3006 to 3035.

Register every entry of a directory with the CNID database after a stale lookup.

The afp_resolveid handler runs it when a CNID resolves to a name that no longer exists. A directory whose ctime still matches the last reenumeration needs no second pass. That keeps a deleted file from triggering the walk again and again.

Parameters:
* `vol`: volume of the directory
* `name`: directory path, "." for the working directory
* `dir`: dircache entry of the directory

Returns: the number of entries, or -1 without a CNID database or after a failure

Calls: [dirreenumerate](directory.c.md#dirreenumerate), [for_each_dirent](enumerate.c.md#for_each_dirent), [ostat](../../libatalk/util/unix.c.md#ostat), [reenumerate_loop](file.c.md#reenumerate_loop), [setdiroffcnt](directory.c.md#setdiroffcnt)

Called by: [afp_resolveid](file.c.md#afp_resolveid)

### afp_resolveid

```c
int afp_resolveid(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3057 to 3200.

FPResolveID: return the parameters of the file that a file ID names.

The volume needs a persistent CNID database, otherwise the call fails with AFPERR_NOOP. ID 0 fails at once with AFPERR_NOID, because Mac OS sends such requests in bulk after a catalog search. When the resolved name no longer exists, reenumerate_id refreshes the directory once and the lookup runs again. A directory fails with AFPERR_NOID from AFP 3.0 on and with AFPERR_BADTYPE before. The reply carries the bitmap and the file parameters.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_NOID, AFPERR_NOOP, AFPERR_ACCESS, AFPERR_MISC when the CNID backend failed, or AFPERR_PARAM

Calls: [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [dirlookup](directory.c.md#dirlookup), [getfilparams](file.c.md#getfilparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [movecwd](directory.c.md#movecwd), [of_stat](ofork.c.md#of_stat), [reenumerate_id](file.c.md#reenumerate_id), [utompath](desktop.c.md#utompath)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_deleteid

```c
int afp_deleteid(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3219 to 3335.

FPDeleteID: delete the CNID record that a file ID names.

The volume needs a persistent CNID database, and a read-only volume fails with AFPERR_VLOCK. The call also deletes the record of a file ID whose object no longer exists, and then replies AFPERR_NOOBJ. A directory fails with AFPERR_BADTYPE.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_NOOBJ when only the record remained, AFPERR_NOID, AFPERR_NOOP, AFPERR_VLOCK, AFPERR_BADTYPE, AFPERR_MISC when the CNID backend failed, or AFPERR_PARAM

Calls: [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve), [dirlookup](directory.c.md#dirlookup), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [movecwd](directory.c.md#movecwd), [ostat](../../libatalk/util/unix.c.md#ostat)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### find_adouble

```c
static struct adouble * find_adouble(const AFPObj *obj, struct vol *vol, struct path *path, struct ofork **of, struct adouble *adp)
```

Defined at lines 3354 to 3405.

AppleDouble of an FPExchangeFiles operand after the access tests.

A failed stat maps to AFPERR_NOID, AFPERR_ACCESS, or AFPERR_PARAM in afp_errno. The user needs write access to the file. A file that this session holds open lends the AppleDouble of its fork, so its locks survive. Otherwise the call opens the metadata read-write. A resource fork without write access fails, because the AFP specification demands read and write privileges on both files.

Parameters:
* `obj`: session object
* `vol`: volume of the file
* `path`: file with its stat
* `of`: receives the open fork of the file, or NULL
* `adp`: AppleDouble to open when no fork holds the file

Returns: the AppleDouble to use, or NULL with afp_errno set

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [file_access](directory.c.md#file_access), [of_findname](ofork.c.md#of_findname)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

### afp_exchangefiles

```c
int afp_exchangefiles(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 3427 to 3890.

FPExchangeFiles: swap the contents of two files while each name keeps its file ID.

Both files must be distinct regular files on the same device that the user may write. Three renames through a .AppleTemp file in the destination directory swap the data and metadata. The CNID records, the AppleDouble IDs, and the permissions then swap back, so each name keeps its file ID and permissions. A failure undoes the renames.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_SAMEOBJ, AFPERR_BADTYPE, AFPERR_NOOBJ, AFPERR_ACCESS, AFPERR_VLOCK, AFPERR_MISC across devices, or another AFPERR_* code

Calls: [absupath](filedir.c.md#absupath), [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_copy_header](../../libatalk/adouble/ad_flush.c.md#ad_copy_header), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_init_offsets](../../libatalk/adouble/ad_open.c.md#ad_init_offsets), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [become_root](../../libatalk/util/unix.c.md#become_root), [cname](directory.c.md#cname), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup_strict](directory.c.md#dirlookup_strict), [find_adouble](file.c.md#find_adouble), [get_afp_errno](directory.c.md#get_afp_errno), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_rename](ofork.c.md#of_rename), [ostat](../../libatalk/util/unix.c.md#ostat), [path_isadir](../../include/atalk/directory.h.md#path_isadir), [renamefile](file.c.md#renamefile), [setfilowner](unix.c.md#setfilowner), [setfilunixmode](unix.c.md#setfilunixmode), [setvoltime_entry](volume.c.md#setvoltime_entry), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Types

### struct reenum

Defined at line 2943.
* `struct vol * vol`
* `cnid_t did`

# Macros

* Undocumented: `APPLETEMP`, `PARAM_NEED_ADP`

# File-scope variables

`old_ufinderi`, `ufinderi`
