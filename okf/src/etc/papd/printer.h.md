---
type: C Header File
title: "etc/papd/printer.h"
description: "1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/printer.h"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [etc/papd/lp.c](lp.c.md)
* [etc/papd/main.c](main.c.md)
* [etc/papd/ppd.c](ppd.c.md)
* [etc/papd/print_cups.c](print_cups.c.md)
* [etc/papd/queries.c](queries.c.md)

# Types

### struct printer

Defined at line 6.
* `char * p_name`
* `char * p_type`
* `char * p_zone`
* `char * p_u_name`
* `char * p_ppdfile`
* `char p_status`
* `char * p_authprintdir`
* `int p_flags`
* `struct at_addr p_addr`
* `char * pr_printer`
* `char * pr_operator`
* `char * pr_spool`
* `int pr_pagecost`
* `char * pr_pagecost_msg`
* `char * pr_lock`
* `struct printer pu_pr`
* `char * pu_cmd`
* `union printer p_un`
* `ATP p_atp`
* `char * p_cupsoptions`
* `struct printer * p_next`

# Macros

* Undocumented: `P_ACCOUNT`, `P_AUTH`, `P_AUTH_CAP`, `P_AUTH_PSSP`, `P_CUPS`, `P_CUPS_AUTOADDED`, `P_CUPS_PPD`, `P_FOOMATIC_HACK`, `P_KRB`, `P_PIPED`, `P_REGISTERED`, `P_SPOOLED`, `p_cmd`, `p_lock`, `p_operator`, `p_pagecost`, `p_pagecost_msg`, `p_printer`, `p_spool`
