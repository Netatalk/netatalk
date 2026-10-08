---
type: C Header File
title: "include/atalk/server_ipc.h"
description: "1 type, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/server_ipc.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](cnid.h.md)
* [atalk/globals.h](globals.h.md)
* [atalk/server_child.h](server_child.h.md)
* System headers: `stdint.h`

# Included by

* [etc/afpd/acls.c](../../etc/afpd/acls.c.md)
* [etc/afpd/afp_dsi.c](../../etc/afpd/afp_dsi.c.md)
* [etc/afpd/auth.c](../../etc/afpd/auth.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/afpd/dircache.c](../../etc/afpd/dircache.c.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/extattrs.c](../../etc/afpd/extattrs.c.md)
* [etc/afpd/file.c](../../etc/afpd/file.c.md)
* [etc/afpd/filedir.c](../../etc/afpd/filedir.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/ofork.c](../../etc/afpd/ofork.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [libatalk/util/server_ipc.c](../../libatalk/util/server_ipc.c.md)

# Types

### struct ipc_cache_hint_payload

Defined at line 33.
* `uint8_t event`
* `uint8_t reserved`
* `uint16_t vid`
* `cnid_t cnid`

# Macros

* Undocumented: `CACHE_HINT_COUNT`, `CACHE_HINT_DELETE`, `CACHE_HINT_DELETE_CHILDREN`, `CACHE_HINT_REFRESH`, `CACHE_HINT_VOLUME_RESET`, `HINT_BUF_SIZE`, `HINT_FLUSH_INTERVAL_MS`, `IPC_CACHE_HINT`, `IPC_DISCOLDSESSION`, `IPC_GETSESSION`, `IPC_HEADERLEN`, `IPC_LOGINDONE`, `IPC_MAXMSGSIZE`, `IPC_SESSIONTOKEN`, `IPC_STATE`, `IPC_VOLUMES`
