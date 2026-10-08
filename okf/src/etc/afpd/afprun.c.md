---
type: C Source File
title: "etc/afpd/afprun.c"
description: "6 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afprun.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 55 to 72.

This is a utility function of [afprun()](afprun.c.md#afprun).

Calls: [tmpdir](../../libatalk/util/unix.c.md#tmpdir)

Called by: [afprun](afprun.c.md#afprun)

### gain_root_privilege

```c
static void gain_root_privilege(void)
```

Defined at lines 78 to 83.

Gain root privilege before doing something.

Note: We want to end up with ruid==euid==0

Called by: [become_user_permanently](afprun.c.md#become_user_permanently)

### gain_root_group_privilege

```c
static void gain_root_group_privilege(void)
```

Defined at lines 89 to 95.

Ensure our real and effective groups are zero.

Note: we want to end up with rgid==egid==0

Called by: [become_user_permanently](afprun.c.md#become_user_permanently)

### become_user_permanently

```c
static int become_user_permanently(uid_t uid, gid_t gid)
```

Defined at lines 101 to 262.

Become the specified uid and gid - permanently !

Note: there should be no way back if possible

Calls: [gain_root_group_privilege](afprun.c.md#gain_root_group_privilege), [gain_root_privilege](afprun.c.md#gain_root_privilege)

Called by: [afprun](afprun.c.md#afprun), [afprun_bg](afprun.c.md#afprun_bg)

### afprun

```c
int afprun(char *cmd, int *outfd)
```

Defined at lines 270 to 400.

run a command

Note: being careful about uid/gid handling and putting the output in outfd (or discard it if outfd is NULL).

Calls: [become_user_permanently](afprun.c.md#become_user_permanently), [setup_out_fd](afprun.c.md#setup_out_fd)

Called by: [afp_openvol](volume.c.md#afp_openvol), [closevol](volume.c.md#closevol)

Mentioned in the documentation of: [setup_out_fd](afprun.c.md#setup_out_fd)

### afprun_bg

```c
int afprun_bg(char *cmd)
```

Defined at lines 406 to 463.

Run a command in the background without waiting.

Note: being careful about uid/gid handling

Calls: [become_user_permanently](afprun.c.md#become_user_permanently)

Called by: [send_fce_event](fce_api.c.md#send_fce_event)

# Macros

* Undocumented: `USE_SETEUID`
