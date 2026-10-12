---
type: C Source File
title: "libatalk/adouble/ad_flush.c"
description: "Writing AppleDouble headers to disk and closing the forks."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/adouble/ad_flush.c"
tags: ["libatalk/adouble"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 66 to 104.

Serialize the header of a version 2 AppleDouble into ad->ad_data.

Writes the magic, the version, the filler, and one entry descriptor for every entry with an offset, in on-disk byte order.

Parameters:
* `ad`: AppleDouble whose in-memory entries are current

Returns: the offset of the resource fork entry, the length of the header to write

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called through [`adouble_fops::ad_rebuild_header`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_flush_hf](ad_flush.c.md#ad_flush_hf)

Dispatched via: [ad_adouble](ad_open.c.md#ad_adouble)

Mentioned in the documentation of: [ad_rebuild_adouble_header_ea](ad_flush.c.md#ad_rebuild_adouble_header_ea)

### ad_rebuild_adouble_header_ea

```c
int ad_rebuild_adouble_header_ea(struct adouble *ad)
```

Defined at lines 115 to 153.

Serialize the header of an EA AppleDouble into ad->ad_data.

Like [ad_rebuild_adouble_header_v2()](ad_flush.c.md#ad_rebuild_adouble_header_v2) without the resource fork entry, which the EA form keeps in a separate extended attribute.

Parameters:
* `ad`: AppleDouble whose in-memory entries are current

Returns: AD_DATASZ_EA, the length of the header to write

Called through [`adouble_fops::ad_rebuild_header`](../../include/atalk/adouble.h.md#struct-adouble_fops) by: [ad_flush_hf](ad_flush.c.md#ad_flush_hf)

Dispatched via: [ad_adouble_ea](ad_open.c.md#ad_adouble_ea)

### ad_rebuild_adouble_header_osx

```c
int ad_rebuild_adouble_header_osx(struct adouble *ad, char *adbuf)
```

Defined at lines 165 to 214.

Serialize the header of a macOS native AppleDouble into adbuf.

The macOS form carries the FinderInfo entry, the resource fork entry, and the netatalk filler string. The FinderInfo bytes come from ad.

Parameters:
* `ad`: AppleDouble whose in-memory entries are current
* `adbuf`: buffer of at least AD_DATASZ_OSX bytes

Returns: AD_DATASZ_OSX, the length of the header to write

Calls: [ad_entry](ad_open.c.md#ad_entry)

Called by: [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_flush_rf](ad_flush.c.md#ad_flush_rf)

### ad_copy_header_entry

```c
static char * ad_copy_header_entry(const struct adouble *add, const struct adouble *ads, uint32_t eid, uint32_t *len)
```

Defined at lines 228 to 263.

Source entry to transfer for eid, or NULL to skip it.

Forks and the comment never transfer. Any other entry must exist on both sides with a positive source length and pass the bounds test of [ad_entry](../../include/atalk/adouble.h.md#struct-ad_entry).

Parameters:
* `add`: destination AppleDouble
* `ads`: source AppleDouble
* `eid`: entry id, ADEID_*
* `len`: receives the source entry length

Returns: the source entry, or NULL to skip it

Calls: [ad_entry](ad_open.c.md#ad_entry)

Called by: [ad_copy_header](ad_flush.c.md#ad_copy_header)

### ad_copy_header

```c
int ad_copy_header(struct adouble *add, struct adouble *ads)
```

Defined at lines 281 to 337.

Transfer the header entries of one AppleDouble into another.

ad_copy_header checks every destination entry against the source length before writing anything. exchangefiles runs this on live open forks, so a failure must leave the header untouched. Between the version 2 and EA forms the PRIVID entry changes byte order.

Bug: Only entries of the same size, or the comment, transfer cleanly. Pairs of other AppleDouble versions do not work well.

Parameters:
* `add`: destination AppleDouble with valid data
* `ads`: source AppleDouble

Returns: 0, or -1 when an entry does not fit (errno EIO) or the destination has no valid data

Calls: [ad_copy_header_entry](ad_flush.c.md#ad_copy_header_entry), [ad_entry](ad_open.c.md#ad_entry), [ad_entry_fits](ad_open.c.md#ad_entry_fits), [ad_getentryoff](ad_open.c.md#ad_getentryoff)

Called by: [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [copy_source_header](../../bin/nad/nad_cp.c.md#copy_source_header), [copyfile](../../etc/afpd/file.c.md#copyfile)

### ad_flush_hf

```c
static int ad_flush_hf(struct adouble *ad)
```

Defined at lines 353 to 497.

Write the header of an AppleDouble to its metadata store.

The store must be open for writing, else nothing happens. The resource fork entry takes the current fork length, capped at 4 GiB minus one byte. Version 2 writes ad_data to the metadata file. The EA form stores it in the AD_EA_META extended attribute of the file, or of the directory through a temporary chdir. On macOS it also refreshes the native FinderInfo EA of an existing file. A sticky directory that refuses the EA with EPERM is an error unless the volume sets ADVOL_FORCE_STICKY_XATTR.

Parameters:
* `ad`: AppleDouble whose header entries are current

Returns: 0, or -1 with errno set

Calls: [ad_entry](ad_open.c.md#ad_entry), [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [adf_pwrite](ad_write.c.md#adf_pwrite), [adflags2logstr](ad_open.c.md#adflags2logstr), [become_root](../util/unix.c.md#become_root), [sys_fsetxattr](../vfs/extattr.c.md#sys_fsetxattr), [unbecome_root](../util/unix.c.md#unbecome_root)

Called by: [ad_flush](ad_flush.c.md#ad_flush)

Calls through [`adouble_fops::ad_rebuild_header`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_rebuild_adouble_header_ea](ad_flush.c.md#ad_rebuild_adouble_header_ea), [ad_rebuild_adouble_header_v2](ad_flush.c.md#ad_rebuild_adouble_header_v2)

### ad_flush_rf

```c
static int ad_flush_rf(struct adouble *ad)
```

Defined at lines 511 to 546.

Rewrite the ._ sidecar header of an ea AppleDouble with an open resource fork.

Only the ea layout without HAVE_EAFD keeps a sidecar header, so every other case returns at once. A writable resource fork gets its length entry from ad_rlen, capped at 0xffffffff. The ad_rebuild_adouble_header_osx call then builds the header, and the call writes it at offset 0.

Parameters:
* `ad`: AppleDouble whose sidecar header to write

Returns: 0, or -1 when the write fell short, with EIO when errno was 0

Calls: [ad_getentryoff](ad_open.c.md#ad_getentryoff), [ad_rebuild_adouble_header_osx](ad_flush.c.md#ad_rebuild_adouble_header_osx), [adf_pwrite](ad_write.c.md#adf_pwrite), [adflags2logstr](ad_open.c.md#adflags2logstr)

Called by: [ad_flush](ad_flush.c.md#ad_flush)

### ad_flush

```c
int ad_flush(struct adouble *ad)
```

Defined at lines 558 to 573.

Write the header and the resource fork metadata of an AppleDouble to disk.

The header goes out when the metadata store is open, the resource fork when it is open.

Parameters:
* `ad`: AppleDouble to flush

Returns: 0, or -1 with errno set

Calls: [ad_flush_hf](ad_flush.c.md#ad_flush_hf), [ad_flush_rf](ad_flush.c.md#ad_flush_rf), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [adflags2logstr](ad_open.c.md#adflags2logstr)

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_open_hf_ea](ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_setforkparams](../../etc/afpd/fork.c.md#afp_setforkparams), [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [copy](../../bin/nad/nad_cp.c.md#copy), [copyfile](../../etc/afpd/file.c.md#copyfile), [do_move](../../bin/nad/nad_mv.c.md#do_move), [flushfork](../../etc/afpd/fork.c.md#flushfork), [get_id](../../etc/afpd/file.c.md#get_id), [getvolparams](../../etc/afpd/volume.c.md#getvolparams), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_close](../../bin/nad/nad_adouble.c.md#nad_close), [nad_header_write](../../bin/nad/nad_adouble.c.md#nad_header_write), [nad_set](../../bin/nad/nad_set.c.md#nad_set), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [renamedir](../../etc/afpd/directory.c.md#renamedir), [renamefile](../../etc/afpd/file.c.md#renamefile), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

### ad_data_closefd

```c
static int ad_data_closefd(struct adouble *ad)
```

Defined at lines 582 to 597.

Close the data fork descriptor of an AppleDouble, or drop the symlink target it stands for.

Parameters:
* `ad`: AppleDouble whose data fork is open

Returns: 0, or -1 when close fails

Called by: [ad_close](ad_flush.c.md#ad_close)

### ad_close

```c
int ad_close(struct adouble *ad, int adflags)
```

Defined at lines 614 to 750.

Close the forks of an AppleDouble that adflags names and release their resources.

Each fork carries a reference count, and its descriptor closes when the count reaches zero. ADFLAGS_SETSHRMD and ADFLAGS_CHECK_OF also close the data fork, because the share-mode locks live there. For version 2 closing the resource fork also closes the metadata file, because opening the resource fork opens it. ad_close logs and asserts a count that is already zero.

Parameters:
* `ad`: AppleDouble to close, or NULL
* `adflags`: ADFLAGS_DF, ADFLAGS_HF, ADFLAGS_RF, and the modifiers above

Returns: 0, or -1 when a close failed

Calls: [ad_data_closefd](ad_flush.c.md#ad_data_closefd), [adf_lock_free](ad_lock.c.md#adf_lock_free), [adflags2logstr](ad_open.c.md#adflags2logstr)

Called by: [RF_renamefile_adouble](../vfs/vfs.c.md#rf_renamefile_adouble), [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_conv_v22ea_hf](ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](ad_conv.c.md#ad_conv_v22ea_rf), [ad_convert_osx](ad_open.c.md#ad_convert_osx), [ad_error](ad_open.c.md#ad_error), [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached), [ad_open_rf_ea](ad_open.c.md#ad_open_rf_ea), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [afp_setvolparams](../../etc/afpd/volume.c.md#afp_setvolparams), [check_addir](../../bin/dbd/cmd_dbd_scanvol.c.md#check_addir), [check_adfile](../../bin/dbd/cmd_dbd_scanvol.c.md#check_adfile), [check_cnid](../../bin/dbd/cmd_dbd_scanvol.c.md#check_cnid), [check_delete_inhibit](../../etc/afpd/file.c.md#check_delete_inhibit), [copy](../../bin/nad/nad_cp.c.md#copy), [copy_source_header](../../bin/nad/nad_cp.c.md#copy_source_header), [copyfile](../../etc/afpd/file.c.md#copyfile), [dir_add](../../etc/afpd/directory.c.md#dir_add), [do_move](../../bin/nad/nad_mv.c.md#do_move), [ea_copyfile](../vfs/ea_ad.c.md#ea_copyfile), [ea_renamefile](../vfs/ea_ad.c.md#ea_renamefile), [find_adouble](../../etc/afpd/file.c.md#find_adouble), [getvolparams](../../etc/afpd/volume.c.md#getvolparams), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_close](../../bin/nad/nad_adouble.c.md#nad_close), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open), [nad_set](../../bin/nad/nad_set.c.md#nad_set), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [print_flags](../../bin/nad/nad_ls.c.md#print_flags), [renamedir](../../etc/afpd/directory.c.md#renamedir), [renamefile](../../etc/afpd/file.c.md#renamefile), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [update_created_file_cnid](../../bin/nad/megatron.c.md#update_created_file_cnid)

# Macros

* Undocumented: `EID_DISK`

# File-scope variables

`set_eid`
