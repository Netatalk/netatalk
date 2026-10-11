---
type: C Source File
title: "libatalk/nbp/nbp_lkup.c"
description: "2 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/nbp/nbp_lkup.c"
tags: ["libatalk/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/nbp](../nbp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [nbp_conf.h](nbp_conf.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [netatalk/ddp.h](../../sys/netatalk/ddp.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `netdb.h`, `signal.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`

# Functions

### nbp_lookup

```c
int nbp_lookup(const char *, const char *, const char *, struct nbpnve *, const int, const struct at_addr *)
```

Defined at lines 45 to 50. Declared in [include/atalk/nbp.h](../../include/atalk/nbp.h.md).

Look up NBP names through the local atalkd with an NBPOP_BRRQ request.

Parameters:
* `obj`: object name, or NULL for `=`
* `type`: type name, or NULL for `=`
* `zone`: zone name, or NULL for the default zone `*`
* `nn`: array that receives the matches
* `nncnt`: size of nn
* `ataddr`: local address to send from, or NULL

Returns: the number of matches, or -1 for an error

Calls: [nbp_do_lookup_op](nbp_lkup.c.md#nbp_do_lookup_op)

Called by: [main](../../bin/aecho/aecho.c.md#main), [main](../../bin/pap/pap.c.md#main), [main](../../bin/pap/papstatus.c.md#main), [nbp_rgstr](nbp_rgstr.c.md#nbp_rgstr)

### nbp_do_lookup_op

```c
int nbp_do_lookup_op(const char *, const char *, const char *, struct nbpnve *, const int, const struct at_addr *, const struct at_addr *, uint8_t)
```

Defined at lines 72 to 286. Declared in [include/atalk/nbp.h](../../include/atalk/nbp.h.md).

Send an NBP lookup operation and collect the distinct replies.

The request goes to the NBP port of dstaddr, or of the local address when dstaddr is NULL, which reaches the local atalkd. The call sends it 3 times and waits 2 seconds for replies after each send. It stops early once nn is full. Two replies that match without regard to zone and wildcards count once. Success sets errno to 0, and a bad array sets EINVAL.

Parameters:
* `obj`: object name of up to NBPSTRLEN bytes, or NULL for `=`
* `type`: type name of up to NBPSTRLEN bytes, or NULL for `=`
* `zone`: zone name of up to NBPSTRLEN bytes, or NULL for `*`
* `nn`: array that receives the matches
* `nncnt`: size of nn
* `srcaddr`: local address to send from, or NULL
* `dstaddr`: node to ask, or NULL for the local atalkd
* `op`: NBP operation, such as NBPOP_BRRQ or NBPOP_LKUP

Returns: the number of matches, or -1 for a bad array, a name that is too long, or a socket error

Calls: [nbp_match](nbp_util.c.md#nbp_match), [nbp_parse](nbp_util.c.md#nbp_parse), [netddp_close](../../include/atalk/netddp.h.md#netddp_close), [netddp_open](../netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [main](../../bin/nbp/nbplkup.c.md#main), [nbp_lookup](nbp_lkup.c.md#nbp_lookup)

Uses file-scope variables: `nbp_id` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_port` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_recv` in [libatalk/nbp/nbp_util.c](nbp_util.c.md), `nbp_send` in [libatalk/nbp/nbp_util.c](nbp_util.c.md)

# Macros

* Undocumented: `SOCKLEN_T`
