---
type: C Header File
title: "include/atalk/asp.h"
description: "1 type, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/asp.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](afp.h.md)
* [atalk/atp.h](atp.h.md)
* [atalk/server_child.h](server_child.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `sys/types.h`

# Included by

* [etc/afpd/afp_asp.c](../../etc/afpd/afp_asp.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/auth.c](../../etc/afpd/auth.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/afpd/fork.c](../../etc/afpd/fork.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/status.c](../../etc/afpd/status.c.md)
* [etc/afpd/status.h](../../etc/afpd/status.h.md)
* [etc/papd/uam.c](../../etc/papd/uam.c.md)
* [libatalk/asp/asp_attn.c](../../libatalk/asp/asp_attn.c.md)
* [libatalk/asp/asp_close.c](../../libatalk/asp/asp_close.c.md)
* [libatalk/asp/asp_cmdreply.c](../../libatalk/asp/asp_cmdreply.c.md)
* [libatalk/asp/asp_getreq.c](../../libatalk/asp/asp_getreq.c.md)
* [libatalk/asp/asp_getsess.c](../../libatalk/asp/asp_getsess.c.md)
* [libatalk/asp/asp_init.c](../../libatalk/asp/asp_init.c.md)
* [libatalk/asp/asp_shutdown.c](../../libatalk/asp/asp_shutdown.c.md)
* [libatalk/asp/asp_tickle.c](../../libatalk/asp/asp_tickle.c.md)
* [libatalk/asp/asp_write.c](../../libatalk/asp/asp_write.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)

# Types

### struct ASP

Defined at line 49.
* `ATP asp_atp`
* `struct sockaddr_at asp_sat`
* `uint8_t asp_wss`
* `uint8_t asp_sid`
* `char * as_status`
* `int as_slen`
* `struct ASP asu_status`
* `uint16_t asu_seq`
* `union ASP asp_u`
* `int asp_flags`
* `char child`
* `char inited`
* `char * commands`
* `char cmdbuf`
* `char data`
* `size_t cmdlen`
* `size_t datalen`
* `off_t read_count`
* `off_t write_count`
* `int asp_ipc_fd`
* `int asp_hint_fd`
* `int asp_cnx_cnt`
* `int asp_cnx_max`

# Typedefs and enums

* `typedef struct ASP * ASP`

# Macros

* Undocumented: `ASPERR_BADVERS`, `ASPERR_BUFSMALL`, `ASPERR_NOACK`, `ASPERR_NOSERV`, `ASPERR_NOSESS`, `ASPERR_OK`, `ASPERR_PARM`, `ASPERR_SERVBUSY`, `ASPERR_SESSCLOS`, `ASPERR_SIZERR`, `ASPERR_TOOMANY`, `ASPFL_SLS`, `ASPFL_SSS`, `ASPFUNC_ATTN`, `ASPFUNC_CLOSE`, `ASPFUNC_CMD`, `ASPFUNC_OPEN`, `ASPFUNC_STAT`, `ASPFUNC_TICKLE`, `ASPFUNC_WRITE`, `ASPFUNC_WRTCONT`, `ASP_CMDMAXSIZ`, `ASP_CMDSIZ`, `ASP_DATAMAXSIZ`, `ASP_DATASIZ`, `ASP_ERR_READ`, `ASP_ERR_SEQ`, `ASP_ERR_SID`, `ASP_HDRSIZ`, `ASP_MAXPACKETS`, `ASP_NOREQUEST`, `asp_seq`, `asp_slen`, `asp_status`, `asp_wrtreply`
