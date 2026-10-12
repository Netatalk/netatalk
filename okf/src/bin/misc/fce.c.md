---
type: C Source File
title: "bin/misc/fce.c"
description: "The fce_listen command: receive and print FCE events from afpd."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/misc/fce.c"
tags: ["bin/misc"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [bin/misc](../misc.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `inttypes.h`, `netdb.h`, `netinet/in.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/types.h`, `unistd.h`

# Functions

### sig_handler

```c
static void sig_handler(int signo)
```

Defined at lines 37 to 41.

Note SIGINT or SIGTERM, so the listener loop stops.

Parameters:
* `signo`: signal number, unused

Called by: [main](fce.c.md#main)

Uses file-scope variables: `stop_requested`

### unpack_fce_packet

```c
static int unpack_fce_packet(unsigned char *buf, size_t buflen, struct fce_packet *packet)
```

Defined at lines 69 to 217.

Decode an FCE datagram into a packet structure.

A version above 1 adds an options byte, a padding byte, and 8 reserved bytes. The options pick the optional PID, user, and source path fields. Each field has to fit the datagram, and each string has to fit its buffer with room for the NUL. The call leaves the magic check to its caller.

Parameters:
* `buf`: datagram
* `buflen`: datagram length
* `packet`: receives the fields

Returns: 0, or -1 for a short datagram or a field that is too long

Called by: [main](fce.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 232 to 398.

Listen for FCE datagrams from afpd and print each event.

The listener binds a UDP socket to the -h host and the -p port. The defaults are localhost and the FCE default port. It prints one line per event and stops on SIGINT or SIGTERM. A datagram that does not decode gets a line on stderr, and the call ignores datagrams with another magic.

Parameters:
* `argc`: argument count
* `argv`: argument vector

Returns: 0 after SIGINT or SIGTERM, 2 when no socket binds, or 1 for another error

Calls: [sig_handler](fce.c.md#sig_handler), [unpack_fce_packet](fce.c.md#unpack_fce_packet)

Uses file-scope variables: `fce_ev_names`, `stop_requested`

# Macros

* Undocumented: `MAXBUFLEN`

# File-scope variables

`fce_ev_names`, `stop_requested`
