---
type: C Source File
title: "bin/nad/nad_mv.c"
description: "The nad mv command: move entries with their CNIDs and metadata."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_mv.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/bstrlib_compat.h](../../include/atalk/bstrlib_compat.h.md)
* [atalk/queue.h](../../include/atalk/queue.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* [atalk/volume.h](../../include/atalk/volume.h.md)
* [nad.h](nad.h.md)
* System headers: `dirent.h`, `errno.h`, `libgen.h`, `limits.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### usage_mv

```c
static void usage_mv(void)
```

Defined at lines 62 to 85.

Print the usage and the options of nad mv, then exit with EXIT_FAILURE.

Called by: [nad_mv](nad_mv.c.md#nad_mv)

### nad_mv

```c
int nad_mv(int argc, char *argv[], AFPObj *obj)
```

Defined at lines 100 to 241. Declared in [bin/nad/nad.h](nad.h.md).

Run the nad mv command, which moves entries with their metadata.

The options are -f, -i, -n, and -v. With two arguments and a target that is not a directory, the call moves the source to the target. Otherwise the last argument names a directory, and each source moves into it under its base name. Within one volume an entry keeps its CNID.

Parameters:
* `argc`: argument count, starting one word before mv
* `argv`: argument vector, starting one word before mv
* `obj`: AFP configuration with the volumes

Returns: 0, or 1 when a source fails, while usage errors exit

Calls: [closevol](nad_util.c.md#closevol), [cnid_init](../../libatalk/cnid/cnid_init.c.md#cnid_init), [do_move](nad_mv.c.md#do_move), [openvol_optional](nad_util.c.md#openvol_optional), [set_signal](nad_util.c.md#set_signal), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [usage_mv](nad_mv.c.md#usage_mv)

Called by: [main](nad.c.md#main)

Uses file-scope variables: `afp_obj`, `fflg`, `iflg`, `nflg`, `vflg`

### do_move

```c
static int do_move(const char *from, const char *to)
```

Defined at lines 260 to 493.

Move one source to a target path.

Without -f an existing target stays under -n, and -i asks before the move. A target without write access asks too, and an answer other than yes keeps the target.

Outside AFP volumes the call renames the entry. Within one volume it also moves the AppleDouble file of a regular file. It then updates the CNID record with the new parent and name and stores the CNID in the target metadata. Across volumes or devices it runs nad cp -R -p and then deletes the source.

Parameters:
* `from`: source path
* `to`: target path

Returns: 0, or 1 or -1 for an error

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [cnid_for_path](../../libatalk/util/cnid.c.md#cnid_for_path), [cnid_for_paths_parent](nad_util.c.md#cnid_for_paths_parent), [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update), [copy](nad_mv.c.md#copy), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset)

Called by: [nad_mv](nad_mv.c.md#nad_mv)

Uses file-scope variables: `fflg`, `iflg`, `nflg`, `vflg`

### remove_path

```c
static int remove_path(const char *path)
```

Defined at lines 504 to 541.

Delete a path outside a volume, and for a directory everything below it.

The call does not follow symbolic links. The first failure ends it.

Parameters:
* `path`: path to delete

Returns: 0, or -1 with errno for an error

Called by: [copy](nad_mv.c.md#copy)

### copy

```c
static int copy(const char *from, const char *to)
```

Defined at lines 555 to 607.

Move an entry across volumes or devices by running nad cp and then deleting the source.

The call first deletes an existing target, which works only for a file or an empty directory. It then runs nad cp -R -p, with -v under -v. Inside a volume nad rm -R deletes the source, and outside one remove_path does.

Parameters:
* `from`: source path
* `to`: target path

Returns: 0, or 1 for an error

Calls: [nad_cp](nad_cp.c.md#nad_cp), [nad_rm](nad_rm.c.md#nad_rm), [remove_path](nad_mv.c.md#remove_path)

Called by: [do_move](nad_mv.c.md#do_move)

Uses file-scope variables: `afp_obj`, `vflg`

# Macros

* Undocumented: `STRIP_TRAILING_SLASH`

# File-scope variables

`afp_obj`, `did`, `dvolume`, `fflg`, `iflg`, `nflg`, `pdid`, `svolume`, `vflg`
