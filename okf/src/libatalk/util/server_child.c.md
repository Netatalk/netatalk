---
type: C Source File
title: "libatalk/util/server_child.c"
description: "functions to handle child processes"
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/server_child.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 61 to 72.

Called by: [server_child_add](server_child.c.md#server_child_add)

### unhash_child

```c
static void unhash_child(afp_child_t *child)
```

Defined at lines 74 to 83.

Called by: [server_child_remove](server_child.c.md#server_child_remove)

### server_child_resolve

```c
afp_child_t * server_child_resolve(server_child_t *childs, id_t pid)
```

Defined at lines 85 to 97.

Called by: [ipc_set_state](server_ipc.c.md#ipc_set_state), [ipc_set_volumes](server_ipc.c.md#ipc_set_volumes), [server_child_add](server_child.c.md#server_child_add), [server_child_remove](server_child.c.md#server_child_remove), [server_child_set_session_token](server_child.c.md#server_child_set_session_token)

### server_child_alloc

```c
server_child_t * server_child_alloc(int connections)
```

Defined at lines 100 to 110.

initialize server_child structure

Called by: [main](../../etc/afpd/main.c.md#main)

### server_child_add

```c
afp_child_t * server_child_add(server_child_t *children, pid_t pid, int ipc_fd, int hint_fd)
```

Defined at lines 116 to 154.

add a child

Returns: pointer to struct server_child_data on success, NULL on error

Calls: [hash_child](server_child.c.md#hash_child), [server_child_resolve](server_child.c.md#server_child_resolve)

Called by: [asp_getsession](../asp/asp_getsess.c.md#asp_getsession), [dsi_getsession](../dsi/dsi_getsess.c.md#dsi_getsession)

### server_child_remove

```c
int server_child_remove(server_child_t *children, pid_t pid)
```

Defined at lines 157 to 199.

remove a child and free it

Calls: [explicit_bzero](../compat/explicit_bzero.c.md#explicit_bzero), [server_child_resolve](server_child.c.md#server_child_resolve), [unhash_child](server_child.c.md#unhash_child)

Called by: [asp_getsession](../asp/asp_getsess.c.md#asp_getsession), [child_handler](../../etc/afpd/main.c.md#child_handler)

### server_child_free

```c
void server_child_free(server_child_t *children)
```

Defined at lines 207 to 247.

free everything

by using a hash table, this increases the cost of this part over a linked list by the size of the hash table

Calls: [explicit_bzero](../compat/explicit_bzero.c.md#explicit_bzero)

Called by: [asp_getsession](../asp/asp_getsess.c.md#asp_getsession), [dsi_getsession](../dsi/dsi_getsess.c.md#dsi_getsession)

### server_child_kill

```c
void server_child_kill(server_child_t *children, int sig)
```

Defined at lines 250 to 264.

send signal to all child processes

Called by: [afp_end_sessions](../../etc/afpd/main.c.md#afp_end_sessions), [afp_goaway](../../etc/afpd/main.c.md#afp_goaway), [asp_kill](../asp/asp_getsess.c.md#asp_kill), [main](../../etc/afpd/main.c.md#main)

### kill_child

```c
static int kill_child(afp_child_t *child)
```

Defined at lines 267 to 281.

send kill to a child processes

Called by: [server_child_kill_one_by_id](server_child.c.md#server_child_kill_one_by_id)

### server_child_set_session_token

```c
int server_child_set_session_token(server_child_t *children, pid_t pid, uid_t uid, const void *token, size_t token_len)
```

Defined at lines 287 to 334.

Store an opaque reconnect token for a child session.

Returns: 0 on success, -1 on error

Calls: [explicit_bzero](../compat/explicit_bzero.c.md#explicit_bzero), [server_child_resolve](server_child.c.md#server_child_resolve)

Called by: [ipc_set_session_token](server_ipc.c.md#ipc_set_session_token)

### server_child_transfer_session

```c
int server_child_transfer_session(server_child_t *children, uid_t uid, const void *token, size_t token_len, int afp_socket, uint16_t DSI_requestID)
```

Defined at lines 341 to 398.

Try to find an old session and pass socket.

Returns: -1 on error, 0 if no matching session was found, 1 if session was found and socket passed

Calls: [atalk_ct_memcmp](constant_time.c.md#atalk_ct_memcmp), [send_fd](socket.c.md#send_fd), [writet](socket.c.md#writet)

Called by: [ipc_kill_token](server_ipc.c.md#ipc_kill_token)

### server_child_kill_one_by_id

```c
void server_child_kill_one_by_id(server_child_t *children, pid_t pid, uid_t uid, uint32_t idlen, char *id, uint32_t boottime)
```

Defined at lines 406 to 456.

see if there is a process for the same mac

if the times don't match mac has been rebooted

Calls: [kill_child](server_child.c.md#kill_child)

Called by: [ipc_get_session](server_ipc.c.md#ipc_get_session)

### server_child_login_done

```c
void server_child_login_done(server_child_t *children, pid_t pid, uid_t uid, const char *client_address)
```

Defined at lines 458 to 488.

Called by: [ipc_login_done](server_ipc.c.md#ipc_login_done)

### server_reset_signal

```c
void server_reset_signal(void)
```

Defined at lines 493 to 513.

reset children signals

Called by: [asp_getsession](../asp/asp_getsess.c.md#asp_getsession), [dsi_tcp_open](../dsi/dsi_tcp.c.md#dsi_tcp_open)

# Macros

* Undocumented: `HASH`, `WEXITSTATUS`, `WIFEXITED`, `WIFSIGNALED`, `WIFSTOPPED`, `WTERMSIG`
