---
type: C Header File
title: "etc/netatalk/afp_avahi.h"
description: "Avahi based Zeroconf support."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/netatalk/afp_avahi.h"
tags: ["etc/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `assert.h`, `avahi-client/client.h`, `avahi-client/publish.h`, `avahi-common/alternative.h`, `avahi-common/error.h`, `avahi-common/malloc.h`, `avahi-common/thread-watch.h`, `stdlib.h`, `string.h`

# Included by

* [etc/netatalk/afp_avahi.c](afp_avahi.c.md)

# Types

### struct context

Defined at line 24.
* `int thread_running`
* `AvahiThreadedPoll * threaded_poll`
* `AvahiClient * client`
* `AvahiEntryGroup * group`
* `const AFPObj * obj`
