---
type: C Source File
title: "libatalk/util/gettok.c"
description: "Line tokenizer for configuration files, used by the extmap.conf reader."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/gettok.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/globals.h](../../include/atalk/globals.h.md)
* System headers: `ctype.h`, `pwd.h`, `string.h`, `sys/param.h`

# Functions

### initline

```c
void initline(int len, char *line)
```

Defined at lines 33 to 37.

Set the line that parseline reads its tokens from.

The position lives in static variables, so one line parses at a time.

Parameters:
* `len`: length of line
* `line`: NUL-terminated line, kept until the last parseline call

Called by: [readextmap](netatalk_conf.c.md#readextmap)

Uses file-scope variables: `l_curr`, `l_end`

### parseline

```c
int parseline(int len, char *token)
```

Defined at lines 58 to 113.

Read the next token from the line that initline set.

Space, tab, newline, and NUL separate tokens. A double quote toggles quoting, which keeps separators in the token and drops the quote itself. The call reads through the byte at the line length, the NUL of a line from fgets. A token can take len + 1 characters, so token needs len + 2 bytes.

Parameters:
* `len`: token limit
* `token`: receives the token, or an empty string on a return of -1

Returns: the token length, or -1 at the end of the line, after an unclosed quote, or for a longer token

Called by: [readextmap](netatalk_conf.c.md#readextmap)

Uses file-scope variables: `l_curr`, `l_end`

# Macros

* Undocumented: `ST_BEGIN`, `ST_QUOTE`, `ST_WORD`

# File-scope variables

`l_curr`, `l_end`
