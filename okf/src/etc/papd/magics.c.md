---
type: C Source File
title: "etc/papd/magics.c"
description: "PostScript job type detection and the papd job loop."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/magics.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
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

Initialized at line 39. Dispatches to: [cm_psadobe](magics.c.md#cm_psadobe), [cm_psquery](magics.c.md#cm_psquery), [cm_psswitch](magics.c.md#cm_psswitch)

# Functions

### sig_handler

```c
static void sig_handler(int signo)
```

Defined at lines 38 to 42.

Note SIGINT or SIGTERM, so the job loop ends the job with a shutdown message.

Parameters:
* `signo`: signal number, unused

Called by: [ps](magics.c.md#ps)

### parser_error

```c
static void parser_error(struct papfile *outfile)
```

Defined at lines 52 to 57.

Refuse a job whose DSC comments do not parse.

The client gets a "Comments error, Ignoring job." error, the output ends, and the spool file closes.

Parameters:
* `outfile`: reply buffer to the client

Calls: [lp_close](lp.c.md#lp_close), [spoolerror](file.c.md#spoolerror)

Called by: [ps](magics.c.md#ps)

### ps

```c
int ps(struct papfile *infile, struct papfile *outfile, struct sockaddr_at *sat)
```

Defined at lines 77 to 168.

Run the PostScript job loop over the data a PAP client sent.

The first call routes SIGINT and SIGTERM to sig_handler, after which the loop answers "Server shutting down." and ends the job. At the start of a job the loop drops lines until a `%!PS-Adobe-` magic line pushes its handler. After that, the handler at the top of the comment stack takes each line, and lines outside any comment go to lp_write.

PF_STW opens the spool file, and the client gets "Ignoring job." once when lp_open fails.

Parameters:
* `infile`: data from the client
* `outfile`: reply data for the client
* `sat`: client address

Returns: CH_MORE when a handler needs more data, CH_ERROR on shutdown, or 0 otherwise

Calls: [commatch](comment.c.md#commatch), [compush](comment.c.md#compush), [lp_close](lp.c.md#lp_close), [lp_open](lp.c.md#lp_open), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline), [parser_error](magics.c.md#parser_error), [sig_handler](magics.c.md#sig_handler), [spoolerror](file.c.md#spoolerror), [spoolreply](file.c.md#spoolreply)

Called by: [session](session.c.md#session)

Calls through [`papd_comment::c_handler`](comment.h.md#struct-papd_comment): no table assigns this field

Uses file-scope variables: `magics`, `state`

### cm_psquery

```c
int cm_psquery(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 182 to 222.

Serve a query job that starts with `%!PS-Adobe-3.0 Query`.

The call skips the magic line, pushes the handler of each query comment it finds, and drops other lines. The end of the input, or PF_QUERY at the end of a query, pops the state.

Parameters:
* `in`: data from the client
* `out`: reply data, receives a status while a line is incomplete
* `sat`: client address, unused

Returns: CH_DONE, or CH_MORE while a line is incomplete

Calls: [commatch](comment.c.md#commatch), [compop](comment.c.md#compop), [compush](comment.c.md#compush), [markline](file.c.md#markline), [spoolreply](file.c.md#spoolreply)

Called by: [cm_psswitch](magics.c.md#cm_psswitch)

Uses file-scope variables: `queries` in [etc/papd/queries.c](queries.c.md)

Dispatched via: [magics](magics.c.md#magics)

### cm_psadobe

```c
int cm_psadobe(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 236 to 270.

Serve a print job that starts with `%!PS-Adobe-3.0`.

The call writes the magic line and every other line to the job, and pushes the handler of each header comment it finds. The end of the input pops the state.

Parameters:
* `in`: data from the client
* `out`: reply data, receives PF_EOF at the end of the input
* `sat`: client address, unused

Returns: CH_DONE, or CH_MORE while a line is incomplete

Calls: [commatch](comment.c.md#commatch), [compop](comment.c.md#compop), [compush](comment.c.md#compush), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Called by: [cm_psswitch](magics.c.md#cm_psswitch)

Uses file-scope variables: `headers` in [etc/papd/headers.c](headers.c.md)

Dispatched via: [magics](magics.c.md#magics)

### cm_psswitch

```c
int cm_psswitch(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 287 to 340.

Pick the handler for a job with another `%!PS-Adobe-` version.

When the word after the version starts with Query, the job becomes a query job, and any other job goes to cm_psadobe. The line stays in the input for the new handler, and a missing handler in the magics table exits papd.

Parameters:
* `in`: data from the client
* `out`: reply data, receives PF_EOF at the end of the input
* `sat`: client address, unused

Returns: CH_DONE, also at the end of the input, or CH_MORE while the line is incomplete

Calls: [cm_psadobe](magics.c.md#cm_psadobe), [cm_psquery](magics.c.md#cm_psquery), [compop](comment.c.md#compop), [comswitch](comment.c.md#comswitch), [markline](file.c.md#markline)

Uses file-scope variables: `Query`, `magics`

Dispatched via: [magics](magics.c.md#magics)

# File-scope variables

`Query`, `state`, `stop_requested`
