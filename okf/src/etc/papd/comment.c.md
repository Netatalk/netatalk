---
type: C Source File
title: "etc/papd/comment.c"
description: "5 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/comment.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* [comment.h](comment.h.md)
* System headers: `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### compop

```c
void compop(void)
```

Defined at lines 28 to 34.

Pop the top state off the comment stack and free it.

The stack must not be empty.

Called by: [ch_creator](headers.c.md#ch_creator), [ch_endcomm](headers.c.md#ch_endcomm), [ch_endtranslate](headers.c.md#ch_endtranslate), [ch_for](headers.c.md#ch_for), [ch_starttranslate](headers.c.md#ch_starttranslate), [ch_title](headers.c.md#ch_title), [ch_translateone](headers.c.md#ch_translateone), [cm_psadobe](magics.c.md#cm_psadobe), [cm_psquery](magics.c.md#cm_psquery), [cm_psswitch](magics.c.md#cm_psswitch), [comswitch](comment.c.md#comswitch), [cq_default](queries.c.md#cq_default), [cq_end](queries.c.md#cq_end), [cq_feature](queries.c.md#cq_feature), [cq_font](queries.c.md#cq_font), [cq_fontlist](queries.c.md#cq_fontlist), [cq_printer](queries.c.md#cq_printer), [cq_query](queries.c.md#cq_query), [cq_rbilogin](queries.c.md#cq_rbilogin)

Uses file-scope variables: `comstate`

### compush

```c
void compush(struct papd_comment *comment)
```

Defined at lines 43 to 57.

Push a new state for a DSC comment onto the comment stack.

The state starts with no flags. A failed allocation exits papd.

Parameters:
* `comment`: comment table entry whose handler takes over

Called by: [cm_psadobe](magics.c.md#cm_psadobe), [cm_psquery](magics.c.md#cm_psquery), [comswitch](comment.c.md#comswitch), [ps](magics.c.md#ps)

Uses file-scope variables: `comstate`

### comswitch

```c
int comswitch(struct papd_comment *comments, int(*handler)())
```

Defined at lines 70 to 88. Declared in [etc/papd/comment.h](comment.h.md).

Replace the top state of the comment stack with the table entry that has a given handler.

The last matching entry of the table counts, and the new state starts with no flags.

Parameters:
* `comments`: comment table, ended by an entry without c_begin
* `handler`: handler to look for

Returns: 0, or -1 when no entry has the handler

Calls: [compop](comment.c.md#compop), [compush](comment.c.md#compush)

Called by: [cm_psswitch](magics.c.md#cm_psswitch), [cq_feature](queries.c.md#cq_feature), [cq_printer](queries.c.md#cq_printer), [cq_query](queries.c.md#cq_query)

### comcmp

```c
int comcmp(char *start, char *stop, char *str, int how)
```

Defined at lines 102 to 119.

Match a comment line against a keyword.

With C_FULL in how, the line has to equal str. Otherwise str has to be a prefix of the line.

Parameters:
* `start`: first byte of the line
* `stop`: end of the line, not included
* `str`: keyword, such as `%Title:`
* `how`: comment flags, C_FULL for a full match

Returns: 0 on a match, or 1

Called by: [commatch](comment.c.md#commatch), [cq_default](queries.c.md#cq_default), [cq_feature](queries.c.md#cq_feature), [cq_font](queries.c.md#cq_font), [cq_fontlist](queries.c.md#cq_fontlist), [cq_printer](queries.c.md#cq_printer), [cq_query](queries.c.md#cq_query), [cq_rbilogin](queries.c.md#cq_rbilogin)

### commatch

```c
struct papd_comment * commatch(char *start, char *stop, struct papd_comment comments[])
```

Defined at lines 131 to 147. Declared in [etc/papd/comment.h](comment.h.md).

Find the first table entry whose keyword matches a comment line.

Each entry goes through comcmp with the c_flags that it carries.

Parameters:
* `start`: first byte of the line
* `stop`: end of the line, not included
* `comments`: comment table, ended by an entry without c_begin

Returns: the entry, or NULL without a match

Calls: [comcmp](comment.c.md#comcmp)

Called by: [cm_psadobe](magics.c.md#cm_psadobe), [cm_psquery](magics.c.md#cm_psquery), [ps](magics.c.md#ps)

# File-scope variables

`comcont`, `comstate`
