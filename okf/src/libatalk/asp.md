---
type: Subsystem
title: "libatalk/asp"
description: "10 files, 14 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/asp"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

# Files

* [libatalk/asp/asp_attn.c](asp/asp_attn.c.md): 1 function, includes 4 project headers.
* [libatalk/asp/asp_child.h](asp/asp_child.h.md): 1 type.
* [libatalk/asp/asp_close.c](asp/asp_close.c.md): 1 function, includes 3 project headers.
* [libatalk/asp/asp_cmdreply.c](asp/asp_cmdreply.c.md): 1 function, includes 2 project headers.
* [libatalk/asp/asp_getreq.c](asp/asp_getreq.c.md): 1 function, includes 3 project headers.
* [libatalk/asp/asp_getsess.c](asp/asp_getsess.c.md): 5 functions, includes 9 project headers.
* [libatalk/asp/asp_init.c](asp/asp_init.c.md): 2 functions, includes 3 project headers.
* [libatalk/asp/asp_shutdown.c](asp/asp_shutdown.c.md): 1 function, includes 3 project headers.
* [libatalk/asp/asp_tickle.c](asp/asp_tickle.c.md): 1 function, includes 3 project headers.
* [libatalk/asp/asp_write.c](asp/asp_write.c.md): 1 function, includes 3 project headers.

# Includes headers from

* [include/atalk](../include/atalk.md): 26 includes
* [sys/netatalk](../sys/netatalk.md): 6 includes

# Calls into

* [libatalk/atp](atp.md): 12 calls
* [libatalk/util](util.md): 1 calls

# Called from

* [etc/afpd](../etc/afpd.md): 17 calls

# Most called functions

* [asp_attention](asp/asp_attn.c.md#asp_attention): 4 callers
* [asp_close](asp/asp_close.c.md#asp_close): 2 callers
* [asp_kill](asp/asp_getsess.c.md#asp_kill): 2 callers
* [asp_wrtcont](asp/asp_write.c.md#asp_wrtcont): 2 callers
* [asp_cmdreply](asp/asp_cmdreply.c.md#asp_cmdreply): 1 callers
* [asp_getrequest](asp/asp_getreq.c.md#asp_getrequest): 1 callers
* [asp_getsession](asp/asp_getsess.c.md#asp_getsession): 1 callers
* [asp_init](asp/asp_init.c.md#asp_init): 1 callers
* [asp_setstatus](asp/asp_init.c.md#asp_setstatus): 1 callers
* [asp_shutdown](asp/asp_shutdown.c.md#asp_shutdown): 1 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[set_asp_ac](asp/asp_getsess.c.md#set_asp_ac)
