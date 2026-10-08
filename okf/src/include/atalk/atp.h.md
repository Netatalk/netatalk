---
type: C Header File
title: "include/atalk/atp.h"
description: "ATP packet format."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/atp.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `stdint.h`, `sys/time.h`, `sys/types.h`

# Included by

* [bin/getzones/getzones.c](../../bin/getzones/getzones.c.md)
* [bin/pap/pap.c](../../bin/pap/pap.c.md)
* [bin/pap/papstatus.c](../../bin/pap/papstatus.c.md)
* [etc/afpd/afp_asp.c](../../etc/afpd/afp_asp.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/status.c](../../etc/afpd/status.c.md)
* [etc/atalkd/main.c](../../etc/atalkd/main.c.md)
* [etc/atalkd/nbp.c](../../etc/atalkd/nbp.c.md)
* [etc/atalkd/rtmp.c](../../etc/atalkd/rtmp.c.md)
* [etc/atalkd/zip.c](../../etc/atalkd/zip.c.md)
* [etc/papd/lp.c](../../etc/papd/lp.c.md)
* [etc/papd/main.c](../../etc/papd/main.c.md)
* [etc/papd/ppd.c](../../etc/papd/ppd.c.md)
* [etc/papd/print_cups.c](../../etc/papd/print_cups.c.md)
* [etc/papd/queries.c](../../etc/papd/queries.c.md)
* [etc/papd/session.c](../../etc/papd/session.c.md)
* [etc/papd/session.h](../../etc/papd/session.h.md)
* [include/atalk/asp.h](asp.h.md)
* [libatalk/asp/asp_attn.c](../../libatalk/asp/asp_attn.c.md)
* [libatalk/asp/asp_close.c](../../libatalk/asp/asp_close.c.md)
* [libatalk/asp/asp_cmdreply.c](../../libatalk/asp/asp_cmdreply.c.md)
* [libatalk/asp/asp_getreq.c](../../libatalk/asp/asp_getreq.c.md)
* [libatalk/asp/asp_getsess.c](../../libatalk/asp/asp_getsess.c.md)
* [libatalk/asp/asp_init.c](../../libatalk/asp/asp_init.c.md)
* [libatalk/asp/asp_shutdown.c](../../libatalk/asp/asp_shutdown.c.md)
* [libatalk/asp/asp_tickle.c](../../libatalk/asp/asp_tickle.c.md)
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

# Types

### struct atp_block

Defined at line 151.
* `struct sockaddr_at * atp_saddr`
* `struct sreq_st sreqdata`
* `struct rres_st rresdata`
* `struct rreq_st rreqdata`
* `struct sres_st sresdata`
* `union atp_block atp_data`
* `uint8_t atp_bitmap`

### struct atp_handle

Defined at line 106.
* `int atph_socket`
* `struct sockaddr_at atph_saddr`
* `uint16_t atph_tid`
* `uint16_t atph_rtid`
* `uint8_t atph_rxo`
* `int atph_rreltime`
* `struct atpbuf * atph_sent`
* `struct atpbuf * atph_queue`
* `int atph_reqtries`
* `int atph_reqto`
* `int atph_rrespcount`
* `uint8_t atph_rbitmap`
* `struct atpbuf * atph_reqpkt`
* `struct timeval atph_reqtv`
* `struct atpbuf * atph_resppkt`

### struct atpbuf

Defined at line 96.
* `struct atpbuf * atpbuf_next`
* `size_t atpbuf_dlen`
* `struct sockaddr_at atpbuf_addr`
* `char atpbuf_data`
* `struct atpxobuf atpbuf_xo`
* `union atpbuf atpbuf_info`

### struct atphdr

Defined at line 63.
* `uint8_t atphd_ctrlinfo`
* `uint8_t atphd_bitmap`
* `uint16_t atphd_tid`

### struct atpxobuf

Defined at line 89.
* `uint16_t atpxo_tid`
* `struct timeval atpxo_tv`
* `int atpxo_reltime`
* `struct atpbuf * atpxo_packet`

### struct rreq_st

Defined at line 141.
* `char * atpd_data`
* `int atpd_dlen`

### struct rres_st

Defined at line 136.
* `struct iovec * atpd_iov`
* `int atpd_iovcnt`

### struct sreq_st

Defined at line 129.
* `char * atpd_data`
* `int atpd_dlen`
* `int atpd_tries`
* `int atpd_to`

### struct sres_st

Defined at line 146.
* `struct iovec * atpd_iov`
* `int atpd_iovcnt`

# Typedefs and enums

* `typedef struct atp_handle * ATP`

# Macros

* Undocumented: `ATP_BUFSIZ`, `ATP_EOM`, `ATP_FUNCMASK`, `ATP_HDRSIZE`, `ATP_MAXDATA`, `ATP_MAXQUEUE`, `ATP_MAXRESP`, `ATP_RELTIME`, `ATP_STS`, `ATP_TREL`, `ATP_TREL1M`, `ATP_TREL2M`, `ATP_TREL30`, `ATP_TREL4M`, `ATP_TREL8M`, `ATP_TRELMASK`, `ATP_TREQ`, `ATP_TRESP`, `ATP_TRIES_INFINITE`, `ATP_XO`, `atp_fileno`, `atp_rreqdata`, `atp_rreqdlen`, `atp_rresiov`, `atp_rresiovcnt`, `atp_sockaddr`, `atp_sreqdata`, `atp_sreqdlen`, `atp_sreqto`, `atp_sreqtries`, `atp_sresiov`, `atp_sresiovcnt`
