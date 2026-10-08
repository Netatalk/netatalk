---
type: C Source File
title: "libatalk/adouble/ad_conv.c"
description: "Part of Netatalk's AppleDouble implementatation."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_conv.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* System headers: `arpa/inet.h`, `bstrlib.h`, `errno.h`, `stdarg.h`, `stdlib.h`, `string.h`, `sys/param.h`

# Functions

### ad_conv_v22ea_hf

```c
static int ad_conv_v22ea_hf(const char *path, const struct stat *sp, const struct vol *vol)
```

Defined at lines 56 to 148.

Calls: [ad_close](ad_flush.c.md#ad_close), [ad_copy_header](ad_flush.c.md#ad_copy_header), [ad_flush](ad_flush.c.md#ad_flush), [ad_getdate](ad_date.c.md#ad_getdate), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_init](ad_open.c.md#ad_init), [ad_init_old](ad_open.c.md#ad_init_old), [ad_open](ad_open.c.md#ad_open), [ad_refresh](ad_open.c.md#ad_refresh), [ad_tmplock](ad_lock.c.md#ad_tmplock), [fullpathname](../util/unix.c.md#fullpathname), [sys_setxattr](../vfs/extattr.c.md#sys_setxattr)

Called by: [ad_conv_v22ea](ad_conv.c.md#ad_conv_v22ea)

Calls through [`adouble_fops::ad_header_read`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_header_read](ad_open.c.md#ad_header_read), [ad_header_read_ea](ad_open.c.md#ad_header_read_ea)

Uses file-scope variables: `emptydirad`, `emptyfilad`

### ad_conv_v22ea_rf

```c
static int ad_conv_v22ea_rf(const char *path, const struct stat *sp, const struct vol *vol)
```

Defined at lines 150 to 205.

Calls: [ad_close](ad_flush.c.md#ad_close), [ad_flush](ad_flush.c.md#ad_flush), [ad_init](ad_open.c.md#ad_init), [ad_init_old](ad_open.c.md#ad_init_old), [ad_open](ad_open.c.md#ad_open), [ad_tmplock](ad_lock.c.md#ad_tmplock), [copy_fork](ad_write.c.md#copy_fork), [fullpathname](../util/unix.c.md#fullpathname)

Called by: [ad_conv_v22ea](ad_conv.c.md#ad_conv_v22ea)

### ad_conv_v22ea

```c
static int ad_conv_v22ea(const char *path, const struct stat *sp, const struct vol *vol)
```

Defined at lines 207 to 237.

Calls: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_path](ad_open.c.md#ad_path), [become_root](../util/unix.c.md#become_root), [fullpathname](../util/unix.c.md#fullpathname), [unbecome_root](../util/unix.c.md#unbecome_root)

Called by: [ad_convert](ad_conv.c.md#ad_convert)

### ad_conv_dehex

```c
static int ad_conv_dehex(const char *path, const struct stat *sp, const struct vol *vol, const char **newpathp)
```

Defined at lines 242 to 306.

Remove hexencoded dots and slashes (":2e" and ":2f")

Calls: [become_root](../util/unix.c.md#become_root), [fullpathname](../util/unix.c.md#fullpathname), [strlcpy](../compat/strlcpy.c.md#strlcpy), [unbecome_root](../util/unix.c.md#unbecome_root)

Called by: [ad_convert](ad_conv.c.md#ad_convert)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

### ad_convert

```c
int ad_convert(const char *path, const struct stat *sp, const struct vol *vol, const char **newpath)
```

Defined at lines 319 to 351.

AppleDouble and encoding conversion on the fly.

Parameters:
* `path`: path to file or directory
* `sp`: stat(path)
* `vol`: volume handle
* `newpath`: if encoding changed, new name. Can be NULL.

Returns: -1 on internal error, otherwise 0. newpath is NULL if no character conversion was done, otherwise newpath points to a static string with the converted name

Calls: [ad_conv_dehex](ad_conv.c.md#ad_conv_dehex), [ad_conv_v22ea](ad_conv.c.md#ad_conv_v22ea), [fullpathname](../util/unix.c.md#fullpathname)

Called by: [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [cmd_dbd_scanvol](../../bin/dbd/cmd_dbd_scanvol.c.md#cmd_dbd_scanvol), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [getvolparams](../../etc/afpd/volume.c.md#getvolparams)

# File-scope variables

`emptydirad`, `emptyfilad`
