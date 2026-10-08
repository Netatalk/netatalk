---
type: C Source File
title: "bin/aecho/aecho.c"
description: "4 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/aecho/aecho.c"
tags: ["bin/aecho"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/aecho](../aecho.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/aep.h](../../include/atalk/aep.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `atalk/ddp.h`, `errno.h`, `netdb.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`

# Functions

### done

```c
static void done(int sig)
```

Defined at lines 66 to 81.

Called by: [aep_send](aecho.c.md#aep_send), [main](aecho.c.md#main)

Uses file-scope variables: `maxms`, `minms`, `nrecv`, `nsent`, `target`, `totalms`

### aep_send

```c
static void aep_send(int sig)
```

Defined at lines 83 to 114.

Calls: [done](aecho.c.md#done), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [main](aecho.c.md#main)

Uses file-scope variables: `nsent`, `pings`, `sock`, `target`

### main

```c
int main(int ac, char **av)
```

Defined at lines 116 to 280.

Calls: [aep_send](aecho.c.md#aep_send), [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [done](aecho.c.md#done), [nbp_lookup](../../libatalk/nbp/nbp_lkup.c.md#nbp_lookup), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [netddp_open](../../libatalk/netddp/netddp_open.c.md#netddp_open), [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [usage](aecho.c.md#usage)

Uses file-scope variables: `maxms`, `minms`, `nrecv`, `pings`, `sock`, `target`, `totalms`

### usage

```c
static void usage(char *)
```

Defined at lines 282 to 287.

Called by: [main](aecho.c.md#main)

# Macros

* Undocumented: `SOCKLEN_T`

# File-scope variables

`maxms`, `minms`, `nrecv`, `nsent`, `pings`, `sock`, `target`, `totalms`
