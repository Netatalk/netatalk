---
type: C Source File
title: "libatalk/vfs/ea_ad.c"
description: "Store Extended Attributes inside .AppleDouble folders as follows:"
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/ea_ad.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-26T21:19:58+01:00 }
---

Part of the [libatalk/vfs](../vfs.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `dirent.h`, `errno.h`, `fcntl.h`, `stdint.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### ea_header_mode

```c
static mode_t ea_header_mode(mode_t mode)
```

Defined at lines 61 to 68.

Mode of the EA header file derived from the file mode.

Execute bits go away, and the owner always keeps read and write, so a chmod from 0000 still opens and write-locks the header.

Parameters:
* `mode`: file mode

Returns: the header file mode

Called by: [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir), [ea_chmod_file](ea_ad.c.md#ea_chmod_file)

### ea_mode

```c
static mode_t ea_mode(mode_t mode)
```

Defined at lines 78 to 83.

Mode of an EA data file derived from the file mode.

Execute bits go away.

Parameters:
* `mode`: file mode

Returns: the EA file mode

Called by: [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir), [ea_chmod_file](ea_ad.c.md#ea_chmod_file)

### mtoupath

```c
static char * mtoupath(const struct vol *vol, const char *mpath)
```

Defined at lines 96 to 127.

Convert a Mac name to the volume encoding with hex escapes.

The result lives in a static buffer that the next call overwrites. An empty name yields ".". It mirrors the [desktop.c](../../etc/afpd/desktop.c.md) helper of the same name.

Parameters:
* `vol`: volume supplying the character sets
* `mpath`: name in CH_UTF8_MAC, or NULL

Returns: the converted name in a static buffer, "." for an empty name, or NULL on failure

Calls: [convert_charset](../unicode/charcnv.c.md#convert_charset)

Called by: [ea_path](ea_ad.c.md#ea_path)

### ea_attrname_invalid

```c
static int ea_attrname_invalid(const char *func, const char *attruname)
```

Defined at lines 137 to 145.

Reject a missing EA name with a log line that names the calling function.

Parameters:
* `func`: name of the calling function, for the log line
* `attruname`: EA name to test

Returns: 1 when attruname is NULL, 0 otherwise

Called by: [delete_ea_file](ea_ad.c.md#delete_ea_file), [ea_addentry](ea_ad.c.md#ea_addentry), [ea_delentry](ea_ad.c.md#ea_delentry), [get_eacontent](ea_ad.c.md#get_eacontent), [get_easize](ea_ad.c.md#get_easize), [remove_ea](ea_ad.c.md#remove_ea), [set_ea](ea_ad.c.md#set_ea), [write_ea](ea_ad.c.md#write_ea)

### unpack_header

```c
static int unpack_header(struct ea *ea)
```

Defined at lines 157 to 238.

unpack and verify header file data buffer at ea->ea_data into struct ea

Parameters:
* `ea`: handle to struct ea

Returns: 0 on success, -1 on error

Note: Verifies magic and version.

Called by: [ea_open](ea_ad.c.md#ea_open)

### pack_header

```c
static int pack_header(struct ea *ea)
```

Defined at lines 249 to 322.

pack everything from struct ea into buffer at ea->ea_data

Parameters:
* `ea`: handle to struct ea

Returns: 0 on success, -1 on error

Note: adjust ea->ea_count in case an ea entry deletetion is detected

Called by: [ea_close](ea_ad.c.md#ea_close)

### ea_addentry

```c
static int ea_addentry(struct ea *ea, const char *attruname, size_t attrsize, int bitmap)
```

Defined at lines 340 to 423.

add one EA into ea->ea_entries[]

Parameters:
* `ea`: pointer to struct ea
* `attruname`: name of EA
* `attrsize`: size of ea
* `bitmap`: bitmap from FP func

Returns: new number of EA entries, -1 on misc error, -2 if EA exists and kXCreateAttr (passed as O_CREAT) flag set, -3 if no EA exists and kXAttrReplace (passed as O_TRUNC) flag set

Note: Grow array ea->ea_entries[]. If ea->ea_entries is still NULL, start allocating. Otherwise realloc and put entry at the end. Increments ea->ea_count.

Calls: [ea_attrname_invalid](ea_ad.c.md#ea_attrname_invalid)

Called by: [ea_copyfile](ea_ad.c.md#ea_copyfile), [ea_renamefile](ea_ad.c.md#ea_renamefile), [set_ea](ea_ad.c.md#set_ea)

### write_ea

```c
static int write_ea(const struct ea *ea, const char *attruname, const char *ibuf, size_t attrsize)
```

Defined at lines 437 to 489.

write an EA to disk

Parameters:
* `ea`: struct ea handle
* `attruname`: EA name
* `ibuf`: buffer with EA content
* `attrsize`: size of EA

Returns: 0 on success, -1 on error

Note: Creates/overwrites EA file.

Calls: [ea_attrname_invalid](ea_ad.c.md#ea_attrname_invalid), [ea_path](ea_ad.c.md#ea_path)

Called by: [set_ea](ea_ad.c.md#set_ea)

### ea_delentry

```c
static int ea_delentry(struct ea *ea, const char *attruname)
```

Defined at lines 503 to 534.

delete one EA from ea->ea_entries[]

Parameters:
* `ea`: pointer to struct ea
* `attruname`: EA name

Returns: new number of EA entries, -1 on error

Note: Remove entry from ea->ea_entries[]. Decrement ea->ea_count. Marks it as unused just by freeing name and setting it to NULL. ea_close and pack_buffer must honor this.

Calls: [ea_attrname_invalid](ea_ad.c.md#ea_attrname_invalid)

Called by: [ea_renamefile](ea_ad.c.md#ea_renamefile), [remove_ea](ea_ad.c.md#remove_ea), [set_ea](ea_ad.c.md#set_ea)

### delete_ea_file

```c
static int delete_ea_file(const struct ea *ea, const char *eaname)
```

Defined at lines 545 to 572.

delete EA file from disk

Parameters:
* `ea`: struct ea handle
* `eaname`: EA name

Returns: 0 when no EA file is left under the name, -1 for a missing name or a failed unlink

Calls: [ea_attrname_invalid](ea_ad.c.md#ea_attrname_invalid), [ea_path](ea_ad.c.md#ea_path)

Called by: [ea_deletefile](ea_ad.c.md#ea_deletefile), [remove_ea](ea_ad.c.md#remove_ea), [set_ea](ea_ad.c.md#set_ea)

### ea_path

```c
char * ea_path(const struct ea *ea, const char *eaname, int macname)
```

Defined at lines 592 to 622. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

return name of ea header filename

Parameters:
* `ea`: ea handle
* `eaname`: name of EA or NULL
* `macname`: if != 0 call mtoupath on eaname

Returns: pointer to name in static buffer, NULL on error

Note: - Calls ad_open, copies buffer, appends "::EA" and if supplied append eanme * Files: "file" -> "file/.AppleDouble/file::EA"
* Dirs: "dir" -> "dir/.AppleDouble/.Parent::EA"
* "file" with EA "myEA" -> "file/.AppleDouble/file::EA:myEA"

Calls: [mtoupath](ea_ad.c.md#mtoupath), [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [check_eafiles](../../bin/dbd/cmd_dbd_scanvol.c.md#check_eafiles), [delete_ea_file](ea_ad.c.md#delete_ea_file), [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir), [ea_chmod_file](ea_ad.c.md#ea_chmod_file), [ea_chown](ea_ad.c.md#ea_chown), [ea_close](ea_ad.c.md#ea_close), [ea_copyfile](ea_ad.c.md#ea_copyfile), [ea_open](ea_ad.c.md#ea_open), [ea_renamefile](ea_ad.c.md#ea_renamefile), [get_eacontent](ea_ad.c.md#get_eacontent), [write_ea](ea_ad.c.md#write_ea)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

### ea_open

```c
int ea_open(const struct vol *vol, const char *uname, eaflags_t eaflags, struct ea *ea)
```

Defined at lines 645 to 795. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

open EA header file, create if it doesnt exits and called with O_CREATE

Parameters:
* `vol`: current volume
* `uname`: filename for which we have to open a header
* `eaflags`: flag to control open behavior: * EA_CREATE: create if it doesn't exist (without it won't be created)
* EA_RDONLY: open read only
* EA_RDWR: open read/write
* Either EA_RDONLY or EA_RDWR MUST be requested
* `ea`: pointer to a struct ea that we fill

Returns: 0 on success, -1 on misc error with errno = EFAULT, -2 if no EA header exists with errno = ENOENT

Note: opens header file and stores fd in ea->ea_fd. Size of file is put into ea->ea_size. number of EAs is stored in ea->ea_count. flags are remembered in ea->ea_flags. file is either read or write locked depending on the open flags. When you're done with struct ea you must call ea_close on it.

Calls: [ea_path](ea_ad.c.md#ea_path), [unpack_header](ea_ad.c.md#unpack_header)

Called by: [check_eafiles](../../bin/dbd/cmd_dbd_scanvol.c.md#check_eafiles), [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir), [ea_chmod_file](ea_ad.c.md#ea_chmod_file), [ea_chown](ea_ad.c.md#ea_chown), [ea_copyfile](ea_ad.c.md#ea_copyfile), [ea_openat](ea_ad.c.md#ea_openat), [ea_renamefile](ea_ad.c.md#ea_renamefile), [get_eacontent](ea_ad.c.md#get_eacontent), [get_easize](ea_ad.c.md#get_easize), [list_eas](ea_ad.c.md#list_eas), [remove_ea](ea_ad.c.md#remove_ea), [set_ea](ea_ad.c.md#set_ea)

### ea_openat

```c
int ea_openat(const struct vol *vol, int dirfd, const char *uname, eaflags_t eaflags, struct ea *ea)
```

Defined at lines 819 to 852. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

openat like wrapper for ea_open, takes a additional file descriptor

Parameters:
* `vol`: current volume
* `dirfd`: openat like file descriptor
* `uname`: filename for which we have to open a header
* `eaflags`: flag to control open behavior: * EA_CREATE: create if it doesn't exist (without it won't be created)
* EA_RDONLY: open read only
* EA_RDWR: open read/write
* Either EA_RDONLY or EA_RDWR MUST be requested
* `ea`: pointer to a struct ea that we fill

Returns: 0 on success, -1 on misc error with errno = EFAULT, -2 if no EA header exists with errno = ENOENT

Note: opens header file and stores fd in ea->ea_fd. Size of file is put into ea->ea_size. number of EAs is stored in ea->ea_count. flags are remembered in ea->ea_flags. file is either read or write locked depending on the open flags. When you're done with struct ea you must call ea_close on it.

Calls: [ea_open](ea_ad.c.md#ea_open)

Called by: [ea_copyfile](ea_ad.c.md#ea_copyfile), [ea_deletefile](ea_ad.c.md#ea_deletefile), [ea_renamefile](ea_ad.c.md#ea_renamefile)

### ea_close

```c
int ea_close(struct ea *ea)
```

Defined at lines 864 to 963. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

flushes and closes an ea handle

Parameters:
* `ea`: pointer to ea handle

Returns: 0 on success, -1 on error

Note: Flushes and then closes and frees all resouces held by ea handle. Pack data in ea into ea_data, then write ea_data to disk

Calls: [ea_path](ea_ad.c.md#ea_path), [netatalk_unlinkat](unix.c.md#netatalk_unlinkat), [pack_header](ea_ad.c.md#pack_header), [statat](unix.c.md#statat)

Called by: [check_eafiles](../../bin/dbd/cmd_dbd_scanvol.c.md#check_eafiles), [ea_chmod_dir](ea_ad.c.md#ea_chmod_dir), [ea_chmod_file](ea_ad.c.md#ea_chmod_file), [ea_chown](ea_ad.c.md#ea_chown), [ea_copyfile](ea_ad.c.md#ea_copyfile), [ea_deletefile](ea_ad.c.md#ea_deletefile), [ea_renamefile](ea_ad.c.md#ea_renamefile), [get_eacontent](ea_ad.c.md#get_eacontent), [get_easize](ea_ad.c.md#get_easize), [list_eas](ea_ad.c.md#list_eas), [remove_ea](ea_ad.c.md#remove_ea), [set_ea](ea_ad.c.md#set_ea)

### ea_absent

```c
int ea_absent(const struct vol *vol, char *rbuf, size_t *rbuflen)
```

Defined at lines 980 to 985. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

The reply for an attribute the object lacks: a zero size word.

Parameters:
* `vol`: volume of the object, whose session's AFP version picks the error
* `rbuf`: reply buffer, receives the four zero bytes
* `rbuflen`: reply length, grows by 4

Returns: AFPERR_NOITEM on an AFP 3.4 session, AFPERR_MISC before

Called by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [get_eacontent](ea_ad.c.md#get_eacontent), [get_easize](ea_ad.c.md#get_easize), [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent), [sys_get_easize](ea_sys.c.md#sys_get_easize)

### get_easize

```c
int get_easize(const struct vol *vol, char *rbuf, size_t *rbuflen, const char *uname, int oflag, const char *attruname, int fd)
```

Defined at lines 1002 to 1051. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

get size of an EA

Parameters:
* `vol`: current volume
* `rbuf`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) reply buffer
* `rbuflen`: current length of data in reply buffer
* `uname`: filename
* `oflag`: link and create flag
* `attruname`: name of attribute
* `fd`: file descriptor

Returns: AFP code: AFP_OK on success or appropriate AFP error code

Note: Copies EA size into rbuf in network order. Increments *rbuflen +4.

Calls: [ea_absent](ea_ad.c.md#ea_absent), [ea_attrname_invalid](ea_ad.c.md#ea_attrname_invalid), [ea_close](ea_ad.c.md#ea_close), [ea_open](ea_ad.c.md#ea_open)

Called through [`vfs_ops::vfs_ea_getsize`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [vfs_ea_getsize](vfs.c.md#vfs_ea_getsize)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### get_eacontent

```c
int get_eacontent(const struct vol *vol, char *rbuf, size_t *rbuflen, const char *uname, int oflag, const char *attruname, int maxreply, int fd)
```

Defined at lines 1069 to 1177. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

copy EA into rbuf

Parameters:
* `vol`: current volume
* `rbuf`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) reply buffer
* `rbuflen`: current length of data in reply buffer
* `uname`: filename
* `oflag`: link and create flag
* `attruname`: name of attribute
* `maxreply`: maximum EA size as of current specs/real-life
* `fd`: file descriptor

Returns: AFP code: AFP_OK on success or appropriate AFP error code

Note: Copies EA into rbuf. Increments *rbuflen accordingly.

Calls: [ea_absent](ea_ad.c.md#ea_absent), [ea_attrname_invalid](ea_ad.c.md#ea_attrname_invalid), [ea_close](ea_ad.c.md#ea_close), [ea_open](ea_ad.c.md#ea_open), [ea_path](ea_ad.c.md#ea_path)

Called through [`vfs_ops::vfs_ea_getcontent`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [vfs_ea_getcontent](vfs.c.md#vfs_ea_getcontent)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### list_eas

```c
int list_eas(const struct vol *vol, char *attrnamebuf, size_t *buflen, const char *uname, int oflag, int fd)
```

Defined at lines 1194 to 1255. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

copy names of EAs into attrnamebuf

Parameters:
* `vol`: current volume
* `attrnamebuf`: store names a consecutive C strings here
* `buflen`: length of names in attrnamebuf
* `uname`: filename
* `oflag`: link and create flag
* `fd`: file descriptor

Returns: AFP code: AFP_OK on success or appropriate AFP error code

Note: Copies names of all EAs of uname as consecutive C strings into rbuf. Increments *buflen accordingly.

Calls: [convert_string](../unicode/charcnv.c.md#convert_string), [ea_close](ea_ad.c.md#ea_close), [ea_open](ea_ad.c.md#ea_open)

Called through [`vfs_ops::vfs_ea_list`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [vfs_ea_list](vfs.c.md#vfs_ea_list)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### set_ea

```c
int set_ea(const struct vol *vol, const char *uname, const char *attruname, const char *ibuf, size_t attrsize, int oflag, int fd)
```

Defined at lines 1273 to 1335. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

set a Solaris native EA

Parameters:
* `vol`: current volume
* `uname`: filename
* `attruname`: EA name
* `ibuf`: buffer with EA content
* `attrsize`: length EA in ibuf
* `oflag`: link and create flag
* `fd`: file descriptor

Returns: AFP code: AFP_OK on success or appropriate AFP error code

Note: Copies names of all EAs of uname as consecutive C strings into rbuf. Increments *rbuflen accordingly.

Calls: [delete_ea_file](ea_ad.c.md#delete_ea_file), [ea_addentry](ea_ad.c.md#ea_addentry), [ea_attrname_invalid](ea_ad.c.md#ea_attrname_invalid), [ea_close](ea_ad.c.md#ea_close), [ea_delentry](ea_ad.c.md#ea_delentry), [ea_open](ea_ad.c.md#ea_open), [write_ea](ea_ad.c.md#write_ea)

Called through [`vfs_ops::vfs_ea_set`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [vfs_ea_set](vfs.c.md#vfs_ea_set)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### remove_ea

```c
int remove_ea(const struct vol *vol, const char *uname, const char *attruname, int oflag, int fd)
```

Defined at lines 1350 to 1387. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

remove a EA from a file

Parameters:
* `vol`: current volume
* `uname`: filename
* `attruname`: EA name
* `oflag`: link and create flag
* `fd`: file descriptor

Returns: AFP code: AFP_OK on success or appropriate AFP error code

Note: Removes EA attruname from file uname.

Calls: [delete_ea_file](ea_ad.c.md#delete_ea_file), [ea_attrname_invalid](ea_ad.c.md#ea_attrname_invalid), [ea_close](ea_ad.c.md#ea_close), [ea_delentry](ea_ad.c.md#ea_delentry), [ea_open](ea_ad.c.md#ea_open)

Called through [`vfs_ops::vfs_ea_remove`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [vfs_ea_remove](vfs.c.md#vfs_ea_remove)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### ea_deletefile

```c
int ea_deletefile(const struct vol *vol, int dirfd, const char *file)
```

Defined at lines 1408 to 1467. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Delete every EA of a file together with the EA header file.

A file without an EA header needs nothing and succeeds. With a dirfd the call changes into that directory for the deletions and back afterwards, and a failed return ends the process with EXITERR_SYS. A failed EA file delete turns the result into AFPERR_MISC and leaves its entry in the header. ea_close deletes the header file once every entry name is NULL.

Parameters:
* `vol`: volume holding the file
* `dirfd`: directory that file is relative to, or -1 for the working directory
* `file`: file whose EAs to delete

Returns: AFP_OK, or AFPERR_MISC when the header open or a delete failed

Calls: [delete_ea_file](ea_ad.c.md#delete_ea_file), [ea_close](ea_ad.c.md#ea_close), [ea_openat](ea_ad.c.md#ea_openat)

Called through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [deletefile](../../etc/afpd/file.c.md#deletefile), [vfs_deletefile](vfs.c.md#vfs_deletefile)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### ea_renamefile

```c
int ea_renamefile(const struct vol *vol, int dirfd, const char *src, const char *dst)
```

Defined at lines 1488 to 1592. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Move every AppleDouble EA of src to dst.

src is relative to dirfd, and dst is relative to the working directory. A source without an EA header needs nothing and succeeds. When the destination header fails to open with ENOENT, ad_open creates the metadata file and its .AppleDouble directory, and the open runs again. Each EA enters the destination header and leaves the source header, and unix_rename then moves the EA file. Closing the source with every entry cleared deletes the source header.

Parameters:
* `vol`: volume holding the file
* `dirfd`: directory that src is relative to, or -1 for the working directory
* `src`: current file name
* `dst`: new file name

Returns: AFP_OK, or AFPERR_MISC when an open, an entry update, or a rename failed

Calls: [ad_close](../adouble/ad_flush.c.md#ad_close), [ad_init](../adouble/ad_open.c.md#ad_init), [ad_open](../adouble/ad_open.c.md#ad_open), [ea_addentry](ea_ad.c.md#ea_addentry), [ea_close](ea_ad.c.md#ea_close), [ea_delentry](ea_ad.c.md#ea_delentry), [ea_open](ea_ad.c.md#ea_open), [ea_openat](ea_ad.c.md#ea_openat), [ea_path](ea_ad.c.md#ea_path), [unix_rename](unix.c.md#unix_rename)

Called through [`vfs_ops::vfs_renamefile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [renamefile](../../etc/afpd/file.c.md#renamefile), [vfs_renamefile](vfs.c.md#vfs_renamefile)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### ea_copyfile

```c
int ea_copyfile(const struct vol *vol, int sfd, const char *src, const char *dst)
```

Defined at lines 1606 to 1700. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

copy EAs

Parameters:
* `vol`: current volume
* `sfd`: source file descriptor
* `src`: source path
* `dst`: destination path

Returns: AFP code AFP_OK on success or appropriate AFP error code

Note: Copies EAs from source file to dest file.

Calls: [ad_close](../adouble/ad_flush.c.md#ad_close), [ad_init](../adouble/ad_open.c.md#ad_init), [ad_open](../adouble/ad_open.c.md#ad_open), [copy_file](unix.c.md#copy_file), [ea_addentry](ea_ad.c.md#ea_addentry), [ea_close](ea_ad.c.md#ea_close), [ea_open](ea_ad.c.md#ea_open), [ea_openat](ea_ad.c.md#ea_openat), [ea_path](ea_ad.c.md#ea_path)

Called through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [copyfile](../../etc/afpd/file.c.md#copyfile), [vfs_copyfile](vfs.c.md#vfs_copyfile)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### ea_chown

```c
int ea_chown(const struct vol *vol, const char *path, uid_t uid, gid_t gid)
```

Defined at lines 1715 to 1778. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Give the EA header file and every EA file of path a new owner.

A file without an EA header needs nothing and succeeds. ochown follows symlinks only when the volume sets AFPVOL_FOLLOWSYM.

Parameters:
* `vol`: volume holding the file
* `path`: file whose EA files to change
* `uid`: new owner
* `gid`: new group

Returns: AFP_OK, AFPERR_ACCESS on EPERM or EACCES, or AFPERR_MISC on any other failure

Calls: [ea_close](ea_ad.c.md#ea_close), [ea_open](ea_ad.c.md#ea_open), [ea_path](ea_ad.c.md#ea_path), [ochown](../util/unix.c.md#ochown)

Called through [`vfs_ops::vfs_chown`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setfilowner](../../etc/afpd/unix.c.md#setfilowner), [vfs_chown](vfs.c.md#vfs_chown)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### ea_chmod_file

```c
int ea_chmod_file(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 1794 to 1866. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Apply a new file mode to the EA header file and every EA file.

The header gets ea_header_mode, so the owner keeps read and write access. Each EA file gets ea_mode. setfilmode applies the volume umask. A file without an EA header needs nothing and succeeds.

Parameters:
* `vol`: volume holding the file
* `name`: file whose EA files to change
* `mode`: new file mode
* `st`: unused

Returns: AFP_OK, AFPERR_ACCESS on EPERM or EACCES, or AFPERR_MISC on any other failure

Calls: [ea_close](ea_ad.c.md#ea_close), [ea_header_mode](ea_ad.c.md#ea_header_mode), [ea_mode](ea_ad.c.md#ea_mode), [ea_open](ea_ad.c.md#ea_open), [ea_path](ea_ad.c.md#ea_path), [setfilmode](unix.c.md#setfilmode)

Called through [`vfs_ops::vfs_setfilmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode), [vfs_setfilmode](vfs.c.md#vfs_setfilmode)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)

### ea_chmod_dir

```c
int ea_chmod_dir(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
```

Defined at lines 1883 to 1959. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

Apply a new directory mode to the EA header file and every EA file.

The call runs as root throughout, because the .AppleDouble directory may already deny the user access. The header gets ea_header_mode, and each EA file gets ea_mode. A directory without an EA header needs nothing and succeeds.

Parameters:
* `vol`: volume holding the directory
* `name`: directory whose EA files to change
* `mode`: new directory mode
* `st`: unused

Returns: AFP_OK, AFPERR_ACCESS on EPERM or EACCES, or AFPERR_MISC on any other failure

Calls: [become_root](../util/unix.c.md#become_root), [ea_close](ea_ad.c.md#ea_close), [ea_header_mode](ea_ad.c.md#ea_header_mode), [ea_mode](ea_ad.c.md#ea_mode), [ea_open](ea_ad.c.md#ea_open), [ea_path](ea_ad.c.md#ea_path), [setfilmode](unix.c.md#setfilmode), [unbecome_root](../util/unix.c.md#unbecome_root)

Called through [`vfs_ops::vfs_setdirunixmode`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode), [vfs_setdirunixmode](vfs.c.md#vfs_setdirunixmode)

Dispatched via: [netatalk_ea_adouble](vfs.c.md#netatalk_ea_adouble)
