---
type: C Header File
title: "etc/afpd/fork.h"
description: "1 function, 2 types, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fork.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [directory.h](directory.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `stdio.h`

# Included by

* [etc/afpd/acls.c](acls.c.md)
* [etc/afpd/afp_asp.c](afp_asp.c.md)
* [etc/afpd/afp_dsi.c](afp_dsi.c.md)
* [etc/afpd/auth.c](auth.c.md)
* [etc/afpd/catsearch.c](catsearch.c.md)
* [etc/afpd/desktop.c](desktop.c.md)
* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/enumerate.c](enumerate.c.md)
* [etc/afpd/extattrs.c](extattrs.c.md)
* [etc/afpd/fce_api.c](fce_api.c.md)
* [etc/afpd/fce_util.c](fce_util.c.md)
* [etc/afpd/file.c](file.c.md)
* [etc/afpd/filedir.c](filedir.c.md)
* [etc/afpd/fork.c](fork.c.md)
* [etc/afpd/main.c](main.c.md)
* [etc/afpd/ofork.c](ofork.c.md)
* [etc/afpd/switch.c](switch.c.md)
* [etc/afpd/unix.c](unix.c.md)
* [etc/afpd/volume.c](volume.c.md)

# Functions

### fork_range_within

```c
static int fork_range_within(off_t offset, off_t length, off_t size)
```

Defined at lines 58 to 62.

Called by: [read_fork](fork.c.md#read_fork), [rfork_cache_serve_from_buf](fork.c.md#rfork_cache_serve_from_buf)

# Types

### struct file_key

Defined at line 17.
* `dev_t dev`
* `ino_t inode`

### struct ofork

Defined at line 22.
* `struct file_key key`
* `struct adouble * of_ad`
* `struct vol * of_vol`
* `cnid_t of_did`
* `uint16_t of_refnum`
* `int of_flags`
* `const unsigned char * of_virtual_data`
* `off_t of_virtual_len`
* `struct ofork ** prevp`
* `struct ofork * next`

# Typedefs and enums

* `enum of_locks_status`: `OF_LOCKS_ERROR`, `OF_LOCKS_OK`, `OF_LOCKS_NOENT`

# Macros

* Undocumented: `AFPFORK_ACCMASK`, `AFPFORK_ACCRD`, `AFPFORK_ACCWR`, `AFPFORK_DATA`, `AFPFORK_DIRTY`, `AFPFORK_ERROR`, `AFPFORK_META`, `AFPFORK_MODIFIED`, `AFPFORK_RSRC`, `AFPFORK_VIRTUAL`, `OPENACC_DRD`, `OPENACC_DWR`, `OPENACC_RD`, `OPENACC_WR`, `OPENFORK_DATA`, `OPENFORK_RSCS`, `of_name`
