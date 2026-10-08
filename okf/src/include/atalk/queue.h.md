---
type: C Header File
title: "include/atalk/queue.h"
description: "1 type."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/queue.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [bin/nad/nad_cp.c](../../bin/nad/nad_cp.c.md)
* [bin/nad/nad_mv.c](../../bin/nad/nad_mv.c.md)
* [bin/nad/nad_rm.c](../../bin/nad/nad_rm.c.md)
* [etc/afpd/dircache.c](../../etc/afpd/dircache.c.md)
* [etc/afpd/idle_worker.c](../../etc/afpd/idle_worker.c.md)
* [include/atalk/directory.h](directory.h.md)
* [libatalk/util/queue.c](../../libatalk/util/queue.c.md)

# Types

### struct qnode

Defined at line 26.
* `struct qnode * prev`
* `struct qnode * next`
* `void * data`

# Typedefs and enums

* `typedef qnode_t q_t`
* `typedef struct qnode qnode_t`

# Macros

* Undocumented: `queue_free`
