---
type: C Header File
title: "include/atalk/server_child.h"
description: "data structures and utility functions for child processes"
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/server_child.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `arpa/inet.h`, `stddef.h`, `sys/types.h`

# Included by

* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/afp_config.h](../../etc/afpd/afp_config.h.md)
* [etc/afpd/afpstats.c](../../etc/afpd/afpstats.c.md)
* [etc/afpd/afpstats.h](../../etc/afpd/afpstats.h.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [include/atalk/asp.h](asp.h.md)
* [include/atalk/dsi.h](dsi.h.md)
* [include/atalk/server_ipc.h](server_ipc.h.md)
* [libatalk/asp/asp_getsess.c](../../libatalk/asp/asp_getsess.c.md)
* [libatalk/dsi/dsi_getsess.c](../../libatalk/dsi/dsi_getsess.c.md)
* [libatalk/util/server_child.c](../../libatalk/util/server_child.c.md)
* [libatalk/util/server_ipc.c](../../libatalk/util/server_ipc.c.md)

# Functions

### server_child_handler

```c
void server_child_handler(server_child_t *)
```

Declared at include/atalk/server_child.h line 70; no definition in the scanned sources.

# Types

### struct afp_child

Defined at line 26.
* `pid_t afpch_pid`
* `uid_t afpch_uid`
* `int afpch_valid`
* `int afpch_killed`
* `uint32_t afpch_boottime`
* `time_t afpch_logintime`
* `uint32_t afpch_idlen`
* `char * afpch_clientid`
* `size_t afpch_sessiontoken_len`
* `char * afpch_sessiontoken`
* `int afpch_ipc_fd`
* `int afpch_hint_fd`
* `char * afpch_hostname`
* `int16_t afpch_state`
* `char * afpch_volumes`
* `struct afp_child ** afpch_prevp`
* `struct afp_child * afpch_next`

### struct server_child_t

Defined at line 47.
* `int servch_count`
* `int servch_nsessions`
* `afp_child_t * servch_table`

# Typedefs and enums

* `typedef struct afp_child afp_child_t`

# Macros

* Undocumented: `ASP_RUNNING`, `CHILD_HASHSIZE`
