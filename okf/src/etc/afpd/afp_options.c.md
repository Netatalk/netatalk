---
type: C Source File
title: "etc/afpd/afp_options.c"
description: "5 functions, includes 9 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afp_options.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [auth.h](auth.h.md)
* [dircache.h](dircache.h.md)
* [status.h](status.h.md)
* System headers: `arpa/inet.h`, `ctype.h`, `grp.h`, `netdb.h`, `netinet/in.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `sys/types.h`, `unistd.h`

# Functions

### show_version

```c
static void show_version(void)
```

Defined at lines 46 to 78.

Show version information about afpd.

Used by "afpd -v".

Called by: [afp_options_parse_cmdline](afp_options.c.md#afp_options_parse_cmdline)

### show_version_extended

```c
static void show_version_extended(void)
```

Defined at lines 85 to 165.

Show extended version information about afpd and Netatalk.

Used by "afpd -V".

Called by: [afp_options_parse_cmdline](afp_options.c.md#afp_options_parse_cmdline)

### show_paths

```c
static void show_paths(void)
```

Defined at lines 170 to 180.

Display compiled-in default paths

Called by: [afp_options_parse_cmdline](afp_options.c.md#afp_options_parse_cmdline)

### show_usage

```c
static void show_usage(void)
```

Defined at lines 185 to 189.

Display usage information about afpd.

Called by: [afp_options_parse_cmdline](afp_options.c.md#afp_options_parse_cmdline)

### afp_options_parse_cmdline

```c
void afp_options_parse_cmdline(AFPObj *obj, int ac, char **av)
```

Defined at lines 191 to 240.

Calls: [show_paths](afp_options.c.md#show_paths), [show_usage](afp_options.c.md#show_usage), [show_version](afp_options.c.md#show_version), [show_version_extended](afp_options.c.md#show_version_extended)

Called by: [main](main.c.md#main)

# Macros

* Undocumented: `LENGTH`
