---
type: C Source File
title: "libatalk/dsi/dsi_tcp.c"
description: "10 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_tcp.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `net/if.h`, `netdb.h`, `netinet/in.h`, `netinet/tcp.h`, `signal.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `tcpd.h`, `unistd.h`

# Functions

### dsi_tcp_close

```c
static void dsi_tcp_close(DSI *dsi)
```

Defined at lines 69 to 77.

Close the session socket, the TCP proto_close hook.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session, its socket becomes -1

Called by: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

### timeout_handler

```c
static void timeout_handler(int sig)
```

Defined at lines 88 to 92.

End a new session process that missed the deadline for its first [DSI](../../include/atalk/dsi.h.md#struct-dsi) request.

The SIGALRM timer of dsi_tcp_open runs it after 120 seconds, the DSI_TCPTIMEOUT limit.

Parameters:
* `sig`: unused

Called by: [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open)

### dsi_init_buffer

```c
static void dsi_init_buffer(DSI *dsi)
```

Defined at lines 102 to 118.

Allocate the command buffer and the read-ahead buffer of a session.

The command buffer holds one server quantum, and the read-ahead buffer holds dsireadbuf quanta. When malloc fails, the process aborts.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session with its quantum and dsireadbuf settled

Called by: [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open)

### dsi_effective_mss

```c
uint32_t dsi_effective_mss(const DSI *)
```

Defined at lines 130 to 173. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Effective TCP segment payload of the session socket.

Live values from TCP_INFO or TCP_CONNECTION_INFO already account for the active TCP options. TCP_MAXSEG, the negotiated value, is the fallback.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session with a connected socket

Returns: the segment payload in bytes, or 0 when it is unavailable or outside 512 to 65536

Called by: [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open)

### dsi_align_quantum

```c
uint32_t dsi_align_quantum(uint32_t configured, uint32_t mss)
```

Defined at lines 192 to 229. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Round the configured server quantum up so FPWriteExt frames fill their last TCP segment.

The result is the smallest 4 KiB multiple at or above configured that fills the last TCP segment well. An FPWriteExt frame is the data plus 36 bytes. A last segment at least 7/8 full counts as filled. Growth stops at DSI_QUANTUM_GROWTH_MAX above configured and at DSI_SERVQUANT_MAX, and the call returns the best fill when no candidate reaches the threshold. Without a usable mss, or when the quantum fits in four segments, the result only rounds up to a page.

Parameters:
* `configured`: quantum from the configuration, capped at DSI_SERVQUANT_MAX
* `mss`: TCP segment payload from dsi_effective_mss, or 0

Returns: the rounded quantum, never below the capped configured value

Called by: [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open)

### dsi_free

```c
void dsi_free(DSI *dsi)
```

Defined at lines 237 to 249. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Release the listener resources of the master afpd and close the server socket.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener, its socket and buffers become invalid

Called by: [configfree](../../etc/afpd/afp_config.c.md#configfree)

### dsi_tcp_open

```c
static pid_t dsi_tcp_open(DSI *dsi)
```

Defined at lines 267 to 419.

Accept a client connection and fork its session process, the TCP proto_open hook.

TCP wrappers can refuse the peer with ECONNREFUSED. The child resets the signal handlers and gives the client 120 seconds, the DSI_TCPTIMEOUT limit, for its first request. It derives the session quantum from the TCP segment size, sizes the read-ahead buffer, and reads the first [DSI](../../include/atalk/dsi.h.md#struct-dsi) header and payload. A malformed header or a payload beyond the server quantum ends the child.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener, which the child keeps as its session

Returns: the child pid in the parent, 0 in the child, or -1 when accept or fork failed

Calls: [dsi_align_quantum](dsi_tcp.c.md#dsi_align_quantum), [dsi_effective_mss](dsi_tcp.c.md#dsi_effective_mss), [dsi_init_buffer](dsi_tcp.c.md#dsi_init_buffer), [dsi_stream_read](dsi_stream.c.md#dsi_stream_read), [getip_port](../util/socket.c.md#getip_port), [getip_string](../util/socket.c.md#getip_string), [netatalk_readbuf_clamp](../util/netatalk_conf.c.md#netatalk_readbuf_clamp), [server_reset_signal](../util/server_child.c.md#server_reset_signal), [timeout_handler](dsi_tcp.c.md#timeout_handler)

Called by: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

Uses file-scope variables: `deny_severity`, `itimer`

### guess_interface

```c
static void guess_interface(DSI *dsi, const char *hostname, const char *port)
```

Defined at lines 439 to 490.

Advertise the address of the first usable network interface.

The call runs when hostname resolves to nothing or only to a loopback address. It walks the interface list, skips loopback, point-to-point, bonding slave, and down interfaces, and takes the first remaining one. dsi->server receives that IPv4 address with port. Without a match it logs a message naming hostname.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener whose server address to set
* `hostname`: server host name, for the log message
* `port`: TCP port as a decimal string

Calls: [freeifacelist](../util/getiface.c.md#freeifacelist), [getifacelist](../util/getiface.c.md#getifacelist), [getip_string](../util/socket.c.md#getip_string), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

### dsi_tcp_listen

```c
static int dsi_tcp_listen(const char *address, const char *port, struct addrinfo *hints, DSI *dsi, bool *psocket_err_afnotsup)
```

Defined at lines 510 to 617.

Create the listening socket for address and port.

getaddrinfo supplies the candidates, and the call keeps the first one that binds and listens. Each socket gets SO_REUSEADDR and TCP_NODELAY, and an IPv6 socket also accepts IPv4 except on OpenBSD. The listen backlog is DSI_TCPMAXPEND. When every AF_INET6 socket call fails with EAFNOSUPPORT, the caller learns that the host has no IPv6 and retries with AF_UNSPEC.

Parameters:
* `address`: address to bind, or NULL for the wildcard address
* `port`: TCP port as a decimal string
* `hints`: getaddrinfo hints
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) listener receiving the socket and the bound address
* `psocket_err_afnotsup`: receives true when IPv6 is unavailable

Returns: 0, or -1 when no candidate accepted a bind and listen

Called by: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

### dsi_tcp_init

```c
int dsi_tcp_init(DSI *dsi, const char *hostname, const char *address, const char *port)
```

Defined at lines 645 to 769. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Initialize [DSI](../../include/atalk/dsi.h.md#struct-dsi) over TCP.

Creates listening AFP/DSI socket. If the parameter inaddress is NULL, then we listen on the wildcard address, i,e, on all interfaces. That should mean listening on the IPv6 address "::" on IPv4/IPv6 dual stack kernels, accepting both v4 and v6 requests.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) handle
* `hostname`: pointer to hostname string
* `inaddress`: Optional IPv4 or IPv6 address with an optional port, may be NULL
* `inport`: pointer to port string

If the parameter inaddress is not NULL, then we only listen on the given address. The parameter may contain a port number using the URL format for address and port:

IPv4, IPv4:port, IPv6, [IPv6], [IPv6]:port

Parameter inport must be a valid pointer to a port string and is used if the inaddress parameter doesn't contain a port.

Returns: 0 on success, -1 on failure

Calls: [dsi_tcp_close](dsi_tcp.c.md#dsi_tcp_close), [dsi_tcp_listen](dsi_tcp.c.md#dsi_tcp_listen), [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open), [guess_interface](dsi_tcp.c.md#guess_interface), [tokenize_ip_port](../util/socket.c.md#tokenize_ip_port)

Called by: [dsi_init](dsi_init.c.md#dsi_init)

# Macros

* `DSI_TCPMAXPEND`: max # of pending connections
* `DSI_TCPTIMEOUT`: timeout in seconds for connections
* Undocumented: `AI_NUMERICSERV`, `IFF_SLAVE`, `SOCKLEN_T`, `SOL_TCP`, `min`

# File-scope variables

`allow_severity`, `deny_severity`, `itimer`
