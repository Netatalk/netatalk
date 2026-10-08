---
type: C Source File
title: "etc/papd/headers.c"
description: "9 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/headers.c"
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
* System headers: `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Function tables

### headers

Initialized at line 36. Dispatches to: [ch_creator](headers.c.md#ch_creator), [ch_endcomm](headers.c.md#ch_endcomm), [ch_endtranslate](headers.c.md#ch_endtranslate), [ch_for](headers.c.md#ch_for), [ch_starttranslate](headers.c.md#ch_starttranslate), [ch_title](headers.c.md#ch_title), [ch_translateone](headers.c.md#ch_translateone)

# Functions

### get_text

```c
static char * get_text(char *start, int linelength)
```

Defined at lines 25 to 84.

Called by: [ch_creator](headers.c.md#ch_creator), [ch_for](headers.c.md#ch_for), [ch_title](headers.c.md#ch_title)

### ch_for

```c
int ch_for(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Defined at lines 86 to 116.

Calls: [compop](comment.c.md#compop), [get_text](headers.c.md#get_text), [lp_for](lp.c.md#lp_for), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_title

```c
int ch_title(struct papfile *, struct papfile *, struct sockaddr_at *)
```

Defined at lines 118 to 149.

Calls: [compop](comment.c.md#compop), [get_text](headers.c.md#get_text), [lp_job](lp.c.md#lp_job), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### guess_creator

```c
static int guess_creator(char *creator)
```

Defined at lines 151 to 162.

Called by: [ch_creator](headers.c.md#ch_creator)

### ch_creator

```c
int ch_creator(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 165 to 196.

Calls: [compop](comment.c.md#compop), [get_text](headers.c.md#get_text), [guess_creator](headers.c.md#guess_creator), [lp_origin](lp.c.md#lp_origin), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_endcomm

```c
int ch_endcomm(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 198 to 223.

Calls: [compop](comment.c.md#compop), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_starttranslate

```c
int ch_starttranslate(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 225 to 248.

Calls: [compop](comment.c.md#compop), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_endtranslate

```c
int ch_endtranslate(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 250 to 273.

Calls: [compop](comment.c.md#compop), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_translateone

```c
int ch_translateone(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 275 to 299.

Calls: [compop](comment.c.md#compop), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)
