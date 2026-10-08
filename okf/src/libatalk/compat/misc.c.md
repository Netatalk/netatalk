---
type: C Source File
title: "libatalk/compat/misc.c"
description: "3 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/compat/misc.c"
tags: ["libatalk/compat"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/compat](../compat.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* System headers: `stdarg.h`, `stdio.h`

# Functions

### strnlen

```c
size_t strnlen(const char *s, size_t n)
```

Defined at lines 19 to 30. Declared in [include/atalk/compat.h](../../include/atalk/compat.h.md).

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_path_osx](../adouble/ad_open.c.md#ad_path_osx), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [add_filemeta](../../etc/afpd/spotlight.c.md#add_filemeta), [adl_lkup](../../etc/afpd/catsearch.c.md#adl_lkup), [afp_createdir](../../etc/afpd/directory.c.md#afp_createdir), [afp_createfile](../../etc/afpd/file.c.md#afp_createfile), [afp_createid](../../etc/afpd/file.c.md#afp_createid), [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_rmvappl](../../etc/afpd/appl.c.md#afp_rmvappl), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [afp_setfilparams](../../etc/afpd/file.c.md#afp_setfilparams), [bin_header_write](../../bin/nad/macbin.c.md#bin_header_write), [build_fce_packet](../../etc/afpd/fce_api.c.md#build_fce_packet), [catsearch](../../etc/afpd/catsearch.c.md#catsearch), [cname](../../etc/afpd/directory.c.md#cname), [collect_new_password](../../bin/afppasswd/afppasswd.c.md#collect_new_password), [configinit](../../etc/afpd/afp_config.c.md#configinit), [convert_utf8_to_mac](../util/pathconv.c.md#convert_utf8_to_mac), [create_file](../../bin/afppasswd/afppasswd.c.md#create_file), [create_srp_directory](../../bin/afppasswd/afppasswd.c.md#create_srp_directory), [creatvol](../util/netatalk_conf.c.md#creatvol), [crit_check](../../etc/afpd/catsearch.c.md#crit_check), [cups_mangle_printer_name](../../etc/papd/print_cups.c.md#cups_mangle_printer_name), [deletefile](../../etc/afpd/file.c.md#deletefile), [dir_add](../../etc/afpd/directory.c.md#dir_add), [disable_srp_verifier](../../bin/afppasswd/afppasswd.c.md#disable_srp_verifier), [do_getnetinfo](../../bin/getzones/getzones.c.md#do_getnetinfo), [enumerate_loop](../../etc/afpd/enumerate.c.md#enumerate_loop), [fce_init_ign_paths](../../etc/afpd/fce_api.c.md#fce_init_ign_paths), [ftw_dir](../../bin/nad/ftw.c.md#ftw_dir), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [hqx_header_write](../../bin/nad/hqx.c.md#hqx_header_write), [login](../../etc/afpd/auth.c.md#login), [main](../../bin/afppasswd/afppasswd.c.md#main), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [nad_header_read](../../bin/nad/nad_adouble.c.md#nad_header_read), [nad_header_write](../../bin/nad/nad_adouble.c.md#nad_header_write), [nad_mv](../../bin/nad/nad_mv.c.md#nad_mv), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [path_resolve_cached_file](../../etc/afpd/file.c.md#path_resolve_cached_file), [print_columns](../../bin/nad/nad_ls.c.md#print_columns), [randnum_make_keypath](../../bin/afppasswd/afppasswd.c.md#randnum_make_keypath), [read_fork](../../etc/afpd/fork.c.md#read_fork), [rfork_invalidate_for_ofork](../../etc/afpd/fork.c.md#rfork_invalidate_for_ofork), [setfilparams](../../etc/afpd/file.c.md#setfilparams), [sl_cnid_collect](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_collect), [sl_cnid_extract_terms](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_extract_terms), [sl_cnid_quoted_value](../../etc/spotlight/cnid/sl_cnid.c.md#sl_cnid_quoted_value), [sl_pack_string](../../etc/afpd/spotlight_marshalling.c.md#sl_pack_string), [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery), [sl_xapian_db_id_safe](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_db_id_safe), [srp_compute_verifier](../../bin/afppasswd/afppasswd.c.md#srp_compute_verifier), [srp_logincont](../../etc/uams/uams_srp.c.md#srp_logincont), [srp_lookup_verifier](../../etc/uams/uams_srp.c.md#srp_lookup_verifier), [srp_valid_username](../../include/atalk/srp.h.md#srp_valid_username), [uam_afpserver_option](../../etc/afpd/uam.c.md#uam_afpserver_option), [update_passwd](../../bin/afppasswd/afppasswd.c.md#update_passwd), [update_srp_passwd](../../bin/afppasswd/afppasswd.c.md#update_srp_passwd)

Mentioned in the documentation of: [strnlen_w](../unicode/util_unistr.c.md#strnlen_w)

### vasprintf

```c
int vasprintf(char **ret, const char *fmt, va_list ap)
```

Defined at lines 34 to 72. Declared in [include/atalk/compat.h](../../include/atalk/compat.h.md).

Called by: [TXTRecordKeyPrintf](../../etc/netatalk/afp_mdns.c.md#txtrecordkeyprintf), [TXTRecordPrintf](../../etc/netatalk/afp_mdns.c.md#txtrecordprintf), [asprintf](misc.c.md#asprintf), [make_log_entry](../util/logger.c.md#make_log_entry)

### asprintf

```c
int asprintf(char **strp, const char *fmt,...)
```

Defined at lines 76 to 84.

Calls: [vasprintf](misc.c.md#vasprintf)

Called by: [TXTRecordKeyPrintf](../../etc/netatalk/afp_mdns.c.md#txtrecordkeyprintf), [cnid_mysql_add](../cnid/mysql/cnid_mysql.c.md#cnid_mysql_add), [cnid_mysql_find](../cnid/mysql/cnid_mysql.c.md#cnid_mysql_find), [cnid_mysql_getstamp](../cnid/mysql/cnid_mysql.c.md#cnid_mysql_getstamp), [cnid_mysql_open](../cnid/mysql/cnid_mysql.c.md#cnid_mysql_open), [cnid_mysql_wipe](../cnid/mysql/cnid_mysql.c.md#cnid_mysql_wipe), [cnid_sqlite_add](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_add), [cnid_sqlite_find](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_find), [cnid_sqlite_open](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_seed_sequence](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_seed_sequence), [cnid_sqlite_wipe](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_wipe), [generate_message](../util/logger.c.md#generate_message), [init_prepared_stmt_add](../cnid/mysql/cnid_mysql.c.md#init_prepared_stmt_add), [init_prepared_stmt_delete](../cnid/mysql/cnid_mysql.c.md#init_prepared_stmt_delete), [init_prepared_stmt_get](../cnid/mysql/cnid_mysql.c.md#init_prepared_stmt_get), [init_prepared_stmt_lookup](../cnid/mysql/cnid_mysql.c.md#init_prepared_stmt_lookup), [init_prepared_stmt_one](../cnid/sqlite/cnid_sqlite.c.md#init_prepared_stmt_one), [init_prepared_stmt_purge](../cnid/mysql/cnid_mysql.c.md#init_prepared_stmt_purge), [init_prepared_stmt_put](../cnid/mysql/cnid_mysql.c.md#init_prepared_stmt_put), [init_prepared_stmt_resolve](../cnid/mysql/cnid_mysql.c.md#init_prepared_stmt_resolve), [log_absolute_name](../util/logger.c.md#log_absolute_name)
