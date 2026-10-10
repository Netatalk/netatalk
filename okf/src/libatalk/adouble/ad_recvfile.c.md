---
type: C Source File
title: "libatalk/adouble/ad_recvfile.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_recvfile.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)

# Functions

### ad_recvfile

```c
ssize_t ad_recvfile(struct adouble *ad, int eid, DSI *dsi, off_t *off)
```

Defined at lines 41 to 64.

Receive the rest of a DSIWrite payload into a fork.

Parameters:
* `ad`: adouble holding the fork
* `eid`: ADEID_DFORK or ADEID_RFORK
* `dsi`: session; its datasize is the payload left to read
* `off`: fork offset of the first byte, advanced past every byte written, also when the call fails

Returns: the bytes written, 0 when the write is left to [dsi_write()](../dsi/dsi_write.c.md#dsi_write) and [ad_write()](ad_write.c.md#ad_write), or -1 with errno set

Calls: [ad_fork_fileno](ad_open.c.md#ad_fork_fileno), [dsi_write_file](../dsi/dsi_write.c.md#dsi_write_file)

Called by: [write_fork](../../etc/afpd/fork.c.md#write_fork)
