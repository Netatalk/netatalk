---
type: C Header File
title: "include/atalk/util.h"
description: "Netatalk utility functions."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/util.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](cnid.h.md)
* [atalk/unicode.h](unicode.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `bstrlib.h`, `poll.h`, `stdbool.h`, `sys/socket.h`, `sys/stat.h`, `sys/types.h`, `unistd.h`

# Included by

* [bin/aecho/aecho.c](../../bin/aecho/aecho.c.md)
* [bin/dbd/cmd_dbd.c](../../bin/dbd/cmd_dbd.c.md)
* [bin/dbd/cmd_dbd_scanvol.c](../../bin/dbd/cmd_dbd_scanvol.c.md)
* [bin/getzones/getzones.c](../../bin/getzones/getzones.c.md)
* [bin/misc/fce.c](../../bin/misc/fce.c.md)
* [bin/nad/ftw.c](../../bin/nad/ftw.c.md)
* [bin/nad/macbin.c](../../bin/nad/macbin.c.md)
* [bin/nad/megatron.c](../../bin/nad/megatron.c.md)
* [bin/nad/nad.c](../../bin/nad/nad.c.md)
* [bin/nad/nad_adouble.c](../../bin/nad/nad_adouble.c.md)
* [bin/nad/nad_cp.c](../../bin/nad/nad_cp.c.md)
* [bin/nad/nad_find.c](../../bin/nad/nad_find.c.md)
* [bin/nad/nad_mkdir.c](../../bin/nad/nad_mkdir.c.md)
* [bin/nad/nad_mv.c](../../bin/nad/nad_mv.c.md)
* [bin/nad/nad_rm.c](../../bin/nad/nad_rm.c.md)
* [bin/nad/nad_rmdir.c](../../bin/nad/nad_rmdir.c.md)
* [bin/nad/nad_stuffit.c](../../bin/nad/nad_stuffit.c.md)
* [bin/nad/nad_util.c](../../bin/nad/nad_util.c.md)
* [bin/nbp/nbplkup.c](../../bin/nbp/nbplkup.c.md)
* [bin/nbp/nbprgstr.c](../../bin/nbp/nbprgstr.c.md)
* [bin/nbp/nbpunrgstr.c](../../bin/nbp/nbpunrgstr.c.md)
* [bin/pap/pap.c](../../bin/pap/pap.c.md)
* [bin/pap/papstatus.c](../../bin/pap/papstatus.c.md)
* [bin/rtmpqry/rtmpqry.c](../../bin/rtmpqry/rtmpqry.c.md)
* [etc/afpd/acls.c](../../etc/afpd/acls.c.md)
* [etc/afpd/ad_cache.c](../../etc/afpd/ad_cache.c.md)
* [etc/afpd/afp_asp.c](../../etc/afpd/afp_asp.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/afp_dsi.c](../../etc/afpd/afp_dsi.c.md)
* [etc/afpd/afp_options.c](../../etc/afpd/afp_options.c.md)
* [etc/afpd/afprun.c](../../etc/afpd/afprun.c.md)
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
* [etc/afpd/mangle.c](../../etc/afpd/mangle.c.md)
* [etc/afpd/messages.c](../../etc/afpd/messages.c.md)
* [etc/afpd/ofork.c](../../etc/afpd/ofork.c.md)
* [etc/afpd/pfd_cache.c](../../etc/afpd/pfd_cache.c.md)
* [etc/afpd/quota.c](../../etc/afpd/quota.c.md)
* [etc/afpd/spotlight.c](../../etc/afpd/spotlight.c.md)
* [etc/afpd/spotlight_marshalling.c](../../etc/afpd/spotlight_marshalling.c.md)
* [etc/afpd/status.c](../../etc/afpd/status.c.md)
* [etc/afpd/uam.c](../../etc/afpd/uam.c.md)
* [etc/afpd/unix.c](../../etc/afpd/unix.c.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [etc/atalkd/config.c](../../etc/atalkd/config.c.md)
* [etc/atalkd/main.c](../../etc/atalkd/main.c.md)
* [etc/atalkd/multicast.c](../../etc/atalkd/multicast.c.md)
* [etc/atalkd/nbp.c](../../etc/atalkd/nbp.c.md)
* [etc/atalkd/zip.c](../../etc/atalkd/zip.c.md)
* [etc/netatalk/afp_avahi.c](../../etc/netatalk/afp_avahi.c.md)
* [etc/netatalk/afp_mdns.c](../../etc/netatalk/afp_mdns.c.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [etc/papd/auth.c](../../etc/papd/auth.c.md)
* [etc/papd/main.c](../../etc/papd/main.c.md)
* [etc/papd/print_cups.c](../../etc/papd/print_cups.c.md)
* [etc/papd/uam.c](../../etc/papd/uam.c.md)
* [etc/spotlight/cnid/sl_cnid.c](../../etc/spotlight/cnid/sl_cnid.c.md)
* [etc/spotlight/localsearch/sl_localsearch.c](../../etc/spotlight/localsearch/sl_localsearch.c.md)
* [etc/spotlight/xapian/sl_xapian.c](../../etc/spotlight/xapian/sl_xapian.c.md)
* [etc/uams/uams_gss.c](../../etc/uams/uams_gss.c.md)
* [etc/uams/uams_guest.c](../../etc/uams/uams_guest.c.md)
* [etc/uams/uams_pam.c](../../etc/uams/uams_pam.c.md)
* [etc/uams/uams_passwd.c](../../etc/uams/uams_passwd.c.md)
* [libatalk/acl/unix.c](../../libatalk/acl/unix.c.md)
* [libatalk/acl/uuid.c](../../libatalk/acl/uuid.c.md)
* [libatalk/adouble/ad_attr.c](../../libatalk/adouble/ad_attr.c.md)
* [libatalk/adouble/ad_conv.c](../../libatalk/adouble/ad_conv.c.md)
* [libatalk/adouble/ad_flush.c](../../libatalk/adouble/ad_flush.c.md)
* [libatalk/adouble/ad_lock.c](../../libatalk/adouble/ad_lock.c.md)
* [libatalk/adouble/ad_open.c](../../libatalk/adouble/ad_open.c.md)
* [libatalk/adouble/ad_read.c](../../libatalk/adouble/ad_read.c.md)
* [libatalk/adouble/ad_write.c](../../libatalk/adouble/ad_write.c.md)
* [libatalk/asp/asp_getsess.c](../../libatalk/asp/asp_getsess.c.md)
* [libatalk/atp/atp_packet.c](../../libatalk/atp/atp_packet.c.md)
* [libatalk/atp/atp_rresp.c](../../libatalk/atp/atp_rresp.c.md)
* [libatalk/atp/atp_rsel.c](../../libatalk/atp/atp_rsel.c.md)
* [libatalk/atp/atp_sreq.c](../../libatalk/atp/atp_sreq.c.md)
* [libatalk/atp/atp_sresp.c](../../libatalk/atp/atp_sresp.c.md)
* [libatalk/cnid/mysql/cnid_mysql.c](../../libatalk/cnid/mysql/cnid_mysql.c.md)
* [libatalk/cnid/sqlite/cnid_sqlite.c](../../libatalk/cnid/sqlite/cnid_sqlite.c.md)
* [libatalk/compat/strlcpy.c](../../libatalk/compat/strlcpy.c.md)
* [libatalk/dalloc/dalloc.c](../../libatalk/dalloc/dalloc.c.md)
* [libatalk/dsi/dsi_attn.c](../../libatalk/dsi/dsi_attn.c.md)
* [libatalk/dsi/dsi_getsess.c](../../libatalk/dsi/dsi_getsess.c.md)
* [libatalk/dsi/dsi_opensess.c](../../libatalk/dsi/dsi_opensess.c.md)
* [libatalk/dsi/dsi_read.c](../../libatalk/dsi/dsi_read.c.md)
* [libatalk/dsi/dsi_stream.c](../../libatalk/dsi/dsi_stream.c.md)
* [libatalk/dsi/dsi_tcp.c](../../libatalk/dsi/dsi_tcp.c.md)
* [libatalk/dsi/dsi_write.c](../../libatalk/dsi/dsi_write.c.md)
* [libatalk/nbp/nbp_util.c](../../libatalk/nbp/nbp_util.c.md)
* [libatalk/unicode/charcnv.c](../../libatalk/unicode/charcnv.c.md)
* [libatalk/unicode/iconv.c](../../libatalk/unicode/iconv.c.md)
* [libatalk/util/atalk_addr.c](../../libatalk/util/atalk_addr.c.md)
* [libatalk/util/bprint.c](../../libatalk/util/bprint.c.md)
* [libatalk/util/cnid.c](../../libatalk/util/cnid.c.md)
* [libatalk/util/fault.c](../../libatalk/util/fault.c.md)
* [libatalk/util/getiface.c](../../libatalk/util/getiface.c.md)
* [libatalk/util/locking.c](../../libatalk/util/locking.c.md)
* [libatalk/util/logger.c](../../libatalk/util/logger.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)
* [libatalk/util/pathconv.c](../../libatalk/util/pathconv.c.md)
* [libatalk/util/server_child.c](../../libatalk/util/server_child.c.md)
* [libatalk/util/server_ipc.c](../../libatalk/util/server_ipc.c.md)
* [libatalk/util/server_lock.c](../../libatalk/util/server_lock.c.md)
* [libatalk/util/sigpipe.c](../../libatalk/util/sigpipe.c.md)
* [libatalk/util/socket.c](../../libatalk/util/socket.c.md)
* [libatalk/util/strdicasecmp.c](../../libatalk/util/strdicasecmp.c.md)
* [libatalk/util/unix.c](../../libatalk/util/unix.c.md)
* [libatalk/vfs/acl.c](../../libatalk/vfs/acl.c.md)
* [libatalk/vfs/ea_ad.c](../../libatalk/vfs/ea_ad.c.md)
* [libatalk/vfs/ea_sys.c](../../libatalk/vfs/ea_sys.c.md)
* [libatalk/vfs/extattr.c](../../libatalk/vfs/extattr.c.md)
* [libatalk/vfs/unix.c](../../libatalk/vfs/unix.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Functions

### mod_open

```c
void * mod_open(const char *)
```

Declared at include/atalk/util.h line 109; no definition in the scanned sources.

Called by: [uam_load](../../etc/afpd/uam.c.md#uam_load), [uam_load](../../etc/papd/uam.c.md#uam_load)

### mod_symbol

```c
void * mod_symbol(void *, const char *)
```

Declared at include/atalk/util.h line 110; no definition in the scanned sources.

Called by: [uam_load](../../etc/afpd/uam.c.md#uam_load), [uam_load](../../etc/papd/uam.c.md#uam_load)

### mod_close

```c
void mod_close(void *)
```

Declared at include/atalk/util.h line 111; no definition in the scanned sources.

Called by: [uam_load](../../etc/afpd/uam.c.md#uam_load), [uam_load](../../etc/papd/uam.c.md#uam_load), [uam_unload](../../etc/afpd/uam.c.md#uam_unload), [uam_unload](../../etc/papd/uam.c.md#uam_unload)

# Types

### struct asev

atalk socket event

Defined at line 194.
* `struct pollfd * fdset`
* `struct asev_data * data`
* `int max`
* `int used`

### struct asev_data

atalk socket event data

Defined at line 184.
* `enum asev_fdtype fdtype`
* `void * private`
* `int protocol`

# Typedefs and enums

* `enum asev_fdtype`: `IPC_FD`, `LISTEN_FD`, `STATS_FD`

# Macros

* `read_lock`: place read lock on file
* `unlock`: unlock a file
* `write_lock`: place write lock on file
* Undocumented: `AFP_ASSERT`, `AFP_PANIC`, `BSTRING_STRIP_SLASH`, `EXITERR_CLNT`, `EXITERR_CLOSED`, `EXITERR_CONF`, `EXITERR_SYS`, `MAX`, `MIN`, `RLIM_MAX`, `SAFE_FREE`, `STRCMP`, `ZERO_STRUCT`, `ZERO_STRUCTP`, `cfrombstr`, `diatolower`, `diatoupper`, `hton64`, `mod_error`, `server_unlock`, `strequal`

# File-scope variables

`_dialowermap`
