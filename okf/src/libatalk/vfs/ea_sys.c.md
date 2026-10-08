---
type: C Source File
title: "libatalk/vfs/ea_sys.c"
description: "7 functions, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/vfs/ea_sys.c"
tags: ["libatalk/vfs"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

### sys_ea_attrname_invalid

```c
static int sys_ea_attrname_invalid(const char *func, const char *attruname)
```

Defined at lines 44 to 52.

Called by: [sys_get_eacontent](ea_sys.c.md#sys_get_eacontent), [sys_get_easize](ea_sys.c.md#sys_get_easize), [sys_remove_ea](ea_sys.c.md#sys_remove_ea), [sys_set_ea](ea_sys.c.md#sys_set_ea)

### sys_get_easize

```c
int sys_get_easize(const struct vol *vol, char *rbuf, size_t *rbuflen, const char *uname, int oflag, const char *attruname, int fd)
```

Defined at lines 69 to 151. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

get size of a native EA

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

Calls: [sys_ea_attrname_invalid](ea_sys.c.md#sys_ea_attrname_invalid), [sys_fgetxattr](extattr.c.md#sys_fgetxattr), [sys_getxattr](extattr.c.md#sys_getxattr), [sys_lgetxattr](extattr.c.md#sys_lgetxattr)

Called through [`vfs_ops::vfs_ea_getsize`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [vfs_ea_getsize](vfs.c.md#vfs_ea_getsize)

Dispatched via: [netatalk_ea_sys](vfs.c.md#netatalk_ea_sys)

### sys_get_eacontent

```c
int sys_get_eacontent(const struct vol *vol, char *rbuf, size_t *rbuflen, const char *uname, int oflag, const char *attruname, int maxreply, int fd)
```

Defined at lines 168 to 288. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

copy native EA into rbuf

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

Calls: [sys_ea_attrname_invalid](ea_sys.c.md#sys_ea_attrname_invalid), [sys_fgetxattr](extattr.c.md#sys_fgetxattr), [sys_getxattr](extattr.c.md#sys_getxattr), [sys_lgetxattr](extattr.c.md#sys_lgetxattr)

Called through [`vfs_ops::vfs_ea_getcontent`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [vfs_ea_getcontent](vfs.c.md#vfs_ea_getcontent)

Dispatched via: [netatalk_ea_sys](vfs.c.md#netatalk_ea_sys)

### sys_list_eas

```c
int sys_list_eas(const struct vol *vol, char *attrnamebuf, size_t *buflen, const char *uname, int oflag, int fd)
```

Defined at lines 307 to 388. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

copy names of native EAs into attrnamebuf

Parameters:
* `vol`: current volume
* `attrnamebuf`: store names a consecutive C strings here
* `buflen`: length of names in attrnamebuf
* `uname`: filename
* `oflag`: link and create flag
* `fd`: file descriptor

Returns: AFP code: AFP_OK on success or appropriate AFP error code

Note: Copies names of all EAs of uname as consecutive C strings into rbuf. Increments *rbuflen accordingly. We hide the adouble:ea extended attributes here, we do not allow reading, writing and deleting them.

Calls: [convert_string](../unicode/charcnv.c.md#convert_string), [sys_flistxattr](extattr.c.md#sys_flistxattr), [sys_listxattr](extattr.c.md#sys_listxattr), [sys_llistxattr](extattr.c.md#sys_llistxattr)

Called through [`vfs_ops::vfs_ea_list`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [vfs_ea_list](vfs.c.md#vfs_ea_list)

Dispatched via: [netatalk_ea_sys](vfs.c.md#netatalk_ea_sys)

### sys_set_ea

```c
int sys_set_ea(const struct vol *vol, const char *uname, const char *attruname, const char *ibuf, size_t attrsize, int oflag, int fd)
```

Defined at lines 403 to 508. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

set a native EA

Parameters:
* `vol`: current volume
* `uname`: filename
* `attruname`: EA name
* `ibuf`: buffer with EA content
* `attrsize`: length EA in ibuf
* `oflag`: link and create flag
* `fd`: file descriptor

Returns: AFP code: AFP_OK on success or appropriate AFP error code

Calls: [getcwdpath](../util/unix.c.md#getcwdpath), [sys_ea_attrname_invalid](ea_sys.c.md#sys_ea_attrname_invalid), [sys_fsetxattr](extattr.c.md#sys_fsetxattr), [sys_lsetxattr](extattr.c.md#sys_lsetxattr), [sys_setxattr](extattr.c.md#sys_setxattr)

Called through [`vfs_ops::vfs_ea_set`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [vfs_ea_set](vfs.c.md#vfs_ea_set)

Dispatched via: [netatalk_ea_sys](vfs.c.md#netatalk_ea_sys)

### sys_remove_ea

```c
int sys_remove_ea(const struct vol *vol, const char *uname, const char *attruname, int oflag, int fd)
```

Defined at lines 523 to 582. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

remove a native EA

Parameters:
* `vol`: current volume
* `uname`: filename
* `attruname`: EA name
* `oflag`: link and create flag
* `fd`: file descriptor

Returns: AFP code: AFP_OK on success or appropriate AFP error code

Note: Removes EA attruname from file uname.

Calls: [sys_ea_attrname_invalid](ea_sys.c.md#sys_ea_attrname_invalid), [sys_fremovexattr](extattr.c.md#sys_fremovexattr), [sys_lremovexattr](extattr.c.md#sys_lremovexattr), [sys_removexattr](extattr.c.md#sys_removexattr)

Called through [`vfs_ops::vfs_ea_remove`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [vfs_ea_remove](vfs.c.md#vfs_ea_remove)

Dispatched via: [netatalk_ea_sys](vfs.c.md#netatalk_ea_sys)

### sys_ea_copyfile

```c
int sys_ea_copyfile(const struct vol *vol, int sfd, const char *src, const char *dst)
```

Defined at lines 596 to 778. Declared in [include/atalk/ea.h](../../include/atalk/ea.h.md).

copy EAs

Parameters:
* `vol`: current volume
* `sfd`: source file descriptor
* `src`: source path
* `dst`: destination path

Returns: AFP code AFP_OK on success or appropriate AFP error code

Note: Copies EAs from source file to dest file.

Calls: [sys_getxattr](extattr.c.md#sys_getxattr), [sys_listxattr](extattr.c.md#sys_listxattr), [sys_setxattr](extattr.c.md#sys_setxattr)

Called through [`vfs_ops::vfs_copyfile`](../../include/atalk/vfs.h.md#struct-vfs_ops) by: [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [vfs_copyfile](vfs.c.md#vfs_copyfile)

Dispatched via: [netatalk_ea_sys](vfs.c.md#netatalk_ea_sys)
