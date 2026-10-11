---
type: C Source File
title: "bin/nad/nad_rm.c"
description: "5 functions, includes 8 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_rm.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/queue.h](../../include/atalk/queue.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [ftw.h](ftw.h.md)
* [nad.h](nad.h.md)
* System headers: `bstrlib.h`, `errno.h`, `limits.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### check_netatalk_dirs

```c
static const char * check_netatalk_dirs(const char *name)
```

Defined at lines 69 to 80.

Tell whether a name is one of the netatalk folders .AppleDB and .AppleDesktop.

Parameters:
* `name`: entry name

Returns: the matching table entry, or NULL

Called by: [rm](nad_rm.c.md#rm)

Uses file-scope variables: `netatalk_dirs`

### upfunc

```c
static void upfunc(void)
```

Defined at lines 86 to 89.

Step the directory ID of the walk back to the parent after nftw leaves a directory.

Called by: [nad_rm](nad_rm.c.md#nad_rm)

Uses file-scope variables: `did`, `pdid`

### usage_rm

```c
static void usage_rm(void)
```

Defined at lines 94 to 106.

Print the usage of nad rm and exit with EXIT_FAILURE.

Called by: [nad_rm](nad_rm.c.md#nad_rm)

### nad_rm

```c
int nad_rm(int argc, char *argv[], AFPObj *obj)
```

Defined at lines 122 to 208. Declared in [bin/nad/nad.h](nad.h.md).

Run the nad rm command, which deletes entries with their CNIDs and AppleDouble data.

The -R option deletes directory trees, and -v prints each deleted entry. Each argument gets a fresh volume state and a depth-first walk. A shell glob can name a data file and its `._` file on adouble:ea, so a `._` argument that vanished with its data file passes quietly.

Parameters:
* `argc`: argument count, from the rm command on
* `argv`: argument vector, from the rm command on
* `obj`: AFP configuration with the volumes

Returns: 0, or 1 when an entry fails, while usage errors exit

Calls: [ad_valid_header_osx](../../libatalk/adouble/ad_open.c.md#ad_valid_header_osx), [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [nftw](ftw.c.md#nftw), [openvol_optional](nad_util.c.md#openvol_optional), [rm](nad_rm.c.md#rm), [set_signal](nad_util.c.md#set_signal), [upfunc](nad_rm.c.md#upfunc), [usage_rm](nad_rm.c.md#usage_rm)

Called by: [copy](nad_mv.c.md#copy), [main](nad.c.md#main)

Uses file-scope variables: `Rflag`, `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `badrm`, `did`, `pdid`, `rval`, `vflag` in [bin/nad/nad.h](nad.h.md), `volume`

### rm

```c
static int rm(const char *path, const struct stat *statp, int tflag, struct FTW *ftw)
```

Defined at lines 228 to 421.

Delete one entry during the depth-first nftw walk of nad rm.

Inside a volume the call deletes the CNID of the entry before it unlinks the entry. For a file it also deletes the AppleDouble data. Entries inside .AppleDouble on adouble:v2 skip the CNID step. A `._` file with a valid header on adouble:ea goes away as plain metadata. A directory needs -R, and devices, sockets, and FIFOs stay as errors.

For .AppleDB and .AppleDesktop the call returns FTW_SKIP_SUBTREE. Since nad rm walks without FTW_ACTIONRETVAL, that value ends the walk.

Parameters:
* `path`: entry path
* `statp`: lstat result of the entry
* `tflag`: nftw type flag
* `ftw`: walk position, unused

Returns: 0 to go on, or a nonzero value that ends the walk

Calls: [ad_valid_header_osx](../../libatalk/adouble/ad_open.c.md#ad_valid_header_osx), [check_netatalk_dirs](nad_rm.c.md#check_netatalk_dirs), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset)

Called by: [nad_rm](nad_rm.c.md#nad_rm)

Calls through [`vfs_ops::vfs_deletefile`](../../include/atalk/vfs.h.md#struct-vfs_ops): [RF_deletefile_adouble](../../libatalk/vfs/vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](../../libatalk/vfs/vfs.c.md#rf_deletefile_ea), [ea_deletefile](../../libatalk/vfs/ea_ad.c.md#ea_deletefile), [vfs_deletefile](../../libatalk/vfs/vfs.c.md#vfs_deletefile)

Uses file-scope variables: `Rflag`, `alarmed` in [bin/dbd/cmd_dbd.c](../dbd/cmd_dbd.c.md), `badrm`, `did`, `pdid`, `rval`, `vflag` in [bin/nad/nad.h](nad.h.md), `volume`

# Macros

* Undocumented: `STRIP_TRAILING_SLASH`

# File-scope variables

`Rflag`, `badrm`, `did`, `netatalk_dirs`, `pdid`, `rval`, `volume`
