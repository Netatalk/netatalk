---
type: C Source File
title: "etc/afpd/afpstats.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afpstats.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afpstats.h](afpstats.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/server_child.h](../../include/atalk/server_child.h.md)
* System headers: `errno.h`, `fcntl.h`, `pwd.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/stat.h`, `sys/un.h`, `time.h`, `unistd.h`

# Functions

### afpstats_init

```c
int afpstats_init(server_child_t *childs_in, const char *sock_path, bool set_group, gid_t gid)
```

Defined at lines 57 to 137.

Open the Unix socket that answers afpstats queries about the sessions.

Only one afpd passes the netatalk PID file check, so the call unlinks a socket file left at the path by an earlier run. The socket gets mode 0660 and, with set_group, the group gid. The call keeps childs_in for the answers.

Parameters:
* `childs_in`: session table of the master afpd
* `sock_path`: socket path
* `set_group`: true to give the socket the group gid
* `gid`: group of the socket when set_group is true

Returns: the listening descriptor, or -1 on error

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [main](main.c.md#main)

Uses file-scope variables: `childs`

### state_name

```c
static const char * state_name(int16_t state)
```

Defined at lines 145 to 162.

Name the state of a session for the afpstats output.

Parameters:
* `state`: afpch_state of the session, a single state flag

Returns: "active", "sleeping", "disconnected", or "unknown"

Called by: [afpstats_handle_accept](afpstats.c.md#afpstats_handle_accept)

### protocol_name

```c
static const char * protocol_name(int16_t state)
```

Defined at lines 173 to 190.

Name the transport of a session from its state flags.

The session table records no transport, so the state flags decide. An [ASP](../../include/atalk/asp.h.md#struct-asp) session reports "unknown" between its login and its first state report.

Parameters:
* `state`: afpch_state of the session

Returns: "AppleTalk", "TCP/IP", or "unknown"

Called by: [afpstats_handle_accept](afpstats.c.md#afpstats_handle_accept)

### user_name

```c
static const char * user_name(uid_t uid, char *buf, size_t buflen)
```

Defined at lines 200 to 211.

Name a user for the afpstats output.

Parameters:
* `uid`: user ID
* `buf`: receives uid= and the number when getpwuid finds no user
* `buflen`: size of buf

Returns: the user name from getpwuid, or buf

Called by: [afpstats_handle_accept](afpstats.c.md#afpstats_handle_accept)

### afpstats_handle_accept

```c
void afpstats_handle_accept(int listen_fd)
```

Defined at lines 224 to 280.

Answer one afpstats client with a line per session and close the connection.

Each line names the user, the pid, the login time, the state, the protocol, the volumes, and the client host. A send timeout of 1 second keeps a stuck client from stalling the master afpd. A failed write ends the answer early.

Parameters:
* `listen_fd`: socket from afpstats_init

Calls: [protocol_name](afpstats.c.md#protocol_name), [state_name](afpstats.c.md#state_name), [user_name](afpstats.c.md#user_name)

Called by: [main](main.c.md#main)

Uses file-scope variables: `childs`

# File-scope variables

`childs`
