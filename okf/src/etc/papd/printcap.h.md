---
type: C Header File
title: "etc/papd/printcap.h"
description: "6 functions."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/printcap.h"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `sys/types.h`

# Included by

* [etc/papd/main.c](main.c.md)
* [etc/papd/printcap.c](printcap.c.md)

# Functions

### pnchktc

```c
int pnchktc(char *)
```

Declared at etc/papd/printcap.h line 7; no definition in the scanned sources.

Called by: [getprinters](main.c.md#getprinters)

### pgetflag

```c
int pgetflag(char *)
```

Declared at etc/papd/printcap.h line 8; no definition in the scanned sources.

Called by: [getprinters](main.c.md#getprinters)

### pgetent

```c
int pgetent(char *, char *, const char *)
```

Declared at etc/papd/printcap.h line 10; no definition in the scanned sources.

Called by: [rprintcap](main.c.md#rprintcap)

### pgetnum

```c
int pgetnum(char *)
```

Declared at etc/papd/printcap.h line 11; no definition in the scanned sources.

Called by: [rprintcap](main.c.md#rprintcap)

### pnamatch

```c
int pnamatch(const char *)
```

Declared at etc/papd/printcap.h line 12; no definition in the scanned sources.

### pgetstr

```c
char * pgetstr(char *id, char **area)
```

Declared at etc/papd/printcap.h line 13; no definition in the scanned sources.

Called by: [getprinters](main.c.md#getprinters), [rprintcap](main.c.md#rprintcap)
