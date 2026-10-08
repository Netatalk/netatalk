---
type: C Source File
title: "sys/netatalk/ddp_input.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp_input.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [at.h](at.h.md)
* [at_var.h](at_var.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [ddp.h](ddp.h.md)
* [ddp_var.h](ddp_var.h.md)
* System headers: `ctype.h`, `net/if.h`, `net/route.h`, `sys/mbuf.h`, `sys/param.h`, `sys/socket.h`, `sys/socketvar.h`, `sys/types.h`

# Functions

### at_cksum

```c
unsigned short at_cksum()
```

Declared at sys/netatalk/ddp_input.c line 27; no definition in the scanned sources.

Called by: [ddp_input](ddp_input.c.md#ddp_input)

### atintr

```c
atintr()
```

Defined at lines 32 to 121.

Calls: [ddp_input](ddp_input.c.md#ddp_input)

### ddp_input

```c
ddp_input(struct mbuf *m, struct ifnet *ifp, struct elaphdr *elh, int phase)
```

Defined at lines 125 to 367.

Calls: [at_cksum](ddp_input.c.md#at_cksum), [ddp_route](ddp_output.c.md#ddp_route), [ddp_search](ddp_usrreq.c.md#ddp_search)

Called by: [atintr](ddp_input.c.md#atintr)

Uses file-scope variables: `ddp_cksum` in [sys/netatalk/ddp_output.c](ddp_output.c.md), `ddp_firewall`, `ddp_forward`, `forwro`

### m_printm

```c
m_printm(struct mbuf *m)
```

Defined at lines 369 to 375.

Calls: [bprint](ddp_input.c.md#bprint)

### bprint

```c
bprint(char *data, int len)
```

Defined at lines 382 to 423.

Called by: [m_printm](ddp_input.c.md#m_printm)

Uses file-scope variables: `hexdig`

# Macros

* Undocumented: `BPALEN`, `BPXLEN`

# File-scope variables

`ddp_firewall`, `ddp_forward`, `forwro`, `hexdig`
