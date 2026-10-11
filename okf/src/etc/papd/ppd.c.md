---
type: C Source File
title: "etc/papd/ppd.c"
description: "6 functions, 1 type, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/ppd.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [ppd.h](ppd.h.md)
* [printer.h](printer.h.md)
* System headers: `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/time.h`, `sys/types.h`

# Functions

### ppd_init

```c
static void ppd_init(void)
```

Defined at lines 58 to 69.

Read the PPD file of the printer of this process, once.

Calls: [read_ppd](ppd.c.md#read_ppd)

Called by: [ppd_feature](ppd.c.md#ppd_feature), [ppd_font](ppd.c.md#ppd_font)

Uses file-scope variables: `ppd_inited`

### my_fgets

```c
static char * my_fgets(char *buf, size_t bufsize, FILE *stream)
```

Defined at lines 84 to 109.

Read a line that ends with CR or LF, for PPD files with Mac line ends.

A CR line end becomes LF. A line longer than the buffer spans calls, and only the call that reaches its end returns a line end.

Parameters:
* `buf`: line buffer, NUL-terminated on return
* `bufsize`: size of buf
* `stream`: PPD file

Returns: buf, or NULL at the end of the file

Called by: [getppdent](ppd.c.md#getppdent)

### getppdent

```c
static struct ppdent * getppdent(FILE *stream)
```

Defined at lines 122 to 221.

Read the next main keyword entry of a PPD file.

An entry has the form `*Main Option/Translation: Value`, and the option, translation, and value are optional. The call skips comments, queries, lines that do not start with `*`, and lines over 1023 bytes.

Parameters:
* `stream`: PPD file

Returns: the entry in a static buffer that the next call overwrites, or NULL at the end of the file

Calls: [my_fgets](ppd.c.md#my_fgets)

Called by: [read_ppd](ppd.c.md#read_ppd)

### read_ppd

```c
int read_ppd(char *file, int fcnt)
```

Defined at lines 234 to 309.

Read the fonts and the known features of a PPD file.

Each `*Font` entry adds its name to ppd_fonts once, and an entry for a feature of the ppd_features table stores its value. An `*Include` entry reads the named file, up to 20 levels deep.

Parameters:
* `file`: PPD file path
* `fcnt`: include depth, 0 for the top file

Returns: 0, or -1 when the file does not open or the includes go too deep

Calls: [getppdent](ppd.c.md#getppdent)

Called by: [ppd_init](ppd.c.md#ppd_init)

Uses file-scope variables: `ppd_features`, `ppd_fonts`

### ppd_font

```c
struct ppd_font * ppd_font(char *font)
```

Defined at lines 319 to 337.

Look up a font of the PPD file by name.

The first lookup reads the PPD file.

Parameters:
* `font`: font name

Returns: the font entry, or NULL for a font the PPD file does not list

Calls: [ppd_init](ppd.c.md#ppd_init)

Uses file-scope variables: `ppd_fonts`, `ppd_inited`

### ppd_feature

```c
struct ppd_feature * ppd_feature(const char *feature, int len)
```

Defined at lines 350 to 386.

Look up a feature of the PPD file by its main keyword.

The keyword runs up to the CR or LF that has to follow within len + 1 bytes, and len cannot exceed 255. Only a feature with a value matches. The first lookup reads the PPD file.

Parameters:
* `feature`: keyword such as `*PSVersion`, followed by a line end
* `len`: length of the keyword

Returns: the feature, or NULL for an unknown keyword or one without a value

Calls: [ppd_init](ppd.c.md#ppd_init)

Uses file-scope variables: `ppd_features`, `ppd_inited`

# Types

### struct ppdent

Defined at line 45.
* `char * pe_main`
* `char * pe_option`
* `char * pe_translation`
* `char * pe_value`

# File-scope variables

`ppd_features`, `ppd_fonts`, `ppd_inited`
