---
type: C Header File
title: "sys/netatalk/ddp.h"
description: "ELAP and DDP packet header types, field aliases and size constants."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp.h"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [endian.h](endian.h.md)

# Included by

* [libatalk/nbp/nbp_lkup.c](../../libatalk/nbp/nbp_lkup.c.md)
* [sys/netatalk/ddp_input.c](ddp_input.c.md)
* [sys/netatalk/ddp_output.c](ddp_output.c.md)

# Types

### struct ddpehdr

Extended DDP header.

Defined at line 31.
* `unsigned dub_pad`
* `unsigned dub_hops`
* `unsigned dub_len`
* `unsigned dub_sum`
* `struct ddpehdr du_bits`
* `unsigned du_bytes`
* `union ddpehdr deh_u`
* `unsigned short deh_dnet`
* `unsigned short deh_snet`
* `unsigned char deh_dnode`
* `unsigned char deh_snode`
* `unsigned char deh_dport`
* `unsigned char deh_sport`

### struct ddpshdr

Defined at line 69.
* `unsigned dub_pad`
* `unsigned dub_len`
* `unsigned dub_dport`
* `unsigned dub_sport`
* `struct ddpshdr du_bits`
* `unsigned du_bytes`
* `union ddpshdr dsh_u`

### struct elaphdr

Defined at line 16.
* `unsigned char el_dnode`
* `unsigned char el_snode`
* `unsigned char el_type`

# Macros

* Undocumented: `DDP_MAXHOPS`, `ELAP_DDPEXTEND`, `ELAP_DDPSHORT`, `SZ_DDPEHDR`, `SZ_DDPSHDR`, `SZ_ELAPHDR`, `deh_bytes`, `deh_hops`, `deh_len`, `deh_pad`, `deh_sum`, `dsh_bytes`, `dsh_dport`, `dsh_len`, `dsh_pad`, `dsh_sport`
