---
type: C Header File
title: "include/atalk/ea.h"
description: "1 function, 2 types, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/ea.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/vfs.h](vfs.h.md)

# Included by

* [bin/dbd/cmd_dbd_scanvol.c](../../bin/dbd/cmd_dbd_scanvol.c.md)
* [etc/afpd/ad_cache.c](../../etc/afpd/ad_cache.c.md)
* [etc/afpd/extattrs.c](../../etc/afpd/extattrs.c.md)
* [etc/afpd/ofork.c](../../etc/afpd/ofork.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [libatalk/adouble/ad_conv.c](../../libatalk/adouble/ad_conv.c.md)
* [libatalk/adouble/ad_flush.c](../../libatalk/adouble/ad_flush.c.md)
* [libatalk/adouble/ad_open.c](../../libatalk/adouble/ad_open.c.md)
* [libatalk/adouble/ad_read.c](../../libatalk/adouble/ad_read.c.md)
* [libatalk/adouble/ad_write.c](../../libatalk/adouble/ad_write.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)
* [libatalk/util/unix.c](../../libatalk/util/unix.c.md)
* [libatalk/vfs/ea_ad.c](../../libatalk/vfs/ea_ad.c.md)
* [libatalk/vfs/ea_sys.c](../../libatalk/vfs/ea_sys.c.md)
* [libatalk/vfs/extattr.c](../../libatalk/vfs/extattr.c.md)
* [libatalk/vfs/unix.c](../../libatalk/vfs/unix.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Functions

### sys_copyxattr

```c
int sys_copyxattr(const char *src, const char *dst)
```

Declared at include/atalk/ea.h line 119; no definition in the scanned sources.

# Types

### struct ea

Defined at line 164.
* `uint32_t ea_inited`: needed for interfacing ea_open w.
* `const struct vol * vol`: vol handle, ea_close needs it
* `int dirfd`: for *at (cf openat) semantics, -1 means ignore
* `char * filename`: name of file, needed by ea_close too
* `unsigned int ea_count`: number of EAs in ea_entries array
* `struct ea_entry(* ea_entries`: malloced and realloced as needed by ea_count
* `int ea_fd`: open fd for ea_data
* `eaflags_t ea_flags`: flags
* `size_t ea_size`: size of header file = size of ea_data buffer
* `char * ea_data`: pointer to buffer into that we actually read the disc file into

### struct ea_entry

Defined at line 156.
* `size_t ea_namelen`: len of ea_name without terminating 0 i.e.
* `size_t ea_size`: size of EA
* `char * ea_name`: name of the EA

# Typedefs and enums

* `enum (anonymous)`: `kXAttrNoFollow`, `kXAttrCreate`, `kXAttrReplace`
* `enum eaflags_t`: `EA_CREATE`, `EA_RDONLY`, `EA_RDWR`, `EA_DIR`

# Macros

* `ATTRNAMEBUFSIZ`: Library user must provide a static buffer of size ATTRNAMEBUFSIZ.
* `EA_INITED`: ea"INT", for interfacing ea_open w.
* `EA_MAGIC`: "adEA"
* `MAX_EA_SIZE`: This seems to be the current limit fo HFS+, we arbitrarily force that which also safes us from buffer overflows.
* `MAX_REPLY_EXTRA_BYTES`: req_count has space for AFP response bitmap and length as well, so 6 bytes
* `XATTR_CREATE`: set value, fail if attr already exists
* `XATTR_REPLACE`: set value, fail if attr does not exist
* Undocumented: `AD_EA_META`, `AD_EA_META_LEN`, `AD_EA_RESO`, `EA_COUNT_LEN`, `EA_COUNT_OFF`, `EA_HEADER_SIZE`, `EA_MAGIC_LEN`, `EA_MAGIC_OFF`, `EA_VERSION`, `EA_VERSION1`, `EA_VERSION_LEN`, `EA_VERSION_OFF`, `ENOATTR`, `NOT_NETATALK_EA`
