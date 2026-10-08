---
type: C Source File
title: "libatalk/util/bprint.c"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/bprint.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `ctype.h`, `stdio.h`, `stdlib.h`, `string.h`

# Functions

### bprint

```c
void bprint(char *data, int len)
```

Defined at lines 17 to 48.

Called by: [atp_recv_atp](../atp/atp_packet.c.md#atp_recv_atp), [atp_rresp](../atp/atp_rresp.c.md#atp_rresp), [atp_rsel](../atp/atp_rsel.c.md#atp_rsel), [atp_sreq](../atp/atp_sreq.c.md#atp_sreq), [atp_sresp](../atp/atp_sresp.c.md#atp_sresp), [main](../../etc/atalkd/main.c.md#main), [resend_request](../atp/atp_rsel.c.md#resend_request), [write_fork](../../etc/afpd/fork.c.md#write_fork)

Uses file-scope variables: `hexdig`

# Macros

* Undocumented: `BPALEN`, `BPXLEN`

# File-scope variables

`hexdig`
