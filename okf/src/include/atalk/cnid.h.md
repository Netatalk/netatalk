---
type: C Header File
title: "include/atalk/cnid.h"
description: "CNID database types, backend operations and catalog lookup interface."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/cnid.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-17T10:01:45+11:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](adouble.h.md)
* [atalk/uuid.h](uuid.h.md)
* System headers: `errno.h`, `stdbool.h`, `stdint.h`

# Included by

* [bin/dbd/cmd_dbd_scanvol.c](../../bin/dbd/cmd_dbd_scanvol.c.md)
* [bin/nad/megatron.c](../../bin/nad/megatron.c.md)
* [bin/nad/nad.c](../../bin/nad/nad.c.md)
* [bin/nad/nad.h](../../bin/nad/nad.h.md)
* [bin/nad/nad_find.c](../../bin/nad/nad_find.c.md)
* [bin/nad/nad_ls.c](../../bin/nad/nad_ls.c.md)
* [bin/nad/nad_rmdir.c](../../bin/nad/nad_rmdir.c.md)
* [bin/nad/nad_set.c](../../bin/nad/nad_set.c.md)
* [bin/nad/nad_stuffit.c](../../bin/nad/nad_stuffit.c.md)
* [bin/nad/nad_util.c](../../bin/nad/nad_util.c.md)
* [etc/afpd/acls.c](../../etc/afpd/acls.c.md)
* [etc/afpd/catsearch.c](../../etc/afpd/catsearch.c.md)
* [etc/afpd/dircache.c](../../etc/afpd/dircache.c.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/enumerate.c](../../etc/afpd/enumerate.c.md)
* [etc/afpd/fce_api.c](../../etc/afpd/fce_api.c.md)
* [etc/afpd/fce_util.c](../../etc/afpd/fce_util.c.md)
* [etc/afpd/file.c](../../etc/afpd/file.c.md)
* [etc/afpd/filedir.c](../../etc/afpd/filedir.c.md)
* [etc/afpd/fork.c](../../etc/afpd/fork.c.md)
* [etc/afpd/mangle.h](../../etc/afpd/mangle.h.md)
* [etc/afpd/volume.h](../../etc/afpd/volume.h.md)
* [etc/spotlight/cnid/sl_cnid.c](../../etc/spotlight/cnid/sl_cnid.c.md)
* [etc/spotlight/localsearch/sl_localsearch.c](../../etc/spotlight/localsearch/sl_localsearch.c.md)
* [etc/spotlight/xapian/sl_xapian.c](../../etc/spotlight/xapian/sl_xapian.c.md)
* [include/atalk/directory.h](directory.h.md)
* [include/atalk/server_ipc.h](server_ipc.h.md)
* [include/atalk/util.h](util.h.md)
* [include/atalk/volume.h](volume.h.md)
* [libatalk/cnid/cnid.c](../../libatalk/cnid/cnid.c.md)
* [libatalk/cnid/cnid_init.c](../../libatalk/cnid/cnid_init.c.md)
* [libatalk/util/cnid.c](../../libatalk/util/cnid.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)

# Types

### struct _cnid_db

This is instance of CNID database object.

Defined at line 70.
* `uint32_t cnid_db_flags`
* `struct vol * cnid_db_vol`
* `void * cnid_db_private`
* `cnid_t(* cnid_add`: Called through by [cnid_add](../../libatalk/cnid/cnid.c.md#cnid_add).
* `int(* cnid_delete`: Called through by [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete).
* `cnid_t(* cnid_get`: Called through by [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get).
* `cnid_t(* cnid_lookup`: Called through by [cnid_lookup](../../libatalk/cnid/cnid.c.md#cnid_lookup).
* `char *(* cnid_resolve`: Called through by [cnid_resolve](../../libatalk/cnid/cnid.c.md#cnid_resolve).
* `int(* cnid_update`: Called through by [cnid_update](../../libatalk/cnid/cnid.c.md#cnid_update).
* `void(* cnid_close`: Called through by [cnid_close](../../libatalk/cnid/cnid.c.md#cnid_close).
* `int(* cnid_getstamp`: Called through by [cnid_getstamp](../../libatalk/cnid/cnid.c.md#cnid_getstamp).
* `int(* cnid_find`: Called through by [cnid_find_scoped](../../libatalk/cnid/cnid.c.md#cnid_find_scoped).
* `int(* cnid_wipe`: Called through by [cnid_wipe](../../libatalk/cnid/cnid.c.md#cnid_wipe).

### struct _cnid_module

Defined at line 108.
* `char * name`
* `struct _cnid_module * next`: Permanent registry link, owned by cnid.c.
* `struct _cnid_db *(* cnid_open`: Assigned in [cnid_mysql_module](../../libatalk/cnid/cnid_mysql.c.md#cnid_mysql_module), [cnid_sqlite_module](../../libatalk/cnid/cnid_sqlite.c.md#cnid_sqlite_module); called through by [cnid_open](../../libatalk/cnid/cnid.c.md#cnid_open).

### struct cnid_open_args

Defined at line 100.
* `uint32_t cnid_args_flags`
* `struct vol * cnid_args_vol`

# Typedefs and enums

* `typedef struct _cnid_db cnid_db`
* `typedef struct _cnid_module cnid_module`

# Macros

* `CNID_ERRNO_IS_BACKEND_FAILURE`: Whether errno says the backend could not answer at all.
* `CNID_ERR_BUSY`: contended or transient, retryable
* `CNID_ERR_CLOSE`: the db was not open
* `CNID_ERR_CORRUPT`: unusable data returned for a CNID
* `CNID_ERR_DB`: the backend itself is gone
* `CNID_ERR_NOTFOUND`: no row for the query
* `CNID_ERR_RESET`: CNID table was reset, all CNIDs stale.
* `CNID_FLAG_NODEV`: don't use device number only inode
* `CNID_FLAG_PERSISTENT`: This backend implements DID persistence.
* `CNID_START`: first valid ID
* Undocumented: `CNID_ERRNO`, `CNID_ERR_MAX`, `CNID_ERR_PARAM`, `CNID_ERR_PATH`, `CNID_FIND_MIN_BUFLEN`, `CNID_FIND_MIN_RESULTS`, `CNID_INVALID`
