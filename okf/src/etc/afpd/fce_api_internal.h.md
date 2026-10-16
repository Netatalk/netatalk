---
type: C Header File
title: "etc/afpd/fce_api_internal.h"
description: "Internal file change event packet, history and coalescing types."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fce_api_internal.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
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

Defined at line 41.
* `time_t time`
* `char path`

### struct fce_history

Defined at line 35.
* `fce_ev_t fce_h_event`
* `char fce_h_path`
* `struct timeval fce_h_tv`

### struct udp_entry

Defined at line 26.
* `int sock`
* `char * addr`
* `char * port`
* `struct addrinfo addrinfo`
* `struct sockaddr_storage sockaddr`
* `time_t next_try_on_error`: In case of error set next timestamp to retry.

# Macros

* `FCE_HISTORY_LEN`: This is used to coalesce events.
* `FCE_MAX_UDP_SOCKS`: Allow a maximum of udp listeners for file change events.
* `FCE_SOCKET_RETRY_DELAY_S`: Pause this time in s after socket was broken.
* `MAX_COALESCE_TIME_MS`: Events older than this are not coalesced.
* Undocumented: `FCE_COALESCE_ALL`, `FCE_COALESCE_CREATE`, `FCE_COALESCE_DELETE`, `PACKET_HDR_LEN`
