---
type: C Source File
title: "etc/afpd/main.c"
description: "13 functions, includes 20 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/main.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afp_config.h](afp_config.h.md)
* [afpstats.h](afpstats.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_child.h](../../include/atalk/server_child.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [fork.h](fork.h.md)
* [status.h](status.h.md)
* [uam_auth.h](uam_auth.h.md)
* System headers: `errno.h`, `poll.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/resource.h`, `sys/socket.h`, `sys/time.h`, `sys/uio.h`, `sys/wait.h`, `time.h`

# Functions

### afp_exit

```c
static void afp_exit(int ret)
```

Defined at lines 64 to 67.

Called by: [main](main.c.md#main), [master_out_of_fds](main.c.md#master_out_of_fds)

### init_listening_sockets

```c
static bool init_listening_sockets(const AFPObj *dsiconfig, const AFPObj *aspconfig)
```

Defined at lines 73 to 104.

Calls: [asev_add_fd](../../libatalk/util/socket.c.md#asev_add_fd), [asev_init](../../libatalk/util/socket.c.md#asev_init)

Called by: [main](main.c.md#main)

### reset_listening_sockets

```c
static bool reset_listening_sockets(const AFPObj *dsiconfig, const AFPObj *aspconfig)
```

Defined at lines 106 to 128.

Calls: [asev_del_fd](../../libatalk/util/socket.c.md#asev_del_fd)

Called by: [main](main.c.md#main)

### afp_end_sessions

```c
static void afp_end_sessions(void)
```

Defined at lines 133 to 150.

End the master's sessions and remove its stats socket before it exits.

Calls: [asp_cleanup](main.c.md#asp_cleanup), [server_child_kill](../../libatalk/util/server_child.c.md#server_child_kill)

Called by: [afp_goaway](main.c.md#afp_goaway), [master_out_of_fds](main.c.md#master_out_of_fds)

Uses file-scope variables: `afpstats_listen_fd`, `asp_obj`, `server_children`

### afp_goaway

```c
static void afp_goaway(int sig)
```

Defined at lines 153 to 192.

Calls: [afp_end_sessions](main.c.md#afp_end_sessions), [asp_kill](../../libatalk/asp/asp_getsess.c.md#asp_kill), [auth_unload](auth.c.md#auth_unload), [server_child_kill](../../libatalk/util/server_child.c.md#server_child_kill)

Called by: [main](main.c.md#main)

Uses file-scope variables: `gotsigchld`, `nologin`, `reloadconfig`, `server_children`

### child_handler

```c
static void child_handler(void)
```

Defined at lines 194 to 229.

Calls: [asev_del_fd](../../libatalk/util/socket.c.md#asev_del_fd), [server_child_remove](../../libatalk/util/server_child.c.md#server_child_remove)

Called by: [main](main.c.md#main)

Uses file-scope variables: `server_children`

### raise_nofile_to

```c
static int raise_nofile_to(const char *who, rlim_t target, rlim_t *out)
```

Defined at lines 245 to 302.

Raise RLIMIT_NOFILE toward a target, probing down on rejection.

Requests `target` and, if the kernel refuses it with EINVAL/EPERM, binary-searches for the largest acceptable value. The realised soft limit is delivered via `out` (not the return value) so RLIM_INFINITY is never confused with the -1 error sentinel. The caller guarantees the current soft limit is finite and below `target`.

Parameters:
* `who`: caller label for log messages
* `target`: desired soft (and, if lower, hard) limit
* `out`: realised soft limit after the attempt

Returns: 0 on success, -1 only if the current limit cannot be read

Called by: [setlimits](main.c.md#setlimits)

### setlimits

```c
static int setlimits(void)
```

Defined at lines 304 to 336.

Calls: [raise_nofile_to](main.c.md#raise_nofile_to)

Called by: [main](main.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 338 to 708.

Calls: [afp_config_free](../../libatalk/util/netatalk_conf.c.md#afp_config_free), [afp_config_parse](../../libatalk/util/netatalk_conf.c.md#afp_config_parse), [afp_exit](main.c.md#afp_exit), [afp_goaway](main.c.md#afp_goaway), [afp_options_parse_cmdline](afp_options.c.md#afp_options_parse_cmdline), [afpstats_handle_accept](afpstats.c.md#afpstats_handle_accept), [afpstats_init](afpstats.c.md#afpstats_init), [asev_add_fd](../../libatalk/util/socket.c.md#asev_add_fd), [asev_del_fd](../../libatalk/util/socket.c.md#asev_del_fd), [asp_cleanup](main.c.md#asp_cleanup), [asp_start](main.c.md#asp_start), [child_handler](main.c.md#child_handler), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [configfree](afp_config.c.md#configfree), [configinit](afp_config.c.md#configinit), [daemonize](../../libatalk/util/unix.c.md#daemonize), [dsi_reserve_spare_fd](../../libatalk/dsi/dsi_getsess.c.md#dsi_reserve_spare_fd), [dsi_start](main.c.md#dsi_start), [fault_setup](../../libatalk/util/fault.c.md#fault_setup), [hint_buf_count](../../libatalk/util/server_ipc.c.md#hint_buf_count), [hint_flush_pending](../../libatalk/util/server_ipc.c.md#hint_flush_pending), [init_listening_sockets](main.c.md#init_listening_sockets), [ipc_server_read](../../libatalk/util/server_ipc.c.md#ipc_server_read), [reset_listening_sockets](main.c.md#reset_listening_sockets), [server_child_alloc](../../libatalk/util/server_child.c.md#server_child_alloc), [server_child_kill](../../libatalk/util/server_child.c.md#server_child_kill), [setlimits](main.c.md#setlimits)

Uses file-scope variables: `afpstats_listen_fd`, `asp_obj`, `dsi_obj`, `gotsigchld`, `nologin`, `reloadconfig`, `server_children`

### master_out_of_fds

```c
static void master_out_of_fds(const server_child_t *children)
```

Defined at lines 727 to 752.

Exit for a restart once the master has been out of descriptors too long.

Failures closer together than FDS_STARVED_RESTART form one episode, which any session that starts ends. The master ends every session, as at a shutdown, then exits for the supervisor to start a new one: a new master can neither reconnect, hint nor stop sessions it did not start.

Parameters:
* `children`: the master's session table

Calls: [afp_end_sessions](main.c.md#afp_end_sessions), [afp_exit](main.c.md#afp_exit), [asp_kill](../../libatalk/asp/asp_getsess.c.md#asp_kill), [count_open_fds](../../libatalk/util/unix.c.md#count_open_fds)

Called by: [dsi_start](main.c.md#dsi_start)

Uses file-scope variables: `fds_starved_last`, `fds_starved_since`

### dsi_start

```c
static afp_child_t * dsi_start(AFPObj *obj, DSI *dsi, server_child_t *server_children)
```

Defined at lines 754 to 779.

Calls: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi), [configfree](afp_config.c.md#configfree), [dsi_getsession](../../libatalk/dsi/dsi_getsess.c.md#dsi_getsession), [master_out_of_fds](main.c.md#master_out_of_fds)

Called by: [main](main.c.md#main)

Uses file-scope variables: `fds_starved_last`, `fds_starved_since`, `server_children`

### asp_start

```c
static afp_child_t * asp_start(AFPObj *obj, server_child_t *server_children)
```

Defined at lines 781 to 798.

Calls: [afp_over_asp](afp_asp.c.md#afp_over_asp), [asp_getsession](../../libatalk/asp/asp_getsess.c.md#asp_getsession)

Called by: [main](main.c.md#main)

### asp_cleanup

```c
static void asp_cleanup(const AFPObj *obj)
```

Defined at lines 799 to 805.

Calls: [asp_stop_tickle](../../libatalk/asp/asp_getsess.c.md#asp_stop_tickle), [nbp_unrgstr](../../libatalk/nbp/nbp_unrgstr.c.md#nbp_unrgstr)

Called by: [afp_end_sessions](main.c.md#afp_end_sessions), [main](main.c.md#main)

# Macros

* Undocumented: `ASEV_THRESHHOLD`, `FDS_STARVED_RESTART`, `WAIT_ANY`

# File-scope variables

`afpstats_listen_fd`, `asev`, `asp_obj`, `dsi_obj`, `fds_starved_last`, `fds_starved_since`, `gotsigchld`, `nologin`, `reloadconfig`, `server_children`
