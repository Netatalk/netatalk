---
type: C Source File
title: "libatalk/util/server_child.c"
description: "functions to handle child processes"
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/server_child.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/constant_time.h](../../include/atalk/constant_time.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/server_child.h](../../include/atalk/server_child.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `pthread.h`, `signal.h`, `stdlib.h`, `string.h`, `sys/time.h`, `sys/types.h`, `sys/wait.h`, `unistd.h`

# Functions

### hash_child

```c
static void hash_child(afp_child_t **htable, afp_child_t *child)
```

Defined at lines 67 to 78.

Link a child at the head of its hash chain, keyed by pid.

Parameters:
* `htable`: hash table of the children
* `child`: child to link

Called by: [server_child_add](server_child.c.md#server_child_add)

### unhash_child

```c
static void unhash_child(afp_child_t *child)
```

Defined at lines 85 to 94.

Unlink a child from its hash chain, when it is in one.

Parameters:
* `child`: child to unlink

Called by: [server_child_remove](server_child.c.md#server_child_remove)

### server_child_resolve

```c
afp_child_t * server_child_resolve(server_child_t *childs, id_t pid)
```

Defined at lines 103 to 115.

Find the child entry of a session process by pid.

Parameters:
* `childs`: child table
* `pid`: process ID of the session

Returns: the child, or NULL for an unknown pid

Called by: [ipc_set_state](server_ipc.c.md#ipc_set_state), [ipc_set_volumes](server_ipc.c.md#ipc_set_volumes), [server_child_add](server_child.c.md#server_child_add), [server_child_remove](server_child.c.md#server_child_remove), [server_child_set_session_token](server_child.c.md#server_child_set_session_token)

### server_child_alloc

```c
server_child_t * server_child_alloc(int connections)
```

Defined at lines 123 to 133.

Allocate an empty child table with a session limit.

Parameters:
* `connections`: maximum number of sessions

Returns: the table, or NULL when calloc fails

Called by: [main](../../etc/afpd/main.c.md#main)

### server_child_add

```c
afp_child_t * server_child_add(server_child_t *children, pid_t pid, int ipc_fd, int hint_fd)
```

Defined at lines 149 to 187.

Add a session process to the child table.

The call refuses a new child at the session limit and a process that already exited. A pid already in the table returns its entry, which keeps its descriptors.

Parameters:
* `children`: child table
* `pid`: process ID of the session
* `ipc_fd`: IPC socket of the session
* `hint_fd`: write end of the hint pipe of the session, or -1

Returns: the child, or NULL at the session limit, for a missing process, or when calloc fails

Calls: [hash_child](server_child.c.md#hash_child), [server_child_resolve](server_child.c.md#server_child_resolve)

Called by: [dsi_getsession](../dsi/dsi_getsess.c.md#dsi_getsession)

### server_child_remove

```c
int server_child_remove(server_child_t *children, pid_t pid)
```

Defined at lines 199 to 241.

Remove a session process from the child table and free its entry.

The call closes the IPC socket and the hint pipe of the session. It returns the closed IPC descriptor, so child_handler can drop it from the poll set.

Parameters:
* `children`: child table
* `pid`: process ID of the session

Returns: the closed IPC descriptor, or -1 for an unknown pid

Calls: [explicit_bzero](../compat/explicit_bzero.c.md#explicit_bzero), [server_child_resolve](server_child.c.md#server_child_resolve), [unhash_child](server_child.c.md#unhash_child)

Called by: [child_handler](../../etc/afpd/main.c.md#child_handler)

### server_child_free

```c
void server_child_free(server_child_t *children)
```

Defined at lines 250 to 290.

Free the child table with its entries, and close their descriptors.

The walk visits all CHILD_HASHSIZE slots of the hash table.

Parameters:
* `children`: child table

Calls: [explicit_bzero](../compat/explicit_bzero.c.md#explicit_bzero)

Called by: [dsi_getsession](../dsi/dsi_getsess.c.md#dsi_getsession)

### server_child_kill

```c
void server_child_kill(server_child_t *children, int sig)
```

Defined at lines 298 to 312.

Send a signal to every session process in the child table.

Parameters:
* `children`: child table
* `sig`: signal number

Called by: [afp_end_sessions](../../etc/afpd/main.c.md#afp_end_sessions), [afp_goaway](../../etc/afpd/main.c.md#afp_goaway), [asp_kill](../asp/asp_getsess.c.md#asp_kill), [main](../../etc/afpd/main.c.md#main)

### kill_child

```c
static int kill_child(afp_child_t *child)
```

Defined at lines 321 to 335.

Send SIGTERM to a session process, or SIGKILL when an earlier call sent SIGTERM.

Parameters:
* `child`: child entry, marked after the SIGTERM

Returns: 1

Called by: [server_child_kill_one_by_id](server_child.c.md#server_child_kill_one_by_id)

### server_child_set_session_token

```c
int server_child_set_session_token(server_child_t *children, pid_t pid, uid_t uid, const void *token, size_t token_len)
```

Defined at lines 351 to 398.

Store the reconnect token of a session in a new buffer.

The token replaces an older one, which the call wipes first. The call refuses a token whose uid differs from the uid of the session.

Parameters:
* `children`: child table
* `pid`: process ID of the session
* `uid`: user ID that the session reports
* `token`: opaque token
* `token_len`: length of token

Returns: 0, or -1 for an empty token, an unknown pid, another user, or a failed malloc

Calls: [explicit_bzero](../compat/explicit_bzero.c.md#explicit_bzero), [server_child_resolve](server_child.c.md#server_child_resolve)

Called by: [ipc_set_session_token](server_ipc.c.md#ipc_set_session_token)

### server_child_transfer_session

```c
int server_child_transfer_session(server_child_t *children, uid_t uid, const void *token, size_t token_len, int afp_socket, uint16_t DSI_requestID)
```

Defined at lines 417 to 474.

Pass the socket of a reconnecting client to the session that holds its token.

Each token comparison runs in constant time. The old session has to be logged in as the same user. The call sends the [DSI](../../include/atalk/dsi.h.md#struct-dsi) request ID and the socket over the IPC socket of that session, and then SIGURG.

Parameters:
* `children`: child table
* `uid`: user ID of the new session
* `token`: reconnect token from the client
* `token_len`: length of token
* `afp_socket`: client socket to pass
* `DSI_requestID`: request ID of the reconnect request

Returns: 1 after a transfer, 0 without a usable session, or -1 when the transfer fails

Calls: [atalk_ct_memcmp](constant_time.c.md#atalk_ct_memcmp), [send_fd](socket.c.md#send_fd), [writet](socket.c.md#writet)

Called by: [ipc_kill_token](server_ipc.c.md#ipc_kill_token)

### server_child_kill_one_by_id

```c
void server_child_kill_one_by_id(server_child_t *children, pid_t pid, uid_t uid, uint32_t idlen, char *id, uint32_t boottime)
```

Defined at lines 493 to 543.

Record the client ID of a session, and end the sessions of the same client from before a reboot.

Another session with the same client ID and another boot time belongs to a client that rebooted. The call ends it through kill_child when it has the same user. The entry of pid takes ownership of id and records the boot time and the user.

Parameters:
* `children`: child table
* `pid`: process ID of the session
* `uid`: user ID of the session
* `idlen`: length of id
* `id`: client ID, owned by the table afterwards
* `boottime`: boot time that the client reports

Calls: [kill_child](server_child.c.md#kill_child)

Called by: [ipc_get_session](server_ipc.c.md#ipc_get_session)

### server_child_login_done

```c
void server_child_login_done(server_child_t *children, pid_t pid, uid_t uid, const char *client_address)
```

Defined at lines 554 to 584.

Mark the entry of a session as logged in, with its user and client address.

Parameters:
* `children`: child table
* `pid`: process ID of the session
* `uid`: user ID of the session
* `client_address`: client address, or NULL

Called by: [ipc_login_done](server_ipc.c.md#ipc_login_done)

### server_reset_signal

```c
void server_reset_signal(void)
```

Defined at lines 589 to 609.

reset children signals

Called by: [dsi_tcp_open](../dsi/dsi_tcp.c.md#dsi_tcp_open)

# Macros

* Undocumented: `HASH`, `WEXITSTATUS`, `WIFEXITED`, `WIFSIGNALED`, `WIFSTOPPED`, `WTERMSIG`
