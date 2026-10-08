---
type: C Header File
title: "sys/netatalk/phase2.h"
description: "1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/phase2.h"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `net/if_ieee802.h`

# Included by

* [sys/netatalk/aarp.c](aarp.c.md)
* [sys/netatalk/at_control.c](at_control.c.md)

# Types

### struct llc

Defined at line 30.
* `unsigned char llc_dsap`
* `unsigned char llc_ssap`
* `unsigned char control`
* `unsigned char format_id`
* `unsigned char class`
* `unsigned char window_x2`
* `struct llc type_u`
* `unsigned char num_snd_x2`
* `unsigned char num_rcv_x2`
* `struct llc type_i`
* `struct llc type_s`
* `unsigned char org_code`
* `unsigned short ether_type`
* `struct llc type_snap`
* `union llc llc_un`

# Macros

* Undocumented: `LLC_ISO_LSAP`, `LLC_SNAP_LSAP`, `LLC_TEST`, `LLC_TEST_P`, `LLC_UI`, `LLC_UI_P`, `LLC_XID`, `LLC_XID_P`, `SIOCPHASE1`, `SIOCPHASE2`, `llc_class`, `llc_control`, `llc_ether_type`, `llc_fid`, `llc_org_code`, `llc_window`
