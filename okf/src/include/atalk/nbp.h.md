---
type: C Header File
title: "include/atalk/nbp.h"
description: "3 types, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/nbp.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [netatalk/endian.h](../../sys/netatalk/endian.h.md)
* System headers: `sys/types.h`

# Included by

* [bin/aecho/aecho.c](../../bin/aecho/aecho.c.md)
* [bin/nbp/nbplkup.c](../../bin/nbp/nbplkup.c.md)
* [bin/nbp/nbprgstr.c](../../bin/nbp/nbprgstr.c.md)
* [bin/nbp/nbpunrgstr.c](../../bin/nbp/nbpunrgstr.c.md)
* [bin/pap/pap.c](../../bin/pap/pap.c.md)
* [bin/pap/papstatus.c](../../bin/pap/papstatus.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/atalkd/main.c](../../etc/atalkd/main.c.md)
* [etc/atalkd/nbp.c](../../etc/atalkd/nbp.c.md)
* [etc/papd/main.c](../../etc/papd/main.c.md)
* [libatalk/nbp/nbp_conf.h](../../libatalk/nbp/nbp_conf.h.md)
* [libatalk/nbp/nbp_lkup.c](../../libatalk/nbp/nbp_lkup.c.md)
* [libatalk/nbp/nbp_rgstr.c](../../libatalk/nbp/nbp_rgstr.c.md)
* [libatalk/nbp/nbp_unrgstr.c](../../libatalk/nbp/nbp_unrgstr.c.md)
* [libatalk/nbp/nbp_util.c](../../libatalk/nbp/nbp_util.c.md)

# Types

### struct nbphdr

Defined at line 38.
* `uint32_t nh_op`
* `uint32_t nh_cnt`
* `uint32_t nh_id`

### struct nbpnve

Defined at line 67.
* `struct sockaddr_at nn_sat`
* `uint8_t nn_objlen`
* `char nn_obj`
* `uint8_t nn_typelen`
* `char nn_type`
* `uint8_t nn_zonelen`
* `char nn_zone`

### struct nbptuple

Defined at line 54.
* `uint16_t nt_net`
* `uint8_t nt_node`
* `uint8_t nt_port`
* `uint8_t nt_enum`

# Macros

* Undocumented: `ATP_OPEN_2ARGS`, `NBPMATCH_NOGLOB`, `NBPMATCH_NOZONE`, `NBPOP_BRRQ`, `NBPOP_CLOSE_NOTE`, `NBPOP_CONFIRM`, `NBPOP_ERROR`, `NBPOP_FWD`, `NBPOP_LKUP`, `NBPOP_LKUPREPLY`, `NBPOP_OK`, `NBPOP_RGSTR`, `NBPOP_UNRGSTR`, `NBPSTRLEN`, `NBP_UNRGSTR_4ARGS`, `SZ_NBPHDR`, `SZ_NBPTUPLE`
