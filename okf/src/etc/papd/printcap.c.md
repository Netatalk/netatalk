---
type: C Source File
title: "etc/papd/printcap.c"
description: "routines for dealing with the terminal capability data base based on termcap"
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/printcap.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [printcap.h](printcap.h.md)
* System headers: `ctype.h`, `fcntl.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### getprent

```c
int getprent(char *cap, char *bp, int bufsize)
```

Defined at lines 107 to 177.

Read the next entry of papd.conf or a printcap file into a buffer.

The first call opens the file, and the end of the file closes it again. A backslash before a newline joins lines, and lines that start with `#` are comments. The call drops spaces and tabs at the start of an entry. An entry that fills the buffer ends there with a log line, and the call closes the file.

Parameters:
* `cap`: file path
* `bp`: buffer that receives the entry, which tbuf then points at
* `bufsize`: size of bp

Returns: 1 for an entry, 0 at the end of the file, or -1 when the file does not open

Called by: [getprinters](main.c.md#getprinters)

Uses file-scope variables: `pfp`, `tbuf`

### endprent

```c
void endprent(void)
```

Defined at lines 182 to 187.

Close the file that getprent reads.

Called by: [getprinters](main.c.md#getprinters), [rprintcap](main.c.md#rprintcap)

Uses file-scope variables: `pfp`

### pgetent

```c
int pgetent(char *cap, char *bp, const char *name)
```

Defined at lines 204 to 309.

Find the entry with a given name in a printcap-style file.

The call reads the file entry by entry into bp, joining lines that end with a backslash and skipping `#` comments, until pnamatch accepts name. The name check of pnamatch and the capability readers use tbuf, which only getprent sets. A tc= capability at the end of the entry pulls in the named entry through pnchktc.

Parameters:
* `cap`: file path
* `bp`: buffer of BUFSIZ bytes that receives the entry
* `name`: entry name to look for

Returns: 1 for a match, 0 when the name is missing or a tc= reference fails, or -1 when the file does not open

Called by: [rprintcap](main.c.md#rprintcap)

### pnchktc

```c
int pnchktc(char *cap)
```

Defined at lines 324 to 390.

Resolve a tc= capability that ends the entry in tbuf.

When the last field is tc=name, the call looks the named entry up with pgetent. Its fields, without its names, then take the place of the tc= field. A field of the entry comes before an included field of the same name, since lookups scan from the left. More than MAXHOP references fail, counted over the life of the process.

Parameters:
* `cap`: file path for the lookup

Returns: 1 when the entry is complete, or 0 for a bad entry, too many references, or a missing entry

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [getprinters](main.c.md#getprinters)

Uses file-scope variables: `hopcount`, `tbuf`

### pnamatch

```c
int pnamatch(const char *np)
```

Defined at lines 401 to 430.

Check whether the entry in tbuf lists a name among its names.

The first field of an entry lists its names, separated by `|`, and the first `:` ends the list. An entry that starts with `#` matches nothing.

Parameters:
* `np`: name to look for

Returns: 1 for a match, or 0

Uses file-scope variables: `tbuf`

### pskip

```c
static char * pskip(char *bp)
```

Defined at lines 441 to 452.

Skip to the next field of an entry.

The call moves past the next run of `:` and does not know about escaped colons, so a value spells a colon as `\072`.

Parameters:
* `bp`: position in the entry

Returns: the start of the next field, or the end of the entry

### pgetnum

```c
int pgetnum(char *id)
```

Defined at lines 463 to 503.

Return the value of a numeric capability of the entry in tbuf.

A numeric capability looks like `pc#200`, and a value with a leading 0 is octal. A capability written as `pc@` counts as missing.

Parameters:
* `id`: two-letter capability name

Returns: the value, or -1 when the entry lacks the capability

Called by: [rprintcap](main.c.md#rprintcap)

Uses file-scope variables: `tbuf`

### pgetflag

```c
int pgetflag(char *id)
```

Defined at lines 513 to 532.

Check a flag capability of the entry in tbuf.

A flag stands alone between colons, as in `:sp:`, and `sp@` turns it off.

Parameters:
* `id`: two-letter capability name

Returns: 1 when the entry has the flag, or 0

Called by: [getprinters](main.c.md#getprinters)

Uses file-scope variables: `tbuf`

### pdecode

```c
static char * pdecode(char *str, char **area)
```

Defined at lines 547 to 595.

Decode the value of a string capability into an area and advance the area.

The value runs to the next `:`. `^X` becomes a control character, and a backslash introduces `\E`, `\^`, `\\`, `\:`, `\n`, `\r`, `\t`, `\b`, `\f`, or up to three octal digits. The call does not check the area for room.

Parameters:
* `str`: start of the value
* `area`: output area, advanced past the decoded string

Returns: the decoded string

### pgetstr

```c
char * pgetstr(char *id, char **area)
```

Defined at lines 609 to 635.

Return the value of a string capability of the entry in tbuf.

A string capability looks like `pr=lp`, and pdecode turns its escapes into bytes in area. A capability written as `pr@` counts as missing. The call does not check the area for room.

Parameters:
* `id`: two-letter capability name
* `area`: output area, advanced past the value

Returns: the decoded value, or NULL when the entry lacks the capability

Called by: [getprinters](main.c.md#getprinters), [rprintcap](main.c.md#rprintcap)

Uses file-scope variables: `tbuf`

### decodename

```c
static char * decodename(char *str, char **area, int bufsize)
```

Defined at lines 649 to 697.

Decode the first name of an entry into an area and advance the area.

The name runs to the first `:` or `|`, takes at most bufsize - 1 bytes, and uses the escapes of pdecode.

Parameters:
* `str`: start of the entry
* `area`: output area, advanced past the decoded name
* `bufsize`: room in the area

Returns: the decoded name

Called by: [getpname](printcap.c.md#getpname)

### getpname

```c
char * getpname(char **area, int bufsize)
```

Defined at lines 708 to 711.

Return the first name of the entry in tbuf, the printer name of a papd.conf entry.

Parameters:
* `area`: output area, advanced past the name
* `bufsize`: room in the area

Returns: the decoded name

Calls: [decodename](printcap.c.md#decodename)

Called by: [getprinters](main.c.md#getprinters)

Uses file-scope variables: `tbuf`

# Macros

* `MAXHOP`: max number of tc= indirections
* Undocumented: `BUFSIZ`, `PRINTCAP`, `V6`, `tdecode`, `tdecode`, `tgetent`, `tgetflag`, `tgetnum`, `tgetstr`, `tnamatch`, `tnchktc`, `tskip`

# File-scope variables

`hopcount`, `pfp`, `tbuf`
