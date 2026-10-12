---
type: C Source File
title: "libatalk/util/sigpipe.c"
description: "Self-pipe that wakes a poll loop from signal handlers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/sigpipe.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `fcntl.h`, `unistd.h`

# Functions

### atalk_sigpipe_init

```c
int atalk_sigpipe_init(void)
```

Defined at lines 54 to 83.

Create the self-pipe that wakes a poll loop from a signal handler.

A second call keeps the existing pipe. Both ends are non-blocking and close-on-exec. A failed setup closes both ends, so no half-configured pipe stays.

Returns: 0, or -1 with errno set

Calls: [setnonblock](socket.c.md#setnonblock)

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `sigpipe_fd`

### atalk_sigpipe_readfd

```c
int atalk_sigpipe_readfd(void)
```

Defined at lines 90 to 93.

Return the read end of the self-pipe for the poll set.

Returns: the descriptor, or -1 before atalk_sigpipe_init

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `sigpipe_fd`

### atalk_sigpipe_notify

```c
void atalk_sigpipe_notify(void)
```

Defined at lines 101 to 107.

Wake the poll loop from a signal handler with 1 byte on the self-pipe.

The call uses only write and restores errno, so a signal handler can call it. A full pipe drops the byte, since a wake-up is pending already.

Called by: [afp_asp_die_handler](../../etc/afpd/afp_asp.c.md#afp_asp_die_handler), [afp_asp_getmesg](../../etc/afpd/afp_asp.c.md#afp_asp_getmesg), [afp_asp_reload](../../etc/afpd/afp_asp.c.md#afp_asp_reload), [afp_asp_timedown](../../etc/afpd/afp_asp.c.md#afp_asp_timedown), [afp_dsi_debug](../../etc/afpd/afp_dsi.c.md#afp_dsi_debug), [afp_dsi_die_handler](../../etc/afpd/afp_dsi.c.md#afp_dsi_die_handler), [afp_dsi_getmesg_handler](../../etc/afpd/afp_dsi.c.md#afp_dsi_getmesg_handler), [afp_dsi_reload](../../etc/afpd/afp_dsi.c.md#afp_dsi_reload), [afp_dsi_timedown_handler](../../etc/afpd/afp_dsi.c.md#afp_dsi_timedown_handler), [afp_dsi_transfer_handler](../../etc/afpd/afp_dsi.c.md#afp_dsi_transfer_handler), [alarm_handler](../../etc/afpd/afp_dsi.c.md#alarm_handler)

Uses file-scope variables: `sigpipe_fd`

### atalk_sigpipe_drain

```c
void atalk_sigpipe_drain(void)
```

Defined at lines 112 to 119.

Read every pending byte from the self-pipe.

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `sigpipe_fd`

# File-scope variables

`sigpipe_fd`
