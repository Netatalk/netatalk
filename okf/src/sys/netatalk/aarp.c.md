---
type: C Source File
title: "sys/netatalk/aarp.c"
description: "10 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/aarp.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [aarp.h](aarp.h.md)
* [at.h](at.h.md)
* [at_var.h](at_var.h.md)
* [ddp_var.h](ddp_var.h.md)
* [phase2.h](phase2.h.md)
* System headers: `net/af.h`, `net/if.h`, `net/route.h`, `netinet/if_ether.h`, `netinet/in.h`, `sys/kernel.h`, `sys/mbuf.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `syslog.h`

# Functions

### aarptimer

```c
void aarptimer(void)
```

Defined at lines 72 to 93.

Calls: [aarptfree](aarp.c.md#aarptfree)

Called by: [aarp_clean](aarp.c.md#aarp_clean), [aarptnew](aarp.c.md#aarptnew)

Uses file-scope variables: `aarptab`

### at_ifawithnet

```c
struct ifaddr * at_ifawithnet(struct sockaddr_at *sat, struct ifaddr *ifa)
```

Defined at lines 95 to 128.

Called by: [aarpresolve](aarp.c.md#aarpresolve), [aarpwhohas](aarp.c.md#aarpwhohas), [at_aarpinput](aarp.c.md#at_aarpinput)

### aarpwhohas

```c
void aarpwhohas(struct arpcom *ac, struct sockaddr_at *sat)
```

Defined at lines 130 to 215.

Calls: [at_ifawithnet](aarp.c.md#at_ifawithnet)

Called by: [aarpresolve](aarp.c.md#aarpresolve)

Uses file-scope variables: `aarp_org_code`, `atmulticastaddr`, `etherbroadcastaddr`

### aarpresolve

```c
int aarpresolve(struct arpcom *ac, struct mbuf *m, struct sockaddr_at *destsat, unsigned char *desten)
```

Defined at lines 217 to 280.

Calls: [aarptnew](aarp.c.md#aarptnew), [aarpwhohas](aarp.c.md#aarpwhohas), [at_broadcast](at_control.c.md#at_broadcast), [at_ifawithnet](aarp.c.md#at_ifawithnet)

Uses file-scope variables: `atmulticastaddr`, `etherbroadcastaddr`

### aarpinput

```c
void aarpinput(struct arpcom *ac, struct mbuf *m)
```

Defined at lines 282 to 320.

Calls: [at_aarpinput](aarp.c.md#at_aarpinput)

### at_aarpinput

```c
void at_aarpinput(struct arpcom *ac, struct mbuf *m)
```

Defined at lines 323 to 524.

Calls: [aarpprobe](aarp.c.md#aarpprobe), [aarptfree](aarp.c.md#aarptfree), [aarptnew](aarp.c.md#aarptnew), [at_ifawithnet](aarp.c.md#at_ifawithnet)

Called by: [aarpinput](aarp.c.md#aarpinput)

Uses file-scope variables: `aarp_org_code`, `etherbroadcastaddr`

### aarptfree

```c
void aarptfree(struct aarptab *aat)
```

Defined at lines 526 to 536.

Called by: [aarptimer](aarp.c.md#aarptimer), [aarptnew](aarp.c.md#aarptnew), [at_aarpinput](aarp.c.md#at_aarpinput)

### aarptnew

```c
struct aarptab * aarptnew(struct at_addr *addr)
```

Defined at lines 538 to 577.

Calls: [aarptfree](aarp.c.md#aarptfree), [aarptimer](aarp.c.md#aarptimer)

Called by: [aarpresolve](aarp.c.md#aarpresolve), [at_aarpinput](aarp.c.md#at_aarpinput)

### aarpprobe

```c
void aarpprobe(struct arpcom *ac)
```

Defined at lines 579 to 679.

Called by: [at_aarpinput](aarp.c.md#at_aarpinput)

Uses file-scope variables: `aarp_org_code`, `atmulticastaddr`, `etherbroadcastaddr`

### aarp_clean

```c
void aarp_clean(void)
```

Defined at lines 681 to 692.

Calls: [aarptimer](aarp.c.md#aarptimer)

# Macros

* Undocumented: `AARPTAB_BSIZ`, `AARPTAB_HASH`, `AARPTAB_LOOK`, `AARPTAB_NB`, `AARPTAB_SIZE`, `AARPT_AGE`, `AARPT_KILLC`, `AARPT_KILLI`

# File-scope variables

`aarp_org_code`, `aarptab`, `aarptab_size`, `at_org_code`, `atmulticastaddr`, `etherbroadcastaddr`
