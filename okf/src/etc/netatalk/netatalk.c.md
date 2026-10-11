---
type: C Source File
title: "etc/netatalk/netatalk.c"
description: "33 functions, includes 14 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/netatalk/netatalk.c"
tags: ["etc/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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
* System headers: `bstrlib.h`, `errno.h`, `ev.h`, `fcntl.h`, `getopt.h`, `inttypes.h`, `poll.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `strings.h`, `sys/param.h`, `sys/resource.h`, `sys/socket.h`, `sys/stat.h`, `sys/time.h`, `sys/uio.h`, `sys/wait.h`, `unistd.h`

# Functions

### service_running

```c
static bool service_running(pid_t pid)
```

Defined at lines 106 to 113.

Tell whether a service slot holds the pid of a running child.

Parameters:
* `pid`: pid slot, or NETATALK_SRV_NEEDED or NETATALK_SRV_OPTIONAL

Returns: true for a pid, false for either marker

Called by: [main](netatalk.c.md#main), [sigchld_impl](netatalk.c.md#sigchld_impl)

### srp_is_the_only_uam

```c
static bool srp_is_the_only_uam(const char *uamlist)
```

Defined at lines 124 to 137.

Tell whether uamlist names uams_srp.so and nothing else.

The call splits the list on commas and spaces, as auth_load does.

Parameters:
* `uamlist`: uam list of afp.conf, or NULL

Returns: true for a list of only uams_srp.so, false otherwise or for a list of MAXPATHLEN bytes or more

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [validate_singleuser_config](netatalk.c.md#validate_singleuser_config)

### path_parent

```c
static int path_parent(char *dst, size_t dstlen, const char *path)
```

Defined at lines 150 to 175.

Write the parent directory of path into dst.

The result follows dirname(3). Trailing slashes do not count, a path without a slash gives ".", and a child of the root gives "/".

Parameters:
* `dst`: receives the parent directory
* `dstlen`: size of dst
* `path`: path, or NULL

Returns: 0, or -1 when path is NULL or does not fit dst

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [dbpath_is_creatable](netatalk.c.md#dbpath_is_creatable), [parent_is_private](netatalk.c.md#parent_is_private)

### owned_private_dir

```c
static bool owned_private_dir(const char *path)
```

Defined at lines 188 to 204.

Tell whether path is a directory of the caller without group or other permission bits.

The last component must not be a symbolic link. The call drops trailing slashes first, since lstat follows a final symbolic link through one.

Parameters:
* `path`: directory

Returns: true when the real uid owns the directory and its mode has no group or other bits

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [parent_is_private](netatalk.c.md#parent_is_private), [srp_verifier_store_is_private](netatalk.c.md#srp_verifier_store_is_private)

### parent_is_private

```c
static bool parent_is_private(const char *path)
```

Defined at lines 213 to 218.

Tell whether the parent directory of path is private to the caller, by owned_private_dir.

Parameters:
* `path`: file or directory

Returns: true when the parent passes owned_private_dir

Calls: [owned_private_dir](netatalk.c.md#owned_private_dir), [path_parent](netatalk.c.md#path_parent)

Called by: [dir_is_private](netatalk.c.md#dir_is_private)

### dir_is_private

```c
static bool dir_is_private(const char *path, mode_t file_mask, bool may_be_absent)
```

Defined at lines 230 to 245.

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

Defined at lines 259 to 281.

Tell whether a CNID directory belongs to the caller and is writable, or can be created.

A symbolic link does not pass. The call drops trailing slashes first. A [Global] vol dbpath gets the volume name and a slash appended, and a trailing slash makes lstat follow a link. A missing directory passes when its parent is writable and searchable.

Parameters:
* `path`: CNID directory of a volume

Returns: true for a usable or creatable directory

Calls: [path_parent](netatalk.c.md#path_parent), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [validate_singleuser_config](netatalk.c.md#validate_singleuser_config)

### srp_verifier_store_is_private

```c
static bool srp_verifier_store_is_private(const char *path)
```

Defined at lines 296 to 322.

Tell whether the SRP verifier store is private to the calling user and holds the verifier of that user.

The store has to pass owned_private_dir. The verifier file has the numeric uid as its name, as the SRP UAM expects. It has to be a regular file with 1 link that the user owns and can read, with a mode that srp_verifier_mode_is_safe accepts. Symbolic links fail, as the UAM refuses them.

Parameters:
* `path`: verifier store, or NULL for _PATH_AFPSRPVERIFIERPATH

Returns: true when the store and the verifier pass

Calls: [owned_private_dir](netatalk.c.md#owned_private_dir), [srp_verifier_mode_is_safe](../../include/atalk/srp.h.md#srp_verifier_mode_is_safe)

Called by: [validate_singleuser_config](netatalk.c.md#validate_singleuser_config)

### validate_singleuser_config

```c
static int validate_singleuser_config(bool volumes_loaded)
```

Defined at lines 342 to 468.

Refuse a single-user configuration that the mode cannot serve.

The call runs after the volumes load and before daemonize, so every message reaches the terminal of the operator. It refuses AppleTalk, afpstats, forced identities, an admin group, and a uam list other than uams_srp.so. The config file and the SRP verifier store must be private to the user, and [Homes] is out.

Every configured volume must load and use the sqlite CNID backend, and a Spotlight volume no backend other than cnid. The user must reach each volume, and its CNID directory must be another directory that the user owns or can create. A port below 1024 only warns.

Parameters:
* `volumes_loaded`: result of the volume load

Returns: 0 when every clause passes, -1 after the message of the first failure

Calls: [dbpath_is_creatable](netatalk.c.md#dbpath_is_creatable), [dir_is_private](netatalk.c.md#dir_is_private), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [realpath_safe](../../libatalk/util/unix.c.md#realpath_safe), [srp_is_the_only_uam](netatalk.c.md#srp_is_the_only_uam), [srp_verifier_store_is_private](netatalk.c.md#srp_verifier_store_is_private)

Called by: [main](netatalk.c.md#main)

Uses file-scope variables: `obj`

### makedirs

```c
static int makedirs(const char *path, mode_t mode)
```

Defined at lines 482 to 509.

Create a directory and its missing parents, as mkdir -p does.

Every new component gets mode under the umask, and an existing one stays as it is.

Parameters:
* `path`: directory to create
* `mode`: mode of the new directories

Returns: 0, or -1 with errno set, ENAMETOOLONG for a path of PATH_MAX bytes or more

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [set_sl_volumes](netatalk.c.md#set_sl_volumes)

### open_dconf_file

```c
static FILE * open_dconf_file(const char *path)
```

Defined at lines 519 to 544.

Open a dconf input file for writing with mode 0644, whatever the umask.

The call truncates the file, and it sets the mode of an existing file too.

Parameters:
* `path`: file to open

Returns: the stream, or NULL with errno set

Called by: [set_sl_volumes](netatalk.c.md#set_sl_volumes)

### set_sl_volumes

```c
static int set_sl_volumes(void)
```

Defined at lines 558 to 710.

Point the localsearch indexer at the Spotlight volumes through a dconf database.

The keyfile lists the path of every Spotlight volume as a recursive index directory, and the locks file pins both index settings. A path with a single quote stays out with a warning, since it would end the quoted string. The call then runs dconf compile and waits for it.

Returns: 0, or -1 without volumes or when a write, the fork, or dconf compile fails

Calls: [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [makedirs](netatalk.c.md#makedirs), [open_dconf_file](netatalk.c.md#open_dconf_file)

Called by: [main](netatalk.c.md#main)

### libev_syserr_cb

```c
static void libev_syserr_cb(const char *msg) EV_NOEXCEPT
```

Defined at lines 724 to 727.

Log a fatal libev error and end the service controller.

Parameters:
* `msg`: error text from libev

Calls: [netatalk_exit](netatalk.c.md#netatalk_exit)

Called by: [main](netatalk.c.md#main)

### sigterm_impl

```c
static void sigterm_impl(void)
```

Defined at lines 764 to 774.

SIGTERM implementation — caller must check in_shutdown before calling.

Calls: [kill_childs](netatalk.c.md#kill_childs)

Called by: [sigterm_cb](netatalk.c.md#sigterm_cb)

Uses file-scope variables: `afpd_pid`, `dbus_pid`, `in_shutdown`

### sigquit_impl

```c
static void sigquit_impl(void)
```

Defined at lines 777 to 781.

SIGQUIT implementation.

Calls: [kill_childs](netatalk.c.md#kill_childs)

Called by: [sigquit_cb](netatalk.c.md#sigquit_cb)

Uses file-scope variables: `afpd_pid`, `dbus_pid`

### sighup_impl

```c
static void sighup_impl(void)
```

Defined at lines 784 to 803.

SIGHUP implementation.

Calls: [kill_childs](netatalk.c.md#kill_childs), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [zeroconf_deregister](afp_zeroconf.c.md#zeroconf_deregister), [zeroconf_register](afp_zeroconf.c.md#zeroconf_register)

Called by: [sighup_cb](netatalk.c.md#sighup_cb)

Uses file-scope variables: `afpd_pid`, `obj`

### sigchld_impl

```c
static bool sigchld_impl(void)
```

Defined at lines 813 to 847.

Reap the exited children and mark their services for a restart.

An exited afpd or dbus gets NETATALK_SRV_ERROR, and the timer restarts it outside a shutdown.

Returns: true during a shutdown once neither service runs

Calls: [service_running](netatalk.c.md#service_running)

Called by: [sigchld_cb](netatalk.c.md#sigchld_cb)

Uses file-scope variables: `afpd_pid`, `dbus_pid`, `in_shutdown`

### timer_impl

```c
static void timer_impl(void)
```

Defined at lines 850 to 878.

timer implementation

Calls: [run_afpd](netatalk.c.md#run_afpd), [run_process](netatalk.c.md#run_process)

Called by: [timer_cb](netatalk.c.md#timer_cb)

Uses file-scope variables: `afpd_pid`, `afpd_restarts`, `dbus_path`, `dbus_pid`, `dbus_restarts`, `in_shutdown`

### kill_timer_cb

```c
static void kill_timer_cb(struct ev_loop *ev_loop, ev_timer *w, int revents)
```

Defined at lines 893 to 897.

End the event loop when the shutdown grace time runs out.

Parameters:
* `ev_loop`: event loop, unused
* `w`: grace timer, unused
* `revents`: events, unused

Called by: [sigterm_cb](netatalk.c.md#sigterm_cb)

Uses file-scope variables: `loop`

### sigterm_cb

```c
static void sigterm_cb(struct ev_loop *ev_loop, ev_signal *w, int revents)
```

Defined at lines 911 to 926.

Start the shutdown on SIGTERM, with KILL_GRACETIME seconds for the children to exit.

The call sends SIGTERM to afpd and dbus. It stops the SIGTERM, SIGQUIT, and SIGHUP watchers and the restart timer, so only SIGCHLD and the grace timer remain. A SIGTERM during a shutdown does nothing.

Parameters:
* `ev_loop`: event loop, unused
* `w`: signal watcher, unused
* `revents`: events, unused

Calls: [kill_timer_cb](netatalk.c.md#kill_timer_cb), [sigterm_impl](netatalk.c.md#sigterm_impl)

Called by: [main](netatalk.c.md#main)

Uses file-scope variables: `in_shutdown`, `kill_timer_ev`, `loop`, `sighup_ev`, `sigquit_ev`, `sigterm_ev`, `timer_ev`

### sigquit_cb

```c
static void sigquit_cb(struct ev_loop *ev_loop, ev_signal *w, int revents)
```

Defined at lines 935 to 939.

Pass SIGQUIT on to afpd and dbus.

Parameters:
* `ev_loop`: event loop, unused
* `w`: signal watcher, unused
* `revents`: events, unused

Calls: [sigquit_impl](netatalk.c.md#sigquit_impl)

Called by: [main](netatalk.c.md#main)

### sighup_cb

```c
static void sighup_cb(struct ev_loop *ev_loop, ev_signal *w, int revents)
```

Defined at lines 951 to 955.

Reload the volumes on SIGHUP, register them with Zeroconf again, and pass the signal on to afpd.

A single-user server ignores SIGHUP.

Parameters:
* `ev_loop`: event loop, unused
* `w`: signal watcher, unused
* `revents`: events, unused

Calls: [sighup_impl](netatalk.c.md#sighup_impl)

Called by: [main](netatalk.c.md#main)

### sigchld_cb

```c
static void sigchld_cb(struct ev_loop *ev_loop, ev_signal *w, int revents)
```

Defined at lines 965 to 971.

Reap the children on SIGCHLD, and end the event loop once a shutdown has no service left.

Parameters:
* `ev_loop`: event loop, unused
* `w`: signal watcher, unused
* `revents`: events, unused

Calls: [sigchld_impl](netatalk.c.md#sigchld_impl)

Called by: [main](netatalk.c.md#main)

Uses file-scope variables: `loop`

### timer_cb

```c
static void timer_cb(struct ev_loop *ev_loop, ev_timer *w, int revents)
```

Defined at lines 982 to 986.

Restart afpd and dbus when they need it, on the 1-second timer.

The timer does nothing during a shutdown.

Parameters:
* `ev_loop`: event loop, unused
* `w`: restart timer, unused
* `revents`: events, unused

Calls: [timer_impl](netatalk.c.md#timer_impl)

Called by: [main](netatalk.c.md#main)

### kill_childs

```c
static void kill_childs(int sig,...)
```

Defined at lines 1046 to 1061.

Send a signal to the services whose pid slots follow, up to a NULL.

A slot that holds NETATALK_SRV_ERROR or NETATALK_SRV_OPTIONAL gets no signal.

Parameters:
* `sig`: signal number
* `...`: pid_t pointers, then NULL

Called by: [main](netatalk.c.md#main), [sighup_impl](netatalk.c.md#sighup_impl), [sigquit_impl](netatalk.c.md#sigquit_impl), [sigterm_impl](netatalk.c.md#sigterm_impl)

### netatalk_exit

```c
static void netatalk_exit(int ret)
```

Defined at lines 1068 to 1072.

Remove the lock file and exit with a status.

Parameters:
* `ret`: exit status

Called by: [libev_syserr_cb](netatalk.c.md#libev_syserr_cb), [main](netatalk.c.md#main)

Uses file-scope variables: `lockfile_path`

### run_process

```c
static pid_t run_process(const char *path,...)
```

Defined at lines 1084 to 1115.

Fork and run a program with a NULL-terminated list of arguments.

The path also serves as argv[0]. A failed exec logs and exits the child with 1.

Parameters:
* `path`: program to run
* `...`: char pointer arguments, then NULL

Returns: the pid of the child, or -1 when fork fails

Called by: [main](netatalk.c.md#main), [run_afpd](netatalk.c.md#run_afpd), [timer_impl](netatalk.c.md#timer_impl)

### run_afpd

```c
static pid_t run_afpd(void)
```

Defined at lines 1125 to 1132.

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

Defined at lines 1138 to 1162.

Print the version, the license note, and the Zeroconf and Spotlight backends of the build.

Called by: [main](netatalk.c.md#main)

### show_netatalk_paths

```c
static void show_netatalk_paths(void)
```

Defined at lines 1171 to 1182.

Print the paths of afp.conf, afpd, the dbus daemon and its configuration, and the lock file.

The dbus lines need the localsearch backend, and Solaris builds have no lock file line.

Called by: [main](netatalk.c.md#main)

### usage

```c
static void usage(void)
```

Defined at lines 1187 to 1193.

Print the command line forms of netatalk.

Called by: [main](netatalk.c.md#main)

### any_volume_uses_localsearch

```c
static bool any_volume_uses_localsearch(void)
```

Defined at lines 1201 to 1214.

Tell whether a Spotlight volume names the localsearch backend.

Returns: true when one volume does, false otherwise

Calls: [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes)

Called by: [main](netatalk.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 1236 to 1472.

Run the netatalk service controller, which starts afpd and the Spotlight dbus daemon and restarts them.

As root the controller serves every user. With single-user it runs as one user other than root, and it needs -P with a pid file in a private directory. It validates the configuration before it daemonizes.

The controller loads the volumes, daemonizes without -d, writes the lock file, and starts afpd. A localsearch volume also starts dbus and writes the indexer configuration. The controller registers with Zeroconf and runs the event loop until a SIGTERM shutdown ends. Any service left after the grace time gets SIGKILL.

Parameters:
* `argc`: argument count
* `argv`: argument vector

Returns: no value, since netatalk_exit ends the process with the status of the event loop

Calls: [afp_config_parse](../../libatalk/util/netatalk_conf.c.md#afp_config_parse), [any_volume_uses_localsearch](netatalk.c.md#any_volume_uses_localsearch), [check_lockfile](../../libatalk/util/server_lock.c.md#check_lockfile), [create_lockfile](../../libatalk/util/server_lock.c.md#create_lockfile), [daemonize](../../libatalk/util/unix.c.md#daemonize), [dir_is_private](netatalk.c.md#dir_is_private), [fault_setup](../../libatalk/util/fault.c.md#fault_setup), [kill_childs](netatalk.c.md#kill_childs), [libev_syserr_cb](netatalk.c.md#libev_syserr_cb), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [netatalk_exit](netatalk.c.md#netatalk_exit), [realpath_safe](../../libatalk/util/unix.c.md#realpath_safe), [run_afpd](netatalk.c.md#run_afpd), [run_process](netatalk.c.md#run_process), [service_running](netatalk.c.md#service_running), [set_sl_volumes](netatalk.c.md#set_sl_volumes), [show_netatalk_paths](netatalk.c.md#show_netatalk_paths), [show_netatalk_version](netatalk.c.md#show_netatalk_version), [sigchld_cb](netatalk.c.md#sigchld_cb), [sighup_cb](netatalk.c.md#sighup_cb), [sigquit_cb](netatalk.c.md#sigquit_cb), [sigterm_cb](netatalk.c.md#sigterm_cb), [timer_cb](netatalk.c.md#timer_cb), [usage](netatalk.c.md#usage), [validate_singleuser_config](netatalk.c.md#validate_singleuser_config), [zeroconf_register](afp_zeroconf.c.md#zeroconf_register)

Uses file-scope variables: `afpd_pid`, `dbus_path`, `dbus_pid`, `lockfile_path`, `loop`, `obj`, `sigchld_ev`, `sighup_ev`, `sigquit_ev`, `sigterm_ev`, `timer_ev`

# Macros

* `KILL_GRACETIME`: how many seconds we wait to shutdown from SIGTERM before we send SIGKILL
* Undocumented: `MYARVSIZE`, `NETATALK_SRV_ERROR`, `NETATALK_SRV_NEEDED`, `NETATALK_SRV_OPTIONAL`

# File-scope variables

`afpd_pid`, `afpd_restarts`, `dbus_path`, `dbus_pid`, `dbus_restarts`, `in_shutdown`, `kill_timer_ev`, `lockfile_path`, `loop`, `obj`, `sigchld_ev`, `sighup_ev`, `sigquit_ev`, `sigterm_ev`, `timer_ev`
