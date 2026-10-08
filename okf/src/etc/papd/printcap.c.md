---
type: C Source File
title: "etc/papd/printcap.c"
description: "routines for dealing with the terminal capability data base based on termcap"
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/printcap.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 98 to 168.

Similar to tgetent except it returns the next entry instead of doing a lookup.

Note: Added a "cap" parameter, so we can use these calls for printcap and papd.conf.

Called by: [getprinters](main.c.md#getprinters)

Uses file-scope variables: `pfp`, `tbuf`

### endprent

```c
void endprent(void)
```

Defined at lines 170 to 175.

Called by: [getprinters](main.c.md#getprinters), [rprintcap](main.c.md#rprintcap)

Uses file-scope variables: `pfp`

### tgetent

```c
int tgetent(char *cap, char *bp, const char *name)
```

Defined at lines 186 to 291.

Get an entry for terminal name in buffer bp, from the termcap file.

Added a "cap" parameter, so we can use these calls for printcap and papd.conf.

Note: Parse is very rudimentary; we just notice escaped newlines.

Calls: [tnamatch](printcap.c.md#tnamatch), [tnchktc](printcap.c.md#tnchktc)

Called by: [tnchktc](printcap.c.md#tnchktc)

### tnchktc

```c
int tnchktc(char *cap)
```

Defined at lines 304 to 370.

check the last entry, see if it's tc=xxx.

If so, recursively find xxx and append that entry (minus the names) to take the place of the tc=xxx entry. This allows termcap entries to say "like an HP2621 but doesn't turn on the labels". Note that this works because of the left to right scan.

Added a "cap" parameter, so we can use these calls for printcap and papd.conf.

Calls: [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [tgetent](printcap.c.md#tgetent)

Called by: [tgetent](printcap.c.md#tgetent)

Uses file-scope variables: `hopcount`, `tbuf`

### tnamatch

```c
int tnamatch(const char *np)
```

Defined at lines 379 to 408.

deals with name matching.

The first field of the termcap entry is a sequence of names separated by |'s, so we compare against each such name. The normal : terminator after the last name (before the first field) stops us.

Called by: [tgetent](printcap.c.md#tgetent)

Uses file-scope variables: `tbuf`

### tskip

```c
static char * tskip(char *bp)
```

Defined at lines 415 to 426.

Skip to the next field. Notice that this is very dumb, not knowing about \: escapes or any such. If necessary, :'s can be put into the termcap file in octal.

Called by: [tgetflag](printcap.c.md#tgetflag), [tgetnum](printcap.c.md#tgetnum), [tgetstr](printcap.c.md#tgetstr)

### tgetnum

```c
int tgetnum(char *id)
```

Defined at lines 439 to 479.

Return the (numeric) option id.

Numeric options look like i.e. the option string is separated from the numeric value by a # character. If the option is not found we return -1. Note that we handle octal numbers beginning with 0.

```
li#80
```

Calls: [tskip](printcap.c.md#tskip)

Uses file-scope variables: `tbuf`

### tgetflag

```c
int tgetflag(char *id)
```

Defined at lines 488 to 507.

Handle a flag option.

Flag options are given "naked", i.e. followed by a : or the end of the buffer.

Returns: 1 if we find the option, or 0 if it is not given.

Calls: [tskip](printcap.c.md#tskip)

Uses file-scope variables: `tbuf`

### tdecode

```c
static char * tdecode(char *str, char **area)
```

Defined at lines 514 to 562.

Tdecode does the grunt work to decode the string capability escapes.

Called by: [tgetstr](printcap.c.md#tgetstr)

### tgetstr

```c
char * tgetstr(char *id, char **area)
```

Defined at lines 576 to 602.

Get a string valued option.

These are given as Much decoding is done on the strings, and the strings are placed in area, which is a ref parameter which is updated. No checking on area overflow.

```
cl=^Z
```

Calls: [tdecode](printcap.c.md#tdecode), [tskip](printcap.c.md#tskip)

Uses file-scope variables: `tbuf`

### decodename

```c
static char * decodename(char *str, char **area, int bufsize)
```

Defined at lines 605 to 653.

Called by: [getpname](printcap.c.md#getpname)

### getpname

```c
char * getpname(char **area, int bufsize)
```

Defined at lines 656 to 659.

Calls: [decodename](printcap.c.md#decodename)

Called by: [getprinters](main.c.md#getprinters)

Uses file-scope variables: `tbuf`

# Macros

* Undocumented: `BUFSIZ`, `MAXHOP`, `PRINTCAP`, `V6`, `tdecode`, `tdecode`, `tgetent`, `tgetflag`, `tgetnum`, `tgetstr`, `tnamatch`, `tnchktc`, `tskip`

# File-scope variables

`hopcount`, `pfp`, `tbuf`
