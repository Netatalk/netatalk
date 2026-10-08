---
type: C Header File
title: "etc/papd/ppd.h"
description: "2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/ppd.h"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `sys/types.h`

# Included by

* [etc/papd/ppd.c](ppd.c.md)
* [etc/papd/queries.c](queries.c.md)

# Types

### struct ppd_feature

Defined at line 16.
* `char * pd_name`
* `char * pd_value`

### struct ppd_font

Defined at line 11.
* `char * pd_font`
* `struct ppd_font * pd_next`
