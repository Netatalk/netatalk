---
type: C Source File
title: "etc/afpd/fce_api.c"
description: "File change event API for netatalk."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fce_api.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-19T23:19:03+11:00 }
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

Defined at lines 105 to 153.

Initialize network structs for any listeners.

Note: We don't give return code because all errors are handled internally (I hope..)

Called by: [send_fce_event](fce_api.c.md#send_fce_event)

Uses file-scope variables: `udp_initialized`, `udp_socket_list`, `udp_sockets`

### fce_cleanup

```c
void fce_cleanup(void)
```

Defined at lines 161 to 176.

Drop every FCE listener and its socket.

[configinit()](afp_config.c.md#configinit) calls it before it reads 'fce listener', at startup and at each reload, so the table holds only the listener afp.conf names.

Called by: [configinit](afp_config.c.md#configinit)

Uses file-scope variables: `udp_initialized`, `udp_socket_list`, `udp_sockets`

### build_fce_packet

```c
static ssize_t build_fce_packet(const AFPObj *obj, unsigned char *iobuf, fce_ev_t event, const char *path, const char *oldpath, pid_t pid, const char *user, uint32_t event_id)
```

Defined at lines 197 to 373.

Build the UDP packet of one FCE event in a buffer.

The packet starts with the 8-byte magic and the protocol version. Version 2 adds an info flags byte, a filler byte, and 8 reserved bytes around the event code. The 4-byte event ID follows, then the pid and the user name when the flags ask for them. The path and the source path of a move end the packet, each with a 2-byte length. All numbers go in network order.

Parameters:
* `obj`: session object with the FCE protocol version
* `iobuf`: buffer of MAXIOBUF bytes
* `event`: event code
* `path`: path of the event, capped at MAXPATHLEN - 1 bytes
* `oldpath`: source path of a move, or NULL
* `pid`: session pid
* `user`: login name
* `event_id`: event counter

Returns: the packet length, or -1 when the packet does not fit

Calls: [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [send_fce_event](fce_api.c.md#send_fce_event)

Uses file-scope variables: `fce_ev_info`, `iobuf`

### send_fce_event

```c
static void send_fce_event(const AFPObj *obj, int event, const char *path, const char *oldpath)
```

Defined at lines 391 to 557.

Send one FCE event to every listener and to the notify script.

The first event records the user, opens the sockets, and sends FCE_CONN_START. The notify script runs in the background with the event as quoted options. The fce sendwait option delays every event by its value, from 1 to 999 ms. A failed socket reopens after FCE_SOCKET_RETRY_DELAY_S seconds and first sends FCE_CONN_BROKEN. A short send closes the socket until then. The event ID rises by 1 per event, and 0xFFFFFFFF rolls over to 0.

Parameters:
* `obj`: session object
* `event`: event code
* `path`: path of the event
* `oldpath`: source path of a move, or NULL

Calls: [afprun_bg](afprun.c.md#afprun_bg), [build_fce_packet](fce_api.c.md#build_fce_packet), [fce_init_udp](fce_api.c.md#fce_init_udp), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [check_saved_close_events](fce_api.c.md#check_saved_close_events), [fce_register](fce_api.c.md#fce_register), [save_close_event](fce_api.c.md#save_close_event)

Uses file-scope variables: `fce_ev_info`, `fce_event_names`, `iobuf`, `udp_socket_list`, `udp_sockets`

### add_udp_socket

```c
static int add_udp_socket(const char *target_ip, const char *target_port)
```

Defined at lines 567 to 588.

Add an FCE listener to the table.

Parameters:
* `target_ip`: host name or address
* `target_port`: port, or NULL for FCE_DEFAULT_PORT_STRING

Returns: AFP_OK, or AFPERR_PARAM when the table holds FCE_MAX_UDP_SOCKS listeners

Called by: [fce_add_udp_socket](fce_api.c.md#fce_add_udp_socket)

Uses file-scope variables: `udp_socket_list`, `udp_sockets`

### save_close_event

```c
static void save_close_event(const AFPObj *obj, const char *path)
```

Defined at lines 599 to 613.

Hold back a file modify event for the fce fmodwait delay.

A held event for another path goes out first. A repeat for the same path only restarts the delay.

Parameters:
* `obj`: session object
* `path`: path of the modified file

Calls: [send_fce_event](fce_api.c.md#send_fce_event)

Called by: [fce_register](fce_api.c.md#fce_register)

Uses file-scope variables: `last_close_event`

### fce_resolved

```c
static const char * fce_resolved(const char *path, char *buf)
```

Defined at lines 627 to 654.

Resolve a path for the ignore-directory compare.

A path resolves while it exists. A path that is gone, or not there yet, resolves through its parent with its own name appended, so a deleted file compares under its directory's resolved name. A path that resolves neither way compares as it is.

Parameters:
* `path`: absolute path
* `buf`: MAXPATHLEN + 1 bytes for the resolved path

Returns: buf, or path

Calls: [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat)

Called by: [fce_init_ign_paths](fce_api.c.md#fce_init_ign_paths), [fce_register](fce_api.c.md#fce_register)

### fce_init_ign_paths

```c
static void fce_init_ign_paths(const char *ignores, const char ***dest_array, bool is_directory)
```

Defined at lines 668 to 713.

Split a comma-separated ignore option into a NULL-terminated array.

A directory is stored resolved, through its parent when it does not exist yet, and without trailing slashes unless it is "/". A failed array allocation, or an option without entries, leaves it NULL, which ignores nothing; a failed entry copy drops that entry.

Parameters:
* `ignores`: fce ignore names or fce ignore directories option
* `dest_array`: receives the allocated array, or NULL
* `is_directory`: true for directory paths

Calls: [fce_resolved](fce_api.c.md#fce_resolved), [stripped_slashes_basename](../../libatalk/util/unix.c.md#stripped_slashes_basename)

Called by: [fce_register](fce_api.c.md#fce_register)

### fce_register

```c
int fce_register(const AFPObj *obj, fce_ev_t event, const char *path, const char *oldpath)
```

Defined at lines 732 to 816.

Send a file change event unless the options filter it out.

An event passes when the fce events option lists it and a listener or a notify script exists. A base name in fce ignore names drops out, and so does a path at or below an fce ignore directories entry, compared resolved, through its parent once it is gone. Events that fce coalesce merges drop out too. With fce fmodwait set, an FCE_FILE_MODIFY event waits fce fmodwait seconds, and repeats for the same file merge.

Parameters:
* `obj`: session object
* `event`: event code
* `path`: path of the changed object
* `oldpath`: source path of a move, or NULL

Returns: AFP_OK

Calls: [basename_safe](../../libatalk/util/unix.c.md#basename_safe), [fce_handle_coalescation](fce_util.c.md#fce_handle_coalescation), [fce_init_ign_paths](fce_api.c.md#fce_init_ign_paths), [fce_initialize_history](fce_util.c.md#fce_initialize_history), [fce_resolved](fce_api.c.md#fce_resolved), [path_at_or_below](file.c.md#path_at_or_below), [save_close_event](fce_api.c.md#save_close_event), [send_fce_event](fce_api.c.md#send_fce_event)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [afp_createdir](directory.c.md#afp_createdir), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [afp_logout](auth.c.md#afp_logout), [login](auth.c.md#login), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [moveandrename](filedir.c.md#moveandrename), [of_closefork](ofork.c.md#of_closefork)

Uses file-scope variables: `fce_ev_enabled`, `fce_event_names`, `skip_directories`, `skip_files`, `udp_sockets`

### check_saved_close_events

```c
static void check_saved_close_events(const AFPObj *obj)
```

Defined at lines 823 to 835.

Send the held file modify event once the fce fmodwait delay passed.

Parameters:
* `obj`: session object

Calls: [send_fce_event](fce_api.c.md#send_fce_event)

Called by: [fce_pending_events](fce_api.c.md#fce_pending_events)

Uses file-scope variables: `last_close_event`

### fce_pending_events

```c
void fce_pending_events(const AFPObj *obj)
```

Defined at lines 847 to 854.

Send a held file modify event whose delay passed.

The session loop calls it after each request. Without a listener or a notify script it does nothing.

Parameters:
* `obj`: session object

Calls: [check_saved_close_events](fce_api.c.md#check_saved_close_events)

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp), [afp_over_dsi](afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `udp_sockets`

### fce_add_udp_socket

```c
int fce_add_udp_socket(const char *target)
```

Defined at lines 866 to 879.

Add an FCE listener from the fce listener option.

The target is a host with an optional port after the first colon, which IPv6 addresses also contain. The port defaults to FCE_DEFAULT_PORT_STRING.

Parameters:
* `target`: host or host:port, at most 255 bytes

Returns: AFP_OK, or AFPERR_PARAM when the table holds FCE_MAX_UDP_SOCKS listeners

Calls: [add_udp_socket](fce_api.c.md#add_udp_socket)

Called by: [configinit](afp_config.c.md#configinit)

### fce_set_events

```c
int fce_set_events(const char *events)
```

Defined at lines 891 to 930.

Set the FCE event types to send from the fce events option.

The option lists fmod, fdel, ddel, fcre, dcre, fmov, dmov, login, and logout, separated by commas or spaces. The list replaces the default set, and unknown names drop out.

Parameters:
* `events`: option value

Returns: AFP_OK, AFPERR_PARAM for NULL, or AFPERR_MISC when the copy fails

Called by: [configinit](afp_config.c.md#configinit)

Uses file-scope variables: `fce_ev_enabled`

# Macros

* Undocumented: `MAXIOBUF`

# File-scope variables

`fce_ev_enabled`, `fce_ev_info`, `fce_event_names`, `iobuf`, `last_close_event`, `skip_directories`, `skip_files`, `udp_initialized`, `udp_socket_list`, `udp_sockets`
