---
type: C Header File
title: "include/atalk/volume.h"
description: "AFP volume state, configuration flags and extension mapping types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/volume.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-21T15:29:27+11:00 }
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
* [libatalk/cnid/cnid_mysql.c](../../libatalk/cnid/cnid_mysql.c.md)
* [libatalk/cnid/cnid_sqlite.c](../../libatalk/cnid/cnid_sqlite.c.md)
* [libatalk/util/pathconv.c](../../libatalk/util/pathconv.c.md)
* [libatalk/vfs/ea_ad.c](../../libatalk/vfs/ea_ad.c.md)
* [libatalk/vfs/ea_sys.c](../../libatalk/vfs/ea_sys.c.md)
* [libatalk/vfs/unix.c](../../libatalk/vfs/unix.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Types

### struct extmap

Defined at line 32.
* `char * em_ext`
* `char em_creator`
* `char em_type`

### struct vol

Defined at line 38.
* `struct vol * v_next`
* `AFPObj * v_obj`
* `uint16_t v_vid`
* `int v_flags`
* `char * v_path`
* `struct dir * v_root`
* `time_t v_mtime`
* `charset_t v_volcharset`
* `charset_t v_maccharset`
* `uint16_t v_mtou_flags`: flags for convert_charset in mtoupath
* `uint16_t v_utom_flags`
* `uint32_t v_kTextEncoding`: AFP text encoding hint in network order.
* `size_t max_filename`
* `char * v_veto`
* `int v_adouble`: adouble format: v1, v2
* `int v_ad_options`: ADVOL_* option bits, such as NODEV and NOCACHE.
* `const char *(* ad_path`: Called through by [RF_chown_adouble](../../libatalk/vfs/vfs.c.md#rf_chown_adouble), [RF_chown_ea](../../libatalk/vfs/vfs.c.md#rf_chown_ea), [RF_deletefile_adouble](../../libatalk/vfs/vfs.c.md#rf_deletefile_adouble), [RF_deletefile_ea](../../libatalk/vfs/vfs.c.md#rf_deletefile_ea), [RF_posix_acl](../../libatalk/vfs/vfs.c.md#rf_posix_acl), [RF_posix_remove_acl](../../libatalk/vfs/vfs.c.md#rf_posix_remove_acl), [RF_renamefile_adouble](../../libatalk/vfs/vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](../../libatalk/vfs/vfs.c.md#rf_renamefile_ea), [RF_setdirmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setdirmode_adouble), [RF_setdirunixmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setdirunixmode_adouble), [RF_setfilmode_adouble](../../libatalk/vfs/vfs.c.md#rf_setfilmode_adouble), [RF_setfilmode_ea](../../libatalk/vfs/vfs.c.md#rf_setfilmode_ea), [RF_solaris_acl](../../libatalk/vfs/vfs.c.md#rf_solaris_acl), [RF_solaris_remove_acl](../../libatalk/vfs/vfs.c.md#rf_solaris_remove_acl), [ad_conv_dehex](../../libatalk/adouble/ad_conv.c.md#ad_conv_dehex), [afp_syncdir](../../etc/afpd/directory.c.md#afp_syncdir), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [ea_path](../../libatalk/vfs/ea_ad.c.md#ea_path), [rmdir_with_cnid](../../bin/nad/nad_rmdir.c.md#rmdir_with_cnid).
* `struct _cnid_db * v_cdb`
* `char v_stamp`
* `VolSpace v_limitsize`: Size limit, if any, in MiB.
* `mode_t v_umask`
* `mode_t v_dperm`: default directories permission value OR with requested perm
* `mode_t v_fperm`: default files permission value OR with requested perm
* `ucs2_t * v_u8mname`: converted to utf8-mac in ucs2
* `ucs2_t * v_macname`: mangled to legacy longname in ucs2
* `ucs2_t * v_name`: either v_u8mname or v_macname
* `time_t v_ctime`: volume creation date, not unix ctime
* `dev_t v_dev`: device of the root, from [stat_vol()](../../etc/afpd/volume.c.md#stat_vol)
* `struct vfs_ops * vfs`: pointer to vfs_master_funcs for chaining
* `const struct vfs_ops * vfs_modules`
* `int v_vfs_ea`: The AFPVOL_EA_xx flag.
* `char * v_gvs`
* `void * v_nfsclient`
* `int v_nfs`
* `VolSpace v_tm_used`: used bytes on a TM volume
* `time_t v_tm_cachetime`: time at which v_tm_used was calculated last
* `VolSpace v_appended`: amount of data appended to files
* `time_t v_root_marked`: the second [setvoltime()](../../etc/afpd/volume.c.md#setvoltime) marked
* `VolSpace v_qfree`: the user's quota, bytes free
* `VolSpace v_qtotal`: the user's quota, bytes in all
* `time_t v_qtime`: when the quota was fetched, or 0
* `int v_root_due`: a change awaits a write of the root
* `int v_root_foreign`: another session's mark, unannounced
* `struct timespec v_root_stamp`: the stamp it last wrote or saw
* `int v_root_attn`: a change awaits the attention
* `int v_casefold`
* `char * v_configname`: as defined in afp.conf
* `char * v_localname`: as defined in afp.conf but with vars expanded
* `char * v_volcodepage`
* `char * v_maccodepage`
* `char * v_password`
* `char * v_cnidscheme`
* `char * v_dbpath`
* `char * v_legacyicon`: virtual icon name
* `unsigned char * v_icon_rfork`: pre-built resource fork
* `size_t v_icon_rfork_len`: length of v_icon_rfork data
* `int v_deleted`: volume open but deleted in new config file
* `char * v_sl_backend_name`: name of search backend, or NULL
* `const struct sl_backend_ops * v_sl_backend`: NULL if spotlight disabled.
* `char * v_preexec`
* `char * v_postexec`
* `int v_preexec_close`
* `char * v_uuid`: For TimeMachine zeroconf record.
* `uid_t v_private_uid`: an AFPVOL_PRIVATE volume's owner
* `int v_qfd`
* `uint32_t v_ignattr`: AFP attributes that shall be ignored.
* `int v_rootfd`: [ochdir()](../../libatalk/util/unix.c.md#ochdir) walks from it, or -1
* `int v_ignorecase`: the root's filesystem ignores case

# Typedefs and enums

* `typedef uint64_t VolSpace`
* `enum lv_flags_t`: `LV_DEFAULT`, `LV_ALL`, `LV_FORCE`

# Macros

* `AFPVOLSIG_FIX`: fixed ids
* `AFPVOLSIG_FLAT`: flat fs
* `AFPVOLSIG_VAR`: variable ids
* `AFPVOL_A2VOL`: prodos volume
* `AFPVOL_ACLS`: Volume supports ACLS.
* `AFPVOL_CHMOD_IGNORE`: try to preserve ACLs
* `AFPVOL_CHMOD_PRESERVE_ACL`: try to preserve ACLs
* `AFPVOL_DELVETO`: delete veto files and dirs
* `AFPVOL_EA_AD`: Store them in adouble files.
* `AFPVOL_EA_AUTO`: try sys, fallback to ad (default)
* `AFPVOL_EA_NONE`: No EAs.
* `AFPVOL_EA_SAMBA`: Store Samba compatible xattrs (append 0 byte).
* `AFPVOL_EA_SYS`: Store them in native EAs.
* `AFPVOL_EILSEQ`: encode illegal sequence 'asis' UCS2, ex "\217-", which is not a valid SHIFT-JIS char, is encoded as U\217 -
* `AFPVOL_FOLLOWSYM`: follow symlinks on the server, default is not to
* `AFPVOL_FORCE_STICKY_XATTR`: write metadata xattr as root on sticky dirs
* `AFPVOL_INV_DOTS`: dots files are invisible
* `AFPVOL_LIMITSIZE`: limit size for older macs
* `AFPVOL_MACNAMELEN`: AFP2 spec.
* `AFPVOL_MULTIPROTO`: other processes mutate the volume: coherency defaults apply
* `AFPVOL_NODEV`: always use 0 for device number in cnid calls help if device number is notconsistent across reboot
* `AFPVOL_NONETIDS`: signal the client it shall do privelege mapping
* `AFPVOL_NOSTAT`: advertise the volume even if we can't stat() it maybe because it will be mounted later in preexec
* `AFPVOL_NOV2TOEACONV`: no adouble:v2 to adouble:ea conversion
* `AFPVOL_PRIVATE`: one account's, its 'private' owner
* `AFPVOL_RO`: read-only volume
* `AFPVOL_SEARCHDB`: Use fast CNID db search instead of filesystem.
* `AFPVOL_SPOTLIGHT`: Index volume for Spotlight searches.
* `AFPVOL_TM`: Supports TimeMachine.
* `AFPVOL_U8MNAMELEN`: AFP3 sepc.
* `AFPVOL_UNIX_PRIV`: support unix privileges
* `VOLPBIT_BSIZE`: block size
* Undocumented: `AFPSRVR_CONFIGINFO`, `AFPSRVR_PASSWD`, `AFPVOLSIG_DEFAULT`, `AFPVOL_CASESENS`, `AFPVOL_GVSMASK`, `AFPVOL_MTOULOWER`, `AFPVOL_MTOUUPPER`, `AFPVOL_NONE`, `AFPVOL_OPEN`, `AFPVOL_ULOWERMUPPER`, `AFPVOL_UMLOWER`, `AFPVOL_UMUPPER`, `AFPVOL_UQUOTA`, `AFPVOL_USTATFS`, `AFPVOL_UTOMLOWER`, `AFPVOL_UTOMUPPER`, `AFPVOL_UUPPERMLOWER`, `VOLPBIT_ATTR`, `VOLPBIT_ATTR_ACLS`, `VOLPBIT_ATTR_BLANKACCESS`, `VOLPBIT_ATTR_CASESENS`, `VOLPBIT_ATTR_CATSEARCH`, `VOLPBIT_ATTR_EXT_ATTRS`, `VOLPBIT_ATTR_FILEID`, `VOLPBIT_ATTR_NONETIDS`, `VOLPBIT_ATTR_NOTFILEXCHG`, `VOLPBIT_ATTR_PASSWD`, `VOLPBIT_ATTR_PRIVPARENT`, `VOLPBIT_ATTR_RO`, `VOLPBIT_ATTR_TM`, `VOLPBIT_ATTR_UNIXPRIV`, `VOLPBIT_ATTR_UTF8`, `VOLPBIT_BDATE`, `VOLPBIT_BFREE`, `VOLPBIT_BTOTAL`, `VOLPBIT_CDATE`, `VOLPBIT_MDATE`, `VOLPBIT_NAME`, `VOLPBIT_SIG`, `VOLPBIT_VID`, `VOLPBIT_XBFREE`, `VOLPBIT_XBTOTAL`, `utf8_encoding`, `vol_chmod_opt`, `vol_dperm`, `vol_fperm`, `vol_inv_dots`, `vol_nodev`, `vol_syml_opt`, `vol_umask`, `vol_unix_priv`, `vol_unix_priv_enabled`
