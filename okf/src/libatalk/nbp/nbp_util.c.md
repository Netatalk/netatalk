---
type: C Source File
title: "libatalk/nbp/nbp_util.c"
description: "3 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/nbp/nbp_util.c"
tags: ["libatalk/nbp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
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

Defined at lines 43 to 106. Declared in [libatalk/nbp/nbp_conf.h](nbp_conf.h.md).

Parse one NBP tuple and its object, type, and zone into an entity.

Each name carries a length byte and has to fit in NBPSTRLEN bytes and in the data left.

Parameters:
* `data`: tuple data
* `nn`: receives the address and the names
* `len`: bytes of data

Returns: the bytes left after the tuple, or -1 for a short or malformed tuple

Called by: [nbp_do_lookup_op](nbp_lkup.c.md#nbp_do_lookup_op)

### nbp_match

```c
int nbp_match(struct nbpnve *, struct nbpnve *, int)
```

Defined at lines 123 to 166. Declared in [libatalk/nbp/nbp_conf.h](nbp_conf.h.md).

Compare two NBP entities without regard to case.

An object or type of `=` in n1 matches anything, unless flags has NBPMATCH_NOGLOB. NBPMATCH_NOZONE leaves the zones out.

Parameters:
* `n1`: entity that may hold wildcards
* `n2`: entity to test
* `flags`: NBPMATCH_NOGLOB, NBPMATCH_NOZONE, or 0

Returns: 1 for a match, or 0

Calls: [strndiacasecmp](../util/strdicasecmp.c.md#strndiacasecmp)

Called by: [nbp_do_lookup_op](nbp_lkup.c.md#nbp_do_lookup_op)

### nbp_name

```c
int nbp_name(const char *, char **, char **, char **)
```

Defined at lines 181 to 209. Declared in [include/atalk/nbp.h](../../include/atalk/nbp.h.md).

Split an NBP name of the form `object:type@zone` into its parts.

The split happens at the last `@` and at the last `:` in a static buffer, which the next call overwrites. A pointer changes only for a part that the name holds, so the caller sets the defaults first.

Parameters:
* `name`: NBP name, or NULL to keep every default
* `objp`: default object, receives the object part
* `typep`: default type, receives the type part
* `zonep`: default zone, receives the zone part

Returns: 0, or -1 for a name over 98 bytes

Called by: [getprinters](../../etc/papd/main.c.md#getprinters), [main](../../bin/aecho/aecho.c.md#main), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [main](../../bin/pap/pap.c.md#main), [main](../../bin/pap/papstatus.c.md#main)

# Macros

* Undocumented: `NBPM_OBJ`, `NBPM_TYPE`, `NBPM_ZONE`

# File-scope variables

`nbp_id`, `nbp_port`, `nbp_recv`, `nbp_send`
