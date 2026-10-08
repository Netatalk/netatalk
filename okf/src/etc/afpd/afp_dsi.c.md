---
type: C Source File
title: "etc/afpd/afp_dsi.c"
description: "20 functions, 1 type, includes 18 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afp_dsi.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 111 to 173.

Calls: [close_all_vol](volume.c.md#close_all_vol), [dircache_rfork_shutdown](dircache.c.md#dircache_rfork_shutdown), [dsi_close](../../libatalk/dsi/dsi_close.c.md#dsi_close), [iw_log_stats](idle_worker.c.md#iw_log_stats), [iw_revoke_signal_safe](idle_worker.c.md#iw_revoke_signal_safe), [log_dircache_stat](dircache.c.md#log_dircache_stat), [of_log_usage](ofork.c.md#of_log_usage), [pfd_log_stats](pfd_cache.c.md#pfd_log_stats), [pfd_shutdown](pfd_cache.c.md#pfd_shutdown)

Called by: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [handle_transfer_session](afp_dsi.c.md#handle_transfer_session), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

Calls through [`AFPObj::logout`](../../include/atalk/globals.h.md#struct-afpobj): no table assigns this field

Mentioned in the documentation of: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die)

### afp_dsi_die_cause

```c
static const char * afp_dsi_die_cause(int sig, char *buf, size_t len)
```

Defined at lines 184 to 198.

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

Defined at lines 210 to 246.

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

Defined at lines 249 to 253.

SIGURG handler (primary reconnect) — async-signal-safe

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `transfer_pending`

### handle_transfer_session

```c
static void handle_transfer_session(AFPObj *obj)
```

Defined at lines 260 to 306.

Process deferred SIGURG: primary reconnect.

Called from the main event loop where non-signal-safe functions are permitted.

Calls: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [dsi_cmdreply](../../libatalk/dsi/dsi_cmdreply.c.md#dsi_cmdreply), [ipc_child_state](../../libatalk/util/server_ipc.c.md#ipc_child_state), [readt](../../libatalk/util/socket.c.md#readt), [recv_fd](../../libatalk/util/socket.c.md#recv_fd)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

Calls through [`DSI::proto_close`](../../include/atalk/dsi.h.md#struct-dsi): no table assigns this field

### afp_dsi_timedown_handler

```c
static void afp_dsi_timedown_handler(int sig)
```

Defined at lines 312 to 316.

SIGUSR1 handler — async-signal-safe

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `timedown_pending`

### handle_timedown

```c
static void handle_timedown(AFPObj *obj)
```

Defined at lines 323 to 369.

Process deferred SIGUSR1: server going down in 5 minutes.

Called from the main event loop.

Calls: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [afp_dsi_die_handler](afp_dsi.c.md#afp_dsi_die_handler), [dsi_attention](../../libatalk/dsi/dsi_attn.c.md#dsi_attention), [setmessage](messages.c.md#setmessage)

Called by: [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

### afp_dsi_reload

```c
static void afp_dsi_reload(int sig)
```

Defined at lines 376 to 380.

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `reload_request`

### afp_dsi_debug

```c
static void afp_dsi_debug(int sig)
```

Defined at lines 387 to 391.

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `debug_request`

### afp_dsi_getmesg_handler

```c
static void afp_dsi_getmesg_handler(int sig)
```

Defined at lines 394 to 398.

SIGUSR2 handler — async-signal-safe

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `getmesg_pending`

### handle_getmesg

```c
static void handle_getmesg(AFPObj *obj)
```

Defined at lines 405 to 413.

Process deferred SIGUSR2: server message.

Called from the main event loop.

Calls: [dsi_attention](../../libatalk/dsi/dsi_attn.c.md#dsi_attention)

Called by: [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

### afp_dsi_die_handler

```c
static void afp_dsi_die_handler(int sig, siginfo_t *si, void *uc)
```

Defined at lines 416 to 425.

SIGTERM/SIGQUIT handler, records the sending process — async-signal-safe

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers), [handle_timedown](afp_dsi.c.md#handle_timedown)

Uses file-scope variables: `die_pending`, `die_sender`

### alarm_handler

```c
static void alarm_handler(int sig)
```

Defined at lines 433 to 440.

SIGALRM handler — async-signal-safe.

Only restarts the timer (setitimer is async-signal-safe) and sets a flag. All tickle/timeout logic is deferred to [handle_alarm()](afp_dsi.c.md#handle_alarm).

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

Uses file-scope variables: `AFPobj`, `alarm_pending`

### handle_alarm

```c
static void handle_alarm(AFPObj *obj)
```

Defined at lines 447 to 532.

Process deferred SIGALRM: tickle/timeout management.

Called from the main event loop where LOG/malloc/syslog are safe.

Calls: [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [dsi_disconnect](../../libatalk/dsi/dsi_stream.c.md#dsi_disconnect), [dsi_tickle](../../libatalk/dsi/dsi_tickle.c.md#dsi_tickle), [pollvoltime](volume.c.md#pollvoltime)

Called by: [process_deferred_signals](afp_dsi.c.md#process_deferred_signals)

Mentioned in the documentation of: [alarm_handler](afp_dsi.c.md#alarm_handler)

### child_handler

```c
static void child_handler(int sig)
```

Defined at lines 534 to 537.

Called by: [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers)

### process_deferred_signals

```c
static int process_deferred_signals(AFPObj *obj)
```

Defined at lines 547 to 592.

Process all pending deferred signals.

Returns: 1 if the caller should restart the outer event loop (primary reconnect), 0 otherwise.

Note: Must be called from the main event loop, never from signal context.

Calls: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [handle_alarm](afp_dsi.c.md#handle_alarm), [handle_getmesg](afp_dsi.c.md#handle_getmesg), [handle_timedown](afp_dsi.c.md#handle_timedown), [handle_transfer_session](afp_dsi.c.md#handle_transfer_session), [iw_revoke](idle_worker.c.md#iw_revoke), [iw_shutdown](idle_worker.c.md#iw_shutdown)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `alarm_pending`, `die_pending`, `die_signalled`, `getmesg_pending`, `timedown_pending`, `transfer_pending`

### pending_request

```c
static void pending_request(DSI *dsi)
```

Defined at lines 598 to 618.

Calls: [dsi_attention](../../libatalk/dsi/dsi_attn.c.md#dsi_attention), [readmessage](messages.c.md#readmessage)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `AFPobj`

### afp_dsi_log_disconnect

```c
static void afp_dsi_log_disconnect(const AFPObj *obj, int rx_errno, bool already)
```

Defined at lines 630 to 650.

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

Defined at lines 652 to 739.

Calls: [afp_dsi_debug](afp_dsi.c.md#afp_dsi_debug), [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [afp_dsi_die_handler](afp_dsi.c.md#afp_dsi_die_handler), [afp_dsi_getmesg_handler](afp_dsi.c.md#afp_dsi_getmesg_handler), [afp_dsi_reload](afp_dsi.c.md#afp_dsi_reload), [afp_dsi_timedown_handler](afp_dsi.c.md#afp_dsi_timedown_handler), [afp_dsi_transfer_handler](afp_dsi.c.md#afp_dsi_transfer_handler), [alarm_handler](afp_dsi.c.md#alarm_handler), [child_handler](afp_dsi.c.md#child_handler)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [login](auth.c.md#login)

### afp_over_dsi

```c
void afp_over_dsi(AFPObj *obj)
```

Defined at lines 744 to 1168.

Calls: [AfpErr2name](../../libatalk/util/afp_util.c.md#afperr2name), [AfpNum2name](../../libatalk/util/afp_util.c.md#afpnum2name), [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [afp_dsi_log_disconnect](afp_dsi.c.md#afp_dsi_log_disconnect), [afp_over_dsi_sighandlers](afp_dsi.c.md#afp_over_dsi_sighandlers), [atalk_sigpipe_drain](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_drain), [atalk_sigpipe_init](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_init), [atalk_sigpipe_readfd](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_readfd), [dir_free_invalid_q](directory.c.md#dir_free_invalid_q), [dircache_dump](dircache.c.md#dircache_dump), [dircache_init](dircache.c.md#dircache_init), [dsi_attention](../../libatalk/dsi/dsi_attn.c.md#dsi_attention), [dsi_cmdreply](../../libatalk/dsi/dsi_cmdreply.c.md#dsi_cmdreply), [dsi_disconnect](../../libatalk/dsi/dsi_stream.c.md#dsi_disconnect), [dsi_stream_receive](../../libatalk/dsi/dsi_stream.c.md#dsi_stream_receive), [dsi_tickle](../../libatalk/dsi/dsi_tickle.c.md#dsi_tickle), [dsi_writeflush](../../libatalk/dsi/dsi_write.c.md#dsi_writeflush), [dsi_writeinit](../../libatalk/dsi/dsi_write.c.md#dsi_writeinit), [fce_pending_events](fce_api.c.md#fce_pending_events), [handle_transfer_session](afp_dsi.c.md#handle_transfer_session), [ipc_child_state](../../libatalk/util/server_ipc.c.md#ipc_child_state), [iw_grant](idle_worker.c.md#iw_grant), [iw_init](idle_worker.c.md#iw_init), [iw_is_active](idle_worker.c.md#iw_is_active), [iw_revoke](idle_worker.c.md#iw_revoke), [iw_shutdown](idle_worker.c.md#iw_shutdown), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [pending_request](afp_dsi.c.md#pending_request), [process_cache_hints](dircache.c.md#process_cache_hints), [process_deferred_signals](afp_dsi.c.md#process_deferred_signals), [setuplog](../../libatalk/util/logger.c.md#setuplog), [tmpdir](../../libatalk/util/unix.c.md#tmpdir), [uuidcache_dump](../../libatalk/acl/cache.c.md#uuidcache_dump)

Called by: [dsi_start](main.c.md#dsi_start)

Uses file-scope variables: `AFPobj`, `afp_switch` in [etc/afpd/switch.c](switch.c.md), `debug_request`, `reload_request`, `replaycache`, `session_master`, `transfer_pending`

# Types

### struct rc_elem_t

Defined at line 73.
* `uint16_t DSIreqID`
* `uint8_t AFPcommand`
* `uint32_t result`

# Macros

* Undocumented: `SOL_TCP`

# File-scope variables

`AFPobj`, `alarm_pending`, `debug_request`, `die_pending`, `die_sender`, `die_signalled`, `getmesg_pending`, `reload_request`, `replaycache`, `session_master`, `timedown_pending`, `transfer_pending`
