---
type: C Source File
title: "libatalk/util/server_ipc.c"
description: "IPC over socketpair between parent and children."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/server_ipc.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/server_child.h](../../include/atalk/server_child.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `limits.h`, `poll.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/types.h`, `sys/un.h`, `time.h`, `unistd.h`

# Functions

### rate_slot

```c
static int rate_slot(pid_t child_pid)
```

Defined at lines 133 to 146.

Find the rate slot of a child for the current second.

The slot index is the pid modulo RATE_TRACK_SIZE. A slot that holds another pid or an older second restarts both counts. Only the main thread calls it.

Parameters:
* `child_pid`: pid of the child

Returns: the slot index

Called by: [check_and_increment_rate](server_ipc.c.md#check_and_increment_rate), [check_and_increment_reset_rate](server_ipc.c.md#check_and_increment_reset_rate)

Uses file-scope variables: `pid`, `rate_track`, `window_start`

### check_and_increment_rate

```c
static int check_and_increment_rate(pid_t child_pid)
```

Defined at lines 154 to 157.

Count a hint of a child in the current 1-second window.

Parameters:
* `child_pid`: pid of the child

Returns: the hint count of the window, this hint included

Calls: [rate_slot](server_ipc.c.md#rate_slot)

Called by: [ipc_relay_cache_hint](server_ipc.c.md#ipc_relay_cache_hint)

Uses file-scope variables: `rate_track`

### check_and_increment_reset_rate

```c
static int check_and_increment_reset_rate(pid_t child_pid)
```

Defined at lines 167 to 170.

Count a volume reset of a child in the current 1-second window.

Resets count apart from the other hints.

Parameters:
* `child_pid`: pid of the child

Returns: the reset count of the window, this reset included

Calls: [rate_slot](server_ipc.c.md#rate_slot)

Called by: [ipc_relay_cache_hint](server_ipc.c.md#ipc_relay_cache_hint)

Uses file-scope variables: `rate_track`

### reset_already_broadcast

```c
static int reset_already_broadcast(uint32_t tag)
```

Defined at lines 188 to 206.

Tell whether the reset of a volume went out in the current second.

Every sibling needs one notification per volume, and each costs a forced flush to every session. A repeat inside the same second returns 1, so the caller drops it. A new tag takes the oldest slot.

Parameters:
* `tag`: cnid_volume_tag of the reset volume, in network byte order

Returns: 1 for a repeat, 0 for a new tag

Called by: [ipc_relay_cache_hint](server_ipc.c.md#ipc_relay_cache_hint)

Uses file-scope variables: `reset_seen`, `tag`, `when`

### reset_broadcast_forget

```c
static void reset_broadcast_forget(uint32_t tag)
```

Defined at lines 216 to 224.

Make a volume's reset broadcastable again.

A sibling that missed a reset batch has no other redelivery path, so a delivery failure must not leave the tag deduped against the next duplicate.

Parameters:
* `tag`: [cnid_volume_tag()](../../etc/afpd/volume.c.md#cnid_volume_tag) of the reset volume, network byte order

Called by: [hint_flush_pending](server_ipc.c.md#hint_flush_pending)

Uses file-scope variables: `reset_seen`, `tag`

### hint_write_batch

```c
static int hint_write_batch(int fd, const char *buf, int len, int has_reset, int *waits_left)
```

Defined at lines 241 to 282.

Write a sibling's hint batch, waiting once if it carries a reset.

A full sibling pipe drops a best-effort batch: those hints only cost a lookup. A batch carrying a reset waits briefly for pipe space instead, since the sibling would otherwise keep resolving recycled CNIDs. Waits are budgeted per second across flushes so backed-up siblings cannot stall the master.

Parameters:
* `fd`: sibling's hint pipe
* `buf`: serialized batch
* `len`: bytes to write
* `has_reset`: batch contains a CACHE_HINT_VOLUME_RESET
* `waits_left`: this second's remaining waits, spent here

Returns: 0 on success, -1 if the batch was not delivered

Called by: [hint_flush_pending](server_ipc.c.md#hint_flush_pending)

### serialize_hint

```c
static int serialize_hint(char *buf, const struct hint_entry *e)
```

Defined at lines 295 to 318.

Serialize a [hint_entry](server_ipc.c.md#struct-hint_entry) to IPC wire format.

Writes IPC_HEADERLEN + sizeof(ipc_cache_hint_payload) = 22 bytes to the output buffer. The header PID/UID fields are set to the source child's PID (for receiver logging) with UID 0.

Parameters:
* `buf`: Output buffer (must have ≥ 22 bytes available)
* `e`: Hint entry to serialize

Returns: Number of bytes written (always 22)

Called by: [hint_flush_pending](server_ipc.c.md#hint_flush_pending)

### ipc_kill_token

```c
static int ipc_kill_token(struct ipc_header *ipc, server_child_t *children)
```

Defined at lines 331 to 343.

Pass the socket of a new session to the disconnected session with the same reconnect token.

Parameters:
* `ipc`: IPC_DISCOLDSESSION message with the token, the socket, and the [DSI](../../include/atalk/dsi.h.md#struct-dsi) request ID
* `children`: child table

Returns: 1 after a transfer, 0 without a matching session, or -1 for an empty token or a failed transfer

Calls: [server_child_transfer_session](server_child.c.md#server_child_transfer_session)

Called by: [ipc_server_read](server_ipc.c.md#ipc_server_read)

### ipc_get_session

```c
static int ipc_get_session(struct ipc_header *ipc, server_child_t *children)
```

Defined at lines 356 to 391.

Record the client ID and boot time of a session, and end older sessions of a rebooted client.

The message holds the ID length in network order, the boot time, and the ID. The entry of the sender takes ownership of the copied ID.

Parameters:
* `ipc`: IPC_GETSESSION message
* `children`: child table

Returns: 0, or -1 for a short message or a failed malloc

Calls: [server_child_kill_one_by_id](server_child.c.md#server_child_kill_one_by_id)

Called by: [ipc_server_read](server_ipc.c.md#ipc_server_read)

### ipc_login_done

```c
static int ipc_login_done(const struct ipc_header *ipc, server_child_t *children)
```

Defined at lines 400 to 410.

Mark a session as logged in, with its user and client address.

Parameters:
* `ipc`: IPC_LOGINDONE message with the client address
* `children`: child table

Returns: 0

Calls: [server_child_login_done](server_child.c.md#server_child_login_done)

Called by: [ipc_server_read](server_ipc.c.md#ipc_server_read)

### ipc_set_session_token

```c
static int ipc_set_session_token(const struct ipc_header *ipc, server_child_t *children)
```

Defined at lines 419 to 429.

Store the reconnect token that a session sent.

Parameters:
* `ipc`: IPC_SESSIONTOKEN message with the token
* `children`: child table

Returns: 0, or -1 when server_child_set_session_token refuses the token

Calls: [server_child_set_session_token](server_child.c.md#server_child_set_session_token)

Called by: [ipc_server_read](server_ipc.c.md#ipc_server_read)

### ipc_set_state

```c
static int ipc_set_state(struct ipc_header *ipc, server_child_t *children)
```

Defined at lines 438 to 450.

Store the session state that a child reports, for afpstats.

Parameters:
* `ipc`: IPC_STATE message with the state in 2 bytes
* `children`: child table

Returns: 0, or -1 for an unknown child

Calls: [server_child_resolve](server_child.c.md#server_child_resolve)

Called by: [ipc_server_read](server_ipc.c.md#ipc_server_read)

### ipc_set_volumes

```c
static int ipc_set_volumes(struct ipc_header *ipc, server_child_t *children)
```

Defined at lines 461 to 481.

Store the list of mounted volumes that a child reports, for afpstats.

An empty message clears the list.

Parameters:
* `ipc`: IPC_VOLUMES message with the volume names
* `children`: child table

Returns: 0, or -1 for an unknown child

Calls: [server_child_resolve](server_child.c.md#server_child_resolve)

Called by: [ipc_server_read](server_ipc.c.md#ipc_server_read)

### ipc_relay_cache_hint

```c
static int ipc_relay_cache_hint(struct ipc_header *ipc, server_child_t *children)
```

Defined at lines 501 to 597.

Buffer a dircache hint from a child for the batched relay to its siblings.

Only the main thread appends to hint_buf, so the buffer needs no lock. A short payload, a server with one session, or an unknown event drops the hint. Any other hint counts against HINT_RATE_LIMIT per second and child, and a full buffer drops it.

A volume reset counts against HINT_RESET_RATE_LIMIT instead. Its cnid field carries the volume tag, and a repeat of a tag in the same second drops. A reset in a full buffer takes the slot of the last hint, and the call then flushes the buffer at once.

Parameters:
* `ipc`: IPC_CACHE_HINT message with the hint payload
* `children`: child table

Returns: 0

Calls: [check_and_increment_rate](server_ipc.c.md#check_and_increment_rate), [check_and_increment_reset_rate](server_ipc.c.md#check_and_increment_reset_rate), [hint_flush_pending](server_ipc.c.md#hint_flush_pending), [reset_already_broadcast](server_ipc.c.md#reset_already_broadcast)

Called by: [ipc_server_read](server_ipc.c.md#ipc_server_read)

Uses file-scope variables: `hint_buf`, `hints_rate_dropped`, `rate_track`

### ipc_server_read

```c
int ipc_server_read(server_child_t *children, int fd)
```

Defined at lines 621 to 754.

Read a IPC message from a child.

This is using an fd with non-blocking IO, so EAGAIN is not an error

Parameters:
* `children`: pointer to our structure with all childs
* `fd`: IPC socket with child

Returns: -1 on error, 0 on success

Calls: [ipc_get_session](server_ipc.c.md#ipc_get_session), [ipc_kill_token](server_ipc.c.md#ipc_kill_token), [ipc_login_done](server_ipc.c.md#ipc_login_done), [ipc_relay_cache_hint](server_ipc.c.md#ipc_relay_cache_hint), [ipc_set_session_token](server_ipc.c.md#ipc_set_session_token), [ipc_set_state](server_ipc.c.md#ipc_set_state), [ipc_set_volumes](server_ipc.c.md#ipc_set_volumes), [readt](socket.c.md#readt), [recv_fd](socket.c.md#recv_fd)

Called by: [main](../../etc/afpd/main.c.md#main)

Uses file-scope variables: `ipc_cmd_str`

### ipc_child_write

```c
int ipc_child_write(AFPObj *obj, uint16_t command, size_t len, void *msg)
```

Defined at lines 768 to 808.

Send an IPC message from a session to the master.

The header carries the command, the pid, the euid, and the length. The write waits up to 1 second for socket space.

Parameters:
* `obj`: AFP object with the IPC socket
* `command`: IPC command
* `len`: length of msg
* `msg`: message body

Returns: 0, or -1 for a message over IPC_MAXMSGSIZE or a failed write

Calls: [writet](socket.c.md#writet)

Called by: [afp_disconnect](../../etc/afpd/auth.c.md#afp_disconnect), [afp_getsession](../../etc/afpd/auth.c.md#afp_getsession), [ipc_child_state](server_ipc.c.md#ipc_child_state), [login](../../etc/afpd/auth.c.md#login), [server_ipc_volumes](../../etc/afpd/volume.c.md#server_ipc_volumes)

Uses file-scope variables: `ipc_cmd_str`

Mentioned in the documentation of: [ipc_send_cache_hint](server_ipc.c.md#ipc_send_cache_hint)

### ipc_child_state

```c
int ipc_child_state(AFPObj *obj, uint16_t state)
```

Defined at lines 817 to 820.

Report the session state to the master, for afpstats.

Parameters:
* `obj`: AFP object with the IPC socket
* `state`: session state flags

Returns: the result of ipc_child_write

Calls: [ipc_child_write](server_ipc.c.md#ipc_child_write)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [afp_zzzzz](../../etc/afpd/auth.c.md#afp_zzzzz), [handle_transfer_session](../../etc/afpd/afp_dsi.c.md#handle_transfer_session), [login](../../etc/afpd/auth.c.md#login)

### hint_buf_count

```c
int hint_buf_count(void)
```

Defined at lines 834 to 837.

Return the number of buffered hints.

The afpd main loop polls with a timeout of HINT_FLUSH_INTERVAL_MS while hints wait, and without a timeout otherwise.

Returns: the number of hints in the buffer

Called by: [main](../../etc/afpd/main.c.md#main)

Uses file-scope variables: `hint_buf`

### hint_flush_pending

```c
void hint_flush_pending(server_child_t *children)
```

Defined at lines 858 to 979.

Flush the buffered hints to the sibling sessions.

The afpd main loop calls it after a poll timeout with buffered hints and when the buffer is full. The ipc_relay_cache_hint call flushes for every volume reset. The master blocks signals while it runs, and no other thread touches the child table, so the walk needs no lock.

The hints go out in priority order: volume resets first, then REFRESH, DELETE, and DELETE_CHILDREN. Each sibling gets every hint except the ones it sent. No chunk exceeds PIPE_BUF bytes, so each write stays atomic. A failed chunk skips the rest for that sibling.

A chunk with a reset can wait for pipe space, within HINT_RESET_WAIT_BUDGET waits per second. When a reset misses a sibling, the call forgets its tag, so a repeat of that reset goes out again.

Parameters:
* `children`: child table

Calls: [hint_write_batch](server_ipc.c.md#hint_write_batch), [reset_broadcast_forget](server_ipc.c.md#reset_broadcast_forget), [serialize_hint](server_ipc.c.md#serialize_hint)

Called by: [ipc_relay_cache_hint](server_ipc.c.md#ipc_relay_cache_hint), [main](../../etc/afpd/main.c.md#main)

Uses file-scope variables: `flush_count`, `hint_buf`, `hints_batched`

### ipc_get_hints_sent

```c
unsigned long long ipc_get_hints_sent(void)
```

Defined at lines 995 to 998.

Return the number of dircache hints that this session sent to the master.

Returns: the count since the session started

Uses file-scope variables: `hints_sent`

### ipc_get_hints_dropped

```c
unsigned long long ipc_get_hints_dropped(void)
```

Defined at lines 1006 to 1009.

Return the number of dircache hints that this session dropped on a full IPC socket.

Returns: the count since the session started

Uses file-scope variables: `hints_dropped`

### cache_hint_name

```c
static const char * cache_hint_name(uint8_t event)
```

Defined at lines 1017 to 1035.

Return the name of a cache hint event for log messages.

Parameters:
* `event`: CACHE_HINT_* value

Returns: the name, or "?" for an unknown event

Called by: [ipc_send_cache_hint](server_ipc.c.md#ipc_send_cache_hint)

### ipc_send_cache_hint

```c
int ipc_send_cache_hint(const AFPObj *obj, uint16_t vid, cnid_t cnid, uint8_t event)
```

Defined at lines 1065 to 1148.

Send a dircache invalidation hint from child to parent.

Called directly from AFP command handlers that modify dircache state. Independent of the external FCE system — always active when IPC is available.

Uses a direct non-blocking write() to the IPC socketpair instead of [ipc_child_write()](server_ipc.c.md#ipc_child_write)/writet() — this ensures the AFP command handler is never blocked waiting for IPC buffer space. If the kernel socket buffer is full (parent not draining fast enough), the hint is silently dropped. Hints are best-effort optimizations, and the dircache validation mechanism & graceful fail-on-use detection makes drops safe.

CACHE_HINT_VOLUME_RESET is the exception: a sibling that misses it keeps resolving CNIDs that the wipe has recycled onto other files, so it waits briefly for pipe space and reports failure to the caller.

The 22-byte message (14-byte IPC header + 8-byte payload) is well under the kernel socket buffer size, so partial writes cannot occur when space is available.

Parameters:
* `obj`: [AFPObj](../../include/atalk/globals.h.md#struct-afpobj) with ipc_fd
* `vid`: Volume ID (network byte order, matches vol->v_vid)
* `cnid`: CNID of affected file/dir (network byte order)
* `event`: Hint type: one of the CACHE_HINT_* values

Returns: 0 on success (or graceful drop), -1 on fatal error or an undeliverable CACHE_HINT_VOLUME_RESET

Calls: [cache_hint_name](server_ipc.c.md#cache_hint_name)

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [cnid_volume_reset](../../etc/afpd/volume.c.md#cnid_volume_reset), [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [validation_expunge_and_hint](../../etc/afpd/dircache.c.md#validation_expunge_and_hint), [validation_refresh_and_hint](../../etc/afpd/dircache.c.md#validation_refresh_and_hint)

Uses file-scope variables: `hints_dropped`, `hints_sent`, `pid`

# Types

### struct hint_entry

Defined at line 96.
* `uint16_t vid`
* `cnid_t cnid`
* `uint8_t event`
* `pid_t source_pid`

### struct ipc_header

Defined at line 46.
* `uint16_t command`
* `pid_t child_pid`
* `uid_t uid`
* `uint32_t len`
* `char * msg`
* `int afp_socket`
* `uint16_t DSI_requestID`

# Typedefs and enums

* `typedef struct ipc_header ipc_header_t`

# Macros

* Undocumented: `HINT_RATE_LIMIT`, `HINT_RESET_RATE_LIMIT`, `HINT_RESET_WAIT_BUDGET`, `HINT_RESET_WAIT_MS`, `RATE_TRACK_SIZE`, `RESET_SEEN_SIZE`

# File-scope variables

`count`, `count_in_window`, `entries`, `flush_count`, `hint_buf`, `hints_batched`, `hints_dropped`, `hints_rate_dropped`, `hints_sent`, `ipc_cmd_str`, `pid`, `rate_track`, `reset_seen`, `resets_in_window`, `tag`, `when`, `window_start`
