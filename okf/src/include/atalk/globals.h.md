---
type: C Header File
title: "include/atalk/globals.h"
description: "AFP server options, session state and transport callback types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/globals.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-17T10:01:45+11:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](afp.h.md)
* [atalk/compat.h](compat.h.md)
* [atalk/uam.h](uam.h.md)
* [atalk/unicode.h](unicode.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `atalk/afp_dtrace.h`, `grp.h`, `iniparser.h`, `netdb.h`, `stdbool.h`, `sys/param.h`, `sys/types.h`

# Included by

* [bin/dbd/cmd_dbd.c](../../bin/dbd/cmd_dbd.c.md)
* [bin/nad/nad.h](../../bin/nad/nad.h.md)
* [bin/nad/nad_util.c](../../bin/nad/nad_util.c.md)
* [etc/afpd/afp_asp.c](../../etc/afpd/afp_asp.c.md)
* [etc/afpd/afp_config.c](../../etc/afpd/afp_config.c.md)
* [etc/afpd/afp_config.h](../../etc/afpd/afp_config.h.md)
* [etc/afpd/afp_dsi.c](../../etc/afpd/afp_dsi.c.md)
* [etc/afpd/afp_options.c](../../etc/afpd/afp_options.c.md)
* [etc/afpd/appl.c](../../etc/afpd/appl.c.md)
* [etc/afpd/auth.c](../../etc/afpd/auth.c.md)
* [etc/afpd/auth.h](../../etc/afpd/auth.h.md)
* [etc/afpd/catsearch.c](../../etc/afpd/catsearch.c.md)
* [etc/afpd/desktop.c](../../etc/afpd/desktop.c.md)
* [etc/afpd/desktop.h](../../etc/afpd/desktop.h.md)
* [etc/afpd/dircache.c](../../etc/afpd/dircache.c.md)
* [etc/afpd/dircache.h](../../etc/afpd/dircache.h.md)
* [etc/afpd/directory.c](../../etc/afpd/directory.c.md)
* [etc/afpd/directory.h](../../etc/afpd/directory.h.md)
* [etc/afpd/enumerate.c](../../etc/afpd/enumerate.c.md)
* [etc/afpd/extattrs.c](../../etc/afpd/extattrs.c.md)
* [etc/afpd/fce_api.c](../../etc/afpd/fce_api.c.md)
* [etc/afpd/fce_util.c](../../etc/afpd/fce_util.c.md)
* [etc/afpd/file.c](../../etc/afpd/file.c.md)
* [etc/afpd/file.h](../../etc/afpd/file.h.md)
* [etc/afpd/filedir.c](../../etc/afpd/filedir.c.md)
* [etc/afpd/filedir.h](../../etc/afpd/filedir.h.md)
* [etc/afpd/fork.c](../../etc/afpd/fork.c.md)
* [etc/afpd/main.c](../../etc/afpd/main.c.md)
* [etc/afpd/mangle.h](../../etc/afpd/mangle.h.md)
* [etc/afpd/messages.c](../../etc/afpd/messages.c.md)
* [etc/afpd/misc.h](../../etc/afpd/misc.h.md)
* [etc/afpd/ofork.c](../../etc/afpd/ofork.c.md)
* [etc/afpd/status.c](../../etc/afpd/status.c.md)
* [etc/afpd/status.h](../../etc/afpd/status.h.md)
* [etc/afpd/switch.c](../../etc/afpd/switch.c.md)
* [etc/afpd/uam.c](../../etc/afpd/uam.c.md)
* [etc/afpd/uam_auth.h](../../etc/afpd/uam_auth.h.md)
* [etc/afpd/virtual_icon.c](../../etc/afpd/virtual_icon.c.md)
* [etc/afpd/virtual_icon.h](../../etc/afpd/virtual_icon.h.md)
* [etc/afpd/volume.c](../../etc/afpd/volume.c.md)
* [etc/afpd/volume.h](../../etc/afpd/volume.h.md)
* [etc/netatalk/afp_zeroconf.h](../../etc/netatalk/afp_zeroconf.h.md)
* [etc/netatalk/netatalk.c](../../etc/netatalk/netatalk.c.md)
* [etc/spotlight/cnid/sl_cnid.c](../../etc/spotlight/cnid/sl_cnid.c.md)
* [etc/spotlight/localsearch/sl_localsearch.c](../../etc/spotlight/localsearch/sl_localsearch.c.md)
* [etc/spotlight/xapian/sl_xapian.c](../../etc/spotlight/xapian/sl_xapian.c.md)
* [etc/uams/uams_dhx2_pam.c](../../etc/uams/uams_dhx2_pam.c.md)
* [etc/uams/uams_gss.c](../../etc/uams/uams_gss.c.md)
* [include/atalk/dsi.h](dsi.h.md)
* [include/atalk/netatalk_conf.h](netatalk_conf.h.md)
* [include/atalk/server_ipc.h](server_ipc.h.md)
* [include/atalk/spotlight.h](spotlight.h.md)
* [include/atalk/unix.h](unix.h.md)
* [include/atalk/volume.h](volume.h.md)
* [libatalk/acl/ldap_config.c](../../libatalk/acl/ldap_config.c.md)
* [libatalk/cnid/cnid_mysql.c](../../libatalk/cnid/cnid_mysql.c.md)
* [libatalk/cnid/cnid_sqlite.c](../../libatalk/cnid/cnid_sqlite.c.md)
* [libatalk/util/gettok.c](../../libatalk/util/gettok.c.md)
* [libatalk/util/netatalk_conf.c](../../libatalk/util/netatalk_conf.c.md)
* [libatalk/util/pathconv.c](../../libatalk/util/pathconv.c.md)
* [libatalk/util/server_ipc.c](../../libatalk/util/server_ipc.c.md)

# Functions

### afp_options_init

```c
void afp_options_init(struct afp_options *)
```

Declared at include/atalk/globals.h line 251; no definition in the scanned sources.

# Types

### struct AFPObj

Defined at line 193.
* `char * cmdlineconfigfile`
* `int cmdlineflags`
* `int proto`
* `const void * signature`
* `struct DSI * dsi`
* `void * handle`
* `int fd`
* `int statuslen`
* `char aspstatus`
* `struct afp_options options`
* `char * Obj`
* `char * Type`
* `char * Zone`
* `dictionary * iniconfig`
* `char username`
* `char oldtmp`
* `char newtmp`
* `void * uam_cookie`: cookie for uams
* `struct session_info sinfo`
* `uid_t uid`: client login user id
* `uid_t euid`: client effective process user id
* `pid_t pid`: client process id
* `int ipc_fd`: anonymous PF_UNIX socket for IPC with afpd parent
* `int hint_fd`: pipe read/child end for receiving cache hints from parent
* `gid_t * groups`
* `int ngroups`
* `int afp_version`
* `int cnx_cnt`
* `int cnx_max`
* `int vols_skipped`: volume sections the last [load_afp_conf_vols()](../../libatalk/util/netatalk_conf.c.md#load_afp_conf_vols) could not turn into volumes
* `void(* logout`
* `void(* exit`
* `int(* reply`
* `int(* attention`
* `void * sl_ctx`: opaque handle for the active search backend

### struct afp_options

Defined at line 123.
* `int connections`: Maximum number of possible AFP connections.
* `int tickleval`
* `int timeout`
* `int flags`
* `int dircachesize`
* `int dircache_mode`: Directory cache mode: 0=LRU, 1=ARC.
* `int dircache_validation_freq`: Validate every Nth access (default 100).
* `int dircache_rfork_budget`: Total rfork cache memory budget (KB, 0=disabled).
* `int dircache_rfork_maxentry`: Max rfork size to cache per entry (KB).
* `int strict_locking_explicit`: operator set strict locking (multi protocol changes defaults only)
* `int dircache_validation_freq_explicit`: operator set dircache validation freq
* `int dircache_rfork_budget_explicit`: operator set dircache rfork budget
* `int sleep`: Maximum time allowed to sleep (in tickles).
* `int disconnected`: Maximum time in disconnected state (in tickles).
* `unsigned int tcp_sndbuf`
* `unsigned int tcp_rcvbuf`
* `unsigned char passwdbits`
* `unsigned char passwdminlen`
* `uint32_t server_quantum`
* `int dsireadbuf`: scale factor for sizefof(dsi->buffer) = server_quantum * dsireadbuf
* `char * hostname`
* `char * zone`
* `struct at_addr ddpaddr`
* `char * listen`
* `char * interfaces`
* `char * port`
* `char * configfile`
* `char * uampath`
* `char * fqdn`
* `char * sigconffile`
* `char * uuidconf`
* `char * guest`
* `char * loginmesg`
* `char * keyfile`
* `char * passwdfile`
* `char * srpverifierpath`
* `char * extmapfile`
* `char * uamlist`
* `char * signatureopt`
* `unsigned char signature`
* `char * k5service`
* `char * k5realm`
* `char * k5keytab`
* `size_t k5principal_buflen`
* `char * k5principal`
* `char * unixcodepage`
* `char * maccodepage`
* `char * volcodepage`
* `charset_t maccharset`
* `charset_t unixcharset`
* `mode_t umask`
* `mode_t save_mask`
* `gid_t admingid`
* `bool afpstats_group`
* `gid_t afpstats_gid`
* `bool force_user`
* `uid_t force_uid`
* `bool force_group`
* `gid_t force_gid`
* `int volnamelen`
* `char * ntdomain`
* `char * ntseparator`
* `char * addomain`
* `char * logconfig`
* `char * logfile`
* `bool log_us_timestamp`
* `char * legacyicon`
* `char * mimicmodel`
* `char * servername`
* `char * ignored_attr`
* `int splice_size`
* `char * cnid_mysql_host`
* `char * cnid_mysql_user`
* `char * cnid_mysql_pw`
* `char * cnid_mysql_db`
* `struct afp_volume_name volfile`
* `uint64_t spotlight_results_limit`

### struct afp_volume_name

Defined at line 102.
* `time_t mtime`
* `int loaded`

# Typedefs and enums

* `typedef int(* AFPCmd`
* `typedef struct AFPObj AFPObj`

# Macros

* `DEFAULT_DIRCACHE_SIZE`: default 'dircache size' (entries)
* `DEFAULT_DIRCACHE_VALIDATION_FREQ`: Validate every Nth access; multi protocol defaults this to 1.
* `DIRCACHE_VALIDATION_FREQ_MAX`: Validate every 100th access.
* `DIRCACHE_VALIDATION_FREQ_MIN`: Validate on every access.
* `MACFILELEN`: Type2, HFS-compat.
* `OPTION_AFPSTATS`: whether to expose afpstats Unix socket
* `OPTION_DDP`: whether to allow connections via appletalk/ddp
* `OPTION_SHARE_RESERV`: whether to use Solaris fcntl F_SHARE locks
* `OPTION_SINGLEUSER`: single-user server without root privileges
* `OPTION_SPOTLIGHT`: whether to initialize Spotlight support
* `OPTION_SPOTLIGHT_EXPR`: whether to allow Spotlight logic expressions
* `OPTION_SPOTLIGHT_VOL`: whether spotlight shall be enabled by default for volumes
* `OPTION_STRICT_LOCKING`: per-IO POSIX byte-range data locks on FPRead/FPWrite (default: no)
* `OPTION_VALID_SHELLCHECK`: whether to check for valid login shell
* `OPTION_VETOMSG`: whether to send an AFP message for veto file access
* `RFORK_BUDGET_MAX_KB`: Hard cap: 10 GB in KB.
* `RFORK_ENTRY_MAX_KB`: Hard cap: 10 MB in KB.
* `UTF8FILELEN_EARLY`: Type3, early Mac OS X 10.0-10.4.?
* Undocumented: `AFPOBJ_TMPSIZ`, `CNID_MAX_PATH_LEN`, `CNID_PATH_OVERHEAD`, `DEFAULT_SPLICE_SIZE`, `IS_AFP_SESSION`, `MAXOPTLEN`, `MAXUSERLEN`, `OPTION_ACL2MACCESS`, `OPTION_ACL2MODE`, `OPTION_ANNOUNCESSH`, `OPTION_CLOSEVOL`, `OPTION_DEBUG`, `OPTION_NOSENDFILE`, `OPTION_NOZEROCONF`, `OPTION_RECVFILE`, `OPTION_SERVERNOTIF`, `OPTION_UUID`, `PASSWD_ALL`, `PASSWD_NONE`, `PASSWD_NOSAVE`, `PASSWD_SET`, `SPLICE_SIZE_MAX`, `SPLICE_SIZE_MIN`, `SPOTLIGHT_RESULTS_LIMIT_DEFAULT`, `SPOTLIGHT_RESULTS_LIMIT_MAX`, `SPOTLIGHT_RESULTS_LIMIT_MIN`

# File-scope variables

`afp_version`, `default_options`, `getwdbuf`
