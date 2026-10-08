---
type: C Source File
title: "etc/papd/magics.c"
description: "6 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/magics.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* [comment.h](comment.h.md)
* [file.h](file.h.md)
* [lp.h](lp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Function tables

### magics

Initialized at line 34. Dispatches to: [cm_psadobe](magics.c.md#cm_psadobe), [cm_psquery](magics.c.md#cm_psquery), [cm_psswitch](magics.c.md#cm_psswitch)

# Functions

### sig_handler

```c
static void sig_handler(int signo)
```

Defined at lines 27 to 31.

Called by: [ps](magics.c.md#ps)

Uses file-scope variables: `stop_requested`

### parser_error

```c
static void parser_error(struct papfile *outfile)
```

Defined at lines 33 to 38.

Calls: [lp_close](lp.c.md#lp_close), [spoolerror](file.c.md#spoolerror)

Called by: [ps](magics.c.md#ps)

### ps

```c
int ps(struct papfile *infile, struct papfile *outfile, struct sockaddr_at *sat)
```

Defined at lines 40 to 131.

Calls: [commatch](comment.c.md#commatch), [compush](comment.c.md#compush), [lp_close](lp.c.md#lp_close), [lp_open](lp.c.md#lp_open), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline), [parser_error](magics.c.md#parser_error), [sig_handler](magics.c.md#sig_handler), [spoolerror](file.c.md#spoolerror), [spoolreply](file.c.md#spoolreply)

Called by: [session](session.c.md#session)

Calls through [`papd_comment::c_handler`](comment.h.md#struct-papd_comment): no table assigns this field

Uses file-scope variables: `magics`, `state`, `stop_requested`

### cm_psquery

```c
int cm_psquery(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 133 to 173.

Calls: [commatch](comment.c.md#commatch), [compop](comment.c.md#compop), [compush](comment.c.md#compush), [markline](file.c.md#markline), [spoolreply](file.c.md#spoolreply)

Called by: [cm_psswitch](magics.c.md#cm_psswitch)

Uses file-scope variables: `queries` in [etc/papd/queries.c](queries.c.md)

Dispatched via: [magics](magics.c.md#magics)

### cm_psadobe

```c
int cm_psadobe(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 175 to 209.

Calls: [commatch](comment.c.md#commatch), [compop](comment.c.md#compop), [compush](comment.c.md#compush), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Called by: [cm_psswitch](magics.c.md#cm_psswitch)

Uses file-scope variables: `headers` in [etc/papd/headers.c](headers.c.md)

Dispatched via: [magics](magics.c.md#magics)

### cm_psswitch

```c
int cm_psswitch(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 213 to 266.

Calls: [cm_psadobe](magics.c.md#cm_psadobe), [cm_psquery](magics.c.md#cm_psquery), [compop](comment.c.md#compop), [comswitch](comment.c.md#comswitch), [markline](file.c.md#markline)

Uses file-scope variables: `Query`, `magics`

Dispatched via: [magics](magics.c.md#magics)

# File-scope variables

`Query`, `state`, `stop_requested`
