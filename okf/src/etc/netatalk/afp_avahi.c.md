---
type: C Source File
title: "etc/netatalk/afp_avahi.c"
description: "Avahi based Zeroconf support."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/netatalk/afp_avahi.c"
tags: ["etc/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-27T17:55:55+01:00 }
---

Part of the [etc/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afp_avahi.h](afp_avahi.h.md)
* [afp_zeroconf.h](afp_zeroconf.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `avahi-common/strlst.h`, `time.h`, `unistd.h`

# Functions

### register_stuff

```c
static void register_stuff(void)
```

Defined at lines 59 to 201.

Register the AFP service, the Time Machine volumes, and the device model with Avahi.

The call fills an empty entry group only. The AFP service goes out under the server name, or the host name without one. Time Machine volumes go into the TXT records of an adisk service on the discard port, each with its volume UUID when it has one. The mimic model adds a device-info service. A failure logs and leaves the group as it is.

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [publish_reply](afp_avahi.c.md#publish_reply)

Called by: [client_callback](afp_avahi.c.md#client_callback)

Uses file-scope variables: `ctx`

### publish_reply

```c
static void publish_reply(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata)
```

Defined at lines 211 to 242.

Log the state of the entry group, and stop the Avahi thread on a name collision or a failure.

Parameters:
* `g`: entry group
* `state`: new state of the group
* `userdata`: unused

Called by: [register_stuff](afp_avahi.c.md#register_stuff)

Uses file-scope variables: `ctx`

### client_callback

```c
static void client_callback(AvahiClient *client, AvahiClientState state, void *userdata)
```

Defined at lines 256 to 313.

Track the state of the Avahi client, and register the services once the daemon runs.

A host name collision resets the entry group. A disconnect from the daemon frees the client and connects again. Any other failure stops the Avahi thread.

Parameters:
* `client`: Avahi client
* `state`: new state of the client
* `userdata`: unused

Calls: [register_stuff](afp_avahi.c.md#register_stuff)

Called by: [av_zeroconf_register](afp_avahi.c.md#av_zeroconf_register)

Uses file-scope variables: `ctx`

### av_zeroconf_register

```c
void av_zeroconf_register(const AFPObj *obj)
```

Defined at lines 327 to 371.

Start the Avahi thread, which registers the AFP services with Zeroconf.

A second call resets the entry group and starts a new thread and client. A failure tears the setup down through av_zeroconf_unregister.

Parameters:
* `obj`: AFP object with the options

Calls: [av_zeroconf_unregister](afp_avahi.c.md#av_zeroconf_unregister), [client_callback](afp_avahi.c.md#client_callback)

Called by: [zeroconf_register](afp_zeroconf.c.md#zeroconf_register)

Uses file-scope variables: `ctx`

### av_zeroconf_unregister

```c
int av_zeroconf_unregister()
```

Defined at lines 381 to 403.

Stop the Avahi thread and free the Zeroconf state.

The call stops the thread with avahi_threaded_poll_stop, so the caller has to run outside the Avahi thread.

Returns: 0

Called by: [av_zeroconf_register](afp_avahi.c.md#av_zeroconf_register), [zeroconf_deregister](afp_zeroconf.c.md#zeroconf_deregister)

Uses file-scope variables: `ctx`

# File-scope variables

`ctx`
