---
type: C Header File
title: "include/atalk/compat.h"
description: "4 functions."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/compat.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `signal.h`, `stdarg.h`, `stddef.h`, `stdio.h`, `sys/stat.h`, `sys/time.h`, `time.h`

# Included by

* [bin/aecho/aecho.c](../../bin/aecho/aecho.c.md)
* [bin/afppasswd/afppasswd.c](../../bin/afppasswd/afppasswd.c.md)
* [bin/afppasswd/afppasswd_migrate.c](../../bin/afppasswd/afppasswd_migrate.c.md)
* [bin/dbd/cmd_dbd_scanvol.c](../../bin/dbd/cmd_dbd_scanvol.c.md)
* [bin/nad/ftw.c](../../bin/nad/ftw.c.md)
* [bin/nad/megatron.h](../../bin/nad/megatron.h.md)
* [bin/nad/nad.h](../../bin/nad/nad.h.md)
* [etc/afpd/afp_asp.c](../../etc/afpd/afp_asp.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/afp_dsi.c](../../etc/afpd/afp_dsi.c.md)
* [etc/afpd/afp_options.c](../../etc/afpd/afp_options.c.md)
* [etc/afpd/afpstats.c](../../etc/afpd/afpstats.c.md)
* [etc/afpd/auth.c](../../etc/afpd/auth.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/quota.c](../../etc/afpd/quota.c.md)
* [etc/afpd/spotlight.c](../../etc/afpd/spotlight.c.md)
* [etc/atalkd/config.c](../../etc/atalkd/config.c.md)
* [etc/atalkd/main.c](../../etc/atalkd/main.c.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [etc/papd/auth.c](../../etc/papd/auth.c.md)
* [etc/papd/main.c](../../etc/papd/main.c.md)
* [etc/papd/print_cups.c](../../etc/papd/print_cups.c.md)
* [etc/papd/printcap.c](../../etc/papd/printcap.c.md)
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
* [include/atalk/globals.h](globals.h.md)
* [libatalk/adouble/ad_conv.c](../../libatalk/adouble/ad_conv.c.md)
* [libatalk/adouble/ad_lock.c](../../libatalk/adouble/ad_lock.c.md)
* [libatalk/adouble/ad_open.c](../../libatalk/adouble/ad_open.c.md)
* [libatalk/asp/asp_getsess.c](../../libatalk/asp/asp_getsess.c.md)
* [libatalk/atp/atp_rsel.c](../../libatalk/atp/atp_rsel.c.md)
* [libatalk/compat/misc.c](../../libatalk/compat/misc.c.md)
* [libatalk/dsi/dsi_tcp.c](../../libatalk/dsi/dsi_tcp.c.md)
* [libatalk/nbp/nbp_lkup.c](../../libatalk/nbp/nbp_lkup.c.md)
* [libatalk/unicode/charcnv.c](../../libatalk/unicode/charcnv.c.md)
* [libatalk/util/logger.c](../../libatalk/util/logger.c.md)
* [libatalk/util/server_child.c](../../libatalk/util/server_child.c.md)
* [libatalk/util/server_lock.c](../../libatalk/util/server_lock.c.md)
* [libatalk/util/unix.c](../../libatalk/util/unix.c.md)
* [libatalk/vfs/ea_ad.c](../../libatalk/vfs/ea_ad.c.md)
* [libatalk/vfs/ea_sys.c](../../libatalk/vfs/ea_sys.c.md)
* [libatalk/vfs/extattr.c](../../libatalk/vfs/extattr.c.md)
* [libatalk/vfs/unix.c](../../libatalk/vfs/unix.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Functions

### pselect

```c
int pselect(int, fd_set *, fd_set *, fd_set *, const struct timespec *, const sigset_t *)
```

Declared at include/atalk/compat.h line 24; no definition in the scanned sources.

### atalk_stat_mtime_timespec

```c
static struct timespec atalk_stat_mtime_timespec(const struct stat *st)
```

Defined at lines 55 to 67.

Called by: [add_filemeta](../../etc/afpd/spotlight.c.md#add_filemeta), [setfile](../../bin/nad/nad_cp.c.md#setfile)

### atalk_stat_atime_timespec

```c
static struct timespec atalk_stat_atime_timespec(const struct stat *st)
```

Defined at lines 69 to 81.

Called by: [add_filemeta](../../etc/afpd/spotlight.c.md#add_filemeta), [setfile](../../bin/nad/nad_cp.c.md#setfile)

### atalk_timespec_to_timeval

```c
static void atalk_timespec_to_timeval(struct timeval *tv, const struct timespec *ts)
```

Defined at lines 83 to 88.

Called by: [add_filemeta](../../etc/afpd/spotlight.c.md#add_filemeta), [setfile](../../bin/nad/nad_cp.c.md#setfile)
