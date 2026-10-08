---
type: C Source File
title: "etc/afpd/ofork.c"
description: "32 functions, 1 type, includes 15 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/ofork.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/ea.h](../../include/atalk/ea.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/server_ipc.h](../../include/atalk/server_ipc.h.md)
* [atalk/spotlight.h](../../include/atalk/spotlight.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [fork.h](fork.h.md)
* [virtual_icon.h](virtual_icon.h.md)
* [volume.h](volume.h.md)
* System headers: `bstrlib.h`, `errno.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `unistd.h`

# Functions

### of_mix_add

```c
static void of_mix_add(struct of_mix *mix, const struct ofork *of)
```

Defined at lines 88 to 94.

Count one fork into its fork and access bucket.

Parameters:
* `mix`: bucket counts
* `of`: fork to count

Called by: [of_breakdown](ofork.c.md#of_breakdown), [of_close_sweep](ofork.c.md#of_close_sweep)

### of_mix_str

```c
static const char * of_mix_str(const struct of_mix *mix, char *buf, size_t len)
```

Defined at lines 105 to 127.

Format the non-empty buckets, e.g. "data rd 10, rsrc rw 1".

Parameters:
* `mix`: bucket counts
* `buf`: destination
* `len`: size of `buf`

Returns: `buf`, empty when every bucket is

Called by: [of_breakdown](ofork.c.md#of_breakdown), [of_close_sweep](ofork.c.md#of_close_sweep)

### of_breakdown

```c
const char * of_breakdown(char *buf, size_t len)
```

Defined at lines 139 to 150. Declared in [etc/afpd/fork.h](fork.h.md).

Format the live forks by fork and access mode.

Scans the whole table, so it is for log lines on rare events only.

Parameters:
* `buf`: destination
* `len`: size of `buf`

Returns: `buf`, empty when no fork is open

Calls: [of_mix_add](ofork.c.md#of_mix_add), [of_mix_str](ofork.c.md#of_mix_str)

Called by: [of_usage_str](ofork.c.md#of_usage_str)

Uses file-scope variables: `nforks`, `oforks`

### of_usage_str

```c
static const char * of_usage_str(char *buf, size_t len)
```

Defined at lines 160 to 175.

Format fork-table and descriptor usage for a log line.

Parameters:
* `buf`: destination
* `len`: size of `buf`

Returns: `buf`

Calls: [count_open_fds](../../libatalk/util/unix.c.md#count_open_fds), [of_breakdown](ofork.c.md#of_breakdown)

Called by: [of_alloc](ofork.c.md#of_alloc), [of_log_nfile](ofork.c.md#of_log_nfile), [of_log_usage](ofork.c.md#of_log_usage), [of_note_open](ofork.c.md#of_note_open)

Uses file-scope variables: `nforks`, `of_curr_forks`, `of_opens`, `of_peak_forks`

### hashfn

```c
static unsigned long hashfn(const struct file_key *key)
```

Defined at lines 178 to 181.

Called by: [of_findname](ofork.c.md#of_findname), [of_findnameat](ofork.c.md#of_findnameat), [of_hash](ofork.c.md#of_hash), [of_rename](ofork.c.md#of_rename)

### of_hash

```c
static void of_hash(struct ofork *of)
```

Defined at lines 183 to 194.

Calls: [hashfn](ofork.c.md#hashfn)

Called by: [of_alloc](ofork.c.md#of_alloc)

Uses file-scope variables: `ofork_table`

### of_unhash

```c
static void of_unhash(struct ofork *of)
```

Defined at lines 196 to 208.

Called by: [of_dealloc](ofork.c.md#of_dealloc)

### of_freeq_empty

```c
static bool of_freeq_empty(void)
```

Defined at lines 210 to 213.

Called by: [of_freeq_pop](ofork.c.md#of_freeq_pop), [of_freeq_push](ofork.c.md#of_freeq_push)

Uses file-scope variables: `of_freeq_head`, `of_freeq_tail`

### of_freeq_push

```c
static void of_freeq_push(uint16_t slot)
```

Defined at lines 215 to 226.

Calls: [of_freeq_empty](ofork.c.md#of_freeq_empty)

Called by: [of_alloc](ofork.c.md#of_alloc), [of_dealloc](ofork.c.md#of_dealloc)

Uses file-scope variables: `of_freeq`, `of_freeq_cap`, `of_freeq_tail`

### of_freeq_pop

```c
static int of_freeq_pop(void)
```

Defined at lines 228 to 241.

Calls: [of_freeq_empty](ofork.c.md#of_freeq_empty)

Called by: [of_alloc](ofork.c.md#of_alloc)

Uses file-scope variables: `of_freeq`, `of_freeq_cap`, `of_freeq_head`

### of_pforkdesc

```c
void of_pforkdesc(FILE *)
```

Defined at lines 243 to 256. Declared in [etc/afpd/fork.h](fork.h.md).

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `nforks`, `oforks`

### of_flush

```c
int of_flush(const struct vol *)
```

Defined at lines 258 to 274. Declared in [etc/afpd/fork.h](fork.h.md).

Calls: [flushfork](fork.c.md#flushfork)

Called by: [afp_flush](fork.c.md#afp_flush)

Uses file-scope variables: `nforks`, `oforks`

### of_rename

```c
int of_rename(const struct vol *, struct ofork *, struct dir *, const char *, struct dir *, const char *)
```

Defined at lines 276 to 314. Declared in [etc/afpd/fork.h](fork.h.md).

Calls: [hashfn](ofork.c.md#hashfn)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [moveandrename](filedir.c.md#moveandrename)

Uses file-scope variables: `ofork_table`

### of_alloc

```c
struct ofork * of_alloc(struct vol *, struct dir *, char *, uint16_t *, const int, struct adouble *, struct stat *)
```

Defined at lines 317 to 448. Declared in [etc/afpd/fork.h](fork.h.md).

Calls: [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [log_backoff](../../libatalk/util/unix.c.md#log_backoff), [of_freeq_pop](ofork.c.md#of_freeq_pop), [of_freeq_push](ofork.c.md#of_freeq_push), [of_hash](ofork.c.md#of_hash), [of_usage_str](ofork.c.md#of_usage_str)

Called by: [afp_openfork](fork.c.md#afp_openfork)

Uses file-scope variables: `nforks`, `of_curr_forks`, `of_fdlimit`, `of_freeq`, `of_freeq_cap`, `of_full_refusals`, `of_peak_forks`, `oforks`

### of_find

```c
struct ofork * of_find(const uint16_t)
```

Defined at lines 450 to 471. Declared in [etc/afpd/fork.h](fork.h.md).

Called by: [afp_closefork](fork.c.md#afp_closefork), [afp_flushfork](fork.c.md#afp_flushfork), [afp_getforkparams](fork.c.md#afp_getforkparams), [afp_setforkparams](fork.c.md#afp_setforkparams), [afp_syncfork](fork.c.md#afp_syncfork), [byte_lock](fork.c.md#byte_lock), [read_fork](fork.c.md#read_fork), [write_fork](fork.c.md#write_fork)

Uses file-scope variables: `nforks`, `oforks`

### of_stat

```c
int of_stat(const struct vol *vol, struct path *)
```

Defined at lines 474 to 487. Declared in [etc/afpd/fork.h](fork.h.md).

Calls: [ostat](../../libatalk/util/unix.c.md#ostat)

Called by: [afp_createdir](directory.c.md#afp_createdir), [afp_opendir](directory.c.md#afp_opendir), [afp_resolveid](file.c.md#afp_resolveid), [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db), [cname](directory.c.md#cname), [enumerate](enumerate.c.md#enumerate), [of_findname](ofork.c.md#of_findname), [of_statdir](ofork.c.md#of_statdir), [setfilparams](file.c.md#setfilparams), [setfilunixmode](unix.c.md#setfilunixmode)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### of_fstatat

```c
int of_fstatat(int dirfd, struct path *path)
```

Defined at lines 489 to 500. Declared in [etc/afpd/fork.h](fork.h.md).

Called by: [of_findnameat](ofork.c.md#of_findnameat)

### of_statdir

```c
int of_statdir(struct vol *vol, struct path *)
```

Defined at lines 507 to 552. Declared in [etc/afpd/fork.h](fork.h.md).

stat the current directory.

Note: stat(".") works even if "." is deleted thus we have to stat ../name because we want to know if it's there

Calls: [dirlookup](directory.c.md#dirlookup), [movecwd](directory.c.md#movecwd), [of_stat](ofork.c.md#of_stat), [ostat](../../libatalk/util/unix.c.md#ostat)

Called by: [afp_access](acls.c.md#afp_access), [afp_delete](filedir.c.md#afp_delete), [afp_getacl](acls.c.md#afp_getacl), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_setacl](acls.c.md#afp_setacl), [afp_setfildirparams](filedir.c.md#afp_setfildirparams)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### of_findname

```c
struct ofork * of_findname(const struct vol *vol, struct path *)
```

Defined at lines 555 to 578. Declared in [etc/afpd/fork.h](fork.h.md).

Calls: [hashfn](ofork.c.md#hashfn), [of_stat](ofork.c.md#of_stat)

Called by: [ad_addcomment](desktop.c.md#ad_addcomment), [ad_getcomment](desktop.c.md#ad_getcomment), [ad_rmvcomment](desktop.c.md#ad_rmvcomment), [adl_lkup](catsearch.c.md#adl_lkup), [afp_copyfile](file.c.md#afp_copyfile), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_openfork](fork.c.md#afp_openfork), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setextattr](extattrs.c.md#afp_setextattr), [check_delete_inhibit](file.c.md#check_delete_inhibit), [find_adouble](file.c.md#find_adouble), [moveandrename](filedir.c.md#moveandrename), [of_ad](ofork.c.md#of_ad), [of_close_inode_forks](ofork.c.md#of_close_inode_forks), [of_get_locks](ofork.c.md#of_get_locks)

Uses file-scope variables: `ofork_table`

### of_findnameat

```c
struct ofork * of_findnameat(int dirfd, struct path *path)
```

Defined at lines 589 to 612. Declared in [etc/afpd/fork.h](fork.h.md).

Search for open fork by dirfd/name.

Function call of_fstatat with dirfd and path and uses dev and ino to search the open fork table.

Parameters:
* `dirfd`: directory fd
* `path`: pointer to struct path

Calls: [hashfn](ofork.c.md#hashfn), [of_fstatat](ofork.c.md#of_fstatat)

Called by: [check_delete_inhibit](file.c.md#check_delete_inhibit), [moveandrename](filedir.c.md#moveandrename), [of_get_locks](ofork.c.md#of_get_locks)

Uses file-scope variables: `ofork_table`

### of_delete_blocked

```c
int of_delete_blocked(const struct ofork *of, uint16_t *band, int *content)
```

Defined at lines 637 to 675. Declared in [etc/afpd/fork.h](fork.h.md).

Does this child hold a delete-blocking claim on the fork's file?

The cross-process conflict check ([deletefile()](file.c.md#deletefile)'s F_GETLK read) cannot by POSIX report the calling process's own locks, so the delete path asks this helper for the local complement: scan the session's own lock bookkeeping for the same claims and apply the same DELETE_BLOCKING_BAND_BITS policy — a deny mode or content byte-range lock refuses the delete whether its holder is a peer process or this session itself. OPEN_* markers never block (an open is just an open; only declared claims do).

Any one fork on the file suffices as `of:` sibling forks on an inode share one adouble, whose data-fork adf_lock[] holds ALL of the session's band entries (OPEN/DENY incl. the RSRC_* mirrors via rf2off) and its own content ranges; the rfork's adf_lock[] (reached via ad_rfp on the same adouble) holds only rfork content ranges. So one lookup sees every claim — unlike the post-delete sweep, which must visit each fork to close it.

Parameters:
* `of`: a fork this child holds on the file
* `band`: held blocking band bits, positional (may be NULL)
* `content`: set if a content byte-range lock is held (may be NULL)

Returns: nonzero if the delete must be refused

Called by: [afp_delete](filedir.c.md#afp_delete)

Mentioned in the documentation of: [of_close_inode_forks](ofork.c.md#of_close_inode_forks)

### of_close_inode_forks

```c
void of_close_inode_forks(const AFPObj *obj, const struct vol *vol, struct path *path)
```

Defined at lines 694 to 733. Declared in [etc/afpd/fork.h](fork.h.md).

Close every fork this child holds on path's inode.

Same-session sweep for FPDelete, called after a successful delete: every refusal condition (DeleteInhibit, peer claims, the session's own claims via [of_delete_blocked()](ofork.c.md#of_delete_blocked)) has already passed and the file is unlinked, so only claim-free forks (any OPEN_* access, no deny mode, no content lock) reach this sweep. Each fork is closed individually — the blocking check needs one fork (shared adouble), the close needs them all. [of_closefork()](ofork.c.md#of_closefork) deallocs even on flush error (nothing is stranded), so a failed close is logged and the sweep continues — matching [of_closevol()](ofork.c.md#of_closevol).

Parameters:
* `obj`: the AFP session object
* `vol`: volume the deleted file lived on
* `path`: deleted file; path->st must be the pre-delete stat (the file no longer exists, so a re-stat cannot rebuild it)

Calls: [of_closefork](ofork.c.md#of_closefork), [of_findname](ofork.c.md#of_findname), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [afp_delete](filedir.c.md#afp_delete)

### of_get_locks

```c
int of_get_locks(const struct vol *vol, int dirfd, struct path *path, int df_get, off_t df_off, off_t df_len, int rf_get, off_t rf_off, off_t rf_len, uint16_t band_request, int try_root, int *df_locked, int *rf_locked, uint16_t *band_held)
```

Defined at lines 774 to 1016. Declared in [etc/afpd/fork.h](fork.h.md).

Conflict GET (F_GETLK only): read another holder's locks on a file.

Reuses a held fork's fd when one exists, else opens+closes a transient one. Reading through a held fd (never a transient open+close on an inode we hold a lock on) is what keeps the held byte locks from being dropped on close. The rfork is never opened onto of->of_ad (would corrupt ad_rlen / underflow the v2 meta refcount).

Data/band fd is fail-closed (any non-ENOENT failure -> ERROR). The rfork-content leg is best-effort (rf_locked stays 0 on open failure / the HAVE_EAFD && SOLARIS skip; the rfork band is still read via the data fd).

Parameters:
* `vol`: volume the target lives on
* `dirfd`: directory fd for openat-style resolution, or -1
* `path`: target; both fork inodes derive from it
* `df_get`: probe data-fork content if non-zero
* `df_off`: data-fork content offset
* `df_len`: data-fork content length (0 = whole data zone)
* `rf_get`: probe resource-fork content if non-zero
* `rf_off`: resource-fork content offset
* `rf_len`: resource-fork content length (0 = whole zone)
* `band_request`: share-mode band bitmap; OR together the per-offset *_BIT aliases from [adouble.h](../../include/atalk/adouble.h.md) (or ALL_BAND_BITS for all ten). Bit n maps to offset AD_FILELOCK_BASE + n.
* `try_root`: retry the open as root on EACCES (the band is the server's own bookkeeping, not a user permission)
* `df_locked`: set if the data-fork content range is locked
* `rf_locked`: set if the resource-fork content range is locked
* `band_held`: positional bitmap of held band bits (same positions as band_request)

Returns: OF_LOCKS_OK (0) outputs valid (all 0 == nothing locked) OF_LOCKS_NOENT (1) target absent; outputs stay zeroed OF_LOCKS_ERROR(-1) lock state indeterminate -> caller must fail closed Outputs are zeroed on entry and only populated on OF_LOCKS_OK, so a caller that ignores the return still reads zeros, never stale data - but the return MUST be checked (zeros do not mean "no locks").

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_openat](../../libatalk/adouble/ad_open.c.md#ad_openat), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_testlock_range](../../libatalk/adouble/ad_lock.c.md#ad_testlock_range), [ad_testlock_whole](../../libatalk/adouble/ad_lock.c.md#ad_testlock_whole), [become_root](../../libatalk/util/unix.c.md#become_root), [of_findname](ofork.c.md#of_findname), [of_findnameat](ofork.c.md#of_findnameat), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Called by: [deletefile](file.c.md#deletefile)

### of_dealloc

```c
void of_dealloc(struct ofork *)
```

Defined at lines 1018 to 1052. Declared in [etc/afpd/fork.h](fork.h.md).

Calls: [of_freeq_push](ofork.c.md#of_freeq_push), [of_unhash](ofork.c.md#of_unhash), [usage_level_update](../../libatalk/util/unix.c.md#usage_level_update)

Called by: [afp_openfork](fork.c.md#afp_openfork), [of_closefork](ofork.c.md#of_closefork)

Uses file-scope variables: `nforks`, `of_curr_forks`, `of_fork_level`, `oforks`

### of_log_usage

```c
void of_log_usage(int level, const char *event)
```

Defined at lines 1063 to 1068. Declared in [etc/afpd/fork.h](fork.h.md).

Log fork-table and descriptor usage at a session event.

After a clean session close no forks remain and only the session's own few descriptors are open; more of either flags a leak.

Parameters:
* `level`: log level
* `event`: session event being logged

Calls: [of_usage_str](ofork.c.md#of_usage_str)

Called by: [afp_dsi_close](afp_dsi.c.md#afp_dsi_close), [afp_dsi_die](afp_dsi.c.md#afp_dsi_die), [afp_dsi_log_disconnect](afp_dsi.c.md#afp_dsi_log_disconnect)

### of_log_nfile

```c
void of_log_nfile(const char *name, int err)
```

Defined at lines 1076 to 1084. Declared in [etc/afpd/fork.h](fork.h.md).

Log an open refused because descriptors ran out.

Parameters:
* `name`: file being opened
* `err`: errno of the failed open

Calls: [log_backoff](../../libatalk/util/unix.c.md#log_backoff), [of_usage_str](ofork.c.md#of_usage_str)

Called by: [afp_openfork](fork.c.md#afp_openfork)

Uses file-scope variables: `of_nfile_refusals`

### of_note_open

```c
void of_note_open(int newfd)
```

Defined at lines 1095 to 1110. Declared in [etc/afpd/fork.h](fork.h.md).

Warn as fork or descriptor use climbs toward its limit.

Runs once the fork is fully open: the open counts, and the breakdown sees its access mode. open(2) returns the lowest free descriptor, so the highest one this open created bounds those in use from below, at no cost.

Parameters:
* `newfd`: highest descriptor the open created, or -1 for none

Calls: [of_usage_str](ofork.c.md#of_usage_str), [usage_level_pct](../../libatalk/util/unix.c.md#usage_level_pct), [usage_level_update](../../libatalk/util/unix.c.md#usage_level_update)

Called by: [afp_openfork](fork.c.md#afp_openfork)

Uses file-scope variables: `nforks`, `of_curr_forks`, `of_fd_level`, `of_fdlimit`, `of_fork_level`, `of_opens`

### of_closefork

```c
int of_closefork(const AFPObj *obj, struct ofork *ofork)
```

Defined at lines 1113 to 1382. Declared in [etc/afpd/fork.h](fork.h.md).

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_data_open](../../include/atalk/adouble.h.md#ad_data_open), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_openforks](../../libatalk/adouble/ad_lock.c.md#ad_openforks), [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [ad_unlock](../../libatalk/adouble/ad_lock.c.md#ad_unlock), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dir_modify](directory.c.md#dir_modify), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [fce_register](fce_api.c.md#fce_register), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [mtoupath](desktop.c.md#mtoupath), [of_dealloc](ofork.c.md#of_dealloc), [ostat](../../libatalk/util/unix.c.md#ostat), [sl_index_event](spotlight.c.md#sl_index_event), [strnlen](../../libatalk/compat/misc.c.md#strnlen), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Called by: [afp_closefork](fork.c.md#afp_closefork), [of_close_inode_forks](ofork.c.md#of_close_inode_forks), [of_close_sweep](ofork.c.md#of_close_sweep)

Calls through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_path](../../libatalk/adouble/ad_open.c.md#ad_path), [ad_path_osx](../../libatalk/adouble/ad_open.c.md#ad_path_osx)

Mentioned in the documentation of: [of_close_inode_forks](ofork.c.md#of_close_inode_forks), [of_close_sweep](ofork.c.md#of_close_sweep)

### of_ad

```c
struct adouble * of_ad(const struct vol *, struct path *, struct adouble *)
```

Defined at lines 1384 to 1398. Declared in [etc/afpd/fork.h](fork.h.md).

Calls: [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [of_findname](ofork.c.md#of_findname)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [getfilparams](file.c.md#getfilparams), [setfilparams](file.c.md#setfilparams)

### of_close_sweep

```c
static void of_close_sweep(const AFPObj *obj, const struct vol *vol, const char *who)
```

Defined at lines 1411 to 1446.

Close every fork on `vol`, or every fork when `vol` is NULL.

Logs what it closed by fork and access mode: at logout these are the forks the client never closed. [of_closefork()](ofork.c.md#of_closefork) deallocs even on flush error, so a failed close is logged and the sweep continues.

Parameters:
* `obj`: AFP session
* `vol`: volume whose forks to close, NULL for all
* `who`: caller name for log lines

Calls: [of_closefork](ofork.c.md#of_closefork), [of_mix_add](ofork.c.md#of_mix_add), [of_mix_str](ofork.c.md#of_mix_str)

Called by: [of_close_all_forks](ofork.c.md#of_close_all_forks), [of_closevol](ofork.c.md#of_closevol)

Uses file-scope variables: `nforks`, `oforks`

### of_closevol

```c
void of_closevol(const AFPObj *obj, const struct vol *vol)
```

Defined at lines 1451 to 1454. Declared in [etc/afpd/fork.h](fork.h.md).

close all forks for a volume

Calls: [of_close_sweep](ofork.c.md#of_close_sweep)

Called by: [closevol](volume.c.md#closevol)

Mentioned in the documentation of: [of_close_inode_forks](ofork.c.md#of_close_inode_forks)

### of_close_all_forks

```c
void of_close_all_forks(const AFPObj *obj)
```

Defined at lines 1459 to 1462. Declared in [etc/afpd/fork.h](fork.h.md).

close all forks

Calls: [of_close_sweep](ofork.c.md#of_close_sweep)

Called by: [afp_logout](auth.c.md#afp_logout)

# Types

### struct of_mix

Defined at line 78.
* `int n`

# Macros

* Undocumented: `OFORK_HASHSIZE`, `OF_MIX_LEN`, `OF_USAGE_LEN`

# File-scope variables

`nforks`, `of_curr_forks`, `of_fd_level`, `of_fdlimit`, `of_fork_level`, `of_freeq`, `of_freeq_cap`, `of_freeq_head`, `of_freeq_tail`, `of_full_refusals`, `of_nfile_refusals`, `of_opens`, `of_peak_forks`, `ofork_table`, `oforks`
