---
type: C Source File
title: "libatalk/adouble/ad_flush.c"
description: "10 functions, includes 6 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_flush.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/adouble](../adouble.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `stdio.h`, `stdlib.h`, `string.h`

# Functions

### ad_rebuild_adouble_header_v2

```c
int ad_rebuild_adouble_header_v2(struct adouble *ad)
```

Defined at lines 54 to 92.

Prepare ad->ad_data buffer from struct adouble for writing on disk

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called through [`adouble_fops::ad_rebuild_header`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_flush_hf](ad_flush.c.md#ad_flush_hf)

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

### ad_rebuild_adouble_header_ea

```c
int ad_rebuild_adouble_header_ea(struct adouble *ad)
```

Defined at lines 94 to 132.

Called through [`adouble_fops::ad_rebuild_header`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_flush_hf](ad_flush.c.md#ad_flush_hf)

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_rebuild_adouble_header_osx

```c
int ad_rebuild_adouble_header_osx(struct adouble *ad, char *adbuf)
```

Defined at lines 137 to 186.

Prepare adbuf buffer from struct adouble for writing on disk

Called by: [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_flush_rf](ad_flush.c.md#ad_flush_rf)

### ad_copy_header_entry

```c
static char * ad_copy_header_entry(const struct adouble *add, const struct adouble *ads, uint32_t eid, uint32_t *len)
```

Defined at lines 194 to 229.

The source entry to copy for eid, or NULL to skip it.

Forks and the comment are never copied; anything else must be present on both sides and valid in the source. `len` receives its source length.

Called by: [ad_copy_header](ad_flush.c.md#ad_copy_header)

### ad_copy_header

```c
int ad_copy_header(struct adouble *add, struct adouble *ads)
```

Defined at lines 239 to 295.

Calls: [ad_copy_header_entry](ad_flush.c.md#ad_copy_header_entry), [ad_entry_fits](ad_open.c.md#ad_entry_fits), [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [copy_source_header](../../bin/nad/nad_cp.c.md#copy_source_header), [copyfile](../../etc/afpd/file.c.md#copyfile)

### ad_flush_hf

```c
static int ad_flush_hf(struct adouble *ad)
```

Defined at lines 297 to 441.

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [adf_pwrite](ad_write.c.md#adf_pwrite), [adflags2logstr](ad_open.c.md#adflags2logstr), [become_root](../util/unix.c.md#become_root), [sys_fsetxattr](../vfs/extattr.c.md#sys_fsetxattr), [unbecome_root](../util/unix.c.md#unbecome_root)

Called by: [ad_flush](ad_flush.c.md#ad_flush)

Calls through [`adouble_fops::ad_rebuild_header`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_rebuild_adouble_header_ea](ad_flush.c.md#ad_rebuild_adouble_header_ea), [ad_rebuild_adouble_header_v2](ad_flush.c.md#ad_rebuild_adouble_header_v2)

### ad_flush_rf

```c
static int ad_flush_rf(struct adouble *ad)
```

Defined at lines 447 to 482.

Flush resofork adouble file if any.

Note: currently adouble:ea and #ifndef HAVE_EAFD e.g. Linux

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_rebuild_adouble_header_osx](ad_flush.c.md#ad_rebuild_adouble_header_osx), [adf_pwrite](ad_write.c.md#adf_pwrite), [adflags2logstr](ad_open.c.md#adflags2logstr)

Called by: [ad_flush](ad_flush.c.md#ad_flush)

### ad_flush

```c
int ad_flush(struct adouble *ad)
```

Defined at lines 484 to 499.

Calls: [ad_flush_hf](ad_flush.c.md#ad_flush_hf), [ad_flush_rf](ad_flush.c.md#ad_flush_rf), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [adflags2logstr](ad_open.c.md#adflags2logstr)

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams), [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [do_move](../../bin/nad/nad_mv.c.md#do_move), [flushfork](../../etc/afpd/fork.c.md#flushfork), [get_id](../../etc/afpd/file.c.md#get_id), [getvolparams](../../etc/afpd/volume.c.md#getvolparams), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_close](../../bin/nad/nad_adouble.c.md#nad_close), [nad_header_write](../../bin/nad/nad_adouble.c.md#nad_header_write), [nad_set](../../bin/nad/nad_set.c.md#nad_set), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [renamedir](../../etc/afpd/directory.c.md#renamedir), [renamefile](../../etc/afpd/file.c.md#renamefile), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

### ad_data_closefd

```c
static int ad_data_closefd(struct adouble *ad)
```

Defined at lines 501 to 516.

Called by: [ad_close](ad_flush.c.md#ad_close)

### ad_close

```c
int ad_close(struct adouble *ad, int adflags)
```

Defined at lines 521 to 657.

Close a struct adouble freeing all resources

Calls: [ad_data_closefd](ad_flush.c.md#ad_data_closefd), [adf_lock_free](ad_lock.c.md#adf_lock_free), [adflags2logstr](ad_open.c.md#adflags2logstr)

Called by: [RF_renamefile_adouble](../vfs/vfs.c.md#rf_renamefile_adouble), [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_error](ad_open.c.md#ad_error), [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit), [copy](../../bin/nad/nad_cp.c.md#copy), [copy_source_header](../../bin/nad/nad_cp.c.md#copy_source_header), [copyfile](../../etc/afpd/file.c.md#copyfile), [dir_add](../../etc/afpd/directory.c.md#dir_add), [do_move](../../bin/nad/nad_mv.c.md#do_move), [ea_copyfile](../vfs/ea_ad.c.md#ea_copyfile), [ea_renamefile](../vfs/ea_ad.c.md#ea_renamefile), [find_adouble](../../etc/afpd/file.c.md#find_adouble), [getvolparams](../../etc/afpd/volume.c.md#getvolparams), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_close](../../bin/nad/nad_adouble.c.md#nad_close), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open), [nad_set](../../bin/nad/nad_set.c.md#nad_set), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [print_flags](../../bin/nad/nad_ls.c.md#print_flags), [renamedir](../../etc/afpd/directory.c.md#renamedir), [renamefile](../../etc/afpd/file.c.md#renamefile), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

Mentioned in the documentation of: [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached)

# Macros

* Undocumented: `EID_DISK`

# File-scope variables

`set_eid`
