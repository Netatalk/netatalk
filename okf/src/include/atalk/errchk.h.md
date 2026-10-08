---
type: C Header File
title: "include/atalk/errchk.h"
description: "Error checking macros."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/errchk.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Included by

* [bin/dbd/cmd_dbd.c](../../bin/dbd/cmd_dbd.c.md)
* [bin/dbd/cmd_dbd_scanvol.c](../../bin/dbd/cmd_dbd_scanvol.c.md)
* [bin/nad/nad_util.c](../../bin/nad/nad_util.c.md)
* [etc/afpd/acls.c](../../etc/afpd/acls.c.md)
* [etc/afpd/ad_cache.c](../../etc/afpd/ad_cache.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/afp_options.c](../../etc/afpd/afp_options.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/filedir.c](../../etc/afpd/filedir.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/spotlight.c](../../etc/afpd/spotlight.c.md)
* [etc/afpd/spotlight_marshalling.c](../../etc/afpd/spotlight_marshalling.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [etc/spotlight/cnid/sl_cnid.c](../../etc/spotlight/cnid/sl_cnid.c.md)
* [etc/spotlight/localsearch/sl_localsearch.c](../../etc/spotlight/localsearch/sl_localsearch.c.md)
* [etc/spotlight/localsearch/sparql_parser.y](../../etc/spotlight/localsearch/sparql_parser.y.md)
* [etc/spotlight/xapian/sl_xapian.c](../../etc/spotlight/xapian/sl_xapian.c.md)
* [libatalk/acl/ldap.c](../../libatalk/acl/ldap.c.md)
* [libatalk/adouble/ad_attr.c](../../libatalk/adouble/ad_attr.c.md)
* [libatalk/adouble/ad_conv.c](../../libatalk/adouble/ad_conv.c.md)
* [libatalk/adouble/ad_flush.c](../../libatalk/adouble/ad_flush.c.md)
* [libatalk/adouble/ad_lock.c](../../libatalk/adouble/ad_lock.c.md)
* [libatalk/adouble/ad_open.c](../../libatalk/adouble/ad_open.c.md)
* [libatalk/adouble/ad_write.c](../../libatalk/adouble/ad_write.c.md)
* [libatalk/cnid/mysql/cnid_mysql.c](../../libatalk/cnid/mysql/cnid_mysql.c.md)
* [libatalk/cnid/sqlite/cnid_sqlite.c](../../libatalk/cnid/sqlite/cnid_sqlite.c.md)
* [libatalk/dalloc/dalloc.c](../../libatalk/dalloc/dalloc.c.md)
* [libatalk/dsi/dsi_tcp.c](../../libatalk/dsi/dsi_tcp.c.md)
* [libatalk/util/cnid.c](../../libatalk/util/cnid.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)
* [libatalk/util/server_child.c](../../libatalk/util/server_child.c.md)
* [libatalk/util/server_ipc.c](../../libatalk/util/server_ipc.c.md)
* [libatalk/util/socket.c](../../libatalk/util/socket.c.md)
* [libatalk/vfs/acl.c](../../libatalk/vfs/acl.c.md)
* [libatalk/vfs/extattr.c](../../libatalk/vfs/extattr.c.md)
* [libatalk/vfs/unix.c](../../libatalk/vfs/unix.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Macros

* Undocumented: `EC_CLEANUP`, `EC_EXIT`, `EC_EXIT_STATUS`, `EC_FAIL`, `EC_FAIL_LOG`, `EC_INIT`, `EC_NEG1`, `EC_NEG1_LOG`, `EC_NEG1_LOGSTR`, `EC_NEG1_LOG_ERR`, `EC_NULL`, `EC_NULL_LOG`, `EC_NULL_LOGSTR`, `EC_NULL_LOG_ERR`, `EC_STATUS`, `EC_ZERO`, `EC_ZERO_ERR`, `EC_ZERO_LOG`, `EC_ZERO_LOGSTR`, `EC_ZERO_LOG_ERR`
