---
type: C Source File
title: "libatalk/util/socket.c"
description: "Netatalk utility functions: socket."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/socket.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-11-26T23:02:49+01:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `fcntl.h`, `limits.h`, `netinet/in.h`, `poll.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/socket.h`, `sys/types.h`, `sys/uio.h`, `unistd.h`

# Functions

### setnonblock

```c
int setnonblock(int fd, int cmd)
```

Defined at lines 54 to 75.

set or unset non-blocking IO on a fd

Parameters:
* `fd`: File descriptor
* `cmd`: 0: disable non-blocking IO, i.e. block <>0: enable non-blocking IO

Returns: 0 on success, -1 on failure

Called by: [atalk_sigpipe_init](sigpipe.c.md#atalk_sigpipe_init), [dsi_getsession](../dsi/dsi_getsess.c.md#dsi_getsession), [dsi_opensession](../dsi/dsi_opensess.c.md#dsi_opensession), [readt](socket.c.md#readt), [writet](socket.c.md#writet)

### socket_wait

```c
static int socket_wait(int socket, short events, int timeout, enum loglevels level)
```

Defined at lines 92 to 119.

Wait up to timeout seconds for socket to be ready for events.

A signal leaves the wait going for the rest of the timeout. The time running out is logged at level, a poll error at log_error.

Parameters:
* `socket`: socket
* `events`: POLLIN or POLLOUT
* `timeout`: seconds to wait, not negative: 0 to wait without a limit; past INT_MAX milliseconds the wait takes INT_MAX
* `level`: log level of the time running out

Returns: 1 when poll() reports the socket, 0 with errno EAGAIN when the time runs out, or -1 with the errno of poll

Calls: [monotonic_ms](unix.c.md#monotonic_ms)

Called by: [readt](socket.c.md#readt), [recv_fd](socket.c.md#recv_fd), [writet](socket.c.md#writet)

### readt

```c
ssize_t readt(int socket, void *data, const size_t length, int setnonblocking, int timeout)
```

Defined at lines 136 to 197.

Read length bytes from a socket, waiting for data with poll().

Parameters:
* `socket`: socket, if in blocking mode, pass "setnonblocking" arg as 1
* `data`: buffer for the read data
* `length`: how many bytes to read
* `setnonblocking`: when non-zero this func will enable and disable non blocking io mode for the socket
* `timeout`: seconds each wait for data may take, not negative; 0 means no timeout

Returns: the bytes read, fewer than length when the peer closes the connection or a wait times out after some arrived; -1 on an error, or with errno EAGAIN when a wait times out before any arrived

Calls: [setnonblock](socket.c.md#setnonblock), [socket_wait](socket.c.md#socket_wait)

Called by: [handle_transfer_session](../../etc/afpd/afp_dsi.c.md#handle_transfer_session), [ipc_server_read](server_ipc.c.md#ipc_server_read)

### writet

```c
ssize_t writet(int socket, void *data, const size_t length, int setnonblocking, int timeout)
```

Defined at lines 214 to 275.

Write length bytes to a socket, waiting for room with poll().

Parameters:
* `socket`: socket, if in blocking mode, pass "setnonblocking" arg as 1
* `data`: bytes to write
* `length`: how many bytes to write
* `setnonblocking`: when non-zero this func will enable and disable non blocking io mode for the socket
* `timeout`: seconds each wait for room may take, not negative; 0 means no timeout

Returns: the bytes written, fewer than length when a wait times out after some went out; -1 on an error, or with errno EAGAIN when a wait times out before any went out

Calls: [setnonblock](socket.c.md#setnonblock), [socket_wait](socket.c.md#socket_wait)

Called by: [afp_disconnect](../../etc/afpd/auth.c.md#afp_disconnect), [ipc_child_write](server_ipc.c.md#ipc_child_write), [server_child_transfer_session](server_child.c.md#server_child_transfer_session)

### getip_string

```c
const char * getip_string(const struct sockaddr *sa)
```

Defined at lines 286 to 323.

Convert an IPv4 or IPv6 address to a static string with inet_ntop.

An IPv6-mapped IPv4 address comes back as the IPv4 address: `::ffff:10.0.0.0` becomes "10.0.0.0".

Parameters:
* `sa`: pointer to a struct sockaddr

Returns: a static string with the address, or "0.0.0.0" or "::0" on error

Called by: [client_address](../../etc/afpd/auth.c.md#client_address), [compare_ip](socket.c.md#compare_ip), [configinit](../../etc/afpd/afp_config.c.md#configinit), [dsi_tcp_open](../dsi/dsi_tcp.c.md#dsi_tcp_open), [guess_interface](../dsi/dsi_tcp.c.md#guess_interface), [uam_afpserver_option](../../etc/afpd/uam.c.md#uam_afpserver_option), [volxlate](netatalk_conf.c.md#volxlate)

Uses file-scope variables: `ipv4mapprefix`

### getip_port

```c
unsigned int getip_port(const struct sockaddr *sa)
```

Defined at lines 332 to 343.

return port number from struct sockaddr

Parameters:
* `sa`: pointer to an struct sockaddr

Returns: port as unsigned int

Called by: [configinit](../../etc/afpd/afp_config.c.md#configinit), [dsi_tcp_open](../dsi/dsi_tcp.c.md#dsi_tcp_open), [volxlate](netatalk_conf.c.md#volxlate)

### apply_ip_mask

```c
void apply_ip_mask(struct sockaddr *sa, int mask)
```

Defined at lines 356 to 406.

apply netmask to IP (v4 or v6)

Modifies IP address in sa->sin[6]_addr-s[6]_addr. The caller is responsible for passing a value for mask that is sensible to the passed address, e.g. 0 <= mask <= 32 for IPv4 or 0<= mask <= 128 for IPv6. mask > 32 for IPv4 is treated as mask = 32, mask > 128 is set to 128 for IPv6.

Parameters:
* `sa`: pointer to an struct sockaddr
* `mask`: number of maskbits

Called by: [hostaccessvol](netatalk_conf.c.md#hostaccessvol)

Uses file-scope variables: `ipv4mapprefix`

### compare_ip

```c
int compare_ip(const struct sockaddr *sa1, const struct sockaddr *sa2)
```

Defined at lines 419 to 429.

compare IP addresses for equality

Parameters:
* `sa1`: pointer to an struct sockaddr
* `sa2`: pointer to an struct sockaddr

Returns: Addresses are converted to strings and compared with strcmp and the result of strcmp is returned.

Note: IPv6 mapped IPv4 addresses are treated as IPv4 addresses.

Calls: [getip_string](socket.c.md#getip_string)

Called by: [hostaccessvol](netatalk_conf.c.md#hostaccessvol)

### tokenize_ip_port

```c
int tokenize_ip_port(const char *ipurl, char **address, char **port)
```

Defined at lines 451 to 518.

Tokenize IP(4/6) addresses with an optional port into address and port.

Tokenize IPv4, IPv4:port, IPv6, [IPv6] or [IPv6:port] URL into address and port and return two allocated strings with the address and the port.

Parameters:
* `ipurl`: IP URL string
* `address`: IP address
* `port`: IP port

Returns: 0 on success, -1 on failure

If the function returns 0, then address point to a newly allocated valid address string, port may either be NULL or point to a newly allocated port number.

If the function returns -1, then the contents of address and port are undefined.

Called by: [dsi_tcp_init](../dsi/dsi_tcp.c.md#dsi_tcp_init)

### asev_init

```c
struct asev * asev_init(int max)
```

Defined at lines 526 to 546.

Allocate an empty poll set with room for max descriptors.

Parameters:
* `max`: capacity of the poll and data arrays

Returns: the set, or NULL when an allocation fails

Calls: [asev_free](socket.c.md#asev_free)

Called by: [asev_renew](socket.c.md#asev_renew)

### asev_free

```c
void asev_free(struct asev *sev)
```

Defined at lines 553 to 560.

Free a poll set.

Parameters:
* `sev`: poll set, or NULL

Called by: [asev_init](socket.c.md#asev_init), [asev_renew](socket.c.md#asev_renew)

### asev_renew

```c
struct asev * asev_renew(struct asev *sev, int max)
```

Defined at lines 570 to 584.

Replace a poll set by one with room for max descriptors beyond the entries it keeps, in their order.

Parameters:
* `sev`: poll set to replace and free, or NULL
* `max`: room beyond the kept entries

Returns: the new set, or NULL with sev unchanged

Calls: [asev_free](socket.c.md#asev_free), [asev_init](socket.c.md#asev_init)

Called by: [init_listening_sockets](../../etc/afpd/main.c.md#init_listening_sockets)

### asev_add_fd

```c
bool asev_add_fd(struct asev *asev, int fd, enum asev_fdtype fdtype, void *private, int protocol)
```

Defined at lines 600 to 621.

Append a descriptor to a poll set, watched for POLLIN.

A second array holds the type, the private data, and the protocol of each entry, at the same index as its pollfd.

Parameters:
* `asev`: poll set
* `fd`: descriptor
* `fdtype`: type that the poll loop dispatches on
* `private`: data of the entry, such as a child or a [DSI](../../include/atalk/dsi.h.md#struct-dsi)
* `protocol`: AFPPROTO_DSI or AFPPROTO_ASP for a listener, 0 otherwise

Returns: true, or false for a NULL or full set

Called by: [init_listening_sockets](../../etc/afpd/main.c.md#init_listening_sockets), [main](../../etc/afpd/main.c.md#main)

### asev_del_fd

```c
bool asev_del_fd(struct asev *asev, int fd)
```

Defined at lines 632 to 679.

Remove a descriptor from a poll set, keeping the order of the others.

The entries after it move down by one.

Parameters:
* `asev`: poll set
* `fd`: descriptor

Returns: true when the set held fd, false otherwise

Called by: [child_handler](../../etc/afpd/main.c.md#child_handler), [main](../../etc/afpd/main.c.md#main), [reset_listening_sockets](../../etc/afpd/main.c.md#reset_listening_sockets)

### recv_fd

```c
int recv_fd(int fd, int nonblocking)
```

Defined at lines 694 to 745.

Receive a descriptor on a PF_UNIX socket.

Parameters:
* `fd`: PF_UNIX socket to receive on
* `nonblocking`: 0 for a blocking fd; 1 for a non-blocking one, on which the call waits up to 2 seconds for the message

Returns: the descriptor, or -1 on error

Calls: [socket_wait](socket.c.md#socket_wait)

Called by: [handle_transfer_session](../../etc/afpd/afp_dsi.c.md#handle_transfer_session), [ipc_server_read](server_ipc.c.md#ipc_server_read)

### send_fd

```c
int send_fd(int socket, int fd)
```

Defined at lines 757 to 803.

Pass a descriptor over a Unix socket with SCM_RIGHTS.

The descriptor needs data to travel with, so the message carries an int of

1. The call repeats sendmsg after EINTR.

Parameters:
* `socket`: PF_UNIX socket
* `fd`: descriptor to pass

Returns: 0, or -1 when malloc or sendmsg fails

Called by: [afp_disconnect](../../etc/afpd/auth.c.md#afp_disconnect), [server_child_transfer_session](server_child.c.md#server_child_transfer_session)

# Macros

* `CMSG_SPACE`: Length of the space taken up by a padded control message of length len.

# File-scope variables

`ipv4mapprefix`
