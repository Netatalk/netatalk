---
type: C Source File
title: "etc/afpd/afp_asp.c"
description: "8 functions, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afp_asp.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp_util.h](../../include/atalk/afp_util.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
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

Defined at lines 41 to 66.

Calls: [asp_close](../../libatalk/asp/asp_close.c.md#asp_close), [close_all_vol](volume.c.md#close_all_vol)

Called by: [afp_asp_die_now](afp_asp.c.md#afp_asp_die_now), [afp_over_asp](afp_asp.c.md#afp_over_asp)

Calls through [`AFPObj::logout`](../../include/atalk/globals.h.md#struct-afpobj): no table assigns this field

### afp_asp_die_now

```c
static void afp_asp_die_now(const int sig)
```

Defined at lines 98 to 114.

Calls: [afp_asp_close](afp_asp.c.md#afp_asp_close), [asp_attention](../../libatalk/asp/asp_attn.c.md#asp_attention), [asp_shutdown](../../libatalk/asp/asp_shutdown.c.md#asp_shutdown)

Called by: [afp_asp_timedown_now](afp_asp.c.md#afp_asp_timedown_now), [afp_over_asp](afp_asp.c.md#afp_over_asp), [asp_process_deferred_signals](afp_asp.c.md#asp_process_deferred_signals)

Uses file-scope variables: `child`

### afp_asp_timedown_now

```c
static void afp_asp_timedown_now(void)
```

Defined at lines 119 to 158.

Calls: [afp_asp_die_handler](afp_asp.c.md#afp_asp_die_handler), [afp_asp_die_now](afp_asp.c.md#afp_asp_die_now), [asp_attention](../../libatalk/asp/asp_attn.c.md#asp_attention)

Called by: [asp_process_deferred_signals](afp_asp.c.md#asp_process_deferred_signals)

Uses file-scope variables: `child`

### afp_asp_reload

```c
static void afp_asp_reload(int sig)
```

Defined at lines 165 to 169.

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `reload_request` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### afp_asp_die_handler

```c
static void afp_asp_die_handler(int sig)
```

Defined at lines 188 to 192.

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_asp_timedown_now](afp_asp.c.md#afp_asp_timedown_now), [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `asp_die_pending`

### afp_asp_timedown

```c
static void afp_asp_timedown(int sig)
```

Defined at lines 194 to 198.

Calls: [atalk_sigpipe_notify](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_notify)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `asp_timedown_pending`

### asp_process_deferred_signals

```c
static void asp_process_deferred_signals(AFPObj *obj)
```

Defined at lines 211 to 237.

Act on signals recorded while the loop was busy or waiting.

Calls: [afp_asp_die_now](afp_asp.c.md#afp_asp_die_now), [afp_asp_timedown_now](afp_asp.c.md#afp_asp_timedown_now), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `asp_die_pending`, `asp_timedown_pending`, `reload_request` in [etc/afpd/afp_dsi.c](afp_dsi.c.md)

### afp_over_asp

```c
void afp_over_asp(AFPObj *obj)
```

Defined at lines 240 to 590.

Calls: [AfpNum2name](../../libatalk/util/afp_util.c.md#afpnum2name), [afp_asp_close](afp_asp.c.md#afp_asp_close), [afp_asp_die_handler](afp_asp.c.md#afp_asp_die_handler), [afp_asp_die_now](afp_asp.c.md#afp_asp_die_now), [afp_asp_reload](afp_asp.c.md#afp_asp_reload), [afp_asp_timedown](afp_asp.c.md#afp_asp_timedown), [asp_attention](../../libatalk/asp/asp_attn.c.md#asp_attention), [asp_cmdreply](../../libatalk/asp/asp_cmdreply.c.md#asp_cmdreply), [asp_getrequest](../../libatalk/asp/asp_getreq.c.md#asp_getrequest), [asp_process_deferred_signals](afp_asp.c.md#asp_process_deferred_signals), [atalk_sigpipe_drain](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_drain), [atalk_sigpipe_init](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_init), [atalk_sigpipe_readfd](../../libatalk/util/sigpipe.c.md#atalk_sigpipe_readfd), [dir_free_invalid_q](directory.c.md#dir_free_invalid_q), [dircache_init](dircache.c.md#dircache_init), [of_pforkdesc](ofork.c.md#of_pforkdesc), [process_cache_hints](dircache.c.md#process_cache_hints)

Called by: [asp_start](main.c.md#asp_start)

Uses file-scope variables: `AFPobj` in [etc/afpd/afp_dsi.c](afp_dsi.c.md), `afp_switch` in [etc/afpd/switch.c](switch.c.md), `child`

# Macros

* Undocumented: `ASP_MAX_READ_ERRORS`, `ASP_STRAY_LOG_INTERVAL`

# File-scope variables

`asp_die_pending`, `asp_timedown_pending`, `child`
