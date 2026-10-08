---
type: C Source File
title: "libatalk/nbp/nbp_util.c"
description: "3 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/nbp/nbp_util.c"
tags: ["libatalk/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/nbp](../nbp.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [nbp_conf.h](nbp_conf.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `atalk/ddp.h`, `netdb.h`, `signal.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`

# Functions

### nbp_parse

```c
int nbp_parse(char *, struct nbpnve *, int)
```

Defined at lines 32 to 95. Declared in [libatalk/nbp/nbp_conf.h](nbp_conf.h.md).

Called by: [nbp_do_lookup_op](nbp_lkup.c.md#nbp_do_lookup_op)

### nbp_match

```c
int nbp_match(struct nbpnve *, struct nbpnve *, int)
```

Defined at lines 101 to 144. Declared in [libatalk/nbp/nbp_conf.h](nbp_conf.h.md).

Calls: [strndiacasecmp](../util/strdicasecmp.c.md#strndiacasecmp)

Called by: [nbp_do_lookup_op](nbp_lkup.c.md#nbp_do_lookup_op)

### nbp_name

```c
int nbp_name(const char *, char **, char **, char **)
```

Defined at lines 146 to 174. Declared in [include/atalk/nbp.h](../../include/atalk/nbp.h.md).

Called by: [getprinters](../../etc/papd/main.c.md#getprinters), [main](../../bin/aecho/aecho.c.md#main), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [main](../../bin/pap/pap.c.md#main), [main](../../bin/pap/papstatus.c.md#main)

# Macros

* Undocumented: `NBPM_OBJ`, `NBPM_TYPE`, `NBPM_ZONE`

# File-scope variables

`nbp_id`, `nbp_port`, `nbp_recv`, `nbp_send`
