---
type: C Header File
title: "include/atalk/vfs.h"
description: "1 type, includes 3 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/vfs.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/acl.h](acl.h.md)
* [atalk/adouble.h](adouble.h.md)
* [atalk/volume.h](volume.h.md)

# Included by

* [bin/nad/nad_cp.c](../../bin/nad/nad_cp.c.md)
* [bin/nad/nad_mkdir.c](../../bin/nad/nad_mkdir.c.md)
* [bin/nad/nad_mv.c](../../bin/nad/nad_mv.c.md)
* [bin/nad/nad_rm.c](../../bin/nad/nad_rm.c.md)
* [bin/nad/nad_rmdir.c](../../bin/nad/nad_rmdir.c.md)
* [etc/afpd/acls.c](../../etc/afpd/acls.c.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/enumerate.c](../../etc/afpd/enumerate.c.md)
* [etc/afpd/extattrs.c](../../etc/afpd/extattrs.c.md)
* [etc/afpd/fce_api.c](../../etc/afpd/fce_api.c.md)
* [etc/afpd/fce_util.c](../../etc/afpd/fce_util.c.md)
* [etc/afpd/file.c](../../etc/afpd/file.c.md)
* [etc/afpd/filedir.c](../../etc/afpd/filedir.c.md)
* [etc/afpd/unix.c](../../etc/afpd/unix.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [include/atalk/ea.h](ea.h.md)
* [include/atalk/volume.h](volume.h.md)
* [libatalk/util/unix.c](../../libatalk/util/unix.c.md)
* [libatalk/vfs/ea_ad.c](../../libatalk/vfs/ea_ad.c.md)
* [libatalk/vfs/ea_sys.c](../../libatalk/vfs/ea_sys.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Types

### struct vfs_ops

Defined at line 98.
* `vfs_validupath_fn vfs_validupath`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [check_dirent](../../etc/afpd/enumerate.c.md#check_dirent), [check_name](../../etc/afpd/filedir.c.md#check_name), [copy](../../bin/nad/nad_cp.c.md#copy), [dbd_readdir](../../bin/dbd/cmd_dbd_scanvol.c.md#dbd_readdir), [vfs_validupath](../../libatalk/vfs/vfs.c.md#vfs_validupath).
* `vfs_chown_fn vfs_chown`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [setfilowner](../../etc/afpd/unix.c.md#setfilowner), [vfs_chown](../../libatalk/vfs/vfs.c.md#vfs_chown).
* `vfs_renamedir_fn vfs_renamedir`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [renamedir](../../etc/afpd/directory.c.md#renamedir), [vfs_renamedir](../../libatalk/vfs/vfs.c.md#vfs_renamedir).
* `vfs_deletecurdir_fn vfs_deletecurdir`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [deletecurdir](../../etc/afpd/directory.c.md#deletecurdir), [vfs_deletecurdir](../../libatalk/vfs/vfs.c.md#vfs_deletecurdir).
* `vfs_setfilmode_fn vfs_setfilmode`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [setfilunixmode](../../etc/afpd/unix.c.md#setfilunixmode), [vfs_setfilmode](../../libatalk/vfs/vfs.c.md#vfs_setfilmode).
* `vfs_setdirmode_fn vfs_setdirmode`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [vfs_setdirmode](../../libatalk/vfs/vfs.c.md#vfs_setdirmode).
* `vfs_setdirunixmode_fn vfs_setdirunixmode`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode), [vfs_setdirunixmode](../../libatalk/vfs/vfs.c.md#vfs_setdirunixmode).
* `vfs_setdirowner_fn vfs_setdirowner`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [setdirowner](../../etc/afpd/unix.c.md#setdirowner), [vfs_setdirowner](../../libatalk/vfs/vfs.c.md#vfs_setdirowner).
* `vfs_deletefile_fn vfs_deletefile`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [deletefile](../../etc/afpd/file.c.md#deletefile), [ftw_copy_file](../../bin/nad/nad_cp.c.md#ftw_copy_file), [rm](../../bin/nad/nad_rm.c.md#rm), [vfs_deletefile](../../libatalk/vfs/vfs.c.md#vfs_deletefile).
* `vfs_renamefile_fn vfs_renamefile`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [do_move](../../bin/nad/nad_mv.c.md#do_move), [renamefile](../../etc/afpd/file.c.md#renamefile), [vfs_renamefile](../../libatalk/vfs/vfs.c.md#vfs_renamefile).
* `vfs_copyfile_fn vfs_copyfile`: Assigned in [netatalk_adouble_ea](../../libatalk/vfs/vfs.c.md#netatalk_adouble_ea), [netatalk_adouble_v2](../../libatalk/vfs/vfs.c.md#netatalk_adouble_v2), [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [netatalk_ea_sys](../../libatalk/vfs/vfs.c.md#netatalk_ea_sys), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [vfs_copyfile](../../libatalk/vfs/vfs.c.md#vfs_copyfile).
* `vfs_solaris_acl_fn vfs_solaris_acl`: Assigned in [netatalk_solaris_acl_adouble](../../libatalk/vfs/vfs.c.md#netatalk_solaris_acl_adouble), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [set_acl](../../etc/afpd/acls.c.md#set_acl), [vfs_solaris_acl](../../libatalk/vfs/vfs.c.md#vfs_solaris_acl).
* `vfs_posix_acl_fn vfs_posix_acl`: Assigned in [netatalk_posix_acl_adouble](../../libatalk/vfs/vfs.c.md#netatalk_posix_acl_adouble), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [vfs_posix_acl](../../libatalk/vfs/vfs.c.md#vfs_posix_acl).
* `vfs_remove_acl_fn vfs_remove_acl`: Assigned in [netatalk_posix_acl_adouble](../../libatalk/vfs/vfs.c.md#netatalk_posix_acl_adouble), [netatalk_solaris_acl_adouble](../../libatalk/vfs/vfs.c.md#netatalk_solaris_acl_adouble), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [remove_acl](../../etc/afpd/acls.c.md#remove_acl), [vfs_remove_acl](../../libatalk/vfs/vfs.c.md#vfs_remove_acl).
* `vfs_ea_getsize_fn vfs_ea_getsize`: Assigned in [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [netatalk_ea_sys](../../libatalk/vfs/vfs.c.md#netatalk_ea_sys), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [vfs_ea_getsize](../../libatalk/vfs/vfs.c.md#vfs_ea_getsize).
* `vfs_ea_getcontent_fn vfs_ea_getcontent`: Assigned in [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [netatalk_ea_sys](../../libatalk/vfs/vfs.c.md#netatalk_ea_sys), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [vfs_ea_getcontent](../../libatalk/vfs/vfs.c.md#vfs_ea_getcontent).
* `vfs_ea_list_fn vfs_ea_list`: Assigned in [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [netatalk_ea_sys](../../libatalk/vfs/vfs.c.md#netatalk_ea_sys), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [vfs_ea_list](../../libatalk/vfs/vfs.c.md#vfs_ea_list).
* `vfs_ea_set_fn vfs_ea_set`: Assigned in [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [netatalk_ea_sys](../../libatalk/vfs/vfs.c.md#netatalk_ea_sys), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [vfs_ea_set](../../libatalk/vfs/vfs.c.md#vfs_ea_set).
* `vfs_ea_remove_fn vfs_ea_remove`: Assigned in [netatalk_ea_adouble](../../libatalk/vfs/vfs.c.md#netatalk_ea_adouble), [netatalk_ea_sys](../../libatalk/vfs/vfs.c.md#netatalk_ea_sys), [vfs_master_funcs](../../libatalk/vfs/vfs.c.md#vfs_master_funcs); called through by [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [vfs_ea_remove](../../libatalk/vfs/vfs.c.md#vfs_ea_remove).

# Typedefs and enums

* `typedef int(* vfs_chown_fn`
* `typedef int(* vfs_copyfile_fn`
* `typedef int(* vfs_deletecurdir_fn`
* `typedef int(* vfs_deletefile_fn`
* `typedef int(* vfs_ea_getcontent_fn`
* `typedef int(* vfs_ea_getsize_fn`
* `typedef int(* vfs_ea_list_fn`
* `typedef int(* vfs_ea_remove_fn`
* `typedef int(* vfs_ea_set_fn`
* `typedef int(* vfs_posix_acl_fn`
* `typedef int(* vfs_remove_acl_fn`
* `typedef int(* vfs_renamedir_fn`
* `typedef int(* vfs_renamefile_fn`
* `typedef int(* vfs_setdirmode_fn`
* `typedef int(* vfs_setdirowner_fn`
* `typedef int(* vfs_setdirunixmode_fn`
* `typedef int(* vfs_setfilmode_fn`
* `typedef int(* vfs_solaris_acl_fn`
* `typedef int(* vfs_validupath_fn`

# Macros

* Undocumented: `VFS_MODULES_MAX`
