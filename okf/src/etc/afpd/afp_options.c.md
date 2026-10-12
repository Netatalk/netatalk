---
type: C Source File
title: "etc/afpd/afp_options.c"
description: "Command line parsing and version output of afpd."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/afp_options.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 51 to 83.

Show version information about afpd.

Used by "afpd -v".

Called by: [afp_options_parse_cmdline](afp_options.c.md#afp_options_parse_cmdline)

### show_version_extended

```c
static void show_version_extended(void)
```

Defined at lines 90 to 170.

Show extended version information about afpd and Netatalk.

Used by "afpd -V".

Called by: [afp_options_parse_cmdline](afp_options.c.md#afp_options_parse_cmdline)

### show_paths

```c
static void show_paths(void)
```

Defined at lines 175 to 185.

Display compiled-in default paths.

Called by: [afp_options_parse_cmdline](afp_options.c.md#afp_options_parse_cmdline)

### show_usage

```c
static void show_usage(void)
```

Defined at lines 190 to 194.

Display usage information about afpd.

Called by: [afp_options_parse_cmdline](afp_options.c.md#afp_options_parse_cmdline)

### afp_options_parse_cmdline

```c
void afp_options_parse_cmdline(AFPObj *obj, int ac, char **av)
```

Defined at lines 209 to 258.

Parse the afpd command line into the command line fields of a server object.

The -d option sets OPTION_DEBUG, -u sets OPTION_SINGLEUSER, and -F stores the configuration file path. The -v, -V, and -h options print and exit 0. An unknown option or an extra argument prints the usage and exits with status 2. Resetting optind lets each server object parse the arguments.

Parameters:
* `obj`: server object
* `ac`: argument count
* `av`: argument vector

Calls: [show_paths](afp_options.c.md#show_paths), [show_usage](afp_options.c.md#show_usage), [show_version](afp_options.c.md#show_version), [show_version_extended](afp_options.c.md#show_version_extended)

Called by: [main](main.c.md#main)

# Macros

* Undocumented: `LENGTH`
