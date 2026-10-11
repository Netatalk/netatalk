---
type: C Header File
title: "include/atalk/atp.h"
description: "ATP packet format."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/atp.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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
* `struct sockaddr_at * atp_saddr`: from/to address
* `struct sreq_st sreqdata`
* `struct rres_st rresdata`
* `struct rreq_st rreqdata`
* `struct sres_st sresdata`
* `union atp_block atp_data`
* `uint8_t atp_bitmap`: response buffer bitmap

### struct atp_handle

Defined at line 106.
* `int atph_socket`: ddp socket
* `struct sockaddr_at atph_saddr`: address
* `uint16_t atph_tid`: last tid used
* `uint16_t atph_rtid`: last received (rreq)
* `uint8_t atph_rxo`: XO flag from last rreq.
* `int atph_rreltime`: release time (secs)
* `struct atpbuf * atph_sent`: packets we send (XO)
* `struct atpbuf * atph_queue`: queue of pending packets
* `int atph_reqtries`: retry count for request
* `int atph_reqto`: retry timeout for request
* `int atph_rrespcount`: expected # of responses
* `uint8_t atph_rbitmap`: bitmap for request
* `struct atpbuf * atph_reqpkt`: last request packet
* `struct timeval atph_reqtv`: when we last sent request
* `struct atpbuf * atph_resppkt`: response to request

### struct atpbuf

Defined at line 96.
* `struct atpbuf * atpbuf_next`: next buffer in chain
* `size_t atpbuf_dlen`: data length <= ATP_BUFSIZ
* `struct sockaddr_at atpbuf_addr`: net address sent/recvd
* `char atpbuf_data`: the data
* `struct atpxobuf atpbuf_xo`: for XO requests
* `union atpbuf atpbuf_info`

### struct atphdr

Defined at line 63.
* `uint8_t atphd_ctrlinfo`: control information
* `uint8_t atphd_bitmap`: bitmap or sequence number
* `uint16_t atphd_tid`: transaction id.

### struct atpxobuf

Defined at line 89.
* `uint16_t atpxo_tid`
* `struct timeval atpxo_tv`
* `int atpxo_reltime`
* `struct atpbuf * atpxo_packet`

### struct rreq_st

Defined at line 141.
* `char * atpd_data`: request data
* `int atpd_dlen`

### struct rres_st

Defined at line 136.
* `struct iovec * atpd_iov`: for response
* `int atpd_iovcnt`

### struct sreq_st

Defined at line 129.
* `char * atpd_data`: request data
* `int atpd_dlen`
* `int atpd_tries`: max.
* `int atpd_to`: retry interval

### struct sres_st

Defined at line 146.
* `struct iovec * atpd_iov`: for response
* `int atpd_iovcnt`

# Typedefs and enums

* `typedef struct atp_handle * ATP`

# Macros

* `ATP_BUFSIZ`: maximum packet size
* `ATP_EOM`: End Of Message.
* `ATP_FUNCMASK`: mask all but function
* `ATP_HDRSIZE`: includes DDP type field
* `ATP_MAXDATA`: maximum ATP data size
* `ATP_MAXQUEUE`: bound on unmatched incoming packets
* `ATP_MAXRESP`: response packets per transaction
* `ATP_RELTIME`: base release timer (in secs)
* `ATP_STS`: Send Transaction Status.
* `ATP_TREL`: Trans.
* `ATP_TREL1M`: these are passed in flags of
* `ATP_TREL2M`: atp_sreq call, and set in the
* `ATP_TREL30`: release time codes
* `ATP_TREL4M`: packet control info.
* `ATP_TRELMASK`: mask all but TREL
* `ATP_TREQ`: Trans.
* `ATP_TRESP`: Trans.
* `ATP_TRIES_INFINITE`: for atp_sreq, etc
* `ATP_XO`: eXactly Once mode
* Undocumented: `ATP_TREL8M`, `atp_fileno`, `atp_rreqdata`, `atp_rreqdlen`, `atp_rresiov`, `atp_rresiovcnt`, `atp_sockaddr`, `atp_sreqdata`, `atp_sreqdlen`, `atp_sreqto`, `atp_sreqtries`, `atp_sresiov`, `atp_sresiovcnt`
