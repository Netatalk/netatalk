---
type: C Source File
title: "libatalk/dsi/dsi_tcp.c"
description: "10 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_tcp.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 64 to 72.

Called by: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

### timeout_handler

```c
static void timeout_handler(int sig)
```

Defined at lines 75 to 79.

alarm handler for tcp_open

Called by: [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open)

### dsi_init_buffer

```c
static void dsi_init_buffer(DSI *dsi)
```

Defined at lines 84 to 100.

Allocate [DSI](../../include/atalk/dsi.h.md#struct-dsi) read buffer and read-ahead buffer

Called by: [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open)

### dsi_effective_mss

```c
uint32_t dsi_effective_mss(const DSI *)
```

Defined at lines 108 to 151. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Effective TCP segment payload for this session's socket, or 0 when it cannot be determined. Live values (TCP_INFO / TCP_CONNECTION_INFO) already account for active TCP options; TCP_MAXSEG is the negotiated fallback.

Called by: [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open)

### dsi_align_quantum

```c
uint32_t dsi_align_quantum(uint32_t configured, uint32_t mss)
```

Defined at lines 161 to 198. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Smallest 4 KiB multiple at or above the configured quantum whose FPWriteExt frame (data + 36 bytes) ends in a TCP segment at least 7/8 full. Growth is capped at DSI_QUANTUM_GROWTH_MAX and DSI_SERVQUANT_MAX; best fill wins when the threshold is unreachable. Never returns less than the configured value, which is itself capped at DSI_SERVQUANT_MAX.

Called by: [dsi_tcp_open](dsi_tcp.c.md#dsi_tcp_open)

### dsi_free

```c
void dsi_free(DSI *dsi)
```

Defined at lines 204 to 216. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Free any allocated resources of the master afpd [DSI](../../include/atalk/dsi.h.md#struct-dsi) objects and close server socket

Called by: [configfree](../../etc/afpd/afp_config.c.md#configfree)

### dsi_tcp_open

```c
static pid_t dsi_tcp_open(DSI *dsi)
```

Defined at lines 220 to 372.

accept the socket and do a little sanity checking

Calls: [dsi_align_quantum](dsi_tcp.c.md#dsi_align_quantum), [dsi_effective_mss](dsi_tcp.c.md#dsi_effective_mss), [dsi_init_buffer](dsi_tcp.c.md#dsi_init_buffer), [dsi_stream_read](dsi_stream.c.md#dsi_stream_read), [getip_port](../util/socket.c.md#getip_port), [getip_string](../util/socket.c.md#getip_string), [netatalk_readbuf_clamp](../util/netatalk_conf.c.md#netatalk_readbuf_clamp), [server_reset_signal](../util/server_child.c.md#server_reset_signal), [timeout_handler](dsi_tcp.c.md#timeout_handler)

Called by: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

Uses file-scope variables: `deny_severity`, `itimer`

### guess_interface

```c
static void guess_interface(DSI *dsi, const char *hostname, const char *port)
```

Defined at lines 379 to 430.

Calls: [freeifacelist](../util/getiface.c.md#freeifacelist), [getifacelist](../util/getiface.c.md#getifacelist), [getip_string](../util/socket.c.md#getip_string), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

### dsi_tcp_listen

```c
static int dsi_tcp_listen(const char *address, const char *port, struct addrinfo *hints, DSI *dsi, bool *psocket_err_afnotsup)
```

Defined at lines 432 to 539.

Called by: [dsi_tcp_init](dsi_tcp.c.md#dsi_tcp_init)

### dsi_tcp_init

```c
int dsi_tcp_init(DSI *dsi, const char *hostname, const char *address, const char *port)
```

Defined at lines 567 to 691. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

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

* Undocumented: `AI_NUMERICSERV`, `DSI_TCPMAXPEND`, `DSI_TCPTIMEOUT`, `IFF_SLAVE`, `SOCKLEN_T`, `SOL_TCP`, `min`

# File-scope variables

`allow_severity`, `deny_severity`, `itimer`
