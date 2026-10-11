---
type: C Source File
title: "bin/nad/nad_rmdir.c"
description: "5 functions, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_rmdir.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/directory.h](../../include/atalk/directory.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [nad.h](nad.h.md)
* System headers: `bstrlib.h`, `errno.h`, `limits.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### usage_rmdir

```c
static void usage_rmdir(void)
```

Defined at lines 43 to 58.

Print the usage of nad rmdir and exit with EXIT_FAILURE.

Called by: [nad_rmdir](nad_rmdir.c.md#nad_rmdir)

### is_protected_dir

```c
static int is_protected_dir(const char *path, const afpvol_t *vol)
```

Defined at lines 70 to 102.

Tell whether a path is the volume root or a directory above it.

The call compares the resolved path with the volume path and logs a refusal. A path that does not resolve, or a missing volume, gives 0.

Parameters:
* `path`: directory path
* `vol`: volume of the path

Returns: 1 for a protected directory, or 0

Called by: [do_rmdir](nad_rmdir.c.md#do_rmdir), [rmdir_with_cnid](nad_rmdir.c.md#rmdir_with_cnid)

### rmdir_with_cnid

```c
static int rmdir_with_cnid(const char *path, afpvol_t *vol)
```

Defined at lines 112 to 163.

Remove a single empty directory with CNID and AppleDouble cleanup.

Parameters:
* `path`: absolute path of directory to remove
* `vol`: open AFP volume

Returns: 0 on success, -1 on error

Calls: [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [is_protected_dir](nad_rmdir.c.md#is_protected_dir), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset)

Called by: [do_rmdir](nad_rmdir.c.md#do_rmdir)

Calls through [`vol::ad_path`](../../include/atalk/volume.h.md#struct-vol): no table assigns this field

### do_rmdir

```c
static int do_rmdir(const char *path, afpvol_t *vol)
```

Defined at lines 176 to 230.

Remove directory and optionally empty parent directories.

When pflag is set, removes empty parent directories working upward from the specified path, stopping at the volume root.

Parameters:
* `path`: path of directory to remove
* `vol`: open AFP volume

Returns: 0 on success, 1 on error

Calls: [is_protected_dir](nad_rmdir.c.md#is_protected_dir), [rmdir_with_cnid](nad_rmdir.c.md#rmdir_with_cnid), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [nad_rmdir](nad_rmdir.c.md#nad_rmdir)

Uses file-scope variables: `pflag` in [bin/nad/nad.h](nad.h.md), `vflag` in [bin/nad/nad.h](nad.h.md)

### nad_rmdir

```c
int nad_rmdir(int argc, char *argv[], AFPObj *obj)
```

Defined at lines 245 to 296. Declared in [bin/nad/nad.h](nad.h.md).

Run the nad rmdir command, which deletes empty directories with their CNIDs and metadata.

The -p option also deletes the empty parents, and -v prints each deleted directory. The volume root and the directories above it stay. A signal stops the run.

Parameters:
* `argc`: argument count, from the rmdir command on
* `argv`: argument vector, from the rmdir command on
* `obj`: AFP configuration with the volumes

Returns: 0, or 1 when a directory fails, while usage errors exit

Calls: [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [do_rmdir](nad_rmdir.c.md#do_rmdir), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [usage_rmdir](nad_rmdir.c.md#usage_rmdir)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `pflag` in [bin/nad/nad.h](nad.h.md), `vflag` in [bin/nad/nad.h](nad.h.md)
