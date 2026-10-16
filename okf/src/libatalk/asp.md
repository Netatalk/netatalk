---
type: Subsystem
title: "libatalk/asp"
description: "10 files, 14 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/libatalk/asp"
tags: ["libatalk/asp"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

# Files

* [libatalk/asp/asp_attn.c](asp/asp_attn.c.md): ASP attention requests.
* [libatalk/asp/asp_child.h](asp/asp_child.h.md): Private ASP child session state and status flags.
* [libatalk/asp/asp_close.c](asp/asp_close.c.md): Closing an ASP session in reply to the CloseSession request of the workstation.
* [libatalk/asp/asp_cmdreply.c](asp/asp_cmdreply.c.md): ASP command and write replies.
* [libatalk/asp/asp_getreq.c](asp/asp_getreq.c.md): Reading an ASP request from the session socket.
* [libatalk/asp/asp_getsess.c](asp/asp_getsess.c.md): The ASP session listener: accepting sessions and tickling them.
* [libatalk/asp/asp_init.c](asp/asp_init.c.md): ASP server object creation and status block setup.
* [libatalk/asp/asp_shutdown.c](asp/asp_shutdown.c.md): ASP CloseSession requests to the workstation.
* [libatalk/asp/asp_tickle.c](asp/asp_tickle.c.md): ASP tickles.
* [libatalk/asp/asp_write.c](asp/asp_write.c.md): ASP WriteContinue requests.

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
