---
type: C Source File
title: "etc/afpd/fce_util.c"
description: "4 functions, includes 15 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fce_util.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Called by: [fce_handle_coalescation](fce_util.c.md#fce_handle_coalescation)

### fce_initialize_history

```c
void fce_initialize_history(void)
```

Defined at lines 87 to 92. Declared in [etc/afpd/fce_api_internal.h](fce_api_internal.h.md).

Called by: [fce_register](fce_api.c.md#fce_register)

Uses file-scope variables: `fce_history_list`

### fce_handle_coalescation

```c
bool fce_handle_coalescation(int event, const char *path)
```

Defined at lines 94 to 165. Declared in [etc/afpd/fce_api_internal.h](fce_api_internal.h.md).

Calls: [get_ms_difftime](fce_util.c.md#get_ms_difftime)

Called by: [fce_register](fce_api.c.md#fce_register)

Uses file-scope variables: `coalesce`, `fce_history_list`

### fce_set_coalesce

```c
int fce_set_coalesce(const char *opt)
```

Defined at lines 172 to 195.

Set event coalescation to reduce number of events sent over UDP.

Note: supports event types all|delete|create

Called by: [configinit](afp_config.c.md#configinit)

Uses file-scope variables: `coalesce`

# File-scope variables

`coalesce`, `fce_history_list`
