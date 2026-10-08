---
type: C Source File
title: "sys/netatalk/at_proto.c"
description: "includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/sys/netatalk/at_proto.c"
tags: ["sys/netatalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [sys/netatalk](../netatalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [at.h](at.h.md)
* System headers: `sys/domain.h`, `sys/protosw.h`, `sys/socket.h`, `sys/types.h`

# Function tables

### atalksw

Initialized at line 23. Dispatches to: [ddp_init](ddp_usrreq.c.md#ddp_init), [ddp_output](ddp_output.c.md#ddp_output), [ddp_usrreq](ddp_usrreq.c.md#ddp_usrreq)

# File-scope variables

`atalkdomain`
