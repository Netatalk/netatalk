---
type: C Source File
title: "bin/dbd/cmd_dbd.c"
description: "5 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/dbd/cmd_dbd.c"
tags: ["bin/dbd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/dbd](../dbd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [cmd_dbd.h](cmd_dbd.h.md)
* System headers: `errno.h`, `limits.h`, `pwd.h`, `signal.h`, `stdarg.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/types.h`, `unistd.h`

# Functions

### sig_handler

```c
static void sig_handler(int signo)
```

Defined at lines 53 to 57.

SIGNAL handling.

Note: catch SIGINT and SIGTERM which cause clean exit. Ignore anything else.

Called by: [set_signal](cmd_dbd.c.md#set_signal)

Uses file-scope variables: `alarmed`

### set_signal

```c
static void set_signal(void)
```

Defined at lines 59 to 94.

Calls: [dbd_log](cmd_dbd.c.md#dbd_log), [sig_handler](cmd_dbd.c.md#sig_handler)

Called by: [main](cmd_dbd.c.md#main)

### usage

```c
static void usage(void)
```

Defined at lines 96 to 114.

Called by: [main](cmd_dbd.c.md#main)

### dbd_log

```c
void dbd_log(enum logtype lt, const char *fmt,...)
```

Defined at lines 120 to 131.

Called by: [check_addir](cmd_dbd_scanvol.c.md#check_addir), [check_adfile](cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](cmd_dbd_scanvol.c.md#check_cnid), [check_eafile_in_adouble](cmd_dbd_scanvol.c.md#check_eafile_in_adouble), [check_eafiles](cmd_dbd_scanvol.c.md#check_eafiles), [check_orphaned](cmd_dbd_scanvol.c.md#check_orphaned), [cmd_dbd_scanvol](cmd_dbd_scanvol.c.md#cmd_dbd_scanvol), [dbd_readdir](cmd_dbd_scanvol.c.md#dbd_readdir), [main](cmd_dbd.c.md#main), [read_addir](cmd_dbd_scanvol.c.md#read_addir), [remove_eafiles](cmd_dbd_scanvol.c.md#remove_eafiles), [set_signal](cmd_dbd.c.md#set_signal)

Uses file-scope variables: `flags`

### main

```c
int main(int argc, char **argv)
```

Defined at lines 133 to 347.

Calls: [ad_setfuid](../../libatalk/adouble/ad_open.c.md#ad_setfuid), [afp_config_parse](../../libatalk/util/netatalk_conf.c.md#afp_config_parse), [cmd_dbd_scanvol](cmd_dbd_scanvol.c.md#cmd_dbd_scanvol), [cnid_close](../../libatalk/cnid/cnid.c.md#cnid_close), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [cnid_open](../../libatalk/cnid/cnid.c.md#cnid_open), [cnid_scheme_registered](../../libatalk/cnid/cnid.c.md#cnid_scheme_registered), [cnid_wipe](../../libatalk/cnid/cnid.c.md#cnid_wipe), [dbd_log](cmd_dbd.c.md#dbd_log), [getvolbypath](../../libatalk/util/netatalk_conf.c.md#getvolbypath), [load_afp_conf_vols](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols), [load_charset](../../libatalk/util/netatalk_conf.c.md#load_charset), [realpath_safe](../../libatalk/util/unix.c.md#realpath_safe), [set_signal](cmd_dbd.c.md#set_signal), [setuplog](../../libatalk/util/logger.c.md#setuplog), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [usage](cmd_dbd.c.md#usage)

Uses file-scope variables: `flags`

# File-scope variables

`alarmed`, `flags`
