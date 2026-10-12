---
type: C Source File
title: "libatalk/util/atalk_addr.c"
description: "Parsing of AppleTalk addresses in text form."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/atalk_addr.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 40 to 140.

Parse the text form of an AppleTalk address.

The net.node form takes a net up to 65535 and a node up to 255. In the three-part form the first two parts hold the net, a byte each. A leading 0x, or a first digit from A to F, makes every part hexadecimal. AppleTalk addresses can carry leading zeros, so the call has no octal form. A failed parse can leave part of the address in addr.

Accepted forms for net 4321 and node 65:

```
4321.65
0x10E1.41
16.225.65
0x10.E1.41
```

Parameters:
* `cp`: address text
* `addr`: receives the address with the net in network order, or NULL to check only the syntax, which skips the range checks

Returns: 1 for a valid address, 0 otherwise

Called by: [addr](../../etc/atalkd/config.c.md#addr), [afp_config_free](netatalk_conf.c.md#afp_config_free), [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [getprinters](../../etc/papd/main.c.md#getprinters), [main](../../bin/aecho/aecho.c.md#main), [main](../../bin/getzones/getzones.c.md#main), [main](../../bin/nbp/nbplkup.c.md#main), [main](../../bin/nbp/nbprgstr.c.md#main), [main](../../bin/nbp/nbpunrgstr.c.md#main), [main](../../bin/pap/pap.c.md#main), [main](../../bin/pap/papstatus.c.md#main), [main](../../bin/rtmpqry/rtmpqry.c.md#main)
