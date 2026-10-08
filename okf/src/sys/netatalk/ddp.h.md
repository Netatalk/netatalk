---
type: C Header File
title: "sys/netatalk/ddp.h"
description: "3 types, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/ddp.h"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at line 63.
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

Defined at line 101.
* `unsigned dub_pad`
* `unsigned dub_len`
* `unsigned dub_dport`
* `unsigned dub_sport`
* `struct ddpshdr du_bits`
* `unsigned du_bytes`
* `union ddpshdr dsh_u`

### struct elaphdr

Defined at line 48.
* `unsigned char el_dnode`
* `unsigned char el_snode`
* `unsigned char el_type`

# Macros

* Undocumented: `DDP_MAXHOPS`, `ELAP_DDPEXTEND`, `ELAP_DDPSHORT`, `SZ_DDPEHDR`, `SZ_DDPSHDR`, `SZ_ELAPHDR`, `deh_bytes`, `deh_hops`, `deh_len`, `deh_pad`, `deh_sum`, `dsh_bytes`, `dsh_dport`, `dsh_len`, `dsh_pad`, `dsh_sport`
