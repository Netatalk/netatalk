---
type: C Source File
title: "libatalk/dsi/dsi_getsess.c"
description: "6 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dsi/dsi_getsess.c"
tags: ["libatalk/dsi"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/dsi](../dsi.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/server_child.h](../../include/atalk/server_child.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `fcntl.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/wait.h`, `unistd.h`

# Functions

### dsi_reserve_spare_fd

```c
void dsi_reserve_spare_fd(void)
```

Defined at lines 44 to 51. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Hold one descriptor in reserve for refusing connections.

A master with no descriptor left closes the spare to accept the waiting connection and refuse it, so the connection does not stay queued with poll() reporting the listening socket ready again at once.

Called by: [dsi_refuse_starved](dsi_getsess.c.md#dsi_refuse_starved), [main](../../etc/afpd/main.c.md#main)

Uses file-scope variables: `dsi_fdlimit`, `dsi_spare_fd`

### dsi_close_spare_fd

```c
void dsi_close_spare_fd(void)
```

Defined at lines 56 to 62. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Release the spare descriptor, in a session child or before a refusal.

Called by: [asp_getsession](../asp/asp_getsess.c.md#asp_getsession), [dsi_getsession](dsi_getsess.c.md#dsi_getsession), [dsi_refuse_starved](dsi_getsess.c.md#dsi_refuse_starved)

Uses file-scope variables: `dsi_spare_fd`

### dsi_reject_busy

```c
static bool dsi_reject_busy(DSI *dsi)
```

Defined at lines 74 to 104.

Accept the waiting connection and reply that the server is busy.

The reply echoes the client's request ID when its DSIOpenSession header has already arrived, else uses 0.

Returns: false when accept() fails, with errno set

Parameters:
* `dsi`: listening [DSI](../../include/atalk/dsi.h.md#struct-dsi) handle

Called by: [dsi_getsession](dsi_getsess.c.md#dsi_getsession), [dsi_refuse_starved](dsi_getsess.c.md#dsi_refuse_starved)

### dsi_refuse_starved

```c
static void dsi_refuse_starved(DSI *dsi, const server_child_t *children, int err)
```

Defined at lines 113 to 130.

Refuse the waiting connection when the master is out of descriptors.

Parameters:
* `dsi`: listening [DSI](../../include/atalk/dsi.h.md#struct-dsi) handle
* `children`: the master's session table
* `err`: errno of the failed descriptor allocation

Calls: [count_open_fds](../util/unix.c.md#count_open_fds), [dsi_close_spare_fd](dsi_getsess.c.md#dsi_close_spare_fd), [dsi_reject_busy](dsi_getsess.c.md#dsi_reject_busy), [dsi_reserve_spare_fd](dsi_getsess.c.md#dsi_reserve_spare_fd), [log_backoff](../util/unix.c.md#log_backoff)

Called by: [dsi_getsession](dsi_getsess.c.md#dsi_getsession)

Uses file-scope variables: `dsi_starved_refusals`

### dsi_note_master_fd

```c
static void dsi_note_master_fd(const server_child_t *children, int fd)
```

Defined at lines 141 to 151.

Warn as the master's descriptor use climbs toward its limit.

accept(2) returns the lowest free descriptor, so the accepted socket's number is a lower bound on the master's descriptors in use, at no cost.

Parameters:
* `children`: the master's session table
* `fd`: descriptor the master just accepted

Calls: [count_open_fds](../util/unix.c.md#count_open_fds), [usage_level_pct](../util/unix.c.md#usage_level_pct), [usage_level_update](../util/unix.c.md#usage_level_update)

Called by: [dsi_getsession](dsi_getsess.c.md#dsi_getsession)

Uses file-scope variables: `dsi_fd_level`, `dsi_fdlimit`

### dsi_getsession

```c
int dsi_getsession(DSI *, server_child_t *, const int, afp_child_t **)
```

Defined at lines 162 to 359. Declared in [include/atalk/dsi.h](../../include/atalk/dsi.h.md).

Start a [DSI](../../include/atalk/dsi.h.md#struct-dsi) session, fork an afpd process.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) structure
* `serv_children`: pointer to our structure with all childs
* `tickleval`: tickle interval in seconds
* `childp`: after fork: parent return pointer to child, child returns NULL

Returns: 0 on success, any other value denotes failure

Calls: [dsi_close_spare_fd](dsi_getsess.c.md#dsi_close_spare_fd), [dsi_getstatus](dsi_getstat.c.md#dsi_getstatus), [dsi_note_master_fd](dsi_getsess.c.md#dsi_note_master_fd), [dsi_opensession](dsi_opensess.c.md#dsi_opensession), [dsi_refuse_starved](dsi_getsess.c.md#dsi_refuse_starved), [dsi_reject_busy](dsi_getsess.c.md#dsi_reject_busy), [server_child_add](../util/server_child.c.md#server_child_add), [server_child_free](../util/server_child.c.md#server_child_free), [setnonblock](../util/socket.c.md#setnonblock)

Called by: [dsi_start](../../etc/afpd/main.c.md#dsi_start)

Calls through [`DSI::proto_close`](../../include/atalk/dsi.h.md#struct-dsi): no table assigns this field

Calls through [`DSI::proto_open`](../../include/atalk/dsi.h.md#struct-dsi): no table assigns this field

# File-scope variables

`dsi_fd_level`, `dsi_fdlimit`, `dsi_spare_fd`, `dsi_starved_refusals`
