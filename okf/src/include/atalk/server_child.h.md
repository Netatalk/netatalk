---
type: C Header File
title: "include/atalk/server_child.h"
description: "AFP child process state and session management interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/server_child.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
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

Declared at include/atalk/server_child.h line 68; no definition in the scanned sources.

# Types

### struct afp_child

Defined at line 24.
* `pid_t afpch_pid`: afpd worker process pid (from the worker afpd process )
* `uid_t afpch_uid`: user id of connected client (from the worker afpd process)
* `int afpch_valid`: 1 if we have a clientid
* `int afpch_killed`: 1 if we already tried to kill the client
* `uint32_t afpch_boottime`: client boot time (from the mac client)
* `time_t afpch_logintime`: time the child was added
* `uint32_t afpch_idlen`: clientid len (from the Mac client)
* `char * afpch_clientid`: clientid (from the Mac client)
* `size_t afpch_sessiontoken_len`: reconnect token length
* `char * afpch_sessiontoken`: opaque reconnect token
* `int afpch_ipc_fd`: socket for IPC bw afpd parent and childs
* `int afpch_hint_fd`: pipe write/parent end for sending cache hints to child
* `char * afpch_hostname`: hostname or address of the connected client
* `int16_t afpch_state`: state of AFP session (eg active, sleeping, disconnected)
* `char * afpch_volumes`: mounted volumes
* `struct afp_child ** afpch_prevp`
* `struct afp_child * afpch_next`

### struct server_child_t

Defined at line 45.
* `int servch_count`: Current count of active AFP sessions.
* `int servch_nsessions`: Number of allowed AFP sessions.
* `afp_child_t * servch_table`: Hashtable with data of AFP sesssions.

# Typedefs and enums

* `typedef struct afp_child afp_child_t`

# Macros

* Undocumented: `ASP_RUNNING`, `CHILD_HASHSIZE`
