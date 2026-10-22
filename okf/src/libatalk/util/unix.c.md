---
type: C Source File
title: "libatalk/util/unix.c"
description: "Netatalk utility functions: unix."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/unix.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-22T19:27:15+11:00 }
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
* System headers: `dirent.h`, `errno.h`, `fcntl.h`, `grp.h`, `libgen.h`, `poll.h`, `pwd.h`, `stdint.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `time.h`, `unistd.h`

# Functions

### closeall

```c
static void closeall(int fd)
```

Defined at lines 63 to 70.

Close every descriptor from fd up to the _SC_OPEN_MAX limit.

Parameters:
* `fd`: lowest descriptor to close

Called by: [daemonize](unix.c.md#daemonize)

Mentioned in the documentation of: [log_close_all](logger.c.md#log_close_all)

### daemonize

```c
int daemonize(void)
```

Defined at lines 78 to 131.

Fork, exit parent, setsid(), chdir("/"), close all fds.

Returns: -1 on failure, but you can't do much except exit in that case since we may already have forked

Calls: [closeall](unix.c.md#closeall), [log_close_all](logger.c.md#log_close_all), [log_reopen](logger.c.md#log_reopen)

Called by: [main](../../etc/afpd/main.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main)

Mentioned in the documentation of: [log_reopen](logger.c.md#log_reopen)

### count_fds_probed

```c
static int count_fds_probed(int base, int n)
```

Defined at lines 143 to 154.

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

Defined at lines 167 to 198.

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

Defined at lines 211 to 239. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

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

Defined at lines 257 to 274.

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

Defined at lines 286 to 290. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Count an event, reporting its 1st, 2nd, 4th, 8th... occurrence.

Parameters:
* `count`: occurrences so far

Returns: true when this occurrence should be logged

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [dsi_refuse_starved](../dsi/dsi_getsess.c.md#dsi_refuse_starved), [dsi_stream_read_file](../dsi/dsi_stream.c.md#dsi_stream_read_file), [dsi_write_file](../dsi/dsi_write.c.md#dsi_write_file), [icon_open_failed](../../etc/afpd/desktop.c.md#icon_open_failed), [of_alloc](../../etc/afpd/ofork.c.md#of_alloc), [of_log_nfile](../../etc/afpd/ofork.c.md#of_log_nfile), [open_pipe](../dsi/dsi_write.c.md#open_pipe)

Mentioned in the documentation of: [icon_open_failed](../../etc/afpd/desktop.c.md#icon_open_failed), [open_pipe](../dsi/dsi_write.c.md#open_pipe)

### write_full

```c
int write_full(int fd, const void *buf, size_t len)
```

Defined at lines 302 to 326. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Write len bytes in full at the descriptor's file position, going on after EINTR and after a write that stops short.

Parameters:
* `fd`: descriptor to write to
* `buf`: bytes to write
* `len`: their count

Returns: 0, or -1 with errno set, EIO for a write that makes no progress

Called by: [afp_addicon](../../etc/afpd/desktop.c.md#afp_addicon), [copy_fork](../adouble/ad_write.c.md#copy_fork)

### usage_level_update

```c
bool usage_level_update(int *level, int64_t used, int64_t limit)
```

Defined at lines 341 to 361. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

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

Defined at lines 370 to 373. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

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

Defined at lines 383 to 397. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

seteuid(0) and back, if either fails and panic != 0 we PANIC.

Nesting is tracked: only the outermost [become_root()](unix.c.md#become_root) elevates privileges and only the matching [unbecome_root()](unix.c.md#unbecome_root) drops them.

Called by: [ad_conv_dehex](../adouble/ad_conv.c.md#ad_conv_dehex), [ad_conv_v22ea](../adouble/ad_conv.c.md#ad_conv_v22ea), [ad_flush_hf](../adouble/ad_flush.c.md#ad_flush_hf), [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [ad_metadata](../adouble/ad_open.c.md#ad_metadata), [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [cnid_mysql_open](../cnid/cnid_mysql.c.md#cnid_mysql_open), [cnid_sqlite_open](../cnid/cnid_sqlite.c.md#cnid_sqlite_open), [create_appledesktop_folder](../../etc/afpd/desktop.c.md#create_appledesktop_folder), [creatvol](netatalk_conf.c.md#creatvol), [do_check_ea_support](netatalk_conf.c.md#do_check_ea_support), [ea_chmod_dir](../vfs/ea_ad.c.md#ea_chmod_dir), [getfreespace](../../etc/afpd/quota.c.md#getfreespace), [getvolbypath](netatalk_conf.c.md#getvolbypath), [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols), [log_open_file](logger.c.md#log_open_file), [nfsv4_chmod](../acl/unix.c.md#nfsv4_chmod), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [readmessage](../../etc/afpd/messages.c.md#readmessage), [sl_xapian_ensure_state_root](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ensure_state_root)

Uses file-scope variables: `root_nesting`, `saved_uid`

### unbecome_root

```c
void unbecome_root(void)
```

Defined at lines 406 to 423. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Drop the root privileges of the matching become_root call.

Only the outermost unbecome_root restores the saved euid. The call does nothing in a process whose real uid is not root. An unmatched call or a failed seteuid panics.

Called by: [ad_conv_dehex](../adouble/ad_conv.c.md#ad_conv_dehex), [ad_conv_v22ea](../adouble/ad_conv.c.md#ad_conv_v22ea), [ad_flush_hf](../adouble/ad_flush.c.md#ad_flush_hf), [ad_header_read_ea](../adouble/ad_open.c.md#ad_header_read_ea), [ad_metadata](../adouble/ad_open.c.md#ad_metadata), [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [cnid_mysql_open](../cnid/cnid_mysql.c.md#cnid_mysql_open), [cnid_sqlite_open](../cnid/cnid_sqlite.c.md#cnid_sqlite_open), [create_appledesktop_folder](../../etc/afpd/desktop.c.md#create_appledesktop_folder), [creatvol](netatalk_conf.c.md#creatvol), [do_check_ea_support](netatalk_conf.c.md#do_check_ea_support), [ea_chmod_dir](../vfs/ea_ad.c.md#ea_chmod_dir), [getfreespace](../../etc/afpd/quota.c.md#getfreespace), [getvolbypath](netatalk_conf.c.md#getvolbypath), [load_afp_conf_vols](netatalk_conf.c.md#load_afp_conf_vols), [log_open_file](logger.c.md#log_open_file), [nfsv4_chmod](../acl/unix.c.md#nfsv4_chmod), [of_get_locks](../../etc/afpd/ofork.c.md#of_get_locks), [readmessage](../../etc/afpd/messages.c.md#readmessage), [sl_xapian_ensure_state_root](../../etc/spotlight/xapian/sl_xapian.c.md#sl_xapian_ensure_state_root)

Uses file-scope variables: `root_nesting`, `saved_uid`

Mentioned in the documentation of: [become_root](unix.c.md#become_root)

### become_owner

```c
int become_owner(uid_t uid, gid_t *egid)
```

Defined at lines 440 to 486.

Act as an account for one step, with its groups, egid and euid.

For a process whose euid is 0. [unbecome_owner()](unix.c.md#unbecome_owner) takes euid 0, the egid and the process's supplementary groups back.

Parameters:
* `uid`: the account
* `egid`: the gid to hand [unbecome_owner()](unix.c.md#unbecome_owner)

Returns: 0, or -1 with errno set and the ids as they were

Called by: [cnid_open](../cnid/cnid.c.md#cnid_open), [create_appledesktop_folder](../../etc/afpd/desktop.c.md#create_appledesktop_folder)

Uses file-scope variables: `owner_saved_groups`, `owner_saved_ngroups`

Mentioned in the documentation of: [cnid_open](../cnid/cnid.c.md#cnid_open), [unbecome_owner](unix.c.md#unbecome_owner)

### unbecome_owner

```c
void unbecome_owner(gid_t egid)
```

Defined at lines 494 to 500.

Take euid 0, the egid and the supplementary groups back after [become_owner()](unix.c.md#become_owner).

Parameters:
* `egid`: the gid [become_owner()](unix.c.md#become_owner) gave

Called by: [cnid_open](../cnid/cnid.c.md#cnid_open), [create_appledesktop_folder](../../etc/afpd/desktop.c.md#create_appledesktop_folder)

Uses file-scope variables: `owner_saved_groups`, `owner_saved_ngroups`

Mentioned in the documentation of: [become_owner](unix.c.md#become_owner)

### strip_trailing_slashes

```c
void strip_trailing_slashes(char *path)
```

Defined at lines 510 to 517.

Drop trailing slashes, which make lstat and an O_NOFOLLOW open follow a final symbolic link.

A lone "/" stays.

Parameters:
* `path`: path to trim

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [getvolbypath](netatalk_conf.c.md#getvolbypath), [owned_private_dir](../../etc/netatalk/netatalk.c.md#owned_private_dir), [path_parent](../../etc/netatalk/netatalk.c.md#path_parent), [user_state_paths](netatalk_conf.c.md#user_state_paths)

### shell_quote

```c
char * shell_quote(const char *s)
```

Defined at lines 528 to 557.

Quote a string as one word of a POSIX shell command.

The word is the string between single quotes, with each single quote in it written as '\''.

Parameters:
* `s`: string to quote

Returns: the word, which the caller frees, or NULL when out of memory

Called by: [cnid_sqlite_open](../cnid/cnid_sqlite.c.md#cnid_sqlite_open), [cnid_sqlite_refused](../cnid/cnid_sqlite.c.md#cnid_sqlite_refused)

### getcwdpath

```c
const char * getcwdpath(void)
```

Defined at lines 564 to 574.

get cwd in static buffer

Returns: pointer to path or pointer to error messages on error

Called by: [acltoownermode](../../etc/afpd/acls.c.md#acltoownermode), [ad_mkdir](../adouble/ad_open.c.md#ad_mkdir), [afp_getacl](../../etc/afpd/acls.c.md#afp_getacl), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [catsearch_db](../../etc/afpd/catsearch.c.md#catsearch_db), [check_acl_access](../../etc/afpd/acls.c.md#check_acl_access), [cname](../../etc/afpd/directory.c.md#cname), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [get_id](../../etc/afpd/file.c.md#get_id), [get_nfsv4_acl](../acl/unix.c.md#get_nfsv4_acl), [movecwd](../../etc/afpd/directory.c.md#movecwd), [nad_find](../../bin/nad/nad_find.c.md#nad_find), [nfsv4_chmod](../acl/unix.c.md#nfsv4_chmod), [rel_path_in_vol](cnid.c.md#rel_path_in_vol), [sys_set_ea](../vfs/ea_sys.c.md#sys_set_ea)

### fullpathname

```c
const char * fullpathname(const char *name)
```

Defined at lines 585 to 601.

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

Defined at lines 620 to 629.

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

Called by: [cnid_sqlite_open](../cnid/cnid_sqlite.c.md#cnid_sqlite_open), [convert_dots_encoding](../../bin/nad/nad_util.c.md#convert_dots_encoding), [dbpath_is_creatable](../../etc/netatalk/netatalk.c.md#dbpath_is_creatable), [fce_init_ign_paths](../../etc/afpd/fce_api.c.md#fce_init_ign_paths)

### tmpdir

```c
const char * tmpdir(void)
```

Defined at lines 642 to 737.

Return a private temporary directory for the process, netatalk-PID under TMPDIR or /tmp.

The call creates the directory with mode 0700. It sets the mode of an existing one back to 0700, and that one has to be a directory that the real uid owns. Any failure returns the system directory instead. The result lives in a static buffer, which the next call overwrites.

Returns: the directory path

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [as_debug](../../etc/atalkd/main.c.md#as_debug), [dircache_dump](../../etc/afpd/dircache.c.md#dircache_dump), [setup_out_fd](../../etc/afpd/afprun.c.md#setup_out_fd)

Mentioned in the documentation of: [as_debug](../../etc/atalkd/main.c.md#as_debug)

### coarse_time

```c
time_t coarse_time(void)
```

Defined at lines 756 to 765.

The current second, from the cheapest clock the platform has.

The coarse or fast realtime clock, where the platform names one, comes from memory the kernel shares; CLOCK_REALTIME otherwise costs no more than [time()](../../sys/netatalk/at_control.c.md), and on illumos avoids the system call [time()](../../sys/netatalk/at_control.c.md) makes.

Returns: seconds since the epoch

Called by: [flushvoltime](../../etc/afpd/volume.c.md#flushvoltime), [get_tm_used](../../etc/afpd/volume.c.md#get_tm_used), [getvolquota](../../etc/afpd/volume.c.md#getvolquota), [markvol](../../etc/afpd/volume.c.md#markvol)

Mentioned in the documentation of: [markvol](../../etc/afpd/volume.c.md#markvol)

### ostat

```c
int ostat(const char *path, struct stat *buf, int options)
```

Defined at lines 782 to 789.

Run lstat with O_NOFOLLOW in options, stat otherwise.

Parameters:
* `path`: path
* `buf`: receives the stat data
* `options`: O_NOFOLLOW or 0

Returns: the result of lstat or stat

Called by: [accessmode](../../etc/afpd/unix.c.md#accessmode), [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_deleteid](../../etc/afpd/file.c.md#afp_deleteid), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_setacl](../../etc/afpd/acls.c.md#afp_setacl), [check_acl_access](../../etc/afpd/acls.c.md#check_acl_access), [dirlookup_internal](../../etc/afpd/directory.c.md#dirlookup_internal), [dirlookup_strict](../../etc/afpd/directory.c.md#dirlookup_strict), [ea_written](../../etc/afpd/extattrs.c.md#ea_written), [enumerate](../../etc/afpd/enumerate.c.md#enumerate), [getmetadata](../../etc/afpd/file.c.md#getmetadata), [moveandrename](../../etc/afpd/filedir.c.md#moveandrename), [of_path_holds](../../etc/afpd/ofork.c.md#of_path_holds), [of_stat](../../etc/afpd/ofork.c.md#of_stat), [of_statdir](../../etc/afpd/ofork.c.md#of_statdir), [pfd_get](../../etc/afpd/pfd_cache.c.md#pfd_get), [pfd_ostat](../../etc/afpd/pfd_cache.c.md#pfd_ostat), [process_cache_hints](../../etc/afpd/dircache.c.md#process_cache_hints), [reenumerate_id](../../etc/afpd/file.c.md#reenumerate_id), [reenumerate_loop](../../etc/afpd/file.c.md#reenumerate_loop), [setdirmode_adouble_loop](../vfs/vfs.c.md#setdirmode_adouble_loop), [setdirparams](../../etc/afpd/directory.c.md#setdirparams), [setfilparams](../../etc/afpd/file.c.md#setfilparams)

### ochown

```c
int ochown(const char *path, uid_t owner, gid_t group, int options)
```

Defined at lines 800 to 807.

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

Defined at lines 823 to 849.

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

Defined at lines 861 to 869.

Run fstatat relative to a directory descriptor, without following a final symlink for O_NOFOLLOW.

Parameters:
* `dirfd`: directory descriptor, or -1 for the current directory
* `path`: path
* `st`: receives the stat data
* `options`: O_NOFOLLOW or 0

Returns: the result of fstatat

Called by: [RF_renamefile_adouble](../vfs/vfs.c.md#rf_renamefile_adouble), [RF_renamefile_ea](../vfs/vfs.c.md#rf_renamefile_ea), [copydir](../../etc/afpd/directory.c.md#copydir), [deletedir](../../etc/afpd/directory.c.md#deletedir), [of_findnameat](../../etc/afpd/ofork.c.md#of_findnameat), [pfd_ostat](../../etc/afpd/pfd_cache.c.md#pfd_ostat)

### chdir_getcwd

```c
static int chdir_getcwd(const char *dir)
```

Defined at lines 883 to 963.

chdir() to dir, then report a symlink on its path, which a getcwd() that differs from dir shows

dir has to be a canonical path, and a trailing slash is fine. On a return of 1 the process is already in the symlink target.

Parameters:
* `dir`: absolute directory, or one relative to the current directory

Returns: 0, 1 when the path held a symlink or getcwd() failed after the change, or -1 when chdir() or the first getcwd() fails

Called by: [ochdir](unix.c.md#ochdir)

Mentioned in the documentation of: [ochdir](unix.c.md#ochdir)

### path_has_dot_component

```c
int path_has_dot_component(const char *path, int dot)
```

Defined at lines 973 to 988.

Whether a component of a slash-separated path is "..", or with dot set also ".".

Parameters:
* `path`: the path
* `dot`: nonzero to count a "." component too

Returns: 1 when such a component is there, else 0

Called by: [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery), [symlink_target_safe](../../etc/afpd/file.c.md#symlink_target_safe)

### path_at_or_below

```c
int path_at_or_below(const char *path, const char *dir)
```

Defined at lines 1001 to 1011.

Whether an absolute path is a directory or lies below it.

The prefix must end at a slash or at the end of path, so "/vol2" does not lie below "/vol". The directory "/" holds every absolute path.

Parameters:
* `path`: resolved absolute path
* `dir`: resolved absolute directory, without a trailing slash unless it is "/"

Returns: 1 when path is dir or lies below it, else 0

Called by: [fce_register](../../etc/afpd/fce_api.c.md#fce_register), [ochdir](unix.c.md#ochdir), [sl_path_in_scope](../../etc/afpd/spotlight.c.md#sl_path_in_scope), [symlink_target_safe](../../etc/afpd/file.c.md#symlink_target_safe)

### ochdir

```c
int ochdir(const char *dir, int options, int rootfd, const char *root)
```

Defined at lines 1134 to 1152.

Change the current directory and report a symlink on the way for O_NOFOLLOW.

Without O_NOFOLLOW the call is a plain chdir(). With it, dir has to be a canonical path, and a trailing slash is fine: given rootfd from ochdir_root(root), a dir below root is entered by chdir_below() without getcwd(), any other by [chdir_getcwd()](unix.c.md#chdir_getcwd).

Parameters:
* `dir`: absolute directory, or one relative to the current directory
* `options`: O_NOFOLLOW or 0
* `rootfd`: ochdir_root(root), or -1
* `root`: the folder rootfd is open on, unused when rootfd is -1

Returns: 0, 1 when the path held a symlink or getcwd() failed after the change, or -1 when the change fails

Calls: [chdir_getcwd](unix.c.md#chdir_getcwd), [path_at_or_below](unix.c.md#path_at_or_below)

Called by: [ochdir_vol](../../etc/afpd/directory.c.md#ochdir_vol)

Mentioned in the documentation of: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [ochdir_root](unix.c.md#ochdir_root), [ochdir_vol](../../etc/afpd/directory.c.md#ochdir_vol)

### ascii_tolower

```c
int ascii_tolower(int c)
```

Defined at lines 1161 to 1164.

A character in lower case if it is an ASCII capital, whatever the locale.

Parameters:
* `c`: the character, as an unsigned char

Returns: c, with A to Z taken to a to z

Called by: [other_name](../vfs/vfs.c.md#other_name), [veto_entry_matches](../../etc/afpd/filedir.c.md#veto_entry_matches)

Mentioned in the documentation of: [veto_entry_matches](../../etc/afpd/filedir.c.md#veto_entry_matches)

### path_probe_case

```c
int path_probe_case(const char *dir)
```

Defined at lines 1179 to 1224.

Whether a name in dir, looked up with one letter's case swapped, finds the same object.

The probe takes the first entry that is no folder, has one link and holds an ASCII letter in its name: no second link can answer for it, and a folder looked up by another name keeps that name in FreeBSD's and NetBSD's namecaches, where getcwd() then finds it.

Parameters:
* `dir`: folder to probe

Returns: 1 where the lookup ignores case, 0 where it matches case, -1 where dir cannot be read or holds no such entry

Calls: [strlcpy](../compat/strlcpy.c.md#strlcpy)

Called by: [path_ignores_case](unix.c.md#path_ignores_case)

Mentioned in the documentation of: [path_ignores_case](unix.c.md#path_ignores_case)

### path_ignores_case

```c
int path_ignores_case(const char *path)
```

Defined at lines 1237 to 1264.

Whether the filesystem holding path matches names in either case.

pathconf() answers where the system names the question; elsewhere, or where it has no answer, [path_probe_case()](unix.c.md#path_probe_case) looks, and a folder it cannot tell about counts as ignoring case, so veto entries match in either case.

Parameters:
* `path`: a folder on the filesystem

Returns: 1 where pathconf() reports or the probe finds that the filesystem ignores case, or the probe cannot tell; else 0

Calls: [path_probe_case](unix.c.md#path_probe_case)

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol), [ochdir_root](unix.c.md#ochdir_root)

### ochdir_root

```c
int ochdir_root(const char *root)
```

Defined at lines 1277 to 1296.

The descriptor [ochdir()](unix.c.md#ochdir) walks from below a volume root.

The root is open for reading: a lookup through a descriptor open for search skips the search check of its folder, which chdir() makes on every request. The caller closes the descriptor.

Parameters:
* `root`: the volume root's path

Returns: root open where [ochdir()](unix.c.md#ochdir) enters the folders below it without getcwd(), else -1

Calls: [path_ignores_case](unix.c.md#path_ignores_case)

Called by: [afp_openvol](../../etc/afpd/volume.c.md#afp_openvol)

### randombytes

```c
void randombytes(void *buf, int n)
```

Defined at lines 1307 to 1334.

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

Defined at lines 1344 to 1355. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

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

Defined at lines 1365 to 1400.

Resolve a path with realpath into a new buffer.

A path that does not resolve logs a warning.

Parameters:
* `path`: path to resolve

Returns: the resolved path, which the caller frees, or NULL on failure

Called by: [dbpath_is_volume_root](netatalk_conf.c.md#dbpath_is_volume_root), [getuserbypath](netatalk_conf.c.md#getuserbypath), [getvolbypath](netatalk_conf.c.md#getvolbypath), [link_target_in](../../etc/afpd/file.c.md#link_target_in), [main](../../bin/dbd/cmd_dbd.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main), [readvolfile](netatalk_conf.c.md#readvolfile), [symlink_target_safe](../../etc/afpd/file.c.md#symlink_target_safe), [validate_singleuser_config](../../etc/netatalk/netatalk.c.md#validate_singleuser_config)

### basename_safe

```c
const char * basename_safe(const char *path)
```

Defined at lines 1411 to 1416.

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

Defined at lines 1430 to 1472.

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

Defined at lines 1484 to 1515. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

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

Defined at lines 1528 to 1543. Declared in [include/atalk/unix.h](../../include/atalk/unix.h.md).

Format a group list for a log message.

Each group ID follows a space, in a static buffer of GROUPSTR_BUFSIZE bytes, which the next call overwrites.

Parameters:
* `ngroups`: number of entries in groups
* `groups`: group list

Returns: the list, or "-" for an empty one

Called by: [login](../../etc/afpd/auth.c.md#login)

# Macros

* Undocumented: `COARSE_CLOCK`, `FD_POLL_CHUNK`, `GROUPSTR_BUFSIZE`, `USAGE_LEVELS`

# File-scope variables

`owner_saved_groups`, `owner_saved_ngroups`, `root_nesting`, `saved_uid`, `usage_warn_pct`
