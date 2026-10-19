---
type: C Source File
title: "etc/afpd/afp_asp.c"
description: "AFP sessions over ASP: signal handling and the request loop."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afp_asp.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-19T23:19:03+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp_util.h](../../include/atalk/afp_util.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [auth.h](auth.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [fork.h](fork.h.md)
* [switch.h](switch.h.md)
* System headers: `errno.h`, `poll.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/time.h`

# Functions

### afp_asp_close

```c
static void afp_asp_close(AFPObj *obj)
```

Defined at lines 58 to 83.

End an [ASP](../../include/atalk/asp.h.md#struct-asp) session by closing the volumes, logging out, and closing the [ASP](../../include/atalk/asp.h.md#struct-asp) connection.

An error path that exits after become_root leaves the euid at 0. The call switches back to obj->uid first, because the volume close scripts run as the login user. Only a failed switch ends the process at once.

Parameters:
* `obj`: session object

Calls: [asp_close](../../libatalk/asp/asp_close.c.md#asp_close), [close_all_vol](volume.c.md#close_all_vol)

Called by: [afp_asp_die_now](afp_asp.c.md#afp_asp_die_now), [afp_over_asp](afp_asp.c.md#afp_over_asp)

### afp_asp_die_now

```c
static void afp_asp_die_now(const int sig)
```

Defined at lines 123 to 139.

Shut the [ASP](../../include/atalk/asp.h.md#struct-asp) session down and exit.

The client gets a shutdown attention first. SIGTERM and SIGALRM exit with status 0, and any other value exits with sig itself.

Parameters:
* `sig`: signal that ended the session, or an EXITERR_* status

Calls: [afp_asp_close](afp_asp.c.md#afp_asp_close), [asp_attention](../../libatalk/asp/asp_attn.c.md#asp_attention), [asp_shutdown](../../libatalk/asp/asp_shutdown.c.md#asp_shutdown)

Called by: [afp_asp_timedown_now](afp_asp.c.md#afp_asp_timedown_now), [afp_over_asp](afp_asp.c.md#afp_over_asp), [asp_process_deferred_signals](afp_asp.c.md#asp_process_deferred_signals)

Uses file-scope variables: `child`

### afp_asp_timedown_now

```c
static void afp_asp_timedown_now(void)
```

Defined at lines 148 to 187.

Announce a shutdown in 5 minutes to the [ASP](../../include/atalk/asp.h.md#struct-asp) client and arm the exit timer.

The attention tells the client not to reconnect. SIGALRM after 300 seconds then ends the session, and the session ignores further SIGUSR1 signals.

Calls: [afp_asp_die_handler](afp_asp.c.md#afp_asp_die_handler), [afp_asp_die_now](afp_asp.c.md#afp_asp_die_now), [asp_attention](../../libatalk/asp/asp_attn.c.md#asp_attention)

Called by: [asp_process_deferred_signals](afp_asp.c.md#asp_process_deferred_signals)

Uses file-scope variables: `child`

### afp_asp_reload

```c
static void afp_asp_reload(int sig)
```

Defined at lines 199 to 203.

SIGHUP handler that requests a reload of the volume configuration.

Parameters:
* `sig`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `reload_request` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### afp_asp_getmesg_now

```c
static void afp_asp_getmesg_now(void)
```

Defined at lines 211 to 215.

Read the pending server message and send the [ASP](../../include/atalk/asp.h.md#struct-asp) client a message attention.

Calls: [asp_attention](../../libatalk/asp/asp_attn.c.md#asp_attention), [readmessage](messages.c.md#readmessage)

Called by: [asp_process_deferred_signals](afp_asp.c.md#asp_process_deferred_signals)

Uses file-scope variables: `child`

### afp_asp_die_handler

```c
static void afp_asp_die_handler(int sig)
```

Defined at lines 235 to 239.

SIGTERM and SIGALRM handler that records the request to end the session.

A signal value of 0 counts as SIGTERM.

Parameters:
* `sig`: received signal

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_asp_timedown_now](afp_asp.c.md#afp_asp_timedown_now), [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `asp_die_pending`

### afp_asp_timedown

```c
static void afp_asp_timedown(int sig)
```

Defined at lines 246 to 250.

SIGUSR1 handler that records the request for a shutdown in 5 minutes.

Parameters:
* `sig`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `asp_timedown_pending`

### afp_asp_getmesg

```c
static void afp_asp_getmesg(int sig)
```

Defined at lines 258 to 262.

SIGUSR2 handler that records a pending server message.

Parameters:
* `sig`: unused

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `asp_getmesg_pending`

### asp_process_deferred_signals

```c
static void asp_process_deferred_signals(AFPObj *obj)
```

Defined at lines 274 to 300.

Act on the signals that the handlers recorded while the loop was busy or waiting.

A pending exit runs first and does not return. A shutdown notice, a configuration reload, and a server message follow.

Parameters:
* `obj`: session object

Calls: [afp_asp_die_now](afp_asp.c.md#afp_asp_die_now), [afp_asp_getmesg_now](afp_asp.c.md#afp_asp_getmesg_now), [afp_asp_timedown_now](afp_asp.c.md#afp_asp_timedown_now), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `asp_die_pending`, `asp_getmesg_pending`, `asp_timedown_pending`, `reload_request` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### afp_over_asp

```c
void afp_over_asp(AFPObj *obj)
```

Defined at lines 317 to 683.

Run an AFP session over [ASP](../../include/atalk/asp.h.md#struct-asp) until the client closes it.

The session adopts the connection counters, the IPC socket, and the hint pipe that asp_getsess prepared before the fork. Signal handlers only record their event and wake poll through the self-pipe. The resource fork cache stays off for [ASP](../../include/atalk/asp.h.md#struct-asp) sessions.

Each loop pass acts on the recorded signals, applies the dircache hints, and serves one request per readable datagram. Commands and writes dispatch through afp_switch. ASP_MAX_READ_ERRORS read errors in a row end the session, and stray packets only log.

Parameters:
* `obj`: session object holding the [ASP](../../include/atalk/asp.h.md#struct-asp) session

Calls: [AfpNum2name](../../libatalk/util/afp_util.c.md#afpnum2name), [afp_asp_close](afp_asp.c.md#afp_asp_close), [afp_asp_die_handler](afp_asp.c.md#afp_asp_die_handler), [afp_asp_die_now](afp_asp.c.md#afp_asp_die_now), [afp_asp_getmesg](afp_asp.c.md#afp_asp_getmesg), [afp_asp_reload](afp_asp.c.md#afp_asp_reload), [afp_asp_timedown](afp_asp.c.md#afp_asp_timedown), [asp_attention](../../libatalk/asp/asp_attn.c.md#asp_attention), [asp_cmdreply](../../libatalk/asp/asp_cmdreply.c.md#asp_cmdreply), [asp_getrequest](../../libatalk/asp/asp_getreq.c.md#asp_getrequest), [asp_process_deferred_signals](afp_asp.c.md#asp_process_deferred_signals), [atalk_sigpipe_drain](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_drain), [atalk_sigpipe_init](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_init), [atalk_sigpipe_readfd](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_readfd), [dir_free_invalid_q](directory.c.md#dir_free_invalid_q), [dircache_init](dircache.c.md#dircache_init), [fce_pending_events](fce_api.c.md#fce_pending_events), [of_pforkdesc](ofork.c.md#of_pforkdesc), [process_cache_hints](dircache.c.md#process_cache_hints)

Called by: [asp_start](main.c.md#asp_start)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `afp_switch` in [etc/afpd/switch.c](switch.c.md), `child`

# Macros

* Undocumented: `ASP_MAX_READ_ERRORS`, `ASP_STRAY_LOG_INTERVAL`

# File-scope variables

`asp_die_pending`, `asp_getmesg_pending`, `asp_timedown_pending`, `child`
