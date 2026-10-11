---
type: C Source File
title: "bin/aecho/aecho.c"
description: "AppleTalk Echo Protocol Client."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/aecho/aecho.c"
tags: ["bin/aecho"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Defined at lines 74 to 89.

Print the echo statistics and exit, on SIGINT or after the last reply.

The statistics give the packets sent and received, the loss, and the minimum, average, and maximum round trip in milliseconds.

Parameters:
* `sig`: signal number, unused

Called by: [aep_send](aecho.c.md#aep_send)

Uses file-scope variables: `maxms`, `minms`, `nrecv`, `nsent`, `target`, `totalms`

### aep_send

```c
static void aep_send(int sig)
```

Defined at lines 99 to 130.

Send the next AEP request on each SIGALRM, with a sequence number and the send time.

After the request count of -c, the call ends the run with done.

Parameters:
* `sig`: signal number, unused

Calls: [done](aecho.c.md#done), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Uses file-scope variables: `nsent`, `pings`, `sock`, `target`

### main

```c
int main(int ac, char **av)
```

Defined at lines 144 to 308.

Send AppleTalk echo requests to a node once a second and print the round trip of each reply.

The target is a net.node address or an NBP name, with Workstation as the default type and `*` as the default zone. The -A option sets the source address, and -c stops after a count of requests.

Parameters:
* `ac`: argument count
* `av`: argument vector

Returns: does not return, and a failure exits with 1

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [nbp_lookup](../../libatalk/nbp/nbp_lkup.c.md#nbp_lookup), [usage](aecho.c.md#usage)

Uses file-scope variables: `pings`, `target`

### usage

```c
static void usage(char *av0)
```

Defined at lines 315 to 320.

Print the usage of aecho and exit with 1.

Parameters:
* `av0`: program name

Called by: [main](aecho.c.md#main)

# Macros

* Undocumented: `SOCKLEN_T`

# File-scope variables

`maxms`, `minms`, `nrecv`, `nsent`, `pings`, `sock`, `target`, `totalms`
