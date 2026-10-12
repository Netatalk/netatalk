---
type: C Source File
title: "etc/afpd/fce_util.c"
description: "Event coalescing for the file change event (FCE) API."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fce_util.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [desktop.h](desktop.h.md)
* [directory.h](directory.h.md)
* [fce_api_internal.h](fce_api_internal.h.md)
* [file.h](file.h.md)
* [fork.h](fork.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `netdb.h`, `netinet/in.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `time.h`

# Functions

### get_ms_difftime

```c
static long get_ms_difftime(struct timeval *tv1, struct timeval *tv2)
```

Defined at lines 76 to 81.

Return the milliseconds from one time to a later one.

Parameters:
* `tv1`: earlier time
* `tv2`: later time

Returns: tv2 minus tv1 in milliseconds, with the microseconds truncated

Called by: [fce_handle_coalescation](fce_util.c.md#fce_handle_coalescation)

### fce_initialize_history

```c
void fce_initialize_history(void)
```

Defined at lines 90 to 95. Declared in [etc/afpd/fce_api_internal.h](fce_api_internal.h.md).

Clear the event history that coalescing compares against.

Called by: [fce_register](fce_api.c.md#fce_register)

Uses file-scope variables: `fce_history_list`

### fce_handle_coalescation

```c
bool fce_handle_coalescation(int event, const char *path)
```

Defined at lines 114 to 185. Declared in [etc/afpd/fce_api_internal.h](fce_api_internal.h.md).

Decide whether the fce coalesce option drops an event.

Coalescing assumes a listener that tracks events per directory, and it spares that listener the events of objects inside a reported directory. The history keeps the last FCE_HISTORY_LEN events that passed, each for MAX_COALESCE_TIME_MS ms. A large tree or file can take longer than that.

Create coalescing drops every file create, since a file modify follows it. It also drops every event under a directory created within the window. Delete coalescing drops a delete under any path in the history. Both tests compare path prefixes as strings. An event that passes enters the history.

Parameters:
* `event`: event code
* `path`: path of the event

Returns: true when the event drops out

Calls: [get_ms_difftime](fce_util.c.md#get_ms_difftime)

Called by: [fce_register](fce_api.c.md#fce_register)

Uses file-scope variables: `coalesce`, `fce_history_list`

### fce_set_coalesce

```c
int fce_set_coalesce(const char *opt)
```

Defined at lines 196 to 219.

Set event coalescing from the fce coalesce option.

The option lists all, delete, or create, separated by commas. The last known name in the list decides, so delete and create together need all.

Parameters:
* `opt`: option value

Returns: AFP_OK, or AFPERR_PARAM for NULL

Called by: [configinit](afp_config.c.md#configinit)

Uses file-scope variables: `coalesce`

# File-scope variables

`coalesce`, `fce_history_list`
