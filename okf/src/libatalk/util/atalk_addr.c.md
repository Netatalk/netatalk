---
type: C Source File
title: "libatalk/util/atalk_addr.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/atalk_addr.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `arpa/inet.h`, `ctype.h`, `sys/types.h`

# Functions

### atalk_aton

```c
int atalk_aton(char *cp, struct at_addr *addr)
```

Defined at lines 32 to 132.

Check whether "cp" is a valid ascii representation of an AppleTalk address and convert to a binary address.

Examples of accepted forms are (in decimal, net of 4321, node of 65):

```
4321.65
0x10E1.41
16.225.65
0x10.E1.41
```

If hex is used, and the first digit is one of A-F, the leading 0x is redundant. Returns 1 if the address is valid, 0 if not.

Unlike Internet addresses, AppleTalk addresses can have leading 0's. This means that we can't support octal addressing.

Called by: [addr](../../etc/atalkd/config.c.md#addr), [afp_config_free](netatalk_conf.c.md#afp_config_free), [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [getprinters](../../etc/papd/main.c.md#getprinters), [main](../../bin/aecho/aecho.c.md#main), [main](../../bin/getzones/getzones.c.md#main), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [main](../../bin/pap/pap.c.md#main), [main](../../bin/pap/papstatus.c.md#main), [main](../../bin/rtmpqry/rtmpqry.c.md#main)
