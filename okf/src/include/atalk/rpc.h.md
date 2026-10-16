---
type: C Header File
title: "include/atalk/rpc.h"
description: "RPC locking interface for AppleDouble files."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/rpc.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `inttypes.h`, `sys/types.h`

# Functions

### rpc_init

```c
int rpc_init(const char *addr, unsigned short port)
```

Declared at include/atalk/rpc.h line 29; no definition in the scanned sources.

### rpc_lock

```c
int rpc_lock(struct adouble *, uint32_t eid, int type, off_t off, off_t len, int user)
```

Declared at include/atalk/rpc.h line 30; no definition in the scanned sources.

### rpc_unlock

```c
void rpc_unlock(struct adouble *, int user)
```

Declared at include/atalk/rpc.h line 32; no definition in the scanned sources.

### rpc_tmplock

```c
int rpc_tmplock(struct adouble *, uint32_t eid, int type, off_t off, off_t len, int user)
```

Declared at include/atalk/rpc.h line 33; no definition in the scanned sources.
