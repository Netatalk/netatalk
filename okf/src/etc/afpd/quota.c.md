---
type: C Source File
title: "etc/afpd/quota.c"
description: "Free space under user and group quotas through libquota or quotactl."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/quota.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-20T20:07:02+11:00 }
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

Defined at lines 65 to 132.

Read the free and total space that a user or group quota leaves on a volume.

The quota opens through libquota as root. Usage at the hard limit, or at the soft limit past its grace time, leaves no free space and a total equal to the usage. Otherwise the hard limit sets the total.

Parameters:
* `obj`: session object, unused
* `vol`: volume to check
* `bfree`: free bytes under the quota
* `btotal`: total bytes under the quota
* `id`: user or group ID
* `idtype`: QUOTA_IDTYPE_USER or QUOTA_IDTYPE_GROUP

Returns: 1 with a quota, 0 without quota support or a quota for id, or -1 on error

Calls: [become_root](../../libatalk/util/unix.c.md#become_root), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Called by: [uquota_getvolspace](quota.c.md#uquota_getvolspace)

### uquota_getvolspace

```c
int uquota_getvolspace(const AFPObj *obj, struct vol *, VolSpace *, VolSpace *, const uint32_t)
```

Defined at lines 147 to 197. Declared in [etc/afpd/unix.h](unix.h.md).

Read the space that the user and group quotas leave on a volume.

The call checks the quota of the user and of the first group of the session. With both, the call takes the minimum of the 2 free spaces.

Parameters:
* `obj`: session object
* `vol`: volume to check
* `bfree`: free bytes under the quotas
* `btotal`: total bytes under the quotas
* `bsize`: block size, unused

Returns: AFP_OK, or AFPERR_PARAM when neither quota exists

Calls: [getfreespace](quota.c.md#getfreespace)

Called by: [getvolquota](volume.c.md#getvolquota)
