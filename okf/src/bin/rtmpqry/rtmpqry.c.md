---
type: C Source File
title: "bin/rtmpqry/rtmpqry.c"
description: "7 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/rtmpqry/rtmpqry.c"
tags: ["bin/rtmpqry"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 41 to 46.

Called by: [main](rtmpqry.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 48 to 130.

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [do_rtmp_rdr](rtmpqry.c.md#do_rtmp_rdr), [do_rtmp_request](rtmpqry.c.md#do_rtmp_request), [setup_ddp_socket](rtmpqry.c.md#setup_ddp_socket), [usage](rtmpqry.c.md#usage)

### setup_ddp_socket

```c
int setup_ddp_socket(const struct at_addr *local_addr, struct at_addr **remote_addr)
```

Defined at lines 138 to 159.

Set up a DDP socket ready for sending RTMP packets.

The remote_addr is a pointer to a pointer so that if it's null we can fill it in with the address the kernel provides, as we do in [libatalk/nbp/nbp_lkup.c](../../libatalk/nbp/nbp_lkup.c.md) for example.

Calls: [netddp_open](../../libatalk/netddp/netddp_open.c.md#netddp_open)

Called by: [main](rtmpqry.c.md#main)

### do_rtmp_request

```c
void do_rtmp_request(int sockfd, struct sockaddr_at *sa_remote)
```

Defined at lines 167 to 257.

Send an RTMP Request and wait for a reply.

An RTMP request just requests details of the network this node is connected to; in general, these aren't used on extended networks (instead, a ZIP GetNetInfo is used) but they should be supported anyway.

Calls: [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto)

Called by: [main](rtmpqry.c.md#main)

### do_rtmp_rdr

```c
void do_rtmp_rdr(int sockfd, const struct sockaddr_at *sa_remote, bool get_all, int secs_timeout)
```

Defined at lines 266 to 345.

Send an RTMP Route Data Request (RDR) and wait for a reply.

An RTMP RDR solicits RTMP data packets - generally, these are sent out broadcast on a timer, but one can specifically ask for them to be sent to a non-standard socket by means of an RDR. An RDR can also specify whether or not for the router to do split horizon processing (or whether to return all routes).

Calls: [netddp_recvfrom](../../include/atalk/netddp.h.md#netddp_recvfrom), [netddp_sendto](../../include/atalk/netddp.h.md#netddp_sendto), [print_rtmp_data_packet](rtmpqry.c.md#print_rtmp_data_packet)

Called by: [main](rtmpqry.c.md#main)

### print_rtmp_tuple

```c
const uint8_t * print_rtmp_tuple(const uint8_t *buf, const uint8_t *cursor, size_t len, uint16_t router_net, uint8_t router_node)
```

Defined at lines 351 to 396.

Print an RTMP tuple pointed to by 'cursor' within the buffer 'buf' with length 'len'.

Returns: the next value of 'cursor' or NULL if no more tuples exist.

Called by: [print_rtmp_data_packet](rtmpqry.c.md#print_rtmp_data_packet)

### print_rtmp_data_packet

```c
void print_rtmp_data_packet(const uint8_t *buf, size_t len)
```

Defined at lines 398 to 462.

Calls: [print_rtmp_tuple](rtmpqry.c.md#print_rtmp_tuple)

Called by: [do_rtmp_rdr](rtmpqry.c.md#do_rtmp_rdr)
