---
type: C Source File
title: "bin/misc/fce.c"
description: "3 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/misc/fce.c"
tags: ["bin/misc"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 27 to 31.

Called by: [main](fce.c.md#main)

Uses file-scope variables: `stop_requested`

### unpack_fce_packet

```c
static int unpack_fce_packet(unsigned char *buf, size_t buflen, struct fce_packet *packet)
```

Defined at lines 46 to 194.

Called by: [main](fce.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 196 to 362.

Calls: [sig_handler](fce.c.md#sig_handler), [unpack_fce_packet](fce.c.md#unpack_fce_packet)

Uses file-scope variables: `fce_ev_names`, `stop_requested`

# Macros

* Undocumented: `MAXBUFLEN`

# File-scope variables

`fce_ev_names`, `stop_requested`
