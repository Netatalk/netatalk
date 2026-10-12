---
type: C Source File
title: "bin/nad/nad.c"
description: "The nad command: entry point and subcommand dispatch."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [nad.h](nad.h.md)
* System headers: `errno.h`, `limits.h`, `signal.h`, `stdarg.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/types.h`, `unistd.h`

# Functions

### get_command

```c
static nad_command_t get_command(const char *cmd)
```

Defined at lines 79 to 146.

Map a nad subcommand name to its command code.

The names sit and unsit exist only with StuffIt support, and -v and version map to CMD_VERSION.

Parameters:
* `cmd`: subcommand name

Returns: the command code, or CMD_UNKNOWN

Called by: [main](nad.c.md#main)

### usage_main

```c
static void usage_main(void)
```

Defined at lines 151 to 159.

Print the usage of nad and its subcommands to stdout.

Called by: [main](nad.c.md#main)

### show_version

```c
static void show_version(void)
```

Defined at lines 164 to 167.

Print the netatalk version of nad to stdout.

Called by: [main](nad.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 182 to 295.

Parse the global options of nad, load afp.conf, and run a subcommand.

The -F option names the afp.conf file, and force lets the commands work outside AFP volumes. The tool runs with the real user ID and its groups and logs as afp.conf sets. It loads the volumes and then passes the remaining arguments to the subcommand.

Parameters:
* `argc`: argument count
* `argv`: argument vector

Returns: the exit status of the subcommand, or 1 for a usage or configuration error

Calls: [afp_config_parse](../../libatalk/util/netatalk_conf.c.md#afp_config_parse), [get_command](nad.c.md#get_command), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [nad_archive](megatron.c.md#nad_archive), [nad_cp](nad_cp.c.md#nad_cp), [nad_find](nad_find.c.md#nad_find), [nad_ls](nad_ls.c.md#nad_ls), [nad_mkdir](nad_mkdir.c.md#nad_mkdir), [nad_mv](nad_mv.c.md#nad_mv), [nad_rm](nad_rm.c.md#nad_rm), [nad_rmdir](nad_rmdir.c.md#nad_rmdir), [nad_set](nad_set.c.md#nad_set), [nad_stuffit](nad_stuffit.c.md#nad_stuffit), [setuplog](../../libatalk/util/logger.c.md#setuplog), [show_version](nad.c.md#show_version), [usage_main](nad.c.md#usage_main)

Uses file-scope variables: `forceflag` in [bin/nad/nad_util.c](nad_util.c.md)

# Typedefs and enums

* `enum nad_command_t`: `CMD_UNKNOWN`, `CMD_LS`, `CMD_CP`, `CMD_RM`, `CMD_MV`, `CMD_SET`, `CMD_FIND`, `CMD_MKDIR`, `CMD_RMDIR`, `CMD_BIN`, `CMD_HEX`, `CMD_UNBIN`, `CMD_UNHEX`, `CMD_VERSION`

# Macros

* Undocumented: `my_gid_type`
