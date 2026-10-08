---
type: C Source File
title: "etc/afpd/nfsquota.c"
description: "2 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/nfsquota.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [unix.h](unix.h.md)
* System headers: `netdb.h`, `netinet/in.h`, `rpc/pmap_prot.h`, `rpc/rpc.h`, `rpcsvc/rquota.h`, `stdio.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`

# Functions

### callaurpc

```c
static int callaurpc(struct vol *vol, u_long prognum, u_long versnum, u_long procnum, xdrproc_t inproc, char *in, xdrproc_t outproc, char *out)
```

Defined at lines 64 to 102.

Called by: [getnfsquota](nfsquota.c.md#getnfsquota)

### getnfsquota

```c
int getnfsquota(struct vol *vol, const int uid, const uint32_t bsize, struct dqblk *dqp)
```

Defined at lines 108 to 178.

Calls: [callaurpc](nfsquota.c.md#callaurpc)

# Macros

* Undocumented: `GQR_RQUOTA`, `GQR_STATUS`, `NFS_BSIZE`, `PORTMAP`
