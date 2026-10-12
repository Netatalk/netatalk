---
type: C Source File
title: "bin/rtmpqry/rtmpqry.c"
description: "The rtmpqry command: query an AppleTalk router with RTMP."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/rtmpqry/rtmpqry.c"
tags: ["bin/rtmpqry"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [bin/rtmpqry](../rtmpqry.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/netddp.h](../../include/atalk/netddp.h.md)
* [atalk/rtmp.h](../../include/atalk/rtmp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `arpa/inet.h`, `atalk/ddp.h`, `inttypes.h`, `stdbool.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `unistd.h`

# Functions

### usage

```c
static void usage(char *s)
```

Defined at lines 51 to 56.

Print the usage of rtmpqry to stderr and exit with 1.

Parameters:
* `s`: program name

Called by: [main](rtmpqry.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 71 to 153.

Query an AppleTalk router with RTMP and print its answer.

By default the tool sends a Route Data Request and prints the routing tuples of the replies for about 3 seconds. The -a option asks for all routes without split horizon. The -r option sends an RTMP Request, which asks for the network of the router, and it excludes -a. The -A option sets the local address, and the remote address defaults to the local node.

Parameters:
* `argc`: argument count
* `argv`: argument vector

Returns: 0, while errors exit with 1

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [do_rtmp_rdr](rtmpqry.c.md#do_rtmp_rdr), [do_rtmp_request](rtmpqry.c.md#do_rtmp_request), [setup_ddp_socket](rtmpqry.c.md#setup_ddp_socket), [usage](rtmpqry.c.md#usage)

### setup_ddp_socket

```c
int setup_ddp_socket(const struct at_addr *local_addr, struct at_addr **remote_addr)
```

Defined at lines 167 to 188.

Open a DDP socket for the RTMP queries.

Without a remote address the call fills in the address that the kernel bound the socket to, so the query goes to the local node. A socket error exits with 1.

Parameters:
* `local_addr`: local address to bind, all zeros for any
* `remote_addr`: remote address, which the call fills in when it is NULL

Returns: the socket

Calls: [netddp_open](../../libatalk/netddp/netddp_open.c.md#netddp_open)

Called by: [main](rtmpqry.c.md#main)

### do_rtmp_request

```c
void do_rtmp_request(int sockfd, struct sockaddr_at *sa_remote)
```

Defined at lines 201 to 291.

Send an RTMP Request and print the router and the networks from its reply.

A reply from an extended network carries the network range. The call waits without a timeout and skips other datagrams. A wrong marker byte or RTMP version exits with 1.

Parameters:
* `sockfd`: DDP socket
* `sa_remote`: RTMP socket of the router

Calls: [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [main](rtmpqry.c.md#main)

### do_rtmp_rdr

```c
void do_rtmp_rdr(int sockfd, const struct sockaddr_at *sa_remote, bool get_all, int secs_timeout)
```

Defined at lines 307 to 386.

Send an RTMP Route Data Request and print the routing data of each reply.

Routers send RTMP data packets to every node on a timer, and the request asks for them at this socket. The request asks for split horizon unless `get_all` asks for all routes. The call prints each data packet that arrives within `secs_timeout` plus up to 1 more second.

Parameters:
* `sockfd`: DDP socket
* `sa_remote`: RTMP socket of the router
* `get_all`: true to ask for all routes without split horizon
* `secs_timeout`: minimum seconds to collect replies

Calls: [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto), [print_rtmp_data_packet](rtmpqry.c.md#print_rtmp_data_packet)

Called by: [main](rtmpqry.c.md#main)

### print_rtmp_tuple

```c
const uint8_t * print_rtmp_tuple(const uint8_t *buf, const uint8_t *cursor, size_t len, uint16_t router_net, uint8_t router_node)
```

Defined at lines 401 to 446.

Print one routing tuple of an RTMP data packet.

An extended tuple carries a network range and an RTMP version. The call skips a tuple with another version and prints a message.

Parameters:
* `buf`: packet
* `cursor`: start of the tuple in `buf`
* `len`: packet length
* `router_net`: network of the sending router
* `router_node`: node of the sending router

Returns: the start of the next tuple, or NULL when no tuple fits

Called by: [print_rtmp_data_packet](rtmpqry.c.md#print_rtmp_data_packet)

### print_rtmp_data_packet

```c
void print_rtmp_data_packet(const uint8_t *buf, size_t len)
```

Defined at lines 458 to 522.

Print the routing tuples of an RTMP data packet.

The call skips other datagrams and packets with a node ID length other than 8 bits. On a nonextended network a version marker comes before the tuples.

Parameters:
* `buf`: datagram
* `len`: datagram length

Calls: [print_rtmp_tuple](rtmpqry.c.md#print_rtmp_tuple)

Called by: [do_rtmp_rdr](rtmpqry.c.md#do_rtmp_rdr)
