---
type: C Source File
title: "etc/afpd/quota.c"
description: "2 functions, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/quota.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [auth.h](auth.h.md)
* [directory.h](directory.h.md)
* [unix.h](unix.h.md)
* [volume.h](volume.h.md)
* System headers: `errno.h`, `fcntl.h`, `quota.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `unistd.h`

# Functions

### getfreespace

```c
static int getfreespace(const AFPObj *obj, struct vol *vol, VolSpace *bfree, VolSpace *btotal, id_t id, int idtype)
```

Defined at lines 44 to 111.

Calls: [become_root](../../libatalk/util/unix.c.md#become_root), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Called by: [uquota_getvolspace](quota.c.md#uquota_getvolspace)

### uquota_getvolspace

```c
int uquota_getvolspace(const AFPObj *obj, struct vol *, VolSpace *, VolSpace *, const uint32_t)
```

Defined at lines 113 to 163. Declared in [etc/afpd/unix.h](unix.h.md).

Calls: [getfreespace](quota.c.md#getfreespace)

Called by: [getvolspace](volume.c.md#getvolspace)
