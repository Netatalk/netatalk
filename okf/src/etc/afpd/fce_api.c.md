---
type: C Source File
title: "etc/afpd/fce_api.c"
description: "File change event API for netatalk."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fce_api.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [desktop.h](desktop.h.md)
* [directory.h](directory.h.md)
* [fce_api_internal.h](fce_api_internal.h.md)
* [file.h](file.h.md)
* [fork.h](fork.h.md)
* [volume.h](volume.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `netdb.h`, `netinet/in.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `time.h`, `unistd.h`

# Functions

### fce_init_udp

```c
void fce_init_udp(void)
```

Defined at lines 114 to 167.

Initialize network structs for any listeners.

Note: We don't give return code because all errors are handled internally (I hope..)

Called by: [send_fce_event](fce_api.c.md#send_fce_event)

Uses file-scope variables: `udp_initialized`, `udp_socket_list`, `udp_sockets`

### fce_cleanup

```c
void fce_cleanup(void)
```

Defined at lines 169 to 186.

Uses file-scope variables: `udp_initialized`, `udp_socket_list`, `udp_sockets`

### build_fce_packet

```c
static ssize_t build_fce_packet(const AFPObj *obj, unsigned char *iobuf, fce_ev_t event, const char *path, const char *oldpath, pid_t pid, const char *user, uint32_t event_id)
```

Defined at lines 191 to 367.

Construct a UDP packet for our listeners and return packet size

Calls: [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [send_fce_event](fce_api.c.md#send_fce_event)

Uses file-scope variables: `fce_ev_info`, `iobuf`

### send_fce_event

```c
static void send_fce_event(const AFPObj *obj, int event, const char *path, const char *oldpath)
```

Defined at lines 373 to 530.

Send the fce information to all (connected) listeners.

Note: We don't give return code because all errors are handled internally (I hope..)

Calls: [afprun_bg](afprun.c.md#afprun_bg), [build_fce_packet](fce_api.c.md#build_fce_packet), [fce_init_udp](fce_api.c.md#fce_init_udp)

Called by: [check_saved_close_events](fce_api.c.md#check_saved_close_events), [fce_register](fce_api.c.md#fce_register), [save_close_event](fce_api.c.md#save_close_event)

Uses file-scope variables: `fce_ev_info`, `fce_event_names`, `iobuf`, `udp_socket_list`, `udp_sockets`

### add_udp_socket

```c
static int add_udp_socket(const char *target_ip, const char *target_port)
```

Defined at lines 532 to 553.

Called by: [fce_add_udp_socket](fce_api.c.md#fce_add_udp_socket)

Uses file-scope variables: `udp_socket_list`, `udp_sockets`

### save_close_event

```c
static void save_close_event(const AFPObj *obj, const char *path)
```

Defined at lines 555 to 569.

Calls: [send_fce_event](fce_api.c.md#send_fce_event)

Called by: [fce_register](fce_api.c.md#fce_register)

Uses file-scope variables: `last_close_event`

### fce_init_ign_paths

```c
static void fce_init_ign_paths(const char *ignores, const char ***dest_array, bool is_directory)
```

Defined at lines 571 to 610.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [fce_register](fce_api.c.md#fce_register)

### fce_register

```c
int fce_register(const AFPObj *obj, fce_ev_t event, const char *path, const char *oldpath)
```

Defined at lines 616 to 698.

Dispatcher for all incoming file change events

Calls: [basename_safe](../../libatalk/util/unix.c.md#basename_safe), [fce_handle_coalescation](fce_util.c.md#fce_handle_coalescation), [fce_init_ign_paths](fce_api.c.md#fce_init_ign_paths), [fce_initialize_history](fce_util.c.md#fce_initialize_history), [realpath_safe](../../libatalk/util/unix.c.md#realpath_safe), [save_close_event](fce_api.c.md#save_close_event), [send_fce_event](fce_api.c.md#send_fce_event)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [afp_logout](auth.c.md#afp_logout), [login](auth.c.md#login), [moveandrename](filedir.c.md#moveandrename), [of_closefork](ofork.c.md#of_closefork)

Uses file-scope variables: `fce_ev_enabled`, `fce_event_names`, `skip_directories`, `skip_files`, `udp_sockets`

### check_saved_close_events

```c
static void check_saved_close_events(const AFPObj *obj)
```

Defined at lines 700 to 714.

Calls: [send_fce_event](fce_api.c.md#send_fce_event)

Called by: [fce_pending_events](fce_api.c.md#fce_pending_events)

Uses file-scope variables: `last_close_event`

### fce_pending_events

```c
void fce_pending_events(const AFPObj *obj)
```

Defined at lines 721 to 728.

API-Calls for file change api, called from outside (file.c [directory.c](directory.c.md) [ofork.c](ofork.c.md) [filedir.c](filedir.c.md))

Calls: [check_saved_close_events](fce_api.c.md#check_saved_close_events)

Called by: [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `udp_sockets`

### fce_add_udp_socket

```c
int fce_add_udp_socket(const char *target)
```

Defined at lines 734 to 747.

Extern connect to afpd parameter.

Note: can be called multiple times for multiple listeners (up to MAX_UDP_SOCKS times)

Calls: [add_udp_socket](fce_api.c.md#add_udp_socket)

Called by: [configinit](afp_config.c.md#configinit)

### fce_set_events

```c
int fce_set_events(const char *events)
```

Defined at lines 749 to 785.

Called by: [configinit](afp_config.c.md#configinit)

Uses file-scope variables: `fce_ev_enabled`

# Macros

* Undocumented: `MAXIOBUF`

# File-scope variables

`fce_ev_enabled`, `fce_ev_info`, `fce_event_names`, `iobuf`, `last_close_event`, `skip_directories`, `skip_files`, `udp_initialized`, `udp_socket_list`, `udp_sockets`
