---
type: C Source File
title: "sys/netatalk/at_control.c"
description: "7 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/at_control.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [aarp.h](aarp.h.md)
* [at.h](at.h.md)
* [at_var.h](at_var.h.md)
* [phase2.h](phase2.h.md)
* System headers: `errno.h`, `net/af.h`, `net/if.h`, `net/route.h`, `netinet/if_ether.h`, `netinet/in.h`, `sys/ioctl.h`, `sys/kernel.h`, `sys/mbuf.h`, `sys/param.h`, `sys/socket.h`, `sys/socketvar.h`, `sys/systm.h`, `sys/types.h`, `sys/user.h`

# Functions

### atalk_hash

```c
void atalk_hash(struct sockaddr_at *sat, struct afhash *hp)
```

Defined at lines 38 to 43.

### atalk_netmatch

```c
int atalk_netmatch(struct sockaddr_at *sat1, struct sockaddr_at *sat2)
```

Defined at lines 49 to 65.

Note the magic to get ifa_ifwithnet() to work without adding an ifaddr entry for each net in our local range.

### at_control

```c
at_control(int cmd, caddr_t data, struct ifnet *ifp)
```

Defined at lines 68 to 307.

Calls: [at_ifinit](at_control.c.md#at_ifinit), [at_scrub](at_control.c.md#at_scrub)

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_scrub

```c
at_scrub(struct ifnet *ifp, struct at_ifaddr *aa)
```

Defined at lines 309 to 361.

Called by: [aa_clean](at_control.c.md#aa_clean), [at_control](at_control.c.md#at_control)

### at_ifinit

```c
at_ifinit(struct ifnet *ifp, struct at_ifaddr *aa, struct sockaddr_at *sat)
```

Defined at lines 365 to 569.

Called by: [at_control](at_control.c.md#at_control)

Uses file-scope variables: `time`

### at_broadcast

```c
at_broadcast(struct sockaddr_at *sat)
```

Defined at lines 571 to 593.

Called by: [aarpresolve](aarp.c.md#aarpresolve)

### aa_clean

```c
aa_clean()
```

Defined at lines 595 to 621.

Calls: [at_scrub](at_control.c.md#at_scrub)

# File-scope variables

`time`
