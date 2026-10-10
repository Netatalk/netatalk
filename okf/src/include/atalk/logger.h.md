---
type: C Header File
title: "include/atalk/logger.h"
description: "2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/logger.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `stdbool.h`, `stdio.h`

# Included by

* [bin/dbd/cmd_dbd.c](../../bin/dbd/cmd_dbd.c.md)
* [bin/misc/logger_test.c](../../bin/misc/logger_test.c.md)
* [bin/misc/uuidtest.c](../../bin/misc/uuidtest.c.md)
* [bin/nad/nad.c](../../bin/nad/nad.c.md)
* [bin/nad/nad_util.c](../../bin/nad/nad_util.c.md)
* [etc/afpd/acls.c](../../etc/afpd/acls.c.md)
* [etc/afpd/ad_cache.c](../../etc/afpd/ad_cache.c.md)
* [etc/afpd/afp_asp.c](../../etc/afpd/afp_asp.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/afp_dsi.c](../../etc/afpd/afp_dsi.c.md)
* [etc/afpd/afp_options.c](../../etc/afpd/afp_options.c.md)
* [etc/afpd/afprun.c](../../etc/afpd/afprun.c.md)
* [etc/afpd/afpstats.c](../../etc/afpd/afpstats.c.md)
* [etc/afpd/appl.c](../../etc/afpd/appl.c.md)
* [etc/afpd/auth.c](../../etc/afpd/auth.c.md)
* [etc/afpd/catsearch.c](../../etc/afpd/catsearch.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/afpd/dircache.c](../../etc/afpd/dircache.c.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/enumerate.c](../../etc/afpd/enumerate.c.md)
* [etc/afpd/extattrs.c](../../etc/afpd/extattrs.c.md)
* [etc/afpd/fce_api.c](../../etc/afpd/fce_api.c.md)
* [etc/afpd/fce_util.c](../../etc/afpd/fce_util.c.md)
* [etc/afpd/file.c](../../etc/afpd/file.c.md)
* [etc/afpd/filedir.c](../../etc/afpd/filedir.c.md)
* [etc/afpd/fork.c](../../etc/afpd/fork.c.md)
* [etc/afpd/idle_worker.c](../../etc/afpd/idle_worker.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/mangle.h](../../etc/afpd/mangle.h.md)
* [etc/afpd/messages.c](../../etc/afpd/messages.c.md)
* [etc/afpd/nfsquota.c](../../etc/afpd/nfsquota.c.md)
* [etc/afpd/ofork.c](../../etc/afpd/ofork.c.md)
* [etc/afpd/pfd_cache.c](../../etc/afpd/pfd_cache.c.md)
* [etc/afpd/quota.c](../../etc/afpd/quota.c.md)
* [etc/afpd/spotlight.c](../../etc/afpd/spotlight.c.md)
* [etc/afpd/spotlight_marshalling.c](../../etc/afpd/spotlight_marshalling.c.md)
* [etc/afpd/status.c](../../etc/afpd/status.c.md)
* [etc/afpd/switch.c](../../etc/afpd/switch.c.md)
* [etc/afpd/uam.c](../../etc/afpd/uam.c.md)
* [etc/afpd/unix.c](../../etc/afpd/unix.c.md)
* [etc/afpd/virtual_icon.c](../../etc/afpd/virtual_icon.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [etc/atalkd/aep.c](../../etc/atalkd/aep.c.md)
* [etc/atalkd/config.c](../../etc/atalkd/config.c.md)
* [etc/atalkd/main.c](../../etc/atalkd/main.c.md)
* [etc/atalkd/multicast.c](../../etc/atalkd/multicast.c.md)
* [etc/atalkd/nbp.c](../../etc/atalkd/nbp.c.md)
* [etc/atalkd/rtmp.c](../../etc/atalkd/rtmp.c.md)
* [etc/atalkd/zip.c](../../etc/atalkd/zip.c.md)
* [etc/netatalk/afp_avahi.c](../../etc/netatalk/afp_avahi.c.md)
* [etc/netatalk/afp_avahi.h](../../etc/netatalk/afp_avahi.h.md)
* [etc/netatalk/afp_mdns.c](../../etc/netatalk/afp_mdns.c.md)
* [etc/netatalk/afp_mdns.h](../../etc/netatalk/afp_mdns.h.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [etc/papd/auth.c](../../etc/papd/auth.c.md)
* [etc/papd/comment.c](../../etc/papd/comment.c.md)
* [etc/papd/file.c](../../etc/papd/file.c.md)
* [etc/papd/headers.c](../../etc/papd/headers.c.md)
* [etc/papd/lp.c](../../etc/papd/lp.c.md)
* [etc/papd/magics.c](../../etc/papd/magics.c.md)
* [etc/papd/main.c](../../etc/papd/main.c.md)
* [etc/papd/ppd.c](../../etc/papd/ppd.c.md)
* [etc/papd/print_cups.c](../../etc/papd/print_cups.c.md)
* [etc/papd/printcap.c](../../etc/papd/printcap.c.md)
* [etc/papd/queries.c](../../etc/papd/queries.c.md)
* [etc/papd/session.c](../../etc/papd/session.c.md)
* [etc/papd/uam.c](../../etc/papd/uam.c.md)
* [etc/spotlight/cnid/sl_cnid.c](../../etc/spotlight/cnid/sl_cnid.c.md)
* [etc/spotlight/localsearch/sl_localsearch.c](../../etc/spotlight/localsearch/sl_localsearch.c.md)
* [etc/spotlight/localsearch/sparql_map.c](../../etc/spotlight/localsearch/sparql_map.c.md)
* [etc/spotlight/localsearch/sparql_parser.y](../../etc/spotlight/localsearch/sparql_parser.y.md)
* [etc/spotlight/xapian/sl_xapian.c](../../etc/spotlight/xapian/sl_xapian.c.md)
* [etc/uams/uams_dhx2_pam.c](../../etc/uams/uams_dhx2_pam.c.md)
* [etc/uams/uams_dhx2_passwd.c](../../etc/uams/uams_dhx2_passwd.c.md)
* [etc/uams/uams_dhx_pam.c](../../etc/uams/uams_dhx_pam.c.md)
* [etc/uams/uams_dhx_passwd.c](../../etc/uams/uams_dhx_passwd.c.md)
* [etc/uams/uams_gss.c](../../etc/uams/uams_gss.c.md)
* [etc/uams/uams_guest.c](../../etc/uams/uams_guest.c.md)
* [etc/uams/uams_pam.c](../../etc/uams/uams_pam.c.md)
* [etc/uams/uams_passwd.c](../../etc/uams/uams_passwd.c.md)
* [etc/uams/uams_randnum.c](../../etc/uams/uams_randnum.c.md)
* [etc/uams/uams_srp.c](../../etc/uams/uams_srp.c.md)
* [include/atalk/uam.h](uam.h.md)
* [libatalk/acl/cache.c](../../libatalk/acl/cache.c.md)
* [libatalk/acl/ldap.c](../../libatalk/acl/ldap.c.md)
* [libatalk/acl/ldap_config.c](../../libatalk/acl/ldap_config.c.md)
* [libatalk/acl/unix.c](../../libatalk/acl/unix.c.md)
* [libatalk/acl/uuid.c](../../libatalk/acl/uuid.c.md)
* [libatalk/adouble/ad_attr.c](../../libatalk/adouble/ad_attr.c.md)
* [libatalk/adouble/ad_conv.c](../../libatalk/adouble/ad_conv.c.md)
* [libatalk/adouble/ad_date.c](../../libatalk/adouble/ad_date.c.md)
* [libatalk/adouble/ad_flush.c](../../libatalk/adouble/ad_flush.c.md)
* [libatalk/adouble/ad_lock.c](../../libatalk/adouble/ad_lock.c.md)
* [libatalk/adouble/ad_open.c](../../libatalk/adouble/ad_open.c.md)
* [libatalk/adouble/ad_read.c](../../libatalk/adouble/ad_read.c.md)
* [libatalk/adouble/ad_sendfile.c](../../libatalk/adouble/ad_sendfile.c.md)
* [libatalk/adouble/ad_size.c](../../libatalk/adouble/ad_size.c.md)
* [libatalk/adouble/ad_write.c](../../libatalk/adouble/ad_write.c.md)
* [libatalk/asp/asp_attn.c](../../libatalk/asp/asp_attn.c.md)
* [libatalk/asp/asp_getsess.c](../../libatalk/asp/asp_getsess.c.md)
* [libatalk/asp/asp_tickle.c](../../libatalk/asp/asp_tickle.c.md)
* [libatalk/cnid/cnid.c](../../libatalk/cnid/cnid.c.md)
* [libatalk/cnid/cnid_init.c](../../libatalk/cnid/cnid_init.c.md)
* [libatalk/cnid/mysql/cnid_mysql.c](../../libatalk/cnid/mysql/cnid_mysql.c.md)
* [libatalk/cnid/sqlite/cnid_sqlite.c](../../libatalk/cnid/sqlite/cnid_sqlite.c.md)
* [libatalk/dalloc/dalloc.c](../../libatalk/dalloc/dalloc.c.md)
* [libatalk/dsi/dsi_cmdreply.c](../../libatalk/dsi/dsi_cmdreply.c.md)
* [libatalk/dsi/dsi_getsess.c](../../libatalk/dsi/dsi_getsess.c.md)
* [libatalk/dsi/dsi_opensess.c](../../libatalk/dsi/dsi_opensess.c.md)
* [libatalk/dsi/dsi_read.c](../../libatalk/dsi/dsi_read.c.md)
* [libatalk/dsi/dsi_stream.c](../../libatalk/dsi/dsi_stream.c.md)
* [libatalk/dsi/dsi_tcp.c](../../libatalk/dsi/dsi_tcp.c.md)
* [libatalk/dsi/dsi_write.c](../../libatalk/dsi/dsi_write.c.md)
* [libatalk/unicode/charcnv.c](../../libatalk/unicode/charcnv.c.md)
* [libatalk/unicode/charsets/generic_mb.c](../../libatalk/unicode/charsets/generic_mb.c.md)
* [libatalk/unicode/charsets/mac_hebrew.c](../../libatalk/unicode/charsets/mac_hebrew.c.md)
* [libatalk/unicode/iconv.c](../../libatalk/unicode/iconv.c.md)
* [libatalk/unicode/utf8.c](../../libatalk/unicode/utf8.c.md)
* [libatalk/unicode/util_unistr.c](../../libatalk/unicode/util_unistr.c.md)
* [libatalk/util/cnid.c](../../libatalk/util/cnid.c.md)
* [libatalk/util/fault.c](../../libatalk/util/fault.c.md)
* [libatalk/util/logger.c](../../libatalk/util/logger.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)
* [libatalk/util/pathconv.c](../../libatalk/util/pathconv.c.md)
* [libatalk/util/server_child.c](../../libatalk/util/server_child.c.md)
* [libatalk/util/server_ipc.c](../../libatalk/util/server_ipc.c.md)
* [libatalk/util/socket.c](../../libatalk/util/socket.c.md)
* [libatalk/util/unix.c](../../libatalk/util/unix.c.md)
* [libatalk/vfs/acl.c](../../libatalk/vfs/acl.c.md)
* [libatalk/vfs/ea_ad.c](../../libatalk/vfs/ea_ad.c.md)
* [libatalk/vfs/ea_sys.c](../../libatalk/vfs/ea_sys.c.md)
* [libatalk/vfs/extattr.c](../../libatalk/vfs/extattr.c.md)
* [libatalk/vfs/unix.c](../../libatalk/vfs/unix.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)
* [sys/netatalk/ddp_input.c](../../sys/netatalk/ddp_input.c.md)
* [sys/netatalk/ddp_output.c](../../sys/netatalk/ddp_output.c.md)

# Types

### struct log_config_t

Defined at line 77.
* `bool inited`
* `bool syslog_opened`
* `bool console`
* `char processname`
* `int syslog_facility`
* `int syslog_display_options`

### struct logtype_conf_t

Defined at line 87.
* `bool set`
* `bool syslog`
* `int fd`
* `enum loglevels level`
* `int display_options`
* `bool timestamp_us`
* `char * filename`

# Typedefs and enums

* `enum loglevels`: `log_none`, `log_severe`, `log_error`, `log_warning`, `log_note`, `log_info`, `log_debug`, `log_debug6`, `log_debug7`, `log_debug8`, `log_debug9`, `log_maxdebug`
* `enum logtypes`: `logtype_default`, `logtype_logger`, `logtype_cnid`, `logtype_afpd`, `logtype_dsi`, `logtype_atalkd`, `logtype_papd`, `logtype_uams`, `logtype_fce`, `logtype_ad`, `logtype_sl`, `logtype_end_of_list_marker`

# Macros

* Undocumented: `LOG`, `LOG_MAX`, `UAM_MODULE_EXPORT`, `logfacility_auth`, `logfacility_authpriv`, `logfacility_daemon`, `logfacility_ftp`, `logfacility_lpr`, `logfacility_mail`, `logfacility_syslog`, `logfacility_user`, `logoption_cons`, `logoption_ndelay`, `logoption_nfile`, `logoption_nline`, `logoption_nsrcinfo`, `logoption_perror`, `logoption_pid`
