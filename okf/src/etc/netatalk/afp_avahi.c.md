---
type: C Source File
title: "etc/netatalk/afp_avahi.c"
description: "Avahi based Zeroconf support."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/netatalk/afp_avahi.c"
tags: ["etc/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

### publish_reply

```c
static void publish_reply(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata)
```

Declared at etc/netatalk/afp_avahi.c line 37; no definition in the scanned sources.

Called by: [register_stuff](afp_avahi.c.md#register_stuff)

Uses file-scope variables: `ctx`

### register_stuff

```c
static void register_stuff(void)
```

Defined at lines 45 to 187.

This function tries to register the AFP DNS SRV service type.

Calls: [convert_string](../../libatalk/unicode/charcnv.c.md#convert_string), [getvolumes](../../libatalk/util/netatalk_conf.c.md#getvolumes), [publish_reply](afp_avahi.c.md#publish_reply)

Called by: [client_callback](afp_avahi.c.md#client_callback)

Uses file-scope variables: `ctx`

### client_callback

```c
static void client_callback(AvahiClient *client, AvahiClientState state, void *userdata)
```

Defined at lines 223 to 280.

Calls: [register_stuff](afp_avahi.c.md#register_stuff)

Called by: [av_zeroconf_register](afp_avahi.c.md#av_zeroconf_register)

Uses file-scope variables: `ctx`

### av_zeroconf_register

```c
void av_zeroconf_register(const AFPObj *obj)
```

Defined at lines 290 to 334.

Tries to setup the Zeroconf thread and any neccessary config setting.

Calls: [av_zeroconf_unregister](afp_avahi.c.md#av_zeroconf_unregister), [client_callback](afp_avahi.c.md#client_callback)

Called by: [zeroconf_register](afp_zeroconf.c.md#zeroconf_register)

Uses file-scope variables: `ctx`

### av_zeroconf_unregister

```c
int av_zeroconf_unregister()
```

Defined at lines 340 to 362.

Tries to shutdown this loop impl.

Note: Call this function from inside this thread.

Called by: [av_zeroconf_register](afp_avahi.c.md#av_zeroconf_register), [zeroconf_deregister](afp_zeroconf.c.md#zeroconf_deregister)

Uses file-scope variables: `ctx`

# File-scope variables

`ctx`
