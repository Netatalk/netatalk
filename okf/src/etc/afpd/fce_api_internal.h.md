---
type: C Header File
title: "etc/afpd/fce_api_internal.h"
description: "Internal definitions for File Change Events API."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fce_api_internal.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* System headers: `stdbool.h`

# Included by

* [etc/afpd/fce_api.c](fce_api.c.md)
* [etc/afpd/fce_util.c](fce_util.c.md)

# Types

### struct fce_close_event

Defined at line 38.
* `time_t time`
* `char path`

### struct fce_history

Defined at line 32.
* `fce_ev_t fce_h_event`
* `char fce_h_path`
* `struct timeval fce_h_tv`

### struct udp_entry

Defined at line 23.
* `int sock`
* `char * addr`
* `char * port`
* `struct addrinfo addrinfo`
* `struct sockaddr_storage sockaddr`
* `time_t next_try_on_error`

# Macros

* Undocumented: `FCE_COALESCE_ALL`, `FCE_COALESCE_CREATE`, `FCE_COALESCE_DELETE`, `FCE_HISTORY_LEN`, `FCE_MAX_UDP_SOCKS`, `FCE_SOCKET_RETRY_DELAY_S`, `MAX_COALESCE_TIME_MS`, `PACKET_HDR_LEN`
