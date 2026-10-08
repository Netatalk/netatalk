---
type: C Source File
title: "libatalk/util/server_lock.c"
description: "3 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/server_lock.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `fcntl.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/ioctl.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `termios.h`, `unistd.h`

# Functions

### server_lock

```c
pid_t server_lock(char *program, char *pidfile, int debug)
```

Defined at lines 33 to 109.

this creates an open lock file which hangs around until the program dies. it returns the pid. due to problems w/ solaris, this has been changed to do the kill() thing.

Called by: [main](../../etc/atalkd/main.c.md#main), [main](../../etc/papd/main.c.md#main)

Uses file-scope variables: `itimer`

### check_lockfile

```c
int check_lockfile(const char *program, const char *pidfile)
```

Defined at lines 114 to 136.

Check lockfile

Called by: [create_lockfile](server_lock.c.md#create_lockfile), [main](../../etc/netatalk/netatalk.c.md#main)

### create_lockfile

```c
int create_lockfile(const char *program, const char *pidfile)
```

Defined at lines 141 to 167.

Check and create lockfile

Calls: [check_lockfile](server_lock.c.md#check_lockfile)

Called by: [main](../../etc/netatalk/netatalk.c.md#main)

# File-scope variables

`itimer`
