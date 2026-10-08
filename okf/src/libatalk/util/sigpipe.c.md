---
type: C Source File
title: "libatalk/util/sigpipe.c"
description: "4 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/sigpipe.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 40 to 69.

Calls: [setnonblock](socket.c.md#setnonblock)

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `sigpipe_fd`

### atalk_sigpipe_readfd

```c
int atalk_sigpipe_readfd(void)
```

Defined at lines 71 to 74.

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `sigpipe_fd`

### atalk_sigpipe_notify

```c
void atalk_sigpipe_notify(void)
```

Defined at lines 77 to 83.

Called by: [afp_asp_die_handler](../../etc/afpd/afp_asp.c.md#afp_asp_die_handler), [afp_asp_reload](../../etc/afpd/afp_asp.c.md#afp_asp_reload), [afp_asp_timedown](../../etc/afpd/afp_asp.c.md#afp_asp_timedown), [afp_dsi_debug](../../etc/afpd/afp_dsi.c.md#afp_dsi_debug), [afp_dsi_die_handler](../../etc/afpd/afp_dsi.c.md#afp_dsi_die_handler), [afp_dsi_getmesg_handler](../../etc/afpd/afp_dsi.c.md#afp_dsi_getmesg_handler), [afp_dsi_reload](../../etc/afpd/afp_dsi.c.md#afp_dsi_reload), [afp_dsi_timedown_handler](../../etc/afpd/afp_dsi.c.md#afp_dsi_timedown_handler), [afp_dsi_transfer_handler](../../etc/afpd/afp_dsi.c.md#afp_dsi_transfer_handler), [alarm_handler](../../etc/afpd/afp_dsi.c.md#alarm_handler)

Uses file-scope variables: `sigpipe_fd`

### atalk_sigpipe_drain

```c
void atalk_sigpipe_drain(void)
```

Defined at lines 85 to 92.

Called by: [afp_over_asp](../../etc/afpd/afp_asp.c.md#afp_over_asp), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `sigpipe_fd`

# File-scope variables

`sigpipe_fd`
