---
type: Subsystem
title: "libatalk/dsi"
description: "12 files, 42 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/dsi"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [libatalk/dsi/dsi_attn.c](dsi/dsi_attn.c.md): 1 function, includes 3 project headers.
* [libatalk/dsi/dsi_close.c](dsi/dsi_close.c.md): 1 function, includes 1 project header.
* [libatalk/dsi/dsi_cmdreply.c](dsi/dsi_cmdreply.c.md): 1 function, includes 2 project headers.
* [libatalk/dsi/dsi_getsess.c](dsi/dsi_getsess.c.md): 6 functions, includes 5 project headers.
* [libatalk/dsi/dsi_getstat.c](dsi/dsi_getstat.c.md): 1 function, includes 1 project header.
* [libatalk/dsi/dsi_init.c](dsi/dsi_init.c.md): 1 function, includes 1 project header.
* [libatalk/dsi/dsi_opensess.c](dsi/dsi_opensess.c.md): 1 function, includes 3 project headers.
* [libatalk/dsi/dsi_read.c](dsi/dsi_read.c.md): 3 functions, includes 3 project headers.
* [libatalk/dsi/dsi_stream.c](dsi/dsi_stream.c.md): 13 functions, includes 3 project headers.
* [libatalk/dsi/dsi_tcp.c](dsi/dsi_tcp.c.md): 10 functions, includes 6 project headers.
* [libatalk/dsi/dsi_tickle.c](dsi/dsi_tickle.c.md): 1 function, includes 1 project header.
* [libatalk/dsi/dsi_write.c](dsi/dsi_write.c.md): 3 functions, includes 3 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 32 includes

# Calls into

* [libatalk/util](util.md): 18 calls
* [libatalk/adouble](adouble.md): 1 calls
* [libatalk/compat](compat.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 36 calls
* [libatalk/asp](asp.md): 1 calls

# Most called functions

* [dsi_attention](dsi/dsi_attn.c.md#dsi_attention): 5 callers
* [dsi_stream_read](dsi/dsi_stream.c.md#dsi_stream_read): 5 callers
* [dsi_stream_write](dsi/dsi_stream.c.md#dsi_stream_write): 5 callers
* [dsi_close_spare_fd](dsi/dsi_getsess.c.md#dsi_close_spare_fd): 3 callers
* [dsi_peek](dsi/dsi_stream.c.md#dsi_peek): 3 callers
* [dsi_read](dsi/dsi_read.c.md#dsi_read): 3 callers
* [dsi_readdone](dsi/dsi_read.c.md#dsi_readdone): 3 callers
* [dsi_readinit](dsi/dsi_read.c.md#dsi_readinit): 3 callers
* [dsi_writeflush](dsi/dsi_write.c.md#dsi_writeflush): 3 callers
* [dsi_writeinit](dsi/dsi_write.c.md#dsi_writeinit): 3 callers
