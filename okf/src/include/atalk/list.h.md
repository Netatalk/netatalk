---
type: C Header File
title: "include/atalk/list.h"
description: "Simple doubly linked list implementation."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/list.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [etc/afpd/spotlight.c](../../etc/afpd/spotlight.c.md)
* [include/atalk/cnid.h](cnid.h.md)
* [libatalk/cnid/cnid.c](../../libatalk/cnid/cnid.c.md)
* [libatalk/cnid/cnid_init.c](../../libatalk/cnid/cnid_init.c.md)

# Types

### struct list_head

Defined at line 18.
* `struct list_head * next`
* `struct list_head * prev`

# Macros

* `list_entry`: get the struct for this entry
* `list_for_each`: iterate over a list
* `list_for_each_prev`: iterate over a list in reverse order
* Undocumented: `ATALK_INIT_LIST_HEAD`, `ATALK_LIST_HEAD`, `ATALK_LIST_HEAD_INIT`
