---
type: C Header File
title: "sys/netatalk/at.h"
description: "3 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/at.h"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `netinet/in.h`, `sys/types.h`

# Included by

* [bin/aecho/aecho.c](../../bin/aecho/aecho.c.md)
* [bin/getzones/getzones.c](../../bin/getzones/getzones.c.md)
* [bin/nbp/nbplkup.c](../../bin/nbp/nbplkup.c.md)
* [bin/nbp/nbprgstr.c](../../bin/nbp/nbprgstr.c.md)
* [bin/nbp/nbpunrgstr.c](../../bin/nbp/nbpunrgstr.c.md)
* [bin/pap/pap.c](../../bin/pap/pap.c.md)
* [bin/pap/papstatus.c](../../bin/pap/papstatus.c.md)
* [bin/rtmpqry/rtmpqry.c](../../bin/rtmpqry/rtmpqry.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/atalkd/aep.c](../../etc/atalkd/aep.c.md)
* [etc/atalkd/config.c](../../etc/atalkd/config.c.md)
* [etc/atalkd/main.c](../../etc/atalkd/main.c.md)
* [etc/atalkd/nbp.c](../../etc/atalkd/nbp.c.md)
* [etc/atalkd/route.c](../../etc/atalkd/route.c.md)
* [etc/atalkd/rtmp.c](../../etc/atalkd/rtmp.c.md)
* [etc/atalkd/zip.c](../../etc/atalkd/zip.c.md)
* [etc/papd/headers.c](../../etc/papd/headers.c.md)
* [etc/papd/lp.c](../../etc/papd/lp.c.md)
* [etc/papd/magics.c](../../etc/papd/magics.c.md)
* [etc/papd/main.c](../../etc/papd/main.c.md)
* [etc/papd/ppd.c](../../etc/papd/ppd.c.md)
* [etc/papd/queries.c](../../etc/papd/queries.c.md)
* [etc/papd/session.c](../../etc/papd/session.c.md)
* [include/atalk/asp.h](../../include/atalk/asp.h.md)
* [include/atalk/atp.h](../../include/atalk/atp.h.md)
* [include/atalk/globals.h](../../include/atalk/globals.h.md)
* [include/atalk/nbp.h](../../include/atalk/nbp.h.md)
* [include/atalk/netddp.h](../../include/atalk/netddp.h.md)
* [include/atalk/util.h](../../include/atalk/util.h.md)
* [libatalk/asp/asp_close.c](../../libatalk/asp/asp_close.c.md)
* [libatalk/asp/asp_getreq.c](../../libatalk/asp/asp_getreq.c.md)
* [libatalk/asp/asp_getsess.c](../../libatalk/asp/asp_getsess.c.md)
* [libatalk/asp/asp_init.c](../../libatalk/asp/asp_init.c.md)
* [libatalk/asp/asp_shutdown.c](../../libatalk/asp/asp_shutdown.c.md)
* [libatalk/asp/asp_write.c](../../libatalk/asp/asp_write.c.md)
* [libatalk/atp/atp_bufs.c](../../libatalk/atp/atp_bufs.c.md)
* [libatalk/atp/atp_close.c](../../libatalk/atp/atp_close.c.md)
* [libatalk/atp/atp_internals.h](../../libatalk/atp/atp_internals.h.md)
* [libatalk/atp/atp_open.c](../../libatalk/atp/atp_open.c.md)
* [libatalk/atp/atp_packet.c](../../libatalk/atp/atp_packet.c.md)
* [libatalk/atp/atp_rreq.c](../../libatalk/atp/atp_rreq.c.md)
* [libatalk/atp/atp_rresp.c](../../libatalk/atp/atp_rresp.c.md)
* [libatalk/atp/atp_rsel.c](../../libatalk/atp/atp_rsel.c.md)
* [libatalk/atp/atp_sreq.c](../../libatalk/atp/atp_sreq.c.md)
* [libatalk/atp/atp_sresp.c](../../libatalk/atp/atp_sresp.c.md)
* [libatalk/nbp/nbp_lkup.c](../../libatalk/nbp/nbp_lkup.c.md)
* [libatalk/nbp/nbp_rgstr.c](../../libatalk/nbp/nbp_rgstr.c.md)
* [libatalk/nbp/nbp_unrgstr.c](../../libatalk/nbp/nbp_unrgstr.c.md)
* [libatalk/nbp/nbp_util.c](../../libatalk/nbp/nbp_util.c.md)
* [libatalk/netddp/netddp_open.c](../../libatalk/netddp/netddp_open.c.md)
* [libatalk/util/atalk_addr.c](../../libatalk/util/atalk_addr.c.md)
* [sys/netatalk/aarp.c](aarp.c.md)
* [sys/netatalk/at_control.c](at_control.c.md)
* [sys/netatalk/at_proto.c](at_proto.c.md)
* [sys/netatalk/ddp_input.c](ddp_input.c.md)
* [sys/netatalk/ddp_output.c](ddp_output.c.md)
* [sys/netatalk/ddp_usrreq.c](ddp_usrreq.c.md)

# Types

### struct at_addr

Defined at line 68.
* `unsigned short s_net`
* `unsigned char s_node`

### struct netrange

Defined at line 118.
* `unsigned char nr_phase`
* `unsigned short nr_firstnet`
* `unsigned short nr_lastnet`

### struct sockaddr_at

Defined at line 88.
* `short sat_family`
* `unsigned char sat_port`
* `struct at_addr sat_addr`
* `char sat_zero`

# Macros

* Undocumented: `ATADDR_ANYNET`, `ATADDR_ANYNODE`, `ATADDR_ANYPORT`, `ATADDR_BCAST`, `ATPORT_FIRST`, `ATPORT_LAST`, `ATPORT_RESERVED`, `ATPROTO_AARP`, `ATPROTO_DDP`, `DDP_MAXSZ`, `ETHERTYPE_AARP`, `ETHERTYPE_AT`
