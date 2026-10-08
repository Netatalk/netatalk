---
type: C Source File
title: "libatalk/util/cnid.c"
description: "3 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/cnid.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/bstrlib_compat.h](../../include/atalk/bstrlib_compat.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `fcntl.h`, `libgen.h`, `limits.h`, `stdarg.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Functions

### rel_path_in_vol

```c
bstring rel_path_in_vol(const char *path, const char *volpath)
```

Defined at lines 71 to 149.

Build path relative to volume root.

path might be:

1. relative: "dir/subdir" with cwd: "/afp_volume/topdir"
1. absolute: "/afp_volume/dir/subdir"

Parameters:
* `path`: path relative to cwd() or absolute
* `volpath`: volume path that path is a subdir of (has been computed in volinfo funcs)

Returns: relative path in new bstring, caller must bdestroy it

Calls: [getcwdpath](unix.c.md#getcwdpath)

Called by: [cnid_for_path](cnid.c.md#cnid_for_path), [cnid_for_paths_parent](../../bin/nad/nad_util.c.md#cnid_for_paths_parent)

### cnid_for_path

```c
cnid_t cnid_for_path(struct _cnid_db *cdb, const char *volpath, const char *path, cnid_t *did)
```

Defined at lines 177 to 226.

Resolves CNID of a given path.

path might be:

1. relative: "dir/subdir" with cwd: "/afp_volume/topdir"
1. absolute: "/afp_volume/dir/subdir"

path MUST be pointing inside vol, this is usually the case as vol has been build from path using loadvolinfo and friends.

Allocates, not just resolves: any path component missing from the database is inserted by [cnid_add()](../cnid/cnid.c.md#cnid_add). A caller that only reads, such as Spotlight result resolution, can therefore consume CNIDs and reach the 32-bit ceiling.

Parameters:
* `cdb`: CNID db handle
* `volpath`: UNIX path of volume
* `path`: path, see above
* `did`: parent CNID of returned CNID

Returns: CNID of path, or CNID_INVALID with errno set by the backend. errno is preserved across cleanup so callers can act on it; in particular CNID_ERR_RESET means the volume's CNID table was emptied during this call and every session on it now holds stale CNIDs.

Calls: [cnid_add](../cnid/cnid.c.md#cnid_add), [rel_path_in_vol](cnid.c.md#rel_path_in_vol)

Called by: [copy](../../bin/nad/nad_cp.c.md#copy), [do_move](../../bin/nad/nad_mv.c.md#do_move), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [nad_update_cnid](../../bin/nad/nad_adouble.c.md#nad_update_cnid), [rm](../../bin/nad/nad_rm.c.md#rm), [rmdir_with_cnid](../../bin/nad/nad_rmdir.c.md#rmdir_with_cnid), [sl_cnid_open_query](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_open_query), [sl_xapian_fill_results](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_fill_results), [tracker_cursor_cb](../../etc/spotlight/localsearch/sl_localsearch.c.md#tracker_cursor_cb), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

### uuid_strip_dashes

```c
char * uuid_strip_dashes(const char *uuid)
```

Defined at lines 229 to 255.

Return allocated UUID string with dashes stripped

Called by: [cnid_mysql_open](../cnid/mysql/cnid_mysql.c.md#cnid_mysql_open), [cnid_sqlite_open](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_open)

Mentioned in the documentation of: [cnid_sqlite_uuid_usable](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_uuid_usable)
