---
type: C Header File
title: "include/atalk/volume.h"
description: "2 types, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/volume.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](cnid.h.md)
* [atalk/globals.h](globals.h.md)
* [atalk/hash.h](hash.h.md)
* [atalk/unicode.h](unicode.h.md)
* [atalk/vfs.h](vfs.h.md)
* System headers: `stdint.h`, `sys/types.h`

# Included by

* [bin/dbd/cmd_dbd_scanvol.c](../../bin/dbd/cmd_dbd_scanvol.c.md)
* [bin/nad/megatron.c](../../bin/nad/megatron.c.md)
* [bin/nad/nad.h](../../bin/nad/nad.h.md)
* [bin/nad/nad_adouble.c](../../bin/nad/nad_adouble.c.md)
* [bin/nad/nad_cp.c](../../bin/nad/nad_cp.c.md)
* [bin/nad/nad_mkdir.c](../../bin/nad/nad_mkdir.c.md)
* [bin/nad/nad_mv.c](../../bin/nad/nad_mv.c.md)
* [bin/nad/nad_rm.c](../../bin/nad/nad_rm.c.md)
* [bin/nad/nad_rmdir.c](../../bin/nad/nad_rmdir.c.md)
* [etc/afpd/ad_cache.c](../../etc/afpd/ad_cache.c.md)
* [etc/afpd/ad_cache.h](../../etc/afpd/ad_cache.h.md)
* [etc/afpd/dircache.c](../../etc/afpd/dircache.c.md)
* [etc/afpd/dircache.h](../../etc/afpd/dircache.h.md)
* [etc/afpd/spotlight.c](../../etc/afpd/spotlight.c.md)
* [etc/afpd/spotlight_marshalling.c](../../etc/afpd/spotlight_marshalling.c.md)
* [etc/afpd/uam.c](../../etc/afpd/uam.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [etc/afpd/volume.h](../../etc/afpd/volume.h.md)
* [etc/spotlight/cnid/sl_cnid.c](../../etc/spotlight/cnid/sl_cnid.c.md)
* [etc/spotlight/localsearch/sl_localsearch.c](../../etc/spotlight/localsearch/sl_localsearch.c.md)
* [etc/spotlight/xapian/sl_xapian.c](../../etc/spotlight/xapian/sl_xapian.c.md)
* [include/atalk/netatalk_conf.h](netatalk_conf.h.md)
* [include/atalk/spotlight.h](spotlight.h.md)
* [include/atalk/vfs.h](vfs.h.md)
* [libatalk/adouble/ad_conv.c](../../libatalk/adouble/ad_conv.c.md)
* [libatalk/adouble/ad_open.c](../../libatalk/adouble/ad_open.c.md)
* [libatalk/cnid/cnid.c](../../libatalk/cnid/cnid.c.md)
* [libatalk/cnid/mysql/cnid_mysql.c](../../libatalk/cnid/mysql/cnid_mysql.c.md)
* [libatalk/cnid/sqlite/cnid_sqlite.c](../../libatalk/cnid/sqlite/cnid_sqlite.c.md)
* [libatalk/util/pathconv.c](../../libatalk/util/pathconv.c.md)
* [libatalk/vfs/ea_ad.c](../../libatalk/vfs/ea_ad.c.md)
* [libatalk/vfs/ea_sys.c](../../libatalk/vfs/ea_sys.c.md)
* [libatalk/vfs/unix.c](../../libatalk/vfs/unix.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Types

### struct extmap

Defined at line 27.
* `char * em_ext`
* `char em_creator`
* `char em_type`

### struct vol

Defined at line 33.
* `struct vol * v_next`
* `AFPObj * v_obj`
* `uint16_t v_vid`
* `int v_flags`
* `char * v_path`
* `struct dir * v_root`
* `time_t v_mtime`
* `charset_t v_volcharset`
* `charset_t v_maccharset`
* `uint16_t v_mtou_flags`
* `uint16_t v_utom_flags`
* `uint32_t v_kTextEncoding`
* `size_t max_filename`
* `char * v_veto`
* `int v_adouble`
* `int v_ad_options`
* `const char *(* ad_path`: Called through by [RF_chown_adouble](../../libatalk/vfs/vfs.c.md#rf_chown_adouble), [RF_chown_ea](../../libatalk/vfs/vfs.c.md#rf_chown_ea), [RF_deletefile_adouble](../../libatalk/vfs/vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](../../libatalk/vfs/vfs.c.md#rf_deletefile_ea), [RF_posix_acl](../../libatalk/vfs/vfs.c.md#rf_posix_acl), [RF_posix_remove_acl](../../libatalk/vfs/vfs.c.md#rf_posix_remove_acl), [RF_renamefile_adouble](../../libatalk/vfs/vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](../../libatalk/vfs/vfs.c.md#rf_renamefile_ea), [RF_setdirmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setdirmode_adouble), [RF_setdirunixmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setdirunixmode_adouble), [RF_setfilmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](../../libatalk/vfs/vfs.c.md#rf_setfilmode_ea), [RF_solaris_acl](../../libatalk/vfs/vfs.c.md#rf_solaris_acl), [RF_solaris_remove_acl](../../libatalk/vfs/vfs.c.md#rf_solaris_remove_acl), [ad_conv_dehex](../../libatalk/adouble/ad_conv.c.md#ad_conv_dehex), [afp_syncdir](../../etc/afpd/directory.c.md#afp_syncdir), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [ea_path](../../libatalk/vfs/ea_ad.c.md#ea_path), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open), [rmdir_with_cnid](../../bin/nad/nad_rmdir.c.md#rmdir_with_cnid).
* `struct _cnid_db * v_cdb`
* `char v_stamp`
* `VolSpace v_limitsize`
* `mode_t v_umask`
* `mode_t v_dperm`
* `mode_t v_fperm`
* `ucs2_t * v_u8mname`
* `ucs2_t * v_macname`
* `ucs2_t * v_name`
* `time_t v_ctime`
* `dev_t v_dev`
* `struct vfs_ops * vfs`
* `const struct vfs_ops * vfs_modules`
* `int v_vfs_ea`
* `char * v_gvs`
* `void * v_nfsclient`
* `int v_nfs`
* `VolSpace v_tm_used`
* `time_t v_tm_cachetime`
* `VolSpace v_appended`
* `int v_casefold`
* `char * v_configname`
* `char * v_localname`
* `char * v_volcodepage`
* `char * v_maccodepage`
* `char * v_password`
* `char * v_cnidscheme`
* `char * v_dbpath`
* `char * v_legacyicon`
* `unsigned char * v_icon_rfork`
* `size_t v_icon_rfork_len`
* `int v_deleted`
* `char * v_sl_backend_name`
* `const struct sl_backend_ops * v_sl_backend`
* `char * v_preexec`
* `char * v_postexec`
* `int v_preexec_close`
* `char * v_uuid`
* `int v_qfd`
* `uint32_t v_ignattr`

# Typedefs and enums

* `typedef uint64_t VolSpace`
* `enum lv_flags_t`: `LV_DEFAULT`, `LV_ALL`, `LV_FORCE`

# Macros

* Undocumented: `AFPSRVR_CONFIGINFO`, `AFPSRVR_PASSWD`, `AFPVOLSIG_DEFAULT`, `AFPVOLSIG_FIX`, `AFPVOLSIG_FLAT`, `AFPVOLSIG_VAR`, `AFPVOL_A2VOL`, `AFPVOL_ACLS`, `AFPVOL_CASESENS`, `AFPVOL_CHMOD_IGNORE`, `AFPVOL_CHMOD_PRESERVE_ACL`, `AFPVOL_DELVETO`, `AFPVOL_EA_AD`, `AFPVOL_EA_AUTO`, `AFPVOL_EA_NONE`, `AFPVOL_EA_SAMBA`, `AFPVOL_EA_SYS`, `AFPVOL_EILSEQ`, `AFPVOL_FOLLOWSYM`, `AFPVOL_FORCE_STICKY_XATTR`, `AFPVOL_GVSMASK`, `AFPVOL_INV_DOTS`, `AFPVOL_LIMITSIZE`, `AFPVOL_MACNAMELEN`, `AFPVOL_MTOULOWER`, `AFPVOL_MTOUUPPER`, `AFPVOL_MULTIPROTO`, `AFPVOL_NODEV`, `AFPVOL_NONE`, `AFPVOL_NONETIDS`, `AFPVOL_NOSTAT`, `AFPVOL_NOV2TOEACONV`, `AFPVOL_OPEN`, `AFPVOL_RO`, `AFPVOL_SEARCHDB`, `AFPVOL_SPOTLIGHT`, `AFPVOL_TM`, `AFPVOL_U8MNAMELEN`, `AFPVOL_ULOWERMUPPER`, `AFPVOL_UMLOWER`, `AFPVOL_UMUPPER`, `AFPVOL_UNIX_PRIV`, `AFPVOL_UQUOTA`, `AFPVOL_USTATFS`, `AFPVOL_UTOMLOWER`, `AFPVOL_UTOMUPPER`, `AFPVOL_UUPPERMLOWER`, `VOLPBIT_ATTR`, `VOLPBIT_ATTR_ACLS`, `VOLPBIT_ATTR_BLANKACCESS`, `VOLPBIT_ATTR_CASESENS`, `VOLPBIT_ATTR_CATSEARCH`, `VOLPBIT_ATTR_EXT_ATTRS`, `VOLPBIT_ATTR_FILEID`, `VOLPBIT_ATTR_NONETIDS`, `VOLPBIT_ATTR_NOTFILEXCHG`, `VOLPBIT_ATTR_PASSWD`, `VOLPBIT_ATTR_PRIVPARENT`, `VOLPBIT_ATTR_RO`, `VOLPBIT_ATTR_TM`, `VOLPBIT_ATTR_UNIXPRIV`, `VOLPBIT_ATTR_UTF8`, `VOLPBIT_BDATE`, `VOLPBIT_BFREE`, `VOLPBIT_BSIZE`, `VOLPBIT_BTOTAL`, `VOLPBIT_CDATE`, `VOLPBIT_MDATE`, `VOLPBIT_NAME`, `VOLPBIT_SIG`, `VOLPBIT_VID`, `VOLPBIT_XBFREE`, `VOLPBIT_XBTOTAL`, `utf8_encoding`, `vol_chmod_opt`, `vol_dperm`, `vol_fperm`, `vol_inv_dots`, `vol_nodev`, `vol_syml_opt`, `vol_umask`, `vol_unix_priv`, `vol_unix_priv_enabled`
