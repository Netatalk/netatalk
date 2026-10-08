---
type: C Header File
title: "include/atalk/adouble.h"
description: "Part of Netatalk's AppleDouble implementatation."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/adouble.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `fcntl.h`, `inttypes.h`, `stdbool.h`, `sys/mman.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `unistd.h`

# Included by

* [bin/dbd/cmd_dbd_scanvol.c](../../bin/dbd/cmd_dbd_scanvol.c.md)
* [bin/nad/hqx.c](../../bin/nad/hqx.c.md)
* [bin/nad/macbin.c](../../bin/nad/macbin.c.md)
* [bin/nad/megatron.c](../../bin/nad/megatron.c.md)
* [bin/nad/megatron.h](../../bin/nad/megatron.h.md)
* [bin/nad/nad_adouble.c](../../bin/nad/nad_adouble.c.md)
* [bin/nad/nad_cp.c](../../bin/nad/nad_cp.c.md)
* [bin/nad/nad_find.c](../../bin/nad/nad_find.c.md)
* [bin/nad/nad_ls.c](../../bin/nad/nad_ls.c.md)
* [bin/nad/nad_mkdir.c](../../bin/nad/nad_mkdir.c.md)
* [bin/nad/nad_mv.c](../../bin/nad/nad_mv.c.md)
* [bin/nad/nad_rm.c](../../bin/nad/nad_rm.c.md)
* [bin/nad/nad_rmdir.c](../../bin/nad/nad_rmdir.c.md)
* [bin/nad/nad_set.c](../../bin/nad/nad_set.c.md)
* [bin/nad/nad_stuffit.c](../../bin/nad/nad_stuffit.c.md)
* [bin/nad/nad_util.c](../../bin/nad/nad_util.c.md)
* [etc/afpd/acls.c](../../etc/afpd/acls.c.md)
* [etc/afpd/ad_cache.c](../../etc/afpd/ad_cache.c.md)
* [etc/afpd/ad_cache.h](../../etc/afpd/ad_cache.h.md)
* [etc/afpd/appl.c](../../etc/afpd/appl.c.md)
* [etc/afpd/catsearch.c](../../etc/afpd/catsearch.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/directory.h](../../etc/afpd/directory.h.md)
* [etc/afpd/enumerate.c](../../etc/afpd/enumerate.c.md)
* [etc/afpd/extattrs.c](../../etc/afpd/extattrs.c.md)
* [etc/afpd/fce_api.c](../../etc/afpd/fce_api.c.md)
* [etc/afpd/fce_util.c](../../etc/afpd/fce_util.c.md)
* [etc/afpd/file.c](../../etc/afpd/file.c.md)
* [etc/afpd/file.h](../../etc/afpd/file.h.md)
* [etc/afpd/filedir.c](../../etc/afpd/filedir.c.md)
* [etc/afpd/fork.c](../../etc/afpd/fork.c.md)
* [etc/afpd/fork.h](../../etc/afpd/fork.h.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/mangle.h](../../etc/afpd/mangle.h.md)
* [etc/afpd/unix.c](../../etc/afpd/unix.c.md)
* [etc/afpd/virtual_icon.c](../../etc/afpd/virtual_icon.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [include/atalk/cnid.h](cnid.h.md)
* [include/atalk/vfs.h](vfs.h.md)
* [libatalk/adouble/ad_attr.c](../../libatalk/adouble/ad_attr.c.md)
* [libatalk/adouble/ad_conv.c](../../libatalk/adouble/ad_conv.c.md)
* [libatalk/adouble/ad_date.c](../../libatalk/adouble/ad_date.c.md)
* [libatalk/adouble/ad_flush.c](../../libatalk/adouble/ad_flush.c.md)
* [libatalk/adouble/ad_lock.c](../../libatalk/adouble/ad_lock.c.md)
* [libatalk/adouble/ad_open.c](../../libatalk/adouble/ad_open.c.md)
* [libatalk/adouble/ad_read.c](../../libatalk/adouble/ad_read.c.md)
* [libatalk/adouble/ad_recvfile.c](../../libatalk/adouble/ad_recvfile.c.md)
* [libatalk/adouble/ad_sendfile.c](../../libatalk/adouble/ad_sendfile.c.md)
* [libatalk/adouble/ad_size.c](../../libatalk/adouble/ad_size.c.md)
* [libatalk/adouble/ad_write.c](../../libatalk/adouble/ad_write.c.md)
* [libatalk/cnid/mysql/cnid_mysql.c](../../libatalk/cnid/mysql/cnid_mysql.c.md)
* [libatalk/cnid/sqlite/cnid_sqlite.c](../../libatalk/cnid/sqlite/cnid_sqlite.c.md)
* [libatalk/util/unix.c](../../libatalk/util/unix.c.md)
* [libatalk/vfs/ea_ad.c](../../libatalk/vfs/ea_ad.c.md)
* [libatalk/vfs/ea_sys.c](../../libatalk/vfs/ea_sys.c.md)
* [libatalk/vfs/extattr.c](../../libatalk/vfs/extattr.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Functions

### ad_data_open

```c
static int ad_data_open(const struct adouble *ad)
```

Defined at lines 403 to 406.

Called by: [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork)

### ad_meta_open

```c
static int ad_meta_open(const struct adouble *ad)
```

Defined at lines 408 to 411.

Called by: [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_flush_hf](../../libatalk/adouble/ad_flush.c.md#ad_flush_hf), [ad_open_rf_v2](../../libatalk/adouble/ad_open.c.md#ad_open_rf_v2), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_getforkparams](../../etc/afpd/fork.c.md#afp_getforkparams), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit), [copyfile](../../etc/afpd/file.c.md#copyfile), [getforkparams](../../etc/afpd/fork.c.md#getforkparams)

Mentioned in the documentation of: [ad_meta_loaded](adouble.h.md#ad_meta_loaded)

### ad_rsrc_open

```c
static int ad_rsrc_open(const struct adouble *ad)
```

Defined at lines 413 to 416.

Called by: [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [rfork_cache_store_from_fd](../../etc/afpd/ad_cache.c.md#rfork_cache_store_from_fd)

### ad_meta_loaded

```c
static int ad_meta_loaded(const struct adouble *ad)
```

Defined at lines 423 to 428.

Metadata header read into ad; unlike [ad_meta_open()](adouble.h.md#ad_meta_open), true also for ea RDONLY opens, which read the EA by path and hold no fd. Symlinks short-circuit the metadata open with the refcount already taken and the header never read — both fds must be checked, as ea marks the meta fd and v2 the data fd.

Called by: [adl_lkup](../../etc/afpd/catsearch.c.md#adl_lkup), [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [getfilparams](../../etc/afpd/file.c.md#getfilparams)

### fsetrsrcea

```c
int fsetrsrcea(struct adouble *ad, int fd, const char *eaname, const void *value, size_t size, int flags)
```

Declared at include/atalk/adouble.h line 452; no definition in the scanned sources.

### ad_pread

```c
ssize_t ad_pread(struct ad_fd *, void *, size_t, off_t)
```

Declared at include/atalk/adouble.h line 507; no definition in the scanned sources.

# Types

### struct ad_entry

Defined at line 159.
* `off_t ade_off`
* `ssize_t ade_len`

### struct ad_fd

Defined at line 176.
* `int adf_fd`
* `char * adf_syml`
* `int adf_flags`
* `adf_lock_t * adf_lock`
* `int adf_refcount`
* `int adf_lockcount`
* `int adf_lockmax`

### struct adf_lock_shared

Defined at line 164.
* `int count`
* `off_t start`
* `off_t end`

### struct adf_lock_t

Defined at line 170.
* `struct flock lock`
* `int user`
* `adf_lock_shared_t * refcount`

### struct adouble

Defined at line 198.
* `uint32_t ad_magic`
* `uint32_t ad_version`
* `char ad_filler`
* `struct ad_entry ad_eid`
* `struct ad_fd ad_data_fork`
* `struct ad_fd ad_resource_fork`
* `struct ad_fd * ad_rfp`
* `struct ad_fd * ad_mdp`
* `int ad_vers`
* `int ad_adflags`
* `uint32_t ad_inited`
* `int ad_options`
* `int ad_refcount`
* `int ad_data_refcount`
* `int ad_meta_refcount`
* `int ad_reso_refcount`
* `off_t ad_rlen`
* `char * ad_name`
* `struct adouble_fops * ad_ops`
* `uint16_t ad_open_forks`
* `size_t valid_data_len`
* `char ad_data`

### struct adouble_fops

Defined at line 190.
* `const char *(* ad_path`: Assigned in [ad_adouble](../../libatalk/adouble/ad_open.c.md#ad_adouble), [ad_adouble_ea](../../libatalk/adouble/ad_open.c.md#ad_adouble_ea); called through by [ad_open_hf_v2](../../libatalk/adouble/ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](../../libatalk/adouble/ad_open.c.md#ad_open_rf_ea), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork).
* `int(* ad_mkrf`: Assigned in [ad_adouble](../../libatalk/adouble/ad_open.c.md#ad_adouble), [ad_adouble_ea](../../libatalk/adouble/ad_open.c.md#ad_adouble_ea); called through by [ad_open_hf_v2](../../libatalk/adouble/ad_open.c.md#ad_open_hf_v2).
* `int(* ad_rebuild_header`: Assigned in [ad_adouble](../../libatalk/adouble/ad_open.c.md#ad_adouble), [ad_adouble_ea](../../libatalk/adouble/ad_open.c.md#ad_adouble_ea); called through by [ad_flush_hf](../../libatalk/adouble/ad_flush.c.md#ad_flush_hf).
* `int(* ad_header_read`: Assigned in [ad_adouble](../../libatalk/adouble/ad_open.c.md#ad_adouble), [ad_adouble_ea](../../libatalk/adouble/ad_open.c.md#ad_adouble_ea); called through by [ad_conv_v22ea_hf](../../libatalk/adouble/ad_conv.c.md#ad_conv_v22ea_hf), [ad_open_hf_ea](../../libatalk/adouble/ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](../../libatalk/adouble/ad_open.c.md#ad_open_hf_v2), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh).
* `int(* ad_header_upgrade`: Assigned in [ad_adouble](../../libatalk/adouble/ad_open.c.md#ad_adouble), [ad_adouble_ea](../../libatalk/adouble/ad_open.c.md#ad_adouble_ea).

# Typedefs and enums

* `typedef struct adf_lock_shared adf_lock_shared_t`
* `typedef struct adf_lock_t adf_lock_t`
* `typedef uint32_t cnid_t`
* `enum ad_time_offset_dir_t`: `TO_UTC`, `TO_LOCALTIME`

# Macros

* Undocumented: `ADEDLEN_AFPFILEI`, `ADEDLEN_COMMENT`, `ADEDLEN_DID`, `ADEDLEN_FILEDATESI`, `ADEDLEN_FILEI`, `ADEDLEN_FILLER`, `ADEDLEN_FINDERI`, `ADEDLEN_ICONBW`, `ADEDLEN_ICONCOL`, `ADEDLEN_MACFILEI`, `ADEDLEN_MAGIC`, `ADEDLEN_MSDOSFILEI`, `ADEDLEN_NAME`, `ADEDLEN_NENTRIES`, `ADEDLEN_PRIVDEV`, `ADEDLEN_PRIVID`, `ADEDLEN_PRIVINO`, `ADEDLEN_PRIVSYN`, `ADEDLEN_PRODOSFILEI`, `ADEDLEN_SHORTNAME`, `ADEDLEN_VERSION`, `ADEDOFF_FINDERI_OSX`, `ADEDOFF_RFORK_OSX`, `ADEID_AFPFILEI`, `ADEID_COMMENT`, `ADEID_DFORK`, `ADEID_DID`, `ADEID_FILEDATESI`, `ADEID_FILEI`, `ADEID_FINDERI`, `ADEID_ICONBW`, `ADEID_ICONCOL`, `ADEID_MACFILEI`, `ADEID_MAX`, `ADEID_MSDOSFILEI`, `ADEID_NAME`, `ADEID_NUM_EA`, `ADEID_NUM_OSX`, `ADEID_NUM_V2`, `ADEID_PRIVDEV`, `ADEID_PRIVID`, `ADEID_PRIVINO`, `ADEID_PRIVSYN`, `ADEID_PRODOSFILEI`, `ADEID_RFORK`, `ADEID_SHORTNAME`, `ADFLAGS_CHECK_OF`, `ADFLAGS_CREATE`, `ADFLAGS_DF`, `ADFLAGS_DIR`, `ADFLAGS_EXCL`, `ADFLAGS_HF`, `ADFLAGS_NOHF`, `ADFLAGS_NORF`, `ADFLAGS_RDONLY`, `ADFLAGS_RDWR`, `ADFLAGS_RF`, `ADFLAGS_SETSHRMD`, `ADFLAGS_TRUNC`, `ADLOCK_CLR`, `ADLOCK_FILELOCK`, `ADLOCK_MASK`, `ADLOCK_RD`, `ADLOCK_UPGRADE`, `ADLOCK_WR`, `ADVOL_FOLLO_SYML`, `ADVOL_FORCE_STICKY_XATTR`, `ADVOL_INVDOTS`, `ADVOL_NODEV`, `ADVOL_RO`, `ADVOL_UNIXPRIV`, `AD_AFPFILEI_BLANKACCESS`, `AD_AFPFILEI_GROUP`, `AD_AFPFILEI_OWNER`, `AD_APPLEDOUBLE_MAGIC`, `AD_APPLESINGLE_MAGIC`, `AD_CLOSED`, `AD_DATASZ`, `AD_DATASZ2`, `AD_DATASZ_EA`, `AD_DATASZ_MAX`, `AD_DATASZ_OSX`, `AD_DATE_ACCESS`, `AD_DATE_BACKUP`, `AD_DATE_CREATE`, `AD_DATE_DELTA`, `AD_DATE_FROM_MAC`, `AD_DATE_FROM_UNIX`, `AD_DATE_MAC_DELTA`, `AD_DATE_MASK`, `AD_DATE_MODIFY`, `AD_DATE_START`, `AD_DATE_TO_MAC`, `AD_DATE_TO_UNIX`, `AD_DATE_UNIX`, `AD_DEV`, `AD_ENTRY_LEN`, `AD_FILELOCK_BAND_BITS`, `AD_FILELOCK_BASE`, `AD_FILELOCK_DENY_RD`, `AD_FILELOCK_DENY_WR`, `AD_FILELOCK_OPEN_NONE`, `AD_FILELOCK_OPEN_RD`, `AD_FILELOCK_OPEN_WR`, `AD_FILELOCK_RSRC_DENY_RD`, `AD_FILELOCK_RSRC_DENY_WR`, `AD_FILELOCK_RSRC_OPEN_NONE`, `AD_FILELOCK_RSRC_OPEN_RD`, `AD_FILELOCK_RSRC_OPEN_WR`, `AD_FILLER_NETATALK`, `AD_FILLER_OSX`, `AD_HEADER_LEN`, `AD_ID`, `AD_INITED`, `AD_INO`, `AD_MAGIC`, `AD_SYMLINK`, `AD_SYN`, `AD_VERSION`, `AD_VERSION2`, `AD_VERSION_EA`, `ALL_BAND_BITS`, `ATTRBIT_BACKUP`, `ATTRBIT_DOPEN`, `ATTRBIT_EXPFLDR`, `ATTRBIT_INVISIBLE`, `ATTRBIT_MOUNTED`, `ATTRBIT_MULTIUSER`, `ATTRBIT_NOCOPY`, `ATTRBIT_NODELETE`, `ATTRBIT_NORENAME`, `ATTRBIT_NOWRITE`, `ATTRBIT_ROPEN`, `ATTRBIT_SETCLR`, `ATTRBIT_SHARED`, `ATTRBIT_SYSTEM`, `BYTELOCK_MAX`, `DELETE_BLOCKING_BAND_BITS`, `DENY_RD_BIT`, `DENY_WR_BIT`, `FINDERINFO_CLOSEDVIEW`, `FINDERINFO_COLOR`, `FINDERINFO_CUSTOMICON`, `FINDERINFO_FRCREATOFF`, `FINDERINFO_FRFLAGOFF`, `FINDERINFO_FRTYPEOFF`, `FINDERINFO_FRVIEWOFF`, `FINDERINFO_HASBEENINITED`, `FINDERINFO_HASBUNDLE`, `FINDERINFO_HASCUSTOMICON`, `FINDERINFO_HASNOINITS`, `FINDERINFO_HIDEEXT`, `FINDERINFO_INVISIBLE`, `FINDERINFO_ISALIAS`, `FINDERINFO_ISHARED`, `FINDERINFO_ISONDESK`, `FINDERINFO_ISSTATIONNERY`, `FINDERINFO_NAMELOCKED`, `OPEN_BITS`, `OPEN_NONE_BIT`, `OPEN_RD_BIT`, `OPEN_WR_BIT`, `RSRC_DENY_RD_BIT`, `RSRC_DENY_WR_BIT`, `RSRC_OPEN_NONE_BIT`, `RSRC_OPEN_RD_BIT`, `RSRC_OPEN_WR_BIT`, `ad_data_fileno`, `ad_get_MD_flags`, `ad_get_RF_flags`, `ad_get_syml_opt`, `ad_getentrylen`, `ad_getversion`, `ad_meta_fileno`, `ad_ref`, `ad_reso_fileno`, `ad_setentrylen`, `ad_setentryoff`, `ad_unref`
