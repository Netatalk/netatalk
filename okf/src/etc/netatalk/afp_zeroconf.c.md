---
type: C Source File
title: "etc/netatalk/afp_zeroconf.c"
description: "Zeroconf facade, that abstracts access to a particular Zeroconf implementation."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/netatalk/afp_zeroconf.c"
tags: ["etc/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [etc/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [afp_mdns.h](afp_mdns.h.md)
* [afp_zeroconf.h](afp_zeroconf.h.md)

# Functions

### zeroconf_register

```c
void zeroconf_register(const AFPObj *configs)
```

Defined at lines 31 to 41.

Register the AFP services with mDNSResponder or Avahi, whichever the build has.

The facade prefers mDNSResponder when the build has both. Without either the call does nothing.

Parameters:
* `configs`: AFP object with the options

Calls: [av_zeroconf_register](afp_avahi.c.md#av_zeroconf_register), [md_zeroconf_register](afp_mdns.c.md#md_zeroconf_register)

Called by: [main](netatalk.c.md#main), [sighup_impl](netatalk.c.md#sighup_impl)

### zeroconf_deregister

```c
void zeroconf_deregister(void)
```

Defined at lines 47 to 57.

Withdraw the AFP services from mDNSResponder or Avahi, whichever the build has.

Calls: [av_zeroconf_unregister](afp_avahi.c.md#av_zeroconf_unregister), [md_zeroconf_unregister](afp_mdns.c.md#md_zeroconf_unregister)

Called by: [sighup_impl](netatalk.c.md#sighup_impl)
