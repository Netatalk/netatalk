---
type: C Source File
title: "libatalk/asp/asp_write.c"
description: "1 function, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_write.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 36 to 101. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Calls: [atp_rresp](../atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../atp/atp_sreq.c.md#atp_sreq)

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [write_fork](../../etc/afpd/fork.c.md#write_fork)
