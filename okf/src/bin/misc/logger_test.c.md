---
type: C Source File
title: "bin/misc/logger_test.c"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/misc/logger_test.c"
tags: ["bin/misc"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [bin/misc](../misc.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* System headers: `stdbool.h`, `stdio.h`

# Functions

### main

```c
int main(int argc, char *argv[])
```

Defined at lines 39 to 138.

Write test messages through the netatalk logger to syslog and to test.log.

Each message names its level and its type and states whether it should appear, so a reader checks the output by eye. The first part logs to syslog under the default:severe and the default:info settings. The second part logs to test.log in the current directory under default:severe, dsi:maxdebug, and default:debug. The last part logs runs of repeated messages.

Parameters:
* `argc`: argument count, unused
* `argv`: argument vector, unused

Returns: 0

Calls: [set_processname](../../libatalk/util/logger.c.md#set_processname), [setuplog](../../libatalk/util/logger.c.md#setuplog)
