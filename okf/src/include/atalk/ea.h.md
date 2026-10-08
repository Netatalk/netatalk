---
type: C Header File
title: "include/atalk/ea.h"
description: "1 function, 2 types, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/ea.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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
* `uint32_t ea_inited`
* `const struct vol * vol`
* `int dirfd`
* `char * filename`
* `unsigned int ea_count`
* `struct ea_entry(* ea_entries`
* `int ea_fd`
* `eaflags_t ea_flags`
* `size_t ea_size`
* `char * ea_data`

### struct ea_entry

Defined at line 156.
* `size_t ea_namelen`
* `size_t ea_size`
* `char * ea_name`

# Typedefs and enums

* `enum (anonymous)`: `kXAttrNoFollow`, `kXAttrCreate`, `kXAttrReplace`
* `enum eaflags_t`: `EA_CREATE`, `EA_RDONLY`, `EA_RDWR`, `EA_DIR`

# Macros

* Undocumented: `AD_EA_META`, `AD_EA_META_LEN`, `AD_EA_RESO`, `ATTRNAMEBUFSIZ`, `EA_COUNT_LEN`, `EA_COUNT_OFF`, `EA_HEADER_SIZE`, `EA_INITED`, `EA_MAGIC`, `EA_MAGIC_LEN`, `EA_MAGIC_OFF`, `EA_VERSION`, `EA_VERSION1`, `EA_VERSION_LEN`, `EA_VERSION_OFF`, `ENOATTR`, `MAX_EA_SIZE`, `MAX_REPLY_EXTRA_BYTES`, `NOT_NETATALK_EA`, `XATTR_CREATE`, `XATTR_REPLACE`
