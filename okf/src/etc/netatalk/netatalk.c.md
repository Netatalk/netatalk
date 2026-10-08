---
type: C Source File
title: "etc/netatalk/netatalk.c"
description: "28 functions, includes 14 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/netatalk/netatalk.c"
tags: ["etc/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afp_zeroconf.h](afp_zeroconf.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/iniparser_util.h](../../include/atalk/iniparser_util.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/server_child.h](../../include/atalk/server_child.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/srp.h](../../include/atalk/srp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `bstrlib.h`, `errno.h`, `event2/event.h`, `fcntl.h`, `getopt.h`, `inttypes.h`, `poll.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `strings.h`, `sys/param.h`, `sys/resource.h`, `sys/socket.h`, `sys/stat.h`, `sys/time.h`, `sys/uio.h`, `sys/wait.h`, `unistd.h`

# Functions

### service_running

```c
static bool service_running(pid_t pid)
```

Defined at lines 100 to 107.

Called by: [main](netatalk.c.md#main), [sigchld_impl](netatalk.c.md#sigchld_impl)

### srp_is_the_only_uam

```c
static bool srp_is_the_only_uam(const char *uamlist)
```

Defined at lines 114 to 127.

Whether uamlist names uams_srp.so and nothing else.

Split the way [auth_load()](../afpd/auth.c.md#auth_load) splits it, on commas and spaces.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [validate_singleuser_config](netatalk.c.md#validate_singleuser_config)

### path_parent

```c
static int path_parent(char *dst, size_t dstlen, const char *path)
```

Defined at lines 137 to 162.

Copy the parent directory of path into dst.

dirname(3) semantics: trailing slashes are ignored, a path with no slash yields ".", and a child of the root yields "/".

Returns: 0 on success, -1 when path is NULL or does not fit dst

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [dbpath_is_creatable](netatalk.c.md#dbpath_is_creatable), [parent_is_private](netatalk.c.md#parent_is_private)

### owned_private_dir

```c
static bool owned_private_dir(const char *path)
```

Defined at lines 170 to 186.

A mode-0700 directory owned by the caller, not reached through a symlink.

Trailing slashes are dropped first: with one, lstat() follows a final symbolic link.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [parent_is_private](netatalk.c.md#parent_is_private), [srp_verifier_store_is_private](netatalk.c.md#srp_verifier_store_is_private)

### parent_is_private

```c
static bool parent_is_private(const char *path)
```

Defined at lines 191 to 196.

The directory holding path is private to the caller.

Calls: [owned_private_dir](netatalk.c.md#owned_private_dir), [path_parent](netatalk.c.md#path_parent)

Called by: [dir_is_private](netatalk.c.md#dir_is_private)

### dir_is_private

```c
static bool dir_is_private(const char *path, mode_t file_mask, bool may_be_absent)
```

Defined at lines 208 to 223.

A path only the caller can reach and replace.

Parameters:
* `path`: the file to judge
* `file_mask`: mode bits the file must not carry
* `may_be_absent`: whether an absent path passes

Returns: true when the parent is private to the caller and path is either absent-and-allowed or a regular file the caller owns

Calls: [parent_is_private](netatalk.c.md#parent_is_private)

Called by: [main](netatalk.c.md#main), [validate_singleuser_config](netatalk.c.md#validate_singleuser_config)

### dbpath_is_creatable

```c
static bool dbpath_is_creatable(const char *path)
```

Defined at lines 232 to 254.

The CNID directory is the caller's own and writable, or creatable.

A symbolic link does not pass. Trailing slashes are dropped first: a [Global] vol dbpath gets the volume name and one appended, and a trailing slash makes lstat() follow a link.

Calls: [path_parent](netatalk.c.md#path_parent), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [validate_singleuser_config](netatalk.c.md#validate_singleuser_config)

### srp_verifier_store_is_private

```c
static bool srp_verifier_store_is_private(const char *path)
```

Defined at lines 263 to 289.

The verifier store is the calling user's own, holding their verifier.

The store of a single-user server is the calling user's private directory holding that user's own verifier, named by numeric uid like the SRP UAM expects. Symbolic links are refused, as the UAM refuses them.

Calls: [owned_private_dir](netatalk.c.md#owned_private_dir), [srp_verifier_mode_is_safe](../../include/atalk/srp.h.md#srp_verifier_mode_is_safe)

Called by: [validate_singleuser_config](netatalk.c.md#validate_singleuser_config)

### validate_singleuser_config

```c
static int validate_singleuser_config(bool volumes_loaded)
```

Defined at lines 299 to 425.

Refuse a single-user configuration the mode cannot serve.

Runs after the volumes are loaded and before [daemonize()](../../libatalk/util/unix.c.md#daemonize), so every message reaches the operator's terminal.

Returns: 0 when every clause passes, -1 after printing the first failure

Calls: [dbpath_is_creatable](netatalk.c.md#dbpath_is_creatable), [dir_is_private](netatalk.c.md#dir_is_private), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [realpath_safe](../../libatalk/util/unix.c.md#realpath_safe), [srp_is_the_only_uam](netatalk.c.md#srp_is_the_only_uam), [srp_verifier_store_is_private](netatalk.c.md#srp_verifier_store_is_private)

Called by: [main](netatalk.c.md#main)

Uses file-scope variables: `obj`

### libevent_logmsg_cb

```c
static void libevent_logmsg_cb(int severity, const char *msg)
```

Defined at lines 657 to 680.

libevent logging callback

Called by: [main](netatalk.c.md#main)

### sigterm_impl

```c
static void sigterm_impl(void)
```

Defined at lines 689 to 699.

SIGTERM implementation — caller must check in_shutdown before calling

Calls: [kill_childs](netatalk.c.md#kill_childs)

Called by: [sigterm_cb](netatalk.c.md#sigterm_cb)

Uses file-scope variables: `afpd_pid`, `dbus_pid`, `in_shutdown`

### sigquit_impl

```c
static void sigquit_impl(void)
```

Defined at lines 702 to 706.

SIGQUIT implementation

Calls: [kill_childs](netatalk.c.md#kill_childs)

Called by: [sigquit_cb](netatalk.c.md#sigquit_cb)

Uses file-scope variables: `afpd_pid`, `dbus_pid`

### sighup_impl

```c
static void sighup_impl(void)
```

Defined at lines 709 to 728.

SIGHUP implementation

Calls: [kill_childs](netatalk.c.md#kill_childs), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [zeroconf_deregister](afp_zeroconf.c.md#zeroconf_deregister), [zeroconf_register](afp_zeroconf.c.md#zeroconf_register)

Called by: [sighup_cb](netatalk.c.md#sighup_cb)

Uses file-scope variables: `afpd_pid`, `obj`

### sigchld_impl

```c
static bool sigchld_impl(void)
```

Defined at lines 731 to 765.

SIGCHLD implementation, returns true if all services have exited during shutdown

Calls: [service_running](netatalk.c.md#service_running)

Called by: [sigchld_cb](netatalk.c.md#sigchld_cb)

Uses file-scope variables: `afpd_pid`, `dbus_pid`, `in_shutdown`

### timer_impl

```c
static void timer_impl(void)
```

Defined at lines 768 to 796.

timer implementation

Calls: [run_afpd](netatalk.c.md#run_afpd), [run_process](netatalk.c.md#run_process)

Called by: [timer_cb](netatalk.c.md#timer_cb)

Uses file-scope variables: `afpd_pid`, `afpd_restarts`, `dbus_path`, `dbus_pid`, `dbus_restarts`, `in_shutdown`

### sigterm_cb

```c
static void sigterm_cb(evutil_socket_t fd, short what, void *arg)
```

Defined at lines 856 to 873.

Calls: [sigterm_impl](netatalk.c.md#sigterm_impl)

Called by: [main](netatalk.c.md#main)

Uses file-scope variables: `base`, `in_shutdown`, `sighup_ev`, `sigquit_ev`, `sigterm_ev`, `timer_ev`

### sigquit_cb

```c
static void sigquit_cb(evutil_socket_t fd, short what, void *arg)
```

Defined at lines 875 to 878.

Calls: [sigquit_impl](netatalk.c.md#sigquit_impl)

Called by: [main](netatalk.c.md#main)

### sighup_cb

```c
static void sighup_cb(evutil_socket_t fd, short what, void *arg)
```

Defined at lines 880 to 883.

Calls: [sighup_impl](netatalk.c.md#sighup_impl)

Called by: [main](netatalk.c.md#main)

### sigchld_cb

```c
static void sigchld_cb(evutil_socket_t fd, short what, void *arg)
```

Defined at lines 885 to 890.

Calls: [sigchld_impl](netatalk.c.md#sigchld_impl)

Called by: [main](netatalk.c.md#main)

Uses file-scope variables: `base`

### timer_cb

```c
static void timer_cb(evutil_socket_t fd, short what, void *arg)
```

Defined at lines 892 to 895.

Calls: [timer_impl](netatalk.c.md#timer_impl)

Called by: [main](netatalk.c.md#main)

### kill_childs

```c
static void kill_childs(int sig,...)
```

Defined at lines 904 to 919.

kill processes passed as varargs of type "pid_t *", terminate list with NULL

Called by: [main](netatalk.c.md#main), [sighup_impl](netatalk.c.md#sighup_impl), [sigquit_impl](netatalk.c.md#sigquit_impl), [sigterm_impl](netatalk.c.md#sigterm_impl)

### netatalk_exit

```c
static void netatalk_exit(int ret)
```

Defined at lines 922 to 926.

this get called when error conditions are met that require us to exit gracefully

Called by: [main](netatalk.c.md#main)

Uses file-scope variables: `lockfile_path`

### run_process

```c
static pid_t run_process(const char *path,...)
```

Defined at lines 929 to 960.

this forks() and exec() "path" with varags as argc[]

Called by: [main](netatalk.c.md#main), [run_afpd](netatalk.c.md#run_afpd), [timer_impl](netatalk.c.md#timer_impl)

### run_afpd

```c
static pid_t run_afpd(void)
```

Defined at lines 970 to 977.

Start afpd on the controller's configuration file.

A single-user controller passes the mode on with -u, so afpd applies the uid check in [login()](../afpd/auth.c.md#login).

Returns: the child pid, or NETATALK_SRV_ERROR

Calls: [run_process](netatalk.c.md#run_process)

Called by: [main](netatalk.c.md#main), [timer_impl](netatalk.c.md#timer_impl)

Uses file-scope variables: `obj`

### show_netatalk_version

```c
static void show_netatalk_version(void)
```

Defined at lines 979 to 1003.

Called by: [main](netatalk.c.md#main)

### show_netatalk_paths

```c
static void show_netatalk_paths(void)
```

Defined at lines 1005 to 1016.

Called by: [main](netatalk.c.md#main)

### usage

```c
static void usage(void)
```

Defined at lines 1018 to 1024.

Called by: [main](netatalk.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 1043 to 1279.

Calls: [afp_config_parse](../../libatalk/util/netatalk_conf.c.md#afp_config_parse), [check_lockfile](../../libatalk/util/server_lock.c.md#check_lockfile), [create_lockfile](../../libatalk/util/server_lock.c.md#create_lockfile), [daemonize](../../libatalk/util/unix.c.md#daemonize), [dir_is_private](netatalk.c.md#dir_is_private), [fault_setup](../../libatalk/util/fault.c.md#fault_setup), [kill_childs](netatalk.c.md#kill_childs), [libevent_logmsg_cb](netatalk.c.md#libevent_logmsg_cb), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [netatalk_exit](netatalk.c.md#netatalk_exit), [realpath_safe](../../libatalk/util/unix.c.md#realpath_safe), [run_afpd](netatalk.c.md#run_afpd), [run_process](netatalk.c.md#run_process), [service_running](netatalk.c.md#service_running), [show_netatalk_paths](netatalk.c.md#show_netatalk_paths), [show_netatalk_version](netatalk.c.md#show_netatalk_version), [sigchld_cb](netatalk.c.md#sigchld_cb), [sighup_cb](netatalk.c.md#sighup_cb), [sigquit_cb](netatalk.c.md#sigquit_cb), [sigterm_cb](netatalk.c.md#sigterm_cb), [timer_cb](netatalk.c.md#timer_cb), [usage](netatalk.c.md#usage), [validate_singleuser_config](netatalk.c.md#validate_singleuser_config), [zeroconf_register](afp_zeroconf.c.md#zeroconf_register)

Uses file-scope variables: `afpd_pid`, `base`, `dbus_path`, `dbus_pid`, `lockfile_path`, `obj`, `sigchld_ev`, `sighup_ev`, `sigquit_ev`, `sigterm_ev`, `timer_ev`

# Macros

* Undocumented: `KILL_GRACETIME`, `MYARVSIZE`, `NETATALK_SRV_ERROR`, `NETATALK_SRV_NEEDED`, `NETATALK_SRV_OPTIONAL`

# File-scope variables

`afpd_pid`, `afpd_restarts`, `base`, `dbus_path`, `dbus_pid`, `dbus_restarts`, `in_shutdown`, `lockfile_path`, `obj`, `sigchld_ev`, `sighup_ev`, `sigquit_ev`, `sigterm_ev`, `timer_ev`
