---
type: Subsystem
title: "etc/netatalk"
description: "7 files, 43 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/etc/netatalk"
tags: ["etc/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

# Files

* [etc/netatalk/afp_avahi.c](netatalk/afp_avahi.c.md): Avahi based Zeroconf support.
* [etc/netatalk/afp_avahi.h](netatalk/afp_avahi.h.md): Avahi based Zeroconf support.
* [etc/netatalk/afp_mdns.c](netatalk/afp_mdns.c.md): mDNS based Zeroconf support
* [etc/netatalk/afp_mdns.h](netatalk/afp_mdns.h.md): mDNS based Zeroconf support
* [etc/netatalk/afp_zeroconf.c](netatalk/afp_zeroconf.c.md): Zeroconf facade, that abstracts access to a particular Zeroconf implementation.
* [etc/netatalk/afp_zeroconf.h](netatalk/afp_zeroconf.h.md): Zeroconf facade, that abstracts access to a particular Zeroconf implementation.
* [etc/netatalk/netatalk.c](netatalk/netatalk.c.md): 28 functions, includes 14 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 25 includes

# Calls into

* [libatalk/util](../libatalk/util.md): 12 calls
* [libatalk/compat](../libatalk/compat.md): 7 calls
* [libatalk/unicode](../libatalk/unicode.md): 2 calls
* [include/atalk](../include/atalk.md): 1 calls

# Most called functions

* [kill_childs](netatalk/netatalk.c.md#kill_childs): 4 callers
* [run_process](netatalk/netatalk.c.md#run_process): 3 callers
* [av_zeroconf_unregister](netatalk/afp_avahi.c.md#av_zeroconf_unregister): 2 callers
* [dir_is_private](netatalk/netatalk.c.md#dir_is_private): 2 callers
* [owned_private_dir](netatalk/netatalk.c.md#owned_private_dir): 2 callers
* [path_parent](netatalk/netatalk.c.md#path_parent): 2 callers
* [run_afpd](netatalk/netatalk.c.md#run_afpd): 2 callers
* [service_running](netatalk/netatalk.c.md#service_running): 2 callers
* [unregister_stuff](netatalk/afp_mdns.c.md#unregister_stuff): 2 callers
* [zeroconf_register](netatalk/afp_zeroconf.c.md#zeroconf_register): 2 callers
