---
type: C Header File
title: "sys/netatalk/aarp.h"
description: "4 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/aarp.h"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [sys/netatalk/aarp.c](aarp.c.md)
* [sys/netatalk/at_control.c](at_control.c.md)

# Types

### union aapa

Defined at line 15.
* `unsigned char ap_pa`
* `struct aapa::ap_node ap_node`

### struct aapa::ap_node

Defined at line 17.
* `unsigned char an_zero`
* `unsigned char an_net`
* `unsigned char an_node`

### struct aarptab

Defined at line 43.
* `struct at_addr aat_ataddr`
* `unsigned char aat_enaddr`
* `unsigned char aat_timer`
* `unsigned char aat_flags`
* `struct mbuf * aat_hold`

### struct ether_aarp

Defined at line 24.
* `struct arphdr eaa_hdr`
* `unsigned char aarp_sha`
* `union aapa aarp_spu`
* `unsigned char aarp_tha`
* `union aapa aarp_tpu`

# Macros

* Undocumented: `AARPHRD_ETHER`, `AARPOP_PROBE`, `AARPOP_REQUEST`, `AARPOP_RESPONSE`, `aarp_hln`, `aarp_hrd`, `aarp_op`, `aarp_pln`, `aarp_pro`, `aarp_spa`, `aarp_spnet`, `aarp_spnode`, `aarp_tpa`, `aarp_tpnet`, `aarp_tpnode`
