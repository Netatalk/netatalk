---
type: C Source File
title: "libatalk/compat/strlcpy.c"
description: "2 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/compat/strlcpy.c"
tags: ["libatalk/compat"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/compat](../compat.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `string.h`

# Functions

### strlcpy

```c
size_t strlcpy(char *, const char *, size_t)
```

Defined at lines 36 to 52. Declared in [include/atalk/compat.h](../../include/atalk/compat.h.md).

like strncpy but does not 0 fill the buffer and always null terminates. bufsize is the size of the destination buffer

Called by: [RF_copyfile_adouble](../vfs/vfs.c.md#rf_copyfile_adouble), [RF_copyfile_ea](../vfs/vfs.c.md#rf_copyfile_ea), [ad_conv_dehex](../adouble/ad_conv.c.md#ad_conv_dehex), [ad_path](../adouble/ad_open.c.md#ad_path), [ad_path_osx](../adouble/ad_open.c.md#ad_path_osx), [afp_addappl](../../etc/afpd/appl.c.md#afp_addappl), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [afp_rename](../../etc/afpd/filedir.c.md#afp_rename), [afp_rmvappl](../../etc/afpd/appl.c.md#afp_rmvappl), [afp_spotlight_rpc](../../etc/afpd/spotlight.c.md#afp_spotlight_rpc), [afppasswd_open_keyfile](../../etc/uams/uams_randnum.c.md#afppasswd_open_keyfile), [afpstats_init](../../etc/afpd/afpstats.c.md#afpstats_init), [auth_load](../../etc/afpd/auth.c.md#auth_load), [auth_load](../../etc/papd/auth.c.md#auth_load), [basename_safe](../util/unix.c.md#basename_safe), [bin_open](../../bin/nad/macbin.c.md#bin_open), [cmd_dbd_scanvol](../../bin/dbd/cmd_dbd_scanvol.c.md#cmd_dbd_scanvol), [cname_mtouname](../../etc/afpd/directory.c.md#cname_mtouname), [cnid_sqlite_dir_owner_only](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_dir_owner_only), [cnid_sqlite_getstamp](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_getstamp), [cnid_sqlite_lookup](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_lookup), [cnid_sqlite_resolve](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_resolve), [collect_new_password](../../bin/afppasswd/afppasswd.c.md#collect_new_password), [convert_dots_encoding](../../bin/nad/nad_util.c.md#convert_dots_encoding), [copy](../../bin/nad/nad_cp.c.md#copy), [create_file](../../bin/afppasswd/afppasswd.c.md#create_file), [create_private_srp_verifier](../../bin/afppasswd/afppasswd.c.md#create_private_srp_verifier), [creatvol](../util/netatalk_conf.c.md#creatvol), [cups_print_job](../../etc/papd/print_cups.c.md#cups_print_job), [dbpath_is_creatable](../../etc/netatalk/netatalk.c.md#dbpath_is_creatable), [do_mkdir](../../bin/nad/nad_mkdir.c.md#do_mkdir), [do_rmdir](../../bin/nad/nad_rmdir.c.md#do_rmdir), [ea_path](../vfs/ea_ad.c.md#ea_path), [extract_file_entry](../../bin/nad/nad_stuffit.c.md#extract_file_entry), [fce_init_ign_paths](../../etc/afpd/fce_api.c.md#fce_init_ign_paths), [fill_header_from_entry](../../bin/nad/nad_stuffit.c.md#fill_header_from_entry), [for_each_adouble](../vfs/vfs.c.md#for_each_adouble), [fullpathname](../util/unix.c.md#fullpathname), [get_client_username](../../etc/uams/uams_gss.c.md#get_client_username), [getifconf](../../etc/atalkd/config.c.md#getifconf), [getvolbypath](../util/netatalk_conf.c.md#getvolbypath), [guess_interface](../dsi/dsi_tcp.c.md#guess_interface), [hostaccessvol](../util/netatalk_conf.c.md#hostaccessvol), [hqx_open](../../bin/nad/hqx.c.md#hqx_open), [ifconfig](../../etc/atalkd/main.c.md#ifconfig), [main](../../bin/dbd/cmd_dbd.c.md#main), [make_dir_path](../../bin/nad/nad_stuffit.c.md#make_dir_path), [make_parent_dirs](../../bin/nad/nad_stuffit.c.md#make_parent_dirs), [mangle](../../etc/afpd/mangle.c.md#mangle), [mkdir_with_cnid](../../bin/nad/nad_mkdir.c.md#mkdir_with_cnid), [mtoupath](../../etc/afpd/desktop.c.md#mtoupath), [mystpcpy](../../bin/nad/ftw.c.md#mystpcpy), [nad_adouble_dir](../../bin/nad/nad_adouble.c.md#nad_adouble_dir), [nad_archive_convert](../../bin/nad/megatron.c.md#nad_archive_convert), [nad_cp](../../bin/nad/nad_cp.c.md#nad_cp), [nad_header_read](../../bin/nad/nad_adouble.c.md#nad_header_read), [nad_mv](../../bin/nad/nad_mv.c.md#nad_mv), [nad_open](../../bin/nad/nad_adouble.c.md#nad_open), [newiface](../../etc/atalkd/config.c.md#newiface), [noauth_printer](../../etc/uams/uams_guest.c.md#noauth_printer), [of_close_inode_forks](../../etc/afpd/ofork.c.md#of_close_inode_forks), [owned_private_dir](../../etc/netatalk/netatalk.c.md#owned_private_dir), [pam_printer](../../etc/uams/uams_pam.c.md#pam_printer), [passwd_printer](../../etc/uams/uams_passwd.c.md#passwd_printer), [path_parent](../../etc/netatalk/netatalk.c.md#path_parent), [prefix](../vfs/extattr.c.md#prefix), [randnum_make_keypath](../../bin/afppasswd/afppasswd.c.md#randnum_make_keypath), [readconf](../../etc/atalkd/config.c.md#readconf), [readvolfile](../util/netatalk_conf.c.md#readvolfile), [remove_eafiles](../../bin/dbd/cmd_dbd_scanvol.c.md#remove_eafiles), [set_newname](../../bin/nad/megatron.c.md#set_newname), [setmessage](../../etc/afpd/messages.c.md#setmessage), [split_parent_base](../../bin/nad/nad_stuffit.c.md#split_parent_base), [split_path](../../bin/afppasswd/afppasswd_migrate.c.md#split_path), [srp_is_the_only_uam](../../etc/netatalk/netatalk.c.md#srp_is_the_only_uam), [srp_login](../../etc/uams/uams_srp.c.md#srp_login), [srp_login_ext](../../etc/uams/uams_srp.c.md#srp_login_ext), [symlink_target_safe](../../etc/afpd/file.c.md#symlink_target_safe), [tnchktc](../../etc/papd/printcap.c.md#tnchktc), [to_stringz](../../etc/afpd/extattrs.c.md#to_stringz), [uam_getname](../../etc/afpd/uam.c.md#uam_getname), [uam_load](../../etc/afpd/uam.c.md#uam_load), [uam_load](../../etc/papd/uam.c.md#uam_load), [update_passwd](../../bin/afppasswd/afppasswd.c.md#update_passwd), [volxlate](../util/netatalk_conf.c.md#volxlate)

### strlcat

```c
size_t strlcat(char *, const char *, size_t)
```

Defined at lines 59 to 79. Declared in [include/atalk/compat.h](../../include/atalk/compat.h.md).

like strncat but does not 0 fill the buffer and always null terminates. bufsize is the length of the buffer, which should be one more than the maximum resulting string length

Called by: [ad_ls_r](../../bin/nad/nad_ls.c.md#ad_ls_r), [ad_path_osx](../adouble/ad_open.c.md#ad_path_osx), [adflags2logstr](../adouble/ad_open.c.md#adflags2logstr), [afp_getsrvrmesg](../../etc/afpd/messages.c.md#afp_getsrvrmesg), [afppasswd_open_keyfile](../../etc/uams/uams_randnum.c.md#afppasswd_open_keyfile), [auth_load](../../etc/afpd/auth.c.md#auth_load), [bin_open](../../bin/nad/macbin.c.md#bin_open), [create_file](../../bin/afppasswd/afppasswd.c.md#create_file), [cups_get_printer_status](../../etc/papd/print_cups.c.md#cups_get_printer_status), [cups_print_job](../../etc/papd/print_cups.c.md#cups_print_job), [dbd_readdir](../../bin/dbd/cmd_dbd_scanvol.c.md#dbd_readdir), [dtfile](../../etc/afpd/desktop.c.md#dtfile), [ea_path](../vfs/ea_ad.c.md#ea_path), [for_each_adouble](../vfs/vfs.c.md#for_each_adouble), [fullpathname](../util/unix.c.md#fullpathname), [getvolbypath](../util/netatalk_conf.c.md#getvolbypath), [hqx_open](../../bin/nad/hqx.c.md#hqx_open), [locktypetostr](../adouble/ad_lock.c.md#locktypetostr), [mangle](../../etc/afpd/mangle.c.md#mangle), [openflags2logstr](../adouble/ad_open.c.md#openflags2logstr), [randnum_make_keypath](../../bin/afppasswd/afppasswd.c.md#randnum_make_keypath), [readvolfile](../util/netatalk_conf.c.md#readvolfile), [remove_eafiles](../../bin/dbd/cmd_dbd_scanvol.c.md#remove_eafiles), [update_passwd](../../bin/afppasswd/afppasswd.c.md#update_passwd)
