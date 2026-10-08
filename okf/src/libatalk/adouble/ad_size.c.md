---
type: C Source File
title: "libatalk/adouble/ad_size.c"
description: "1 function, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_size.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `string.h`

# Functions

### ad_size

```c
off_t ad_size(const struct adouble *ad, const uint32_t eid)
```

Defined at lines 17 to 37.

Called by: [ad_readfile_init](ad_sendfile.c.md#ad_readfile_init), [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams), [byte_lock](../../etc/afpd/fork.c.md#byte_lock), [nad_header_read](../../bin/nad/nad_adouble.c.md#nad_header_read), [read_fork](../../etc/afpd/fork.c.md#read_fork), [write_fork](../../etc/afpd/fork.c.md#write_fork)
