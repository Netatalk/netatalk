---
type: C Source File
title: "libatalk/asp/asp_write.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_write.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `string.h`, `sys/types.h`, `sys/uio.h`

# Functions

### asp_wrtcont

```c
int asp_wrtcont(ASP, char *, size_t *)
```

Defined at lines 51 to 116. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Fetch the data of an [ASP](../../include/atalk/asp.h.md#struct-asp) write from the workstation with a WriteContinue request.

The request names the buffer size. It goes to the session socket of the workstation as an exactly once transaction, with 5 tries 2 seconds apart. Up to ASP_MAXPACKETS reply packets land in buf, and the call strips their 4-byte headers in place.

Parameters:
* `asp`: session
* `buf`: buffer of ASP_DATAMAXSIZ bytes that receives the data
* `buflen`: buffer size on entry, data length on return

Returns: 0, or -1 when the request or the reply fails, or with EMSGSIZE for a packet shorter than its header

Calls: [atp_rresp](../atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../atp/atp_sreq.c.md#atp_sreq)

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [write_fork](../../etc/afpd/fork.c.md#write_fork)
