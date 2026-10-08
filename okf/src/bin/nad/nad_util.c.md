---
type: C Source File
title: "bin/nad/nad_util.c"
description: "10 functions, includes 10 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/nad/nad_util.c"
tags: ["bin/nad"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/nad](../nad.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/bstrlib_compat.h](../../include/atalk/bstrlib_compat.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [nad.h](nad.h.md)
* System headers: `errno.h`, `fcntl.h`, `libgen.h`, `limits.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/acl.h`, `sys/mman.h`, `sys/param.h`, `sys/stat.h`, `sys/types.h`, `sysexits.h`, `unistd.h`

# Functions

### nad_not_inside_volume

```c
void nad_not_inside_volume(FILE *out, const char *path, int force_hint)
```

Defined at lines 79 to 88. Declared in [bin/nad/nad.h](nad.h.md).

Called by: [openvol](nad_util.c.md#openvol), [openvol_optional](nad_util.c.md#openvol_optional), [resolve_metadata_volume](megatron.c.md#resolve_metadata_volume)

### sig_handler

```c
static void sig_handler(int signo)
```

Defined at lines 90 to 94.

Called by: [set_signal](nad_util.c.md#set_signal)

Uses file-scope variables: `alarmed`

### set_signal

```c
void set_signal(void)
```

Defined at lines 96 to 126. Declared in [bin/nad/nad.h](nad.h.md).

Calls: [sig_handler](nad_util.c.md#sig_handler)

Called by: [nad_cp](nad_cp.c.md#nad_cp), [nad_find](nad_find.c.md#nad_find), [nad_ls](nad_ls.c.md#nad_ls), [nad_mkdir](nad_mkdir.c.md#nad_mkdir), [nad_mv](nad_mv.c.md#nad_mv), [nad_rm](nad_rm.c.md#nad_rm), [nad_rmdir](nad_rmdir.c.md#nad_rmdir)

### init_null_vol

```c
static void init_null_vol(void)
```

Defined at lines 131 to 142.

Calls: [ad_path_ea](../../libatalk/adouble/ad_open.c.md#ad_path_ea), [ad_path_osx](../../libatalk/adouble/ad_open.c.md#ad_path_osx)

Called by: [openvol_optional](nad_util.c.md#openvol_optional)

Uses file-scope variables: `null_vol`

### openvol_optional

```c
int openvol_optional(AFPObj *obj, const char *path, afpvol_t *vol)
```

Defined at lines 161 to 225. Declared in [bin/nad/nad.h](nad.h.md).

Open an AFP volume, or return a stub for non-AFP paths.

If the path is inside an AFP volume, the volume's CNID database is opened. If the path is outside any AFP volume, a stub volume with v_path=NULL and v_cdb=NULL is returned so that callers can use v_path as a guard for AFP-specific operations.

This is intended for commands like mv and cp that may operate across AFP and non-AFP paths.

Parameters:
* `obj`: [AFPObj](../../include/atalk/globals.h.md#struct-afpobj) of the current connection
* `path`: path to evaluate
* `vol`: structure to initialize

Returns: 0 on success, -1 on error

Calls: [cnid_getstamp](../../libatalk/cnid/cnid.c.md#cnid_getstamp), [cnid_open](../../libatalk/cnid/cnid.c.md#cnid_open), [cnid_scheme_registered](../../libatalk/cnid/cnid.c.md#cnid_scheme_registered), [getvolbypath](../../libatalk/util/netatalk_conf.c.md#getvolbypath), [init_null_vol](nad_util.c.md#init_null_vol), [load_charset](../../libatalk/util/netatalk_conf.c.md#load_charset), [nad_not_inside_volume](nad_util.c.md#nad_not_inside_volume)

Called by: [nad_cp](nad_cp.c.md#nad_cp), [nad_find](nad_find.c.md#nad_find), [nad_ls](nad_ls.c.md#nad_ls), [nad_mkdir](nad_mkdir.c.md#nad_mkdir), [nad_mv](nad_mv.c.md#nad_mv), [nad_rm](nad_rm.c.md#nad_rm), [nad_rmdir](nad_rmdir.c.md#nad_rmdir), [nad_set](nad_set.c.md#nad_set), [openvol](nad_util.c.md#openvol), [set_nad_volume_from_path](nad_stuffit.c.md#set_nad_volume_from_path)

Uses file-scope variables: `forceflag`, `null_vol`

### openvol

```c
int openvol(AFPObj *obj, const char *path, afpvol_t *vol)
```

Defined at lines 238 to 251. Declared in [bin/nad/nad.h](nad.h.md).

Load volinfo and initialize struct vol.

The path must be inside an AFP volume. Returns -1 if it is not.

Parameters:
* `obj`: [AFPObj](../../include/atalk/globals.h.md#struct-afpobj) of the current connection
* `path`: path to evaluate
* `vol`: structure to initialize

Returns: 0 on success, -1 on error

Calls: [nad_not_inside_volume](nad_util.c.md#nad_not_inside_volume), [openvol_optional](nad_util.c.md#openvol_optional)

### closevol

```c
void closevol(afpvol_t *vol)
```

Defined at lines 253 to 262. Declared in [bin/nad/nad.h](nad.h.md).

Calls: [cnid_close](../../libatalk/cnid/cnid.c.md#cnid_close)

Called by: [add_file_to_writer](nad_stuffit.c.md#add_file_to_writer), [nad_cp](nad_cp.c.md#nad_cp), [nad_find](nad_find.c.md#nad_find), [nad_ls](nad_ls.c.md#nad_ls), [nad_mkdir](nad_mkdir.c.md#nad_mkdir), [nad_mv](nad_mv.c.md#nad_mv), [nad_rm](nad_rm.c.md#nad_rm), [nad_rmdir](nad_rmdir.c.md#nad_rmdir), [nad_set](nad_set.c.md#nad_set), [unsit_archive](nad_stuffit.c.md#unsit_archive)

Uses file-scope variables: `null_vol`

Mentioned in the documentation of: [dircache_purge_vol](../../etc/afpd/dircache.c.md#dircache_purge_vol), [volume_free](../../libatalk/util/netatalk_conf.c.md#volume_free)

### convert_dots_encoding

```c
int convert_dots_encoding(const afpvol_t *svol, const afpvol_t *dvol, char *path)
```

Defined at lines 277 to 313. Declared in [bin/nad/nad.h](nad.h.md).

Convert dot encoding of basename *in place*

path arg can be "[/][dir/ | ...]filename". It will be converted in place possible encoding ".file" as ":2efile" which means the result will be longer then the original which means provide a big enough buffer.

Parameters:
* `svol`: source volume
* `dvol`: destination volume
* `path`: path to convert *in place*

Returns: 0 on success, -1 on error

Calls: [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [stripped_slashes_basename](../../libatalk/util/unix.c.md#stripped_slashes_basename), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [copy](nad_cp.c.md#copy)

### nad_report_cnid_reset

```c
void nad_report_cnid_reset(const char *volpath)
```

Defined at lines 323 to 331. Declared in [bin/nad/nad.h](nad.h.md).

Report a CNID table reset behind a failed resolve.

A reset empties the volume's CNID table as a side effect of the failing call, which the caller's generic resolve error would hide.

Parameters:
* `volpath`: path of the volume the failing call ran against

Called by: [cnid_for_paths_parent](nad_util.c.md#cnid_for_paths_parent), [copy](nad_cp.c.md#copy), [do_move](nad_mv.c.md#do_move), [mkdir_with_cnid](nad_mkdir.c.md#mkdir_with_cnid), [nad_update_cnid](nad_adouble.c.md#nad_update_cnid), [rm](nad_rm.c.md#rm), [rmdir_with_cnid](nad_rmdir.c.md#rmdir_with_cnid), [update_created_file_cnid](megatron.c.md#update_created_file_cnid)

### cnid_for_paths_parent

```c
cnid_t cnid_for_paths_parent(const afpvol_t *vol, const char *path, cnid_t *did)
```

Defined at lines 351 to 405. Declared in [bin/nad/nad.h](nad.h.md).

Resolves CNID of a given paths parent directory.

path might be: (a) relative: "dir/subdir" with cwd: "/afp_volume/topdir" (b) absolute: "/afp_volume/dir/subdir"

path MUST be pointing inside vol, this is usually the case as vol has been build from path using loadvolinfo and friends.

Parameters:
* `vol`: pointer to [afpvol_t](nad.h.md#struct-afpvol_t)
* `path`: path, see above
* `did`: parent CNID of returned CNID

Returns: CNID of path

Calls: [cnid_add](../../libatalk/cnid/cnid.c.md#cnid_add), [nad_report_cnid_reset](nad_util.c.md#nad_report_cnid_reset), [rel_path_in_vol](../../libatalk/util/cnid.c.md#rel_path_in_vol)

Called by: [do_move](nad_mv.c.md#do_move)

# File-scope variables

`alarmed`, `forceflag`, `nad_log_verbose`, `null_vol`
