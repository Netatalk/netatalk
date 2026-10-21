---
type: C Source File
title: "etc/afpd/afp_dsi.c"
description: "AFP sessions over DSI: signal handling, session reconnects, and the request loop."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afp_dsi.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T15:29:27+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp_util.h](../../include/atalk/afp_util.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/spotlight.h](../../include/atalk/spotlight.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* [auth.h](auth.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [fork.h](fork.h.md)
* [idle_worker.h](idle_worker.h.md)
* [pfd_cache.h](pfd_cache.h.md)
* [switch.h](switch.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `fcntl.h`, `netinet/in.h`, `netinet/tcp.h`, `poll.h`, `signal.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `sys/wait.h`, `time.h`, `unistd.h`

# Functions

### afp_dsi_close

```c
static void afp_dsi_close(AFPObj *obj)
```

Defined at lines 133 to 196.

End a [DSI](../../include/atalk/dsi.h.md#struct-dsi) session and close its connection.

The call stops the idle worker, closes the volumes, and logs out. An error path that exits after become_root leaves the euid at 0, so the call switches back to obj->uid first. The volume close scripts run as the login user. Signals stay blocked during the logout, so a signal from PAM cannot interrupt it. The session statistics, the cache counters, and the TCP buffer sizes go to the log.

Parameters:
* `obj`: session object

Calls: [close_all_vol](volume.c.md#close_all_vol), [dircache_rfork_shutdown](dircache.c.md#dircache_rfork_shutdown), [dsi_close](../../libatalk/dsi/dsi_close.c.md#dsi_close), [enumerate_log_stats](enumerate.c.md#enumerate_log_stats), [iw_log_stats](idle_worker.c.md#iw_log_stats), [iw_revoke_signal_safe](idle_worker.c.md#iw_revoke_signal_safe), [log_dircache_stat](dircache.c.md#log_dircache_stat), [of_log_usage](ofork.c.md#of_log_usage), [pfd_log_stats](pfd_cache.c.md#pfd_log_stats), [pfd_shutdown](pfd_cache.c.md#pfd_shutdown)

Called by: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [handle_transfer_session](afp_dsi.c.md#handle_transfer_session)

Mentioned in the documentation of: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die)

### afp_dsi_die_cause

```c
static const char * afp_dsi_die_cause(int sig, char *buf, size_t len)
```

Defined at lines 207 to 221.

Describe what made [afp_dsi_die()](afp_dsi.c.md#afp_dsi_die) run.

Parameters:
* `sig`: the [afp_dsi_die()](afp_dsi.c.md#afp_dsi_die) argument
* `buf`: destination
* `len`: size of `buf`

Returns: `buf`

Called by: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die)

Uses file-scope variables: `die_sender`, `die_signalled`, `session_master`

### afp_dsi_die

```c
static void afp_dsi_die(int sig)
```

Defined at lines 233 to 267.

SIGTERM: Terminate the AFP session and exit.

Wraps [afp_dsi_close()](afp_dsi.c.md#afp_dsi_close) + exit() with pre-flight checks and a shutdown attention to notify the client. Other call sites in this file invoke [afp_dsi_close()](afp_dsi.c.md#afp_dsi_close) + exit() directly — a future cleanup could funnel all exit paths through here.

Parameters:
* `sig`: signal number or EXITERR_* code; 0 for maintenance shutdown.

Calls: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [afp_dsi_die_cause](afp_dsi.c.md#afp_dsi_die_cause), [dsi_attention](../../libatalk/dsi/dsi_attn.c.md#dsi_attention), [of_log_usage](ofork.c.md#of_log_usage)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers), [handle_alarm](afp_dsi.c.md#handle_alarm), [handle_timedown](afp_dsi.c.md#handle_timedown), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

Uses file-scope variables: `AFPobj`

Mentioned in the documentation of: [afp_dsi_die_cause](afp_dsi.c.md#afp_dsi_die_cause)

### afp_dsi_transfer_handler

```c
static void afp_dsi_transfer_handler(int sig)
```

Defined at lines 274 to 278.

SIGURG handler that records a primary reconnect for the main loop.

Parameters:
* `sig`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `transfer_pending`

### handle_transfer_session

```c
static void handle_transfer_session(AFPObj *obj)
```

Defined at lines 291 to 341.

Take over the connection of a reconnecting client after SIGURG.

The master afpd sends the [DSI](../../include/atalk/dsi.h.md#struct-dsi) request ID and the new socket over the IPC socket. The session swaps its socket, resets the read-ahead buffer, and answers the FPDisconnectOldSession that the transfer interrupted. Any failure ends the session. The SIGURG handler only records the event, and the main loop runs this function.

Parameters:
* `obj`: session object

Calls: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [dsi_cmdreply](../../libatalk/dsi/dsi_cmdreply.c.md#dsi_cmdreply), [ipc_child_state](../../libatalk/util/server_ipc.c.md#ipc_child_state), [readt](../../libatalk/util/socket.c.md#readt), [recv_fd](../../libatalk/util/socket.c.md#recv_fd)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

Calls through [`DSI::proto_close`](../../include/atalk/dsi.h.md#struct-dsi): no table assigns this field

Uses file-scope variables: `alarm_pending`

### afp_dsi_timedown_handler

```c
static void afp_dsi_timedown_handler(int sig)
```

Defined at lines 351 to 355.

SIGUSR1 handler that records a shutdown notice for the main loop.

Parameters:
* `sig`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `timedown_pending`

### handle_timedown

```c
static void handle_timedown(AFPObj *obj)
```

Defined at lines 367 to 413.

Announce a shutdown in 5 minutes to the [DSI](../../include/atalk/dsi.h.md#struct-dsi) client after SIGUSR1.

The client gets a maintenance message and an attention that forbids a reconnect. An attention that cannot go out now waits as down_request. SIGALRM after 300 seconds then ends the session, and the session ignores further SIGUSR1 signals.

Parameters:
* `obj`: session object

Calls: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [afp_dsi_die_handler](afp_dsi.c.md#afp_dsi_die_handler), [dsi_attention](../../libatalk/dsi/dsi_attn.c.md#dsi_attention), [setmessage](messages.c.md#setmessage)

Called by: [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

### afp_dsi_reload

```c
static void afp_dsi_reload(int sig)
```

Defined at lines 425 to 429.

SIGHUP handler that requests a reload of the volume configuration.

Parameters:
* `sig`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `reload_request`

### afp_dsi_debug

```c
static void afp_dsi_debug(int sig)
```

Defined at lines 445 to 449.

SIGINT handler that requests a debug dump and a switch of the log level.

The main loop then dumps the dircache and the UUID cache and toggles maxdebug logging into a temporary file.

Parameters:
* `sig`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `debug_request`

### afp_dsi_getmesg_handler

```c
static void afp_dsi_getmesg_handler(int sig)
```

Defined at lines 456 to 460.

SIGUSR2 handler that records a server message for the main loop.

Parameters:
* `sig`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `getmesg_pending`

### handle_getmesg

```c
static void handle_getmesg(AFPObj *obj)
```

Defined at lines 472 to 480.

Announce a server message to the [DSI](../../include/atalk/dsi.h.md#struct-dsi) client after SIGUSR2.

The session sends a message attention and sets msg_request, so pending_request loads the message from its file after the next request. A refused attention leaves msg_request at 2, and pending_request sends it again.

Parameters:
* `obj`: session object

Calls: [dsi_attention](../../libatalk/dsi/dsi_attn.c.md#dsi_attention)

Called by: [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

### afp_dsi_die_handler

```c
static void afp_dsi_die_handler(int sig, siginfo_t *si, void *uc)
```

Defined at lines 493 to 502.

SIGTERM and SIGQUIT handler that records the signal and its sender for the main loop.

The sender pid counts only for a signal from kill or sigqueue. macOS reports kill with si_code 0. Any other source leaves the sender at 0.

Parameters:
* `sig`: signal number
* `si`: signal information, or NULL
* `uc`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers), [handle_timedown](afp_dsi.c.md#handle_timedown)

Uses file-scope variables: `die_pending`, `die_sender`

### alarm_handler

```c
static void alarm_handler(int sig)
```

Defined at lines 513 to 520.

SIGALRM handler that rearms the tickle timer and records the tick for the main loop.

A library call to alarm can replace the interval timer, so each tick sets it again from dsi->timer. The main loop runs handle_alarm for the tick.

Parameters:
* `sig`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `AFPobj`, `alarm_pending`

### handle_alarm

```c
static void handle_alarm(AFPObj *obj)
```

Defined at lines 539 to 624.

Run one tick of the session timers after SIGALRM.

A tick after client traffic only clears DSI_DATA. Any other tick adds 1 to the tickle count, and the session state picks the next step:

* A sleeping session ends once the count passes the sleep option.
* A disconnected session without a login ends at once. Any other ends once the count passes the disconnected option.
* A running session disconnects once the count reaches the timeout option.
* Otherwise pollvoltime runs, and a tickle goes out unless it sent a volume change attention. A failed send disconnects the session.

A session without a login ends wherever a session with one disconnects.

Parameters:
* `obj`: session object

Calls: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [dsi_disconnect](../../libatalk/dsi/dsi_stream.c.md#dsi_disconnect), [dsi_tickle](../../libatalk/dsi/dsi_tickle.c.md#dsi_tickle), [pollvoltime](volume.c.md#pollvoltime)

Called by: [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

### child_handler

```c
static void child_handler(int sig)
```

Defined at lines 632 to 635.

SIGCHLD handler that reaps one exited child.

Parameters:
* `sig`: unused

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

### process_deferred_signals

```c
static int process_deferred_signals(AFPObj *obj)
```

Defined at lines 650 to 687.

Run the work of every signal that arrived since the last call.

Only the main loop calls it, never a signal handler. A termination signal goes first and ends the session through afp_dsi_die, which does not return. A primary reconnect comes next and returns at once. The shutdown notice, the server message, and the timer tick follow in that order.

Parameters:
* `obj`: session object

Returns: 1 after a primary reconnect, so the caller restarts its outer loop, 0 otherwise

Calls: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [handle_alarm](afp_dsi.c.md#handle_alarm), [handle_getmesg](afp_dsi.c.md#handle_getmesg), [handle_timedown](afp_dsi.c.md#handle_timedown), [handle_transfer_session](afp_dsi.c.md#handle_transfer_session), [iw_revoke](idle_worker.c.md#iw_revoke)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `alarm_pending`, `die_pending`, `die_signalled`, `getmesg_pending`, `timedown_pending`, `transfer_pending`

### pending_request

```c
static void pending_request(DSI *dsi)
```

Defined at lines 700 to 720.

Send the attentions that signal work left pending and load a pending server message.

The main loop calls it after each request. An attention fails while a reply write is in progress, so handle_getmesg and handle_timedown leave msg_request at 2 or down_request at 1 for this call. The call also loads a pending message from its file through readmessage.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) session

Calls: [dsi_attention](../../libatalk/dsi/dsi_attn.c.md#dsi_attention), [readmessage](messages.c.md#readmessage)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `AFPobj`

### afp_dsi_log_disconnect

```c
static void afp_dsi_log_disconnect(const AFPObj *obj, int rx_errno, bool already)
```

Defined at lines 732 to 752.

Log why the session is disconnecting and what it keeps meanwhile.

A disconnected session holds every fork and descriptor until the client reconnects or the disconnect timer expires.

Parameters:
* `obj`: AFP session
* `rx_errno`: errno of the failed receive, 0 at end of stream
* `already`: the session was disconnected before this receive

Calls: [of_log_usage](ofork.c.md#of_log_usage)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

### afp_over_dsi_sighandlers

```c
void afp_over_dsi_sighandlers(AFPObj *obj)
```

Defined at lines 773 to 862.

Install the signal handlers and start the tickle timer of a [DSI](../../include/atalk/dsi.h.md#struct-dsi) session.

Each handler runs with every signal blocked and only records its signal for the main loop. The login code calls it again, because a PAM module may reset the handlers or the timer. A failed install ends the session.

* SIGHUP reloads the volumes.
* SIGURG takes over a reconnecting client.
* SIGTERM and SIGQUIT end the session.
* SIGUSR1 announces a shutdown, and SIGUSR2 a server message.
* SIGINT toggles the debug log.
* SIGALRM drives the tickle timer, except in DEBUGGING builds.
* SIGCHLD: the kernel reaps children where SA_NOCLDWAIT exists, else a handler does.

Parameters:
* `obj`: session object

Calls: [afp_dsi_debug](afp_dsi.c.md#afp_dsi_debug), [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [afp_dsi_die_handler](afp_dsi.c.md#afp_dsi_die_handler), [afp_dsi_getmesg_handler](afp_dsi.c.md#afp_dsi_getmesg_handler), [afp_dsi_reload](afp_dsi.c.md#afp_dsi_reload), [afp_dsi_timedown_handler](afp_dsi.c.md#afp_dsi_timedown_handler), [afp_dsi_transfer_handler](afp_dsi.c.md#afp_dsi_transfer_handler), [alarm_handler](afp_dsi.c.md#alarm_handler), [child_handler](afp_dsi.c.md#child_handler)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [login](auth.c.md#login)

### afp_write_command

```c
static bool afp_write_command(uint8_t function)
```

Defined at lines 871 to 875.

Tell whether an AFP command takes a payload, which a DSIWrite frame carries.

Parameters:
* `function`: AFP command code

Returns: true for FPWrite, FPWriteExt and FPAddIcon

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

### afp_over_dsi

```c
void afp_over_dsi(AFPObj *obj)
```

Defined at lines 902 to 1372.

Serve the AFP requests of a [DSI](../../include/atalk/dsi.h.md#struct-dsi) session until the session ends.

The call never returns. Setup installs the signal handlers, creates the dircache, and starts the idle worker. It then sets the TCP buffer sizes and TCP_NODELAY and opens the signal pipe. Each pass of the loop runs 4 stages:

1. Run the deferred signal work. Buffered input goes straight on, and the disconnected and dying states after serving the cache hints; otherwise write the volume roots a marked second left due, then wait in poll for the client socket, the cache hint pipe, or the signal pipe, at most a second while a write is due. A request the due write's attention read ahead skips the wait.
1. Receive a request and apply a pending volume reload or debug toggle. A failed receive enters the disconnected state, which serves the deferred signal work and then waits on the signal pipe until a primary reconnect, a termination or the disconnect timer ends it.
1. Run the request. A repeated [DSI](../../include/atalk/dsi.h.md#struct-dsi) request ID with the same AFP command gets the stored result of the replay cache instead of a second run. DSICloseSession ends the session.
1. Send the attentions that waited, load a pending server message, and send the queued FCE events.

Parameters:
* `obj`: session object

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [AfpNum2name](../../libatalk/util/afp_util.c.md#afpnum2name), [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [afp_dsi_log_disconnect](afp_dsi.c.md#afp_dsi_log_disconnect), [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers), [afp_write_command](afp_dsi.c.md#afp_write_command), [atalk_sigpipe_drain](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_drain), [atalk_sigpipe_init](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_init), [atalk_sigpipe_readfd](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_readfd), [dir_free_invalid_q](directory.c.md#dir_free_invalid_q), [dircache_dump](dircache.c.md#dircache_dump), [dircache_init](dircache.c.md#dircache_init), [dsi_attention](../../libatalk/dsi/dsi_attn.c.md#dsi_attention), [dsi_cmdreply](../../libatalk/dsi/dsi_cmdreply.c.md#dsi_cmdreply), [dsi_disconnect](../../libatalk/dsi/dsi_stream.c.md#dsi_disconnect), [dsi_stream_receive](../../libatalk/dsi/dsi_stream.c.md#dsi_stream_receive), [dsi_tickle](../../libatalk/dsi/dsi_tickle.c.md#dsi_tickle), [dsi_writedrain](../../libatalk/dsi/dsi_write.c.md#dsi_writedrain), [fce_pending_events](fce_api.c.md#fce_pending_events), [flushvoltime](volume.c.md#flushvoltime), [handle_transfer_session](afp_dsi.c.md#handle_transfer_session), [ipc_child_state](../../libatalk/util/server_ipc.c.md#ipc_child_state), [iw_grant](idle_worker.c.md#iw_grant), [iw_init](idle_worker.c.md#iw_init), [iw_is_active](idle_worker.c.md#iw_is_active), [iw_revoke](idle_worker.c.md#iw_revoke), [iw_shutdown](idle_worker.c.md#iw_shutdown), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [log_backoff](../../libatalk/util/unix.c.md#log_backoff), [pending_request](afp_dsi.c.md#pending_request), [process_cache_hints](dircache.c.md#process_cache_hints), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals), [setuplog](../../libatalk/util/logger.c.md#setuplog), [tmpdir](../../libatalk/util/unix.c.md#tmpdir), [uuidcache_dump](../../libatalk/acl/cache.c.md#uuidcache_dump)

Called by: [dsi_start](main.c.md#dsi_start)

Uses file-scope variables: `AFPobj`, `afp_switch` in [etc/afpd/switch.c](switch.c.md), `debug_request`, `misframed_requests`, `reload_request`, `replaycache`, `session_master`, `transfer_pending`

# Types

### struct rc_elem_t

Defined at line 80.
* `uint16_t DSIreqID`
* `uint8_t AFPcommand`
* `uint32_t result`

# Macros

* Undocumented: `SOL_TCP`

# File-scope variables

`AFPobj`, `alarm_pending`, `debug_request`, `die_pending`, `die_sender`, `die_signalled`, `getmesg_pending`, `misframed_requests`, `reload_request`, `replaycache`, `session_master`, `timedown_pending`, `transfer_pending`
