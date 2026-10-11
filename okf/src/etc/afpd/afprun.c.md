---
type: C Source File
title: "etc/afpd/afprun.c"
description: "6 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afprun.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `grp.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/types.h`, `sys/wait.h`, `unistd.h`

# Functions

### setup_out_fd

```c
static int setup_out_fd(void)
```

Defined at lines 60 to 77.

Create an unlinked temporary file for the output of an afprun command.

The file lives in tmpdir and has no name after the call, so it disappears with its last descriptor.

Returns: the descriptor, or -1 when mkstemp fails

Calls: [tmpdir](../../libatalk/util/unix.c.md#tmpdir)

Called by: [afprun](afprun.c.md#afprun)

### gain_root_privilege

```c
static void gain_root_privilege(void)
```

Defined at lines 83 to 88.

Gain root privilege before doing something.

Note: We want to end up with ruid==euid==0

Called by: [become_user_permanently](afprun.c.md#become_user_permanently)

### gain_root_group_privilege

```c
static void gain_root_group_privilege(void)
```

Defined at lines 94 to 100.

Ensure our real and effective groups are zero.

Note: we want to end up with rgid==egid==0

Called by: [become_user_permanently](afprun.c.md#become_user_permanently)

### become_user_permanently

```c
static int become_user_permanently(uid_t uid, gid_t gid)
```

Defined at lines 114 to 275.

Switch the real, effective, and saved IDs to a user and group permanently.

The call takes root first, so the final set calls succeed, and clears the supplementary groups. Linux uses setresgid and setresuid. Other systems use setegid, setgid, seteuid, and setuid.

Parameters:
* `uid`: user to become
* `gid`: group to become

Returns: 0, or -1 when a set call fails

Calls: [gain_root_group_privilege](afprun.c.md#gain_root_group_privilege), [gain_root_privilege](afprun.c.md#gain_root_privilege)

Called by: [afprun](afprun.c.md#afprun), [afprun_bg](afprun.c.md#afprun_bg)

### afprun

```c
int afprun(char *cmd, int *outfd)
```

Defined at lines 293 to 423.

Run a shell command as the euid of the caller and wait for its exit status.

The child runs the command through /bin/sh from the root directory, with root dropped permanently and descriptors 3 to 255 closed. SIGCHLD keeps its default action until the wait ends, because the session handler would reap the child first. A child that fails before the command runs uses an exit status between 80 and 83.

Parameters:
* `cmd`: shell command line
* `outfd`: receives a rewound temporary file with the command output, or NULL to keep the standard output of afpd

Returns: the exit status, the raw wait status of a killed command, the errno of a failed fork, or -1 when setup_out_fd or waitpid fails

Calls: [become_user_permanently](afprun.c.md#become_user_permanently), [setup_out_fd](afprun.c.md#setup_out_fd)

Called by: [afp_openvol](volume.c.md#afp_openvol), [closevol](volume.c.md#closevol)

### afprun_bg

```c
int afprun_bg(char *cmd)
```

Defined at lines 435 to 492.

Start a shell command as the euid of the caller without waiting for it.

The child runs the command like afprun but keeps the standard output of afpd. It closes the descriptors from 3 to the open file limit. The SIGCHLD handler of the session reaps it.

Parameters:
* `cmd`: shell command line

Returns: 0 after the fork, or the errno of a failed fork

Calls: [become_user_permanently](afprun.c.md#become_user_permanently)

Called by: [send_fce_event](fce_api.c.md#send_fce_event)

# Macros

* Undocumented: `USE_SETEUID`
