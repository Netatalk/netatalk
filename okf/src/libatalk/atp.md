---
type: Subsystem
title: "libatalk/atp"
description: "10 files, 20 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/atp"
tags: ["libatalk/atp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [libatalk/atp/atp_bufs.c](atp/atp_bufs.c.md): 4 functions, includes 3 project headers.
* [libatalk/atp/atp_close.c](atp/atp_close.c.md): 1 function, includes 4 project headers.
* [libatalk/atp/atp_internals.h](atp/atp_internals.h.md): 1 function, includes 2 project headers.
* [libatalk/atp/atp_open.c](atp/atp_open.c.md): 1 function, includes 4 project headers.
* [libatalk/atp/atp_packet.c](atp/atp_packet.c.md): 5 functions, includes 5 project headers.
* [libatalk/atp/atp_rreq.c](atp/atp_rreq.c.md): 2 functions, includes 3 project headers.
* [libatalk/atp/atp_rresp.c](atp/atp_rresp.c.md): 1 function, includes 4 project headers.
* [libatalk/atp/atp_rsel.c](atp/atp_rsel.c.md): 3 functions, includes 6 project headers.
* [libatalk/atp/atp_sreq.c](atp/atp_sreq.c.md): 1 function, includes 5 project headers.
* [libatalk/atp/atp_sresp.c](atp/atp_sresp.c.md): 1 function, includes 5 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 22 includes
* [sys/netatalk](../sys/netatalk.md): 10 includes

# Calls into

* [include/atalk](../include/atalk.md): 7 calls
* [libatalk/util](util.md): 6 calls
* [libatalk/netddp](netddp.md): 1 calls

# Called from

* [libatalk/asp](asp.md): 15 calls
* [bin/pap](../bin/pap.md): 12 calls
* [etc/papd](../etc/papd.md): 10 calls
* [bin/getzones](../bin/getzones.md): 4 calls
* [etc/afpd](../etc/afpd.md): 3 calls

# Most called functions

* [atp_sreq](atp/atp_sreq.c.md#atp_sreq): 9 callers
* [atp_free_buf](atp/atp_bufs.c.md#atp_free_buf): 8 callers
* [atp_rresp](atp/atp_rresp.c.md#atp_rresp): 8 callers
* [atp_close](atp/atp_close.c.md#atp_close): 7 callers
* [atp_print_bufuse](atp/atp_internals.h.md#atp_print_bufuse): 7 callers
* [atp_alloc_buf](atp/atp_bufs.c.md#atp_alloc_buf): 6 callers
* [atp_open](atp/atp_open.c.md#atp_open): 6 callers
* [atp_sresp](atp/atp_sresp.c.md#atp_sresp): 6 callers
* [atp_rsel](atp/atp_rsel.c.md#atp_rsel): 5 callers
* [atp_rreq](atp/atp_rreq.c.md#atp_rreq): 4 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[atp_bufs_release](atp/atp_bufs.c.md#atp_bufs_release)
