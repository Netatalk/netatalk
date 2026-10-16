---
type: C Header File
title: "include/atalk/netddp.h"
description: "Portable DDP socket lifecycle and datagram I/O interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/netddp.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `sys/socket.h`, `sys/types.h`, `unistd.h`

# Included by

* [bin/aecho/aecho.c](../../bin/aecho/aecho.c.md)
* [bin/getzones/getzones.c](../../bin/getzones/getzones.c.md)
* [bin/nbp/nbprgstr.c](../../bin/nbp/nbprgstr.c.md)
* [bin/rtmpqry/rtmpqry.c](../../bin/rtmpqry/rtmpqry.c.md)
* [libatalk/atp/atp_close.c](../../libatalk/atp/atp_close.c.md)
* [libatalk/atp/atp_open.c](../../libatalk/atp/atp_open.c.md)
* [libatalk/atp/atp_packet.c](../../libatalk/atp/atp_packet.c.md)
* [libatalk/atp/atp_rsel.c](../../libatalk/atp/atp_rsel.c.md)
* [libatalk/atp/atp_sreq.c](../../libatalk/atp/atp_sreq.c.md)
* [libatalk/atp/atp_sresp.c](../../libatalk/atp/atp_sresp.c.md)
* [libatalk/nbp/nbp_lkup.c](../../libatalk/nbp/nbp_lkup.c.md)
* [libatalk/nbp/nbp_rgstr.c](../../libatalk/nbp/nbp_rgstr.c.md)
* [libatalk/nbp/nbp_unrgstr.c](../../libatalk/nbp/nbp_unrgstr.c.md)
* [libatalk/netddp/netddp_open.c](../../libatalk/netddp/netddp_open.c.md)

# Functions

### netddp_close

```c
static int netddp_close(int filedes)
```

Defined at lines 29 to 32.

Close a DDP socket.

Called by: [atp_close](../../libatalk/atp/atp_close.c.md#atp_close), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [main](../../bin/nbp/nbprgstr.c.md#main), [nbp_do_lookup_op](../../libatalk/nbp/nbp_lkup.c.md#nbp_do_lookup_op), [nbp_rgstr](../../libatalk/nbp/nbp_rgstr.c.md#nbp_rgstr), [nbp_unrgstr](../../libatalk/nbp/nbp_unrgstr.c.md#nbp_unrgstr)

### netddp_sendto

```c
static ssize_t netddp_sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
```

Defined at lines 35 to 39.

Send a DDP datagram through sendto.

Called by: [aep_send](../../bin/aecho/aecho.c.md#aep_send), [atp_rsel](../../libatalk/atp/atp_rsel.c.md#atp_rsel), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [atp_sresp](../../libatalk/atp/atp_sresp.c.md#atp_sresp), [do_getnetinfo](../../bin/getzones/getzones.c.md#do_getnetinfo), [do_query](../../bin/getzones/getzones.c.md#do_query), [do_rtmp_rdr](../../bin/rtmpqry/rtmpqry.c.md#do_rtmp_rdr), [do_rtmp_request](../../bin/rtmpqry/rtmpqry.c.md#do_rtmp_request), [nbp_do_lookup_op](../../libatalk/nbp/nbp_lkup.c.md#nbp_do_lookup_op), [nbp_rgstr](../../libatalk/nbp/nbp_rgstr.c.md#nbp_rgstr), [nbp_unrgstr](../../libatalk/nbp/nbp_unrgstr.c.md#nbp_unrgstr), [resend_request](../../libatalk/atp/atp_rsel.c.md#resend_request)

### netddp_recvfrom

```c
static ssize_t netddp_recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
```

Defined at lines 42 to 47.

Receive a DDP datagram through recvfrom.

Called by: [atp_recv_atp](../../libatalk/atp/atp_packet.c.md#atp_recv_atp), [do_getnetinfo](../../bin/getzones/getzones.c.md#do_getnetinfo), [do_query](../../bin/getzones/getzones.c.md#do_query), [do_rtmp_rdr](../../bin/rtmpqry/rtmpqry.c.md#do_rtmp_rdr), [do_rtmp_request](../../bin/rtmpqry/rtmpqry.c.md#do_rtmp_request), [nbp_do_lookup_op](../../libatalk/nbp/nbp_lkup.c.md#nbp_do_lookup_op), [nbp_rgstr](../../libatalk/nbp/nbp_rgstr.c.md#nbp_rgstr), [nbp_unrgstr](../../libatalk/nbp/nbp_unrgstr.c.md#nbp_unrgstr)
