---
type: C Source File
title: "libatalk/util/unix.c"
description: "Netatalk utility functions: unix."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/unix.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-14T21:05:28+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/acl.h](../../include/atalk/acl.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/bstrlib_compat.h](../../include/atalk/bstrlib_compat.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/uam.h](../../include/atalk/uam.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/vfs.h](../../include/atalk/vfs.h.md)
* System headers: `dirent.h`, `errno.h`, `fcntl.h`, `libgen.h`, `poll.h`, `stdint.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### closeall

```c
static void closeall(int fd)
```

Defined at lines 56 to 63.

Close every descriptor from fd up to the _SC_OPEN_MAX limit.

Parameters:
* `fd`: lowest descriptor to close

Called by: [daemonize](unix.c.md#daemonize)

Mentioned in the documentation of: [log_close_all](logger.c.md#log_close_all)

### daemonize

```c
int daemonize(void)
```

Defined at lines 71 to 124.

Fork, exit parent, setsid(), chdir("/"), close all fds.

Returns: -1 on failure, but you can't do much except exit in that case since we may already have forked

Calls: [closeall](unix.c.md#closeall), [log_close_all](logger.c.md#log_close_all), [log_reopen](logger.c.md#log_reopen)

Called by: [main](../../etc/afpd/main.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main)

Mentioned in the documentation of: [log_reopen](logger.c.md#log_reopen)

### count_fds_probed

```c
static int count_fds_probed(int base, int n)
```

Defined at lines 136 to 147.

Count open descriptors in [base, base + n) with one fcntl(2) each.

Parameters:
* `base`: first descriptor
* `n`: descriptors to probe

Returns: number of open descriptors in the range

Called by: [count_fds_polled](unix.c.md#count_fds_polled)

### count_fds_polled

```c
static int count_fds_polled(int max)
```

Defined at lines 160 to 191.

Count open descriptors below `max`, FD_POLL_CHUNK per poll(2).

poll() marks a closed descriptor POLLNVAL without using a descriptor of its own. macOS poll() rejects device descriptors and DragonFly's kqueue-based poll() miscounts them, so both probe instead.

Parameters:
* `max`: number of descriptors to scan from 0

Returns: number of open descriptors below `max`

Calls: [count_fds_probed](unix.c.md#count_fds_probed)

Called by: [count_open_fds](unix.c.md#count_open_fds)

### count_open_fds

```c
int count_open_fds(int *limit)
```

Defined at lines 204 to 232. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Count the descriptors this process has open.

Lists /proc/self/fd where procfs enumerates every descriptor; otherwise, or when no descriptor is free for the listing, scans the descriptors below the limit with poll(2). Meant for logging on rare events.

Parameters:
* `limit`: set to getdtablesize() unless NULL

Returns: number of open descriptors

Calls: [count_fds_polled](unix.c.md#count_fds_polled)

Called by: [dsi_note_master_fd](../dsi/dsi_getsess.c.md#dsi_note_master_fd), [dsi_refuse_starved](../dsi/dsi_getsess.c.md#dsi_refuse_starved), [master_out_of_fds](../../etc/afpd/main.c.md#master_out_of_fds), [of_usage_str](../../etc/afpd/ofork.c.md#of_usage_str), [open_pipe](../dsi/dsi_write.c.md#open_pipe), [uam_log_pam_failure](unix.c.md#uam_log_pam_failure)

### uam_log_pam_failure

```c
void uam_log_pam_failure(enum loglevels level, const char *uam, const char *user, const char *step, const char *errtext, int code, int err)
```

Defined at lines 250 to 267.

Log a failed PAM step with the errno it left and the fds in use.

The descriptor count tells a module that cannot open its files from a wrong password. Takes pam_strerror() text, never a handle, which a failed pam_start() leaves undefined. Both afpd and papd load the PAM UAMs.

Parameters:
* `level`: log level of the line
* `uam`: UAM name that leads the log line
* `user`: user being authenticated
* `step`: PAM function that failed
* `errtext`: pam_strerror() text for `code`
* `code`: PAM return code
* `err`: errno as the failed step left it

Calls: [count_open_fds](unix.c.md#count_open_fds)

Called by: [clrtxt_log_pam_error](../../etc/uams/uams_pam.c.md#clrtxt_log_pam_error), [dhx2_log_pam_error](../../etc/uams/uams_dhx2_pam.c.md#dhx2_log_pam_error), [dhx_log_pam_error](../../etc/uams/uams_dhx_pam.c.md#dhx_log_pam_error)

Uses file-scope variables: `type_configs` in [libatalk/util/logger.c](logger.c.md)

Mentioned in the documentation of: [clrtxt_log_pam_error](../../etc/uams/uams_pam.c.md#clrtxt_log_pam_error), [dhx2_log_pam_error](../../etc/uams/uams_dhx2_pam.c.md#dhx2_log_pam_error), [dhx_log_pam_error](../../etc/uams/uams_dhx_pam.c.md#dhx_log_pam_error)

### log_backoff

```c
bool log_backoff(unsigned int *count)
```

Defined at lines 279 to 283. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Count an event, reporting its 1st, 2nd, 4th, 8th... occurrence.

Parameters:
* `count`: occurrences so far

Returns: true when this occurrence should be logged

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [dsi_refuse_starved](../dsi/dsi_getsess.c.md#dsi_refuse_starved), [dsi_write_file](../dsi/dsi_write.c.md#dsi_write_file), [of_alloc](../../etc/afpd/ofork.c.md#of_alloc), [of_log_nfile](../../etc/afpd/ofork.c.md#of_log_nfile), [open_pipe](../dsi/dsi_write.c.md#open_pipe)

Mentioned in the documentation of: [open_pipe](../dsi/dsi_write.c.md#open_pipe)

### usage_level_update

```c
bool usage_level_update(int *level, int64_t used, int64_t limit)
```

Defined at lines 298 to 318. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Track usage against the 50, 75 and 90 percent warning thresholds.

Raises `level` past each threshold `used` has reached, and lowers it once `used` is ten points below the highest one reached, so a threshold is reported once per climb rather than on every crossing.

Parameters:
* `level`: thresholds reached so far, 0 to 3
* `used`: current usage
* `limit`: capacity the thresholds are percentages of

Returns: true when `level` rose

Called by: [dsi_note_master_fd](../dsi/dsi_getsess.c.md#dsi_note_master_fd), [of_dealloc](../../etc/afpd/ofork.c.md#of_dealloc), [of_note_open](../../etc/afpd/ofork.c.md#of_note_open)

Uses file-scope variables: `usage_warn_pct`

Mentioned in the documentation of: [usage_level_pct](unix.c.md#usage_level_pct)

### usage_level_pct

```c
int usage_level_pct(int level)
```

Defined at lines 327 to 330. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Threshold, in percent, that `level` has reached.

Parameters:
* `level`: a level raised by [usage_level_update()](unix.c.md#usage_level_update), 1 to 3

Returns: the threshold in percent

Called by: [dsi_note_master_fd](../dsi/dsi_getsess.c.md#dsi_note_master_fd), [of_note_open](../../etc/afpd/ofork.c.md#of_note_open)

Uses file-scope variables: `usage_warn_pct`

### become_root

```c
void become_root(void)
```

Defined at lines 340 to 354. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

seteuid(0) and back, if either fails and panic != 0 we PANIC.

Nesting is tracked: only the outermost [become_root()](unix.c.md#become_root) elevates privileges and only the matching [unbecome_root()](unix.c.md#unbecome_root) drops them.

Called by: [ad_conv_dehex](../adouble/ad_conv.c.md#ad_conv_dehex), [ad_conv_v22ea](../adouble/ad_conv.c.md#ad_conv_v22ea), [ad_flush_hf](../adouble/ad_flush.c.md#ad_flush_hf), [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [ad_metadata](../adouble/ad_open.c.md#ad_metadata), [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [cnid_mysql_open](../cnid/mysql/cnid_mysql.c.md#cnid_mysql_open), [cnid_sqlite_open](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_open), [create_appledesktop_folder](../../etc/afpd/desktop.c.md#create_appledesktop_folder), [creatvol](netatalk_conf.c.md#creatvol), [do_check_ea_support](netatalk_conf.c.md#do_check_ea_support), [ea_chmod_dir](../vfs/ea_ad.c.md#ea_chmod_dir), [getfreespace](../../etc/afpd/quota.c.md#getfreespace), [getvolbypath](netatalk_conf.c.md#getvolbypath), [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols), [log_open_file](logger.c.md#log_open_file), [nfsv4_chmod](../acl/unix.c.md#nfsv4_chmod), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [readmessage](../../etc/afpd/messages.c.md#readmessage), [sl_xapian_ensure_state_root](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ensure_state_root)

Uses file-scope variables: `root_nesting`, `saved_uid`

### unbecome_root

```c
void unbecome_root(void)
```

Defined at lines 363 to 380. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Drop the root privileges of the matching become_root call.

Only the outermost unbecome_root restores the saved euid. The call does nothing in a process whose real uid is not root. An unmatched call or a failed seteuid panics.

Called by: [ad_conv_dehex](../adouble/ad_conv.c.md#ad_conv_dehex), [ad_conv_v22ea](../adouble/ad_conv.c.md#ad_conv_v22ea), [ad_flush_hf](../adouble/ad_flush.c.md#ad_flush_hf), [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [ad_metadata](../adouble/ad_open.c.md#ad_metadata), [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [cnid_mysql_open](../cnid/mysql/cnid_mysql.c.md#cnid_mysql_open), [cnid_sqlite_open](../cnid/sqlite/cnid_sqlite.c.md#cnid_sqlite_open), [create_appledesktop_folder](../../etc/afpd/desktop.c.md#create_appledesktop_folder), [creatvol](netatalk_conf.c.md#creatvol), [do_check_ea_support](netatalk_conf.c.md#do_check_ea_support), [ea_chmod_dir](../vfs/ea_ad.c.md#ea_chmod_dir), [getfreespace](../../etc/afpd/quota.c.md#getfreespace), [getvolbypath](netatalk_conf.c.md#getvolbypath), [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols), [log_open_file](logger.c.md#log_open_file), [nfsv4_chmod](../acl/unix.c.md#nfsv4_chmod), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [readmessage](../../etc/afpd/messages.c.md#readmessage), [sl_xapian_ensure_state_root](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ensure_state_root)

Uses file-scope variables: `root_nesting`, `saved_uid`

Mentioned in the documentation of: [become_root](unix.c.md#become_root)

### getcwdpath

```c
const char * getcwdpath(void)
```

Defined at lines 387 to 397.

get cwd in static buffer

Returns: pointer to path or pointer to error messages on error

Called by: [acltoownermode](../../etc/afpd/acls.c.md#acltoownermode), [ad_mkdir](../adouble/ad_open.c.md#ad_mkdir), [afp_getacl](../../etc/afpd/acls.c.md#afp_getacl), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [catsearch_db](../../etc/afpd/catsearch.c.md#catsearch_db), [check_acl_access](../../etc/afpd/acls.c.md#check_acl_access), [cname](../../etc/afpd/directory.c.md#cname), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [get_id](../../etc/afpd/file.c.md#get_id), [get_nfsv4_acl](../acl/unix.c.md#get_nfsv4_acl), [movecwd](../../etc/afpd/directory.c.md#movecwd), [nad_find](../../bin/nad/nad_find.c.md#nad_find), [nfsv4_chmod](../acl/unix.c.md#nfsv4_chmod), [rel_path_in_vol](cnid.c.md#rel_path_in_vol), [sys_set_ea](../vfs/ea_sys.c.md#sys_set_ea)

### fullpathname

```c
const char * fullpathname(const char *name)
```

Defined at lines 408 to 424.

Return the absolute path of a name.

A relative name joins the current directory in a static buffer, which the next call overwrites. A failed getcwd leaves the name relative.

Parameters:
* `name`: file name

Returns: name itself when it is absolute, or the static buffer

Calls: [strlcat](../compat/strlcpy.c.md#strlcat), [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [RF_setdirowner_adouble](../vfs/vfs.c.md#rf_setdirowner_adouble), [ad_conv_dehex](../adouble/ad_conv.c.md#ad_conv_dehex), [ad_conv_v22ea](../adouble/ad_conv.c.md#ad_conv_v22ea), [ad_conv_v22ea_hf](../adouble/ad_conv.c.md#ad_conv_v22ea_hf), [ad_conv_v22ea_rf](../adouble/ad_conv.c.md#ad_conv_v22ea_rf), [ad_convert](../adouble/ad_conv.c.md#ad_convert), [ad_convert_osx](../adouble/ad_open.c.md#ad_convert_osx), [ad_header_read](../adouble/ad_open.c.md#ad_header_read), [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [ad_header_read_osx](../adouble/ad_open.c.md#ad_header_read_osx), [ad_open](../adouble/ad_open.c.md#ad_open), [ad_open_df](../adouble/ad_open.c.md#ad_open_df), [ad_open_hf_ea](../adouble/ad_open.c.md#ad_open_hf_ea), [ad_open_hf_v2](../adouble/ad_open.c.md#ad_open_hf_v2), [ad_open_rf_ea](../adouble/ad_open.c.md#ad_open_rf_ea), [ad_open_rf_v2](../adouble/ad_open.c.md#ad_open_rf_v2), [ad_rtruncate](../adouble/ad_write.c.md#ad_rtruncate), [ad_valid_header_osx](../adouble/ad_open.c.md#ad_valid_header_osx), [afp_openfork](../../etc/afpd/fork.c.md#afp_openfork), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [dir_event_path](../../etc/afpd/directory.c.md#dir_event_path), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [for_each_adouble](../vfs/vfs.c.md#for_each_adouble), [get_id](../../etc/afpd/file.c.md#get_id), [posix_acl_rights](../../etc/afpd/acls.c.md#posix_acl_rights), [posix_chmod](../acl/unix.c.md#posix_chmod), [set_dir_errors](../../etc/afpd/directory.c.md#set_dir_errors), [setdeskmode](../../etc/afpd/desktop.c.md#setdeskmode), [setdeskowner](../../etc/afpd/desktop.c.md#setdeskowner), [setdirowner](../../etc/afpd/unix.c.md#setdirowner), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode), [solaris_acl_rights](../../etc/afpd/acls.c.md#solaris_acl_rights)

### stripped_slashes_basename

```c
char * stripped_slashes_basename(char *p)
```

Defined at lines 443 to 452.

Takes a buffer with a path, strips slashs, returns basename.

path may be or Result is "file" or "dir"

Parameters:
* `p`: (rw) path

```
"[/][dir/[...]]file"
```

```
"[/][dir/[...]]dir/[/]"
```

Returns: pointer to basename in path buffer, buffer is possibly modified

Called by: [convert_dots_encoding](../../bin/nad/nad_util.c.md#convert_dots_encoding)

### tmpdir

```c
const char * tmpdir(void)
```

Defined at lines 465 to 560.

Return a private temporary directory for the process, netatalk-PID under TMPDIR or /tmp.

The call creates the directory with mode 0700. It sets the mode of an existing one back to 0700, and that one has to be a directory that the real uid owns. Any failure returns the system directory instead. The result lives in a static buffer, which the next call overwrites.

Returns: the directory path

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [as_debug](../../etc/atalkd/main.c.md#as_debug), [dircache_dump](../../etc/afpd/dircache.c.md#dircache_dump), [setup_out_fd](../../etc/afpd/afprun.c.md#setup_out_fd)

Mentioned in the documentation of: [as_debug](../../etc/atalkd/main.c.md#as_debug)

### ostat

```c
int ostat(const char *path, struct stat *buf, int options)
```

Defined at lines 577 to 584.

Run lstat with O_NOFOLLOW in options, stat otherwise.

Parameters:
* `path`: path
* `buf`: receives the stat data
* `options`: O_NOFOLLOW or 0

Returns: the result of lstat or stat

Called by: [accessmode](../../etc/afpd/unix.c.md#accessmode), [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_deleteid](../../etc/afpd/file.c.md#afp_deleteid), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [check_acl_access](../../etc/afpd/acls.c.md#check_acl_access), [dirlookup_internal](../../etc/afpd/directory.c.md#dirlookup_internal), [dirlookup_strict](../../etc/afpd/directory.c.md#dirlookup_strict), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [materialize_virtual_icon](../../etc/afpd/fork.c.md#materialize_virtual_icon), [of_closefork](../../etc/afpd/ofork.c.md#of_closefork), [of_stat](../../etc/afpd/ofork.c.md#of_stat), [of_statdir](../../etc/afpd/ofork.c.md#of_statdir), [pfd_get](../../etc/afpd/pfd_cache.c.md#pfd_get), [pfd_ostat](../../etc/afpd/pfd_cache.c.md#pfd_ostat), [process_cache_hints](../../etc/afpd/dircache.c.md#process_cache_hints), [reenumerate_id](../../etc/afpd/file.c.md#reenumerate_id), [reenumerate_loop](../../etc/afpd/file.c.md#reenumerate_loop), [setdirmode_adouble_loop](../vfs/vfs.c.md#setdirmode_adouble_loop), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

### ochown

```c
int ochown(const char *path, uid_t owner, gid_t group, int options)
```

Defined at lines 595 to 602.

Run lchown with O_NOFOLLOW in options, chown otherwise.

Parameters:
* `path`: path
* `owner`: new owner
* `group`: new group
* `options`: O_NOFOLLOW or 0

Returns: the result of lchown or chown

Called by: [ea_chown](../vfs/ea_ad.c.md#ea_chown), [setdirowner](../../etc/afpd/unix.c.md#setdirowner), [setfilowner](../../etc/afpd/unix.c.md#setfilowner)

### ochmod

```c
int ochmod(char *path, mode_t mode, const struct stat *st, int options)
```

Defined at lines 618 to 644.

Change the mode of a path, with options for symlinks and ACLs.

O_IGNORE skips the change. With O_NOFOLLOW the call leaves a symlink alone. O_NETATALK_ACL runs chmod_acl instead of chmod. Without st the call runs lstat itself.

Parameters:
* `path`: path
* `mode`: requested mode
* `st`: lstat data of path, or NULL
* `options`: O_NOFOLLOW, O_NETATALK_ACL, and O_IGNORE bits

Returns: 0 for a skipped change, or the result of chmod_acl, chmod, or a failed lstat

Called by: [RF_setdirmode_adouble](../vfs/vfs.c.md#rf_setdirmode_adouble), [RF_setdirunixmode_adouble](../vfs/vfs.c.md#rf_setdirunixmode_adouble), [setdeskmode](../../etc/afpd/desktop.c.md#setdeskmode), [setdirunixmode](../../etc/afpd/unix.c.md#setdirunixmode), [setfilmode](../vfs/unix.c.md#setfilmode)

### ostatat

```c
int ostatat(int dirfd, const char *path, struct stat *st, int options)
```

Defined at lines 656 to 664.

Run fstatat relative to a directory descriptor, without following a final symlink for O_NOFOLLOW.

Parameters:
* `dirfd`: directory descriptor, or -1 for the current directory
* `path`: path
* `st`: receives the stat data
* `options`: O_NOFOLLOW or 0

Returns: the result of fstatat

Called by: [RF_renamefile_adouble](../vfs/vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](../vfs/vfs.c.md#rf_renamefile_ea), [copydir](../../etc/afpd/directory.c.md#copydir), [deletedir](../../etc/afpd/directory.c.md#deletedir), [pfd_ostat](../../etc/afpd/pfd_cache.c.md#pfd_ostat)

### ochdir

```c
int ochdir(const char *dir, int options)
```

Defined at lines 681 to 766.

Change the current directory and report a symlink on the way for O_NOFOLLOW.

Without O_NOFOLLOW the call is a plain chdir. With it, dir has to be a canonical path, and a trailing slash is fine. The call changes directory first and then compares getcwd with the requested path. On a return of 1 the process is already in the symlink target.

Parameters:
* `dir`: absolute directory, or one relative to the current directory
* `options`: O_NOFOLLOW or 0

Returns: 0, 1 when the path held a symlink or getcwd failed after the change, or -1 when chdir or the first getcwd fails

Called by: [ochdir_vol](../../etc/afpd/directory.c.md#ochdir_vol)

Mentioned in the documentation of: [ochdir_vol](../../etc/afpd/directory.c.md#ochdir_vol)

### randombytes

```c
void randombytes(void *buf, int n)
```

Defined at lines 777 to 804.

Fill a buffer with random bytes from /dev/urandom.

Without /dev/urandom, or after a short read, the call falls back to random seeded with the microseconds of the current time.

Parameters:
* `buf`: buffer
* `n`: number of bytes

Called by: [get_vol_uuid](netatalk_conf.c.md#get_vol_uuid), [set_signature](../../etc/afpd/status.c.md#set_signature)

### gmem

```c
int gmem(gid_t gid, int ngroups, gid_t *groups)
```

Defined at lines 814 to 825. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Tell whether a group ID is in a group list.

Parameters:
* `gid`: group ID
* `ngroups`: number of entries in groups
* `groups`: group list

Returns: 1 when the list holds gid, 0 otherwise

Called by: [accessvol](netatalk_conf.c.md#accessvol), [posix_acl_rights](../../etc/afpd/acls.c.md#posix_acl_rights), [posix_acls_to_uaperms](../../etc/afpd/acls.c.md#posix_acls_to_uaperms), [solaris_acl_rights](../../etc/afpd/acls.c.md#solaris_acl_rights), [utommode](../../etc/afpd/unix.c.md#utommode)

### realpath_safe

```c
char * realpath_safe(const char *path)
```

Defined at lines 835 to 870.

Resolve a path with realpath into a new buffer.

A path that does not resolve logs a warning.

Parameters:
* `path`: path to resolve

Returns: the resolved path, which the caller frees, or NULL on failure

Called by: [dbpath_is_volume_root](netatalk_conf.c.md#dbpath_is_volume_root), [fce_register](../../etc/afpd/fce_api.c.md#fce_register), [getuserbypath](netatalk_conf.c.md#getuserbypath), [getvolbypath](netatalk_conf.c.md#getvolbypath), [main](../../bin/dbd/cmd_dbd.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main), [readvolfile](netatalk_conf.c.md#readvolfile), [symlink_target_safe](../../etc/afpd/file.c.md#symlink_target_safe), [validate_singleuser_config](../../etc/netatalk/netatalk.c.md#validate_singleuser_config)

### basename_safe

```c
const char * basename_safe(const char *path)
```

Defined at lines 881 to 886.

Return the basename of a path without changing the path.

The call runs basename on the path in a static buffer, which the next call overwrites.

Parameters:
* `path`: path

Returns: the basename, within the static buffer or from basename itself

Calls: [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [fce_register](../../etc/afpd/fce_api.c.md#fce_register)

### strtok_quote

```c
char * strtok_quote(char *s, const char *delim)
```

Defined at lines 900 to 942.

Split a string like strtok, with tokens in double quotes kept whole.

A token that starts with a double quote runs to the next double quote, delimiters included, and drops both quotes. An unclosed quote runs to the end of the string. The position lives in a static variable, so one string splits at a time. The code derives from strtok.c of glibc 2.0.6.

Parameters:
* `s`: string to split, or NULL to go on with the last one
* `delim`: delimiter characters

Returns: the next token, or NULL at the end

Called by: [accessvol](netatalk_conf.c.md#accessvol)

### set_groups

```c
int set_groups(AFPObj *obj, struct passwd *pwd)
```

Defined at lines 954 to 985. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Load the supplementary groups of a user into the process and into obj.

Only root runs initgroups, and a failed initgroups only logs an error. The call then reads the group list of the process with getgroups.

Parameters:
* `obj`: AFP object, receives the group list
* `pwd`: user entry

Returns: 0, or -1 when getgroups or calloc fails

Called by: [getvolbypath](netatalk_conf.c.md#getvolbypath), [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols), [login](../../etc/afpd/auth.c.md#login)

### print_groups

```c
const char * print_groups(int ngroups, gid_t *groups)
```

Defined at lines 998 to 1013. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Format a group list for a log message.

Each group ID follows a space, in a static buffer of GROUPSTR_BUFSIZE bytes, which the next call overwrites.

Parameters:
* `ngroups`: number of entries in groups
* `groups`: group list

Returns: the list, or "-" for an empty one

Called by: [login](../../etc/afpd/auth.c.md#login)

# Macros

* Undocumented: `FD_POLL_CHUNK`, `GROUPSTR_BUFSIZE`, `USAGE_LEVELS`

# File-scope variables

`root_nesting`, `saved_uid`, `usage_warn_pct`
