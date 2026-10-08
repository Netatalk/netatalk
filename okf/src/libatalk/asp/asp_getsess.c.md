---
type: C Source File
title: "libatalk/asp/asp_getsess.c"
description: "5 functions, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/asp/asp_getsess.c"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

send tickles and check tickle status of connections

thoughts on using a hashed list:

* + child_cleanup, finding slots
* - tickle_handler, freeing, tickles

if setup for a large number of connections:

* + space: if actual connections < potential
* - space: actual connections ~ potential

Calls: [asp_tickle](asp_tickle.c.md#asp_tickle)

Called by: [asp_getsession](asp_getsess.c.md#asp_getsession)

Uses file-scope variables: `asp_ac`, `children`, `server_asp`

### asp_kill

```c
void asp_kill(int)
```

Defined at lines 84 to 89. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

kill children

Calls: [server_child_kill](../util/server_child.c.md#server_child_kill)

Called by: [afp_goaway](../../etc/afpd/main.c.md#afp_goaway), [master_out_of_fds](../../etc/afpd/main.c.md#master_out_of_fds)

Uses file-scope variables: `children`

### asp_stop_tickle

```c
void asp_stop_tickle(void)
```

Defined at lines 91 to 97. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

Called by: [asp_cleanup](../../etc/afpd/main.c.md#asp_cleanup)

Uses file-scope variables: `server_asp`

### asp_getsession

```c
ASP asp_getsession(ASP, server_child_t *, const int, afp_child_t **)
```

Defined at lines 107 to 419. Declared in [include/atalk/asp.h](../../include/atalk/asp.h.md).

This call handles open, tickle, and getstatus requests.

Note: On a successful open, it forks a child process.

Returns: an [ASP](../../include/atalk/asp.h.md#struct-asp) to the child and parent and NULL if there is an error.

Calls: [atp_close](../atp/atp_close.c.md#atp_close), [atp_open](../atp/atp_open.c.md#atp_open), [atp_rreq](../atp/atp_rreq.c.md#atp_rreq), [atp_sresp](../atp/atp_sresp.c.md#atp_sresp), [dsi_close_spare_fd](../dsi/dsi_getsess.c.md#dsi_close_spare_fd), [server_child_add](../util/server_child.c.md#server_child_add), [server_child_free](../util/server_child.c.md#server_child_free), [server_child_remove](../util/server_child.c.md#server_child_remove), [server_reset_signal](../util/server_child.c.md#server_reset_signal), [set_asp_ac](asp_getsess.c.md#set_asp_ac), [setnonblock](../util/socket.c.md#setnonblock), [tickle_handler](asp_getsess.c.md#tickle_handler)

Called by: [asp_start](../../etc/afpd/main.c.md#asp_start)

Uses file-scope variables: `asp_ac`, `children`, `server_asp`

### set_asp_ac

```c
static void set_asp_ac(int sid, struct asp_child *tmp)
```

Defined at lines 422 to 425.

Called by: [asp_getsession](asp_getsess.c.md#asp_getsession)

Uses file-scope variables: `asp_ac`

# Macros

* Undocumented: `WEXITSTATUS`, `WIFEXITED`

# File-scope variables

`asp_ac`, `children`, `server_asp`
