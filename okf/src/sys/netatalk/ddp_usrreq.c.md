---
type: C Source File
title: "sys/netatalk/ddp_usrreq.c"
description: "10 functions, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp_usrreq.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [at.h](at.h.md)
* [at_var.h](at_var.h.md)
* [ddp_var.h](ddp_var.h.md)
* System headers: `errno.h`, `net/if.h`, `net/route.h`, `sys/ioctl.h`, `sys/mbuf.h`, `sys/param.h`, `sys/protosw.h`, `sys/socket.h`, `sys/socketvar.h`, `sys/systm.h`, `sys/types.h`, `sys/user.h`

# Functions

### ddp_usrreq

```c
ddp_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *addr, struct mbuf *rights)
```

Defined at lines 32 to 184. Declared in [sys/netatalk/at_proto.c](at_proto.c.md).

Calls: [at_control](at_control.c.md#at_control), [at_pcballoc](ddp_usrreq.c.md#at_pcballoc), [at_pcbconnect](ddp_usrreq.c.md#at_pcbconnect), [at_pcbdetach](ddp_usrreq.c.md#at_pcbdetach), [at_pcbdisconnect](ddp_usrreq.c.md#at_pcbdisconnect), [at_pcbsetaddr](ddp_usrreq.c.md#at_pcbsetaddr), [at_sockaddr](ddp_usrreq.c.md#at_sockaddr), [ddp_output](ddp_output.c.md#ddp_output)

Uses file-scope variables: `ddp_recvspace`, `ddp_sendspace`

Dispatched via: [atalksw](at_proto.c.md#atalksw)

### at_sockaddr

```c
at_sockaddr(struct ddpcb *ddp, struct mbuf *addr)
```

Defined at lines 186 to 194.

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_pcbsetaddr

```c
at_pcbsetaddr(struct ddpcb *ddp, struct mbuf *addr)
```

Defined at lines 196 to 312.

Called by: [at_pcbconnect](ddp_usrreq.c.md#at_pcbconnect), [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_pcbconnect

```c
at_pcbconnect(struct ddpcb *ddp, struct mbuf *addr)
```

Defined at lines 314 to 424.

Calls: [at_pcbsetaddr](ddp_usrreq.c.md#at_pcbsetaddr)

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_pcbdisconnect

```c
at_pcbdisconnect(struct ddpcb *ddp)
```

Defined at lines 426 to 432.

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_pcballoc

```c
at_pcballoc(struct socket *so)
```

Defined at lines 434 to 455.

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

Uses file-scope variables: `ddpcb`

### at_pcbdetach

```c
at_pcbdetach(struct socket *so, struct ddpcb *ddp)
```

Defined at lines 457 to 494.

Called by: [ddp_clean](ddp_usrreq.c.md#ddp_clean), [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### ddp_search

```c
struct ddpcb * ddp_search(struct sockaddr_at *from, struct sockaddr_at *to, struct at_ifaddr *aa)
```

Defined at lines 503 to 548.

Called by: [ddp_input](ddp_input.c.md#ddp_input)

### ddp_init

```c
int ddp_init()
```

Defined at lines 550 to 554. Declared in [sys/netatalk/at_proto.c](at_proto.c.md).

Dispatched via: [atalksw](at_proto.c.md#atalksw)

### ddp_clean

```c
ddp_clean()
```

Defined at lines 556 to 563.

Calls: [at_pcbdetach](ddp_usrreq.c.md#at_pcbdetach)

# File-scope variables

`ddp_recvspace`, `ddp_sendspace`, `ddpcb`
