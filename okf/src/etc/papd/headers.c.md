---
type: C Source File
title: "etc/papd/headers.c"
description: "DSC header comment handlers of papd."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/headers.c"
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
* System headers: `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Function tables

### headers

Initialized at line 41. Dispatches to: [ch_creator](headers.c.md#ch_creator), [ch_endcomm](headers.c.md#ch_endcomm), [ch_endtranslate](headers.c.md#ch_endtranslate), [ch_for](headers.c.md#ch_for), [ch_starttranslate](headers.c.md#ch_starttranslate), [ch_title](headers.c.md#ch_title), [ch_translateone](headers.c.md#ch_translateone)

# Functions

### get_text

```c
static char * get_text(char *start, int linelength)
```

Defined at lines 43 to 102.

Extract the value of a DSC comment line, the text after the colon.

The call skips spaces and tabs after the colon. A value in parentheses yields the text inside, with nested parentheses kept and without escape handling. Any other value yields the rest of the line, and a line without a colon yields an empty string.

Parameters:
* `start`: first byte of the line
* `linelength`: line length without the line end

Returns: a NUL-terminated string to free, or NULL for a line over 1023 bytes or a failed allocation

Called by: [ch_creator](headers.c.md#ch_creator), [ch_for](headers.c.md#ch_for), [ch_title](headers.c.md#ch_title)

### ch_for

```c
int ch_for(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 116 to 146.

Take the user name the job is for from a `%For:` header comment.

The call passes the value to lp_for and writes the line to the job with PF_TRANSLATE set. It then pops the comment state and consumes the line.

Parameters:
* `in`: input buffer of the job
* `out`: reply buffer, unused
* `sat`: client address, unused

Returns: CH_DONE, also at the end of the job, or CH_MORE while the line is incomplete

Calls: [compop](comment.c.md#compop), [get_text](headers.c.md#get_text), [lp_for](lp.c.md#lp_for), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_title

```c
int ch_title(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 160 to 191.

Take the job name from a `%Title:` header comment.

The call passes the value to lp_job and writes the line to the job with PF_TRANSLATE set. It then pops the comment state and consumes the line.

Parameters:
* `in`: input buffer of the job
* `out`: reply buffer, unused
* `sat`: client address, unused

Returns: CH_DONE, also at the end of the job, or CH_MORE while the line is incomplete

Calls: [compop](comment.c.md#compop), [get_text](headers.c.md#get_text), [lp_job](lp.c.md#lp_job), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### guess_creator

```c
static int guess_creator(char *creator)
```

Defined at lines 202 to 213.

Tell the driver that made a job from its `%Creator:` value.

The result picks the charset of the job name and user name: Mac Roman for 1 and decomposed UTF-8 for 2.

Parameters:
* `creator`: value of the comment

Returns: 1 for an Apple LaserWriter driver, 2 for cgpdftops, or 0

Called by: [ch_creator](headers.c.md#ch_creator)

### ch_creator

```c
int ch_creator(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 229 to 260.

Record the driver that made the job from a `%Creator:` header comment.

The call stores the guess_creator result in the input buffer, passes it to lp_origin, and writes the line to the job with PF_TRANSLATE set. It then pops the comment state and consumes the line.

Parameters:
* `in`: input buffer of the job, receives the origin
* `out`: reply buffer, unused
* `sat`: client address, unused

Returns: CH_DONE, also at the end of the job, or CH_MORE while the line is incomplete

Calls: [compop](comment.c.md#compop), [get_text](headers.c.md#get_text), [guess_creator](headers.c.md#guess_creator), [lp_origin](lp.c.md#lp_origin), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_endcomm

```c
int ch_endcomm(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 275 to 300.

End the header comments of a job at `%EndComments`.

The call sets PF_STW, so the job loop opens the spool file. It writes the line to the job with PF_TRANSLATE set, pops the comment state, and consumes the line.

Parameters:
* `in`: input buffer of the job, receives PF_STW
* `out`: reply buffer, unused
* `sat`: client address, unused

Returns: CH_DONE, also at the end of the job, or CH_MORE while the line is incomplete

Calls: [compop](comment.c.md#compop), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_starttranslate

```c
int ch_starttranslate(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 316 to 339.

Start a translated section at `%BeginFeature` or `%BeginPageSetup`.

The call sets PF_TRANSLATE and leaves it set until the matching end comment. PF_TRANSLATE makes lp_write turn CR line ends into LF for printers with P_FOOMATIC_HACK. The call writes the line, pops the comment state, and consumes the line.

Parameters:
* `in`: input buffer of the job, receives PF_TRANSLATE
* `out`: reply buffer, unused
* `sat`: client address, unused

Returns: CH_DONE, also at the end of the job, or CH_MORE while the line is incomplete

Calls: [compop](comment.c.md#compop), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_endtranslate

```c
int ch_endtranslate(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 353 to 376.

End a translated section at `%EndFeature` or `%EndPageSetup`.

The call writes the line while PF_TRANSLATE is still set, then clears it. It pops the comment state and consumes the line.

Parameters:
* `in`: input buffer of the job, without PF_TRANSLATE afterwards
* `out`: reply buffer, unused
* `sat`: client address, unused

Returns: CH_DONE, also at the end of the job, or CH_MORE while the line is incomplete

Calls: [compop](comment.c.md#compop), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)

### ch_translateone

```c
int ch_translateone(struct papfile *in, struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 390 to 414.

Write any other `%%` header comment with PF_TRANSLATE set for that line only.

The call pops the comment state and consumes the line.

Parameters:
* `in`: input buffer of the job
* `out`: reply buffer, unused
* `sat`: client address, unused

Returns: CH_DONE, also at the end of the job, or CH_MORE while the line is incomplete

Calls: [compop](comment.c.md#compop), [lp_write](lp.c.md#lp_write), [markline](file.c.md#markline)

Dispatched via: [headers](headers.c.md#headers)
