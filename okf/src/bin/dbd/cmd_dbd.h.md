---
type: C Header File
title: "bin/dbd/cmd_dbd.h"
description: "includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/dbd/cmd_dbd.h"
tags: ["bin/dbd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [bin/dbd](../dbd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* System headers: `limits.h`, `signal.h`

# Included by

* [bin/dbd/cmd_dbd.c](cmd_dbd.c.md)
* [bin/dbd/cmd_dbd_scanvol.c](cmd_dbd_scanvol.c.md)

# Typedefs and enums

* `typedef unsigned int dbd_flags_t`
* `enum logtype`: `LOGSTD`, `LOGDEBUG`

# Macros

* `DBD_FLAGS_STRIP_AD`: Invalidate AppleDouble CNID hints.
* `DBD_FLAGS_V2TOEA`: Convert adouble:v2 to adouble:ea.
* Undocumented: `ADv2_DIRNAME`, `DBD_FLAGS_FORCE`, `DBD_FLAGS_SCAN`, `DBD_FLAGS_STATS`, `DBD_FLAGS_VERBOSE`, `DIR_DOT_OR_DOTDOT`
