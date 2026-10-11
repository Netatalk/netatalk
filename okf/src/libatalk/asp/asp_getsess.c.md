---
type: C Source File
title: "libatalk/asp/asp_getsess.c"
description: "5 functions, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_getsess.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/asp](../asp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [asp_child.h](asp_child.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/server_child.h](../../include/atalk/server_child.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`, `sys/wait.h`, `unistd.h`

# Functions

### tickle_handler

```c
static void tickle_handler(int sig)
```

Defined at lines 58 to 81.

Age each session on SIGALRM, tickle the live ones, and end the silent ones.

Each tick raises the state of every live session slot, and a tickle from the workstation sets it back to ACSTATE_OK. A session that reaches ACSTATE_BAD, six ticks without a tickle, gets SIGTERM and the dead state. Every other live session gets a tickle from the server.

Parameters:
* `sig`: signal number, unused

Calls: [asp_tickle](asp_tickle.c.md#asp_tickle)

Called by: [asp_getsession](asp_getsess.c.md#asp_getsession)

Uses file-scope variables: `asp_ac`, `children`, `server_asp`

### asp_kill

```c
void asp_kill(int)
```

Defined at lines 88 to 93. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Send a signal to every [ASP](../../include/atalk/asp.h.md#struct-asp) session child of the server.

Parameters:
* `sig`: signal to send

Calls: [server_child_kill](../util/server_child.c.md#server_child_kill)

Called by: [afp_goaway](../../etc/afpd/main.c.md#afp_goaway), [master_out_of_fds](../../etc/afpd/main.c.md#master_out_of_fds)

Uses file-scope variables: `children`

### asp_stop_tickle

```c
void asp_stop_tickle(void)
```

Defined at lines 99 to 105. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Stop the tickle timer of the [ASP](../../include/atalk/asp.h.md#struct-asp) server, once asp_getsession has set it up.

Called by: [asp_cleanup](../../etc/afpd/main.c.md#asp_cleanup)

Uses file-scope variables: `server_asp`

### asp_getsession

```c
ASP asp_getsession(ASP, server_child_t *, const int, afp_child_t **)
```

Defined at lines 131 to 443. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Serve one request on the [ASP](../../include/atalk/asp.h.md#struct-asp) server socket: a session open, a tickle, or a status request.

The first call sets up the session slot table and a SIGALRM timer that runs tickle_handler every tickleval seconds. A status request gets the server status, cut to the packets the requester asks for. A tickle refreshes the slot of its session.

An open request with a free slot gets a new ATP socket, an IPC socket pair, and a hint pipe, then a fork. The child returns the session, with asp->child set. The parent records the child, answers with the session socket and the session ID, or ASPERR_SERVBUSY on any failure, and returns the server object.

Parameters:
* `asp`: server object
* `server_children`: child table of the server
* `tickleval`: seconds between tickles, 0 for none
* `childp`: receives the new child in the parent, or NULL

Returns: the session in the child, the server object in the parent, or NULL for a failed read or setup

Calls: [atp_close](../atp/atp_close.c.md#atp_close), [atp_open](../atp/atp_open.c.md#atp_open), [atp_rreq](../atp/atp_rreq.c.md#atp_rreq), [atp_sresp](../atp/atp_sresp.c.md#atp_sresp), [dsi_close_spare_fd](../dsi/dsi_getsess.c.md#dsi_close_spare_fd), [server_child_add](../util/server_child.c.md#server_child_add), [server_child_free](../util/server_child.c.md#server_child_free), [server_child_remove](../util/server_child.c.md#server_child_remove), [server_reset_signal](../util/server_child.c.md#server_reset_signal), [set_asp_ac](asp_getsess.c.md#set_asp_ac), [setnonblock](../util/socket.c.md#setnonblock), [tickle_handler](asp_getsess.c.md#tickle_handler)

Called by: [asp_start](../../etc/afpd/main.c.md#asp_start)

Uses file-scope variables: `asp_ac`, `children`, `server_asp`

### set_asp_ac

```c
static void set_asp_ac(int sid, struct asp_child *tmp)
```

Defined at lines 455 to 458.

Store a child record in the session slot table.

The store sits in a separate function, defined after its use, to keep the compiler from moving it ahead of the record setup. The SIGALRM handler tickle_handler reads the table at any time.

Parameters:
* `sid`: session slot
* `tmp`: filled child record

Called by: [asp_getsession](asp_getsess.c.md#asp_getsession)

Uses file-scope variables: `asp_ac`

# Macros

* Undocumented: `WEXITSTATUS`, `WIFEXITED`

# File-scope variables

`asp_ac`, `children`, `server_asp`
