---
type: C Header File
title: "etc/afpd/unix.h"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/unix.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `config.h`

# Included by

* [etc/afpd/acls.c](acls.c.md)
* [etc/afpd/ad_cache.c](ad_cache.c.md)
* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/file.c](file.c.md)
* [etc/afpd/filedir.c](filedir.c.md)
* [etc/afpd/nfsquota.c](nfsquota.c.md)
* [etc/afpd/quota.c](quota.c.md)
* [etc/afpd/unix.c](unix.c.md)
* [etc/afpd/volume.c](volume.c.md)

# Functions

### setdirmode

```c
int setdirmode(const struct vol *, const char *, mode_t)
```

Declared at etc/afpd/unix.h line 195; no definition in the scanned sources.

# Macros

* Undocumented: `f_frsize`

# File-scope variables

`default_options`
