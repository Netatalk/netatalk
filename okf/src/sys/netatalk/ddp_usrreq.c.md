---
type: C Source File
title: "sys/netatalk/ddp_usrreq.c"
description: "DDP socket requests and control blocks in the BSD kernel stack."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp_usrreq.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [at.h](at.h.md)
* [at_var.h](at_var.h.md)
* [ddp_var.h](ddp_var.h.md)
* System headers: `errno.h`, `net/if.h`, `net/route.h`, `sys/ioctl.h`, `sys/mbuf.h`, `sys/param.h`, `sys/protosw.h`, `sys/socket.h`, `sys/socketvar.h`, `sys/systm.h`, `sys/types.h`, `sys/user.h`

# Functions

### ddp_usrreq

```c
ddp_usrreq(struct socket *so, int req, struct mbuf *m, struct mbuf *addr, struct mbuf *rights)
```

Defined at lines 53 to 205. Declared in [sys/netatalk/at_proto.c](at_proto.c.md).

Serve the socket requests of the DDP protocol.

PRU_CONTROL goes to at_control. Attach, detach, bind, and the socket address map to the control block functions. Connect and disconnect set and clear the foreign address. A send with an address connects for that packet alone. Requests that DDP lacks, such as listen and accept, return EOPNOTSUPP.

Parameters:
* `so`: socket
* `req`: PRU_* request
* `m`: data, or the ioctl command for PRU_CONTROL
* `addr`: address, or the ioctl data for PRU_CONTROL
* `rights`: access rights, which DDP refuses, or the interface for PRU_CONTROL

Returns: 0, or an errno value

Calls: [at_control](at_control.c.md#at_control), [at_pcballoc](ddp_usrreq.c.md#at_pcballoc), [at_pcbconnect](ddp_usrreq.c.md#at_pcbconnect), [at_pcbdetach](ddp_usrreq.c.md#at_pcbdetach), [at_pcbdisconnect](ddp_usrreq.c.md#at_pcbdisconnect), [at_pcbsetaddr](ddp_usrreq.c.md#at_pcbsetaddr), [at_sockaddr](ddp_usrreq.c.md#at_sockaddr), [ddp_output](ddp_output.c.md#ddp_output)

Uses file-scope variables: `ddp_recvspace`, `ddp_sendspace`

Dispatched via: [atalksw](at_proto.c.md#atalksw)

### at_sockaddr

```c
at_sockaddr(struct ddpcb *ddp, struct mbuf *addr)
```

Defined at lines 213 to 221.

Return the local address of a DDP socket in an mbuf.

Parameters:
* `ddp`: DDP socket control block
* `addr`: mbuf that receives the address

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_pcbsetaddr

```c
at_pcbsetaddr(struct ddpcb *ddp, struct mbuf *addr)
```

Defined at lines 236 to 352.

Bind a DDP socket to a local address and port.

A given address has to belong to a local interface, and ports below ATPORT_RESERVED need the super-user. Without an address the socket takes the first interface address. Without a port the socket gets the first free port from ATPORT_RESERVED up. A port and address that another socket holds fail with EADDRINUSE.

Parameters:
* `ddp`: DDP socket control block, not bound yet
* `addr`: mbuf with the address, or NULL

Returns: 0, or EINVAL, EAFNOSUPPORT, EADDRNOTAVAIL, EACCES, or EADDRINUSE

Called by: [at_pcbconnect](ddp_usrreq.c.md#at_pcbconnect), [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_pcbconnect

```c
at_pcbconnect(struct ddpcb *ddp, struct mbuf *addr)
```

Defined at lines 367 to 477.

Set the foreign address of a DDP socket after checking its route.

Net 0 means the net the socket is bound to, so it needs a bound socket. A cached route that no longer fits the destination gives way to a new lookup. The route has to lead out of an AppleTalk interface. An unbound socket gets bound to a local address.

Parameters:
* `ddp`: DDP socket control block
* `addr`: mbuf with the destination

Returns: 0, or EINVAL, EAFNOSUPPORT, EADDRNOTAVAIL, ENETUNREACH, or a bind error

Calls: [at_pcbsetaddr](ddp_usrreq.c.md#at_pcbsetaddr)

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_pcbdisconnect

```c
at_pcbdisconnect(struct ddpcb *ddp)
```

Defined at lines 484 to 490.

Clear the foreign address of a DDP socket.

Parameters:
* `ddp`: DDP socket control block

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_pcballoc

```c
at_pcballoc(struct socket *so)
```

Defined at lines 499 to 520.

Allocate a DDP control block for a socket and link it into the list of control blocks.

Parameters:
* `so`: socket, receives the control block

Returns: 0

Called by: [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### at_pcbdetach

```c
at_pcbdetach(struct socket *so, struct ddpcb *ddp)
```

Defined at lines 531 to 568.

Detach and free the DDP control block of a socket.

The call frees the socket, unlinks the block from its port and from the control block list, and releases its route.

Parameters:
* `so`: socket
* `ddp`: control block, freed on return

Called by: [ddp_clean](ddp_usrreq.c.md#ddp_clean), [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

### ddp_search

```c
struct ddpcb * ddp_search(struct sockaddr_at *from, struct sockaddr_at *to, struct at_ifaddr *aa)
```

Defined at lines 585 to 630.

Find the DDP socket that an incoming packet belongs to.

The call looks only at the destination, among the sockets bound to its port, and takes the first socket that fits one of these cases:

* the socket has the destination address
* for ATADDR_BCAST, the socket sits on the net of the receiving interface
* for net.0, the net of the socket falls within the range of the interface

Parameters:
* `from`: source address, unused
* `to`: destination address
* `aa`: receiving interface address

Returns: the control block, or NULL

Called by: [ddp_input](ddp_input.c.md#ddp_input)

### ddp_init

```c
int ddp_init()
```

Defined at lines 637 to 641. Declared in [sys/netatalk/at_proto.c](at_proto.c.md).

Set the length limit of both AppleTalk input queues.

Returns: no defined value, since the K&R definition has no return statement

Dispatched via: [atalksw](at_proto.c.md#atalksw)

### ddp_clean

```c
ddp_clean()
```

Defined at lines 646 to 653.

Detach every DDP socket.

Calls: [at_pcbdetach](ddp_usrreq.c.md#at_pcbdetach)

# File-scope variables

`ddp_recvspace`, `ddp_sendspace`, `ddpcb`
