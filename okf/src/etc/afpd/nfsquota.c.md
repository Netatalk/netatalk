---
type: C Source File
title: "etc/afpd/nfsquota.c"
description: "User quota of an NFS volume through rquotad."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/nfsquota.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 86 to 124.

Call an RPC procedure on the NFS server of a volume.

The first call creates a UDP client for the host in v_gvs, with Unix credentials and a 6-second retry timeout, and keeps it in v_nfsclient. Each call waits up to 10 seconds for the answer. The code comes from the BSD quota program.

Parameters:
* `vol`: volume with the NFS host name in v_gvs
* `prognum`: RPC program number
* `versnum`: program version
* `procnum`: procedure number
* `inproc`: XDR encoder of the arguments
* `in`: arguments
* `outproc`: XDR decoder of the result
* `out`: result

Returns: RPC_SUCCESS, RPC_UNKNOWNHOST, or another clnt_stat code

Called by: [getnfsquota](nfsquota.c.md#getnfsquota)

### getnfsquota

```c
int getnfsquota(struct vol *vol, const int uid, const uint32_t bsize, struct dqblk *dqp)
```

Defined at lines 145 to 215.

Fetch a user's quota on an NFS volume from its rquotad.

The v_gvs field holds host:/path. The block counts become bsize units, scaled by the integer ratio of the server block size and bsize. The time limit becomes an absolute time.

Parameters:
* `vol`: volume of an NFS mount, v_gvs restored on return
* `uid`: user to query
* `bsize`: block size of the result
* `dqp`: receives the hard and soft block limits, the usage, and the time limit

Returns: AFP_OK, or AFPERR_PARAM for a bad v_gvs, a failed call, no quota, or a permission error

Calls: [callaurpc](nfsquota.c.md#callaurpc)

# Macros

* Undocumented: `GQR_RQUOTA`, `GQR_STATUS`, `NFS_BSIZE`, `PORTMAP`
