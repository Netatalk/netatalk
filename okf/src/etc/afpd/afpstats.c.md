---
type: C Source File
title: "etc/afpd/afpstats.c"
description: "5 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afpstats.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afpstats.h](afpstats.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/server_child.h](../../include/atalk/server_child.h.md)
* System headers: `errno.h`, `fcntl.h`, `pwd.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/socket.h`, `sys/stat.h`, `sys/un.h`, `time.h`, `unistd.h`

# Functions

### afpstats_init

```c
int afpstats_init(server_child_t *childs_in, const char *sock_path, bool set_group, gid_t gid)
```

Defined at lines 43 to 123.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [main](main.c.md#main)

Uses file-scope variables: `childs`

### state_name

```c
static const char * state_name(int16_t state)
```

Defined at lines 125 to 142.

Called by: [afpstats_handle_accept](afpstats.c.md#afpstats_handle_accept)

### protocol_name

```c
static const char * protocol_name(int16_t state)
```

Defined at lines 144 to 161.

Called by: [afpstats_handle_accept](afpstats.c.md#afpstats_handle_accept)

### user_name

```c
static const char * user_name(uid_t uid, char *buf, size_t buflen)
```

Defined at lines 163 to 174.

Called by: [afpstats_handle_accept](afpstats.c.md#afpstats_handle_accept)

### afpstats_handle_accept

```c
void afpstats_handle_accept(int listen_fd)
```

Defined at lines 176 to 232.

Calls: [protocol_name](afpstats.c.md#protocol_name), [state_name](afpstats.c.md#state_name), [user_name](afpstats.c.md#user_name)

Called by: [main](main.c.md#main)

Uses file-scope variables: `childs`

# File-scope variables

`childs`
