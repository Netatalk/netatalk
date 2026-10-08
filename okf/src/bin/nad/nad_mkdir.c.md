---
type: C Source File
title: "bin/nad/nad_mkdir.c"
description: "4 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_mkdir.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/bstrlib_compat.h](../../include/atalk/bstrlib_compat.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [nad.h](nad.h.md)
* System headers: `errno.h`, `libgen.h`, `limits.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### usage_mkdir

```c
static void usage_mkdir(void)
```

Defined at lines 38 to 51.

Called by: [nad_mkdir](nad_mkdir.c.md#nad_mkdir)

### mkdir_with_cnid

```c
static int mkdir_with_cnid(const char *path, mode_t mode, afpvol_t *vol)
```

Defined at lines 62 to 133.

Create a single directory with CNID and AppleDouble metadata.

Parameters:
* `path`: absolute path of directory to create
* `mode`: permission mode for the new directory
* `vol`: open AFP volume

Returns: 0 on success, -1 on error

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [convert_utf8_to_mac](../../libatalk/util/pathconv.c.md#convert_utf8_to_mac), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [do_mkdir](nad_mkdir.c.md#do_mkdir)

### do_mkdir

```c
static int do_mkdir(const char *path, afpvol_t *vol)
```

Defined at lines 146 to 219.

Create directory and optionally intermediate parents.

When pflag is set, creates all missing intermediate directories along the path, registering each with the CNID database.

Parameters:
* `path`: path of directory to create
* `vol`: open AFP volume

Returns: 0 on success, 1 on error

Calls: [mkdir_with_cnid](nad_mkdir.c.md#mkdir_with_cnid), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [nad_mkdir](nad_mkdir.c.md#nad_mkdir)

Uses file-scope variables: `pflag` in [bin/nad/nad.h](nad.h.md), `vflag` in [bin/nad/nad.h](nad.h.md)

### nad_mkdir

```c
int nad_mkdir(int argc, char *argv[], AFPObj *obj)
```

Defined at lines 221 to 272. Declared in [bin/nad/nad.h](nad.h.md).

Calls: [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [do_mkdir](nad_mkdir.c.md#do_mkdir), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [usage_mkdir](nad_mkdir.c.md#usage_mkdir)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `pflag` in [bin/nad/nad.h](nad.h.md), `vflag` in [bin/nad/nad.h](nad.h.md)
