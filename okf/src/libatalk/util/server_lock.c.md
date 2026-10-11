---
type: C Source File
title: "libatalk/util/server_lock.c"
description: "3 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/server_lock.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 45 to 121.

Check the pid file of a daemon, then fork it into the background and record the pid of the child.

A pid file that names a live process stops the start, and a pid that a new process reused does too. The child closes stdin, stdout, and stderr, opens /dev/null in their place, and leaves its controlling terminal. With debug the call skips all of this. On Solaris the call returns 0 at once.

Parameters:
* `program`: daemon name for the error messages
* `pidfile`: path of the pid file
* `debug`: nonzero to stay in the foreground without a pid file

Returns: the pid of the child in the parent, 0 in the child or with debug, or -1 on error

Called by: [main](../../etc/atalkd/main.c.md#main), [main](../../etc/papd/main.c.md#main)

Uses file-scope variables: `itimer`

### check_lockfile

```c
int check_lockfile(const char *program, const char *pidfile)
```

Defined at lines 133 to 155.

Refuse to start while the pid file names a live process.

A reused pid in a stale pid file also counts as live. The message goes to stderr. On Solaris the call always passes.

Parameters:
* `program`: daemon name for the error message
* `pidfile`: path of the pid file

Returns: 0, or -1 when the pid file names a live process

Called by: [create_lockfile](server_lock.c.md#create_lockfile), [main](../../etc/netatalk/netatalk.c.md#main)

### create_lockfile

```c
int create_lockfile(const char *program, const char *pidfile)
```

Defined at lines 166 to 192.

Check the pid file, then write the pid of the calling process into it.

A new file gets mode 0644 through a umask of 022.

Parameters:
* `program`: daemon name for the error messages
* `pidfile`: path of the pid file

Returns: 0, or -1 when the pid file names a live process or cannot open

Calls: [check_lockfile](server_lock.c.md#check_lockfile)

Called by: [main](../../etc/netatalk/netatalk.c.md#main)

# File-scope variables

`itimer`
