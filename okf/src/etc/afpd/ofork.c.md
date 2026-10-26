---
type: C Source File
title: "etc/afpd/ofork.c"
description: "Open fork table of a session."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/ofork.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-26T14:42:48+11:00 }
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
* [file.h](file.h.md)
* [filedir.h](filedir.h.md)
* [fork.h](fork.h.md)
* [virtual_icon.h](virtual_icon.h.md)
* [volume.h](volume.h.md)
* System headers: `bstrlib.h`, `errno.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/stat.h`, `unistd.h`

# Functions

### of_mix_add

```c
static void of_mix_add(struct of_mix *mix, const struct ofork *of)
```

Defined at lines 95 to 101.

Count one fork into its fork and access bucket.

Parameters:
* `mix`: bucket counts
* `of`: fork to count

Called by: [of_breakdown](ofork.c.md#of_breakdown), [of_close_sweep](ofork.c.md#of_close_sweep)

### of_mix_str

```c
static const char * of_mix_str(const struct of_mix *mix, char *buf, size_t len)
```

Defined at lines 112 to 134.

Format the non-empty buckets, e.g.

"data rd 10, rsrc rw 1"

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

Defined at lines 146 to 157. Declared in [etc/afpd/fork.h](fork.h.md).

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

Defined at lines 167 to 182.

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

Defined at lines 192 to 195.

Bucket of the open fork hash table for a file.

The bucket is the inode masked to the 64 buckets of OFORK_HASHSIZE.

Parameters:
* `key`: device and inode of the file

Returns: the bucket index

Called by: [of_findnameat](ofork.c.md#of_findnameat), [of_hash](ofork.c.md#of_hash), [of_rename](ofork.c.md#of_rename), [of_rfork_changed](ofork.c.md#of_rfork_changed)

### of_hash

```c
static void of_hash(struct ofork *of)
```

Defined at lines 202 to 213.

Insert an open fork at the head of its hash bucket.

Parameters:
* `of`: open fork with its device and inode filled in

Calls: [hashfn](ofork.c.md#hashfn)

Called by: [of_alloc](ofork.c.md#of_alloc), [of_rekey](ofork.c.md#of_rekey)

Uses file-scope variables: `ofork_table`

### of_unhash

```c
static void of_unhash(struct ofork *of)
```

Defined at lines 222 to 234.

Take an open fork out of its hash bucket.

A second call does nothing, because the first one clears the links.

Parameters:
* `of`: open fork

Called by: [of_closefork](ofork.c.md#of_closefork), [of_dealloc](ofork.c.md#of_dealloc), [of_rekey](ofork.c.md#of_rekey)

### of_rekey

```c
void of_rekey(struct ofork *of, const struct stat *st)
```

Defined at lines 243 to 249. Declared in [etc/afpd/fork.h](fork.h.md).

Give an open fork the device and inode of st, in the hash bucket they select.

Parameters:
* `of`: open fork
* `st`: stat of the file the fork opened

Calls: [of_hash](ofork.c.md#of_hash), [of_unhash](ofork.c.md#of_unhash)

Called by: [materialize_virtual_icon](fork.c.md#materialize_virtual_icon)

### of_freeq_empty

```c
static bool of_freeq_empty(void)
```

Defined at lines 256 to 259.

Whether the free slot queue holds no slot.

Returns: true when no fork slot is free

Called by: [of_freeq_pop](ofork.c.md#of_freeq_pop), [of_freeq_push](ofork.c.md#of_freeq_push)

Uses file-scope variables: `of_freeq_head`, `of_freeq_tail`

### of_freeq_push

```c
static void of_freeq_push(uint16_t slot)
```

Defined at lines 269 to 280.

Return a fork slot to the tail of the free slot queue.

The queue holds nforks plus one entries, so it cannot overflow, and an assertion guards that.

Parameters:
* `slot`: slot to free, 1 to nforks minus 1

Calls: [of_freeq_empty](ofork.c.md#of_freeq_empty)

Called by: [of_alloc](ofork.c.md#of_alloc), [of_dealloc](ofork.c.md#of_dealloc)

Uses file-scope variables: `of_freeq`, `of_freeq_cap`, `of_freeq_tail`

### of_freeq_pop

```c
static int of_freeq_pop(void)
```

Defined at lines 287 to 300.

Take the oldest free fork slot from the head of the queue.

Returns: the slot, or -1 when no slot is free

Calls: [of_freeq_empty](ofork.c.md#of_freeq_empty)

Called by: [of_alloc](ofork.c.md#of_alloc)

Uses file-scope variables: `of_freeq`, `of_freeq_cap`, `of_freeq_head`

### of_pforkdesc

```c
void of_pforkdesc(FILE *)
```

Defined at lines 307 to 320. Declared in [etc/afpd/fork.h](fork.h.md).

Print the reference number and name of every open fork.

Parameters:
* `f`: stream to print to

Called by: [afp_over_asp](afp_asp.c.md#afp_over_asp)

Uses file-scope variables: `nforks`, `oforks`

### of_flush

```c
int of_flush(const struct vol *)
```

Defined at lines 330 to 346. Declared in [etc/afpd/fork.h](fork.h.md).

Flush every open fork of a volume through flushfork.

Failures only log.

Parameters:
* `vol`: volume whose forks to flush

Returns: 0

Calls: [flushfork](fork.c.md#flushfork)

Called by: [afp_flush](fork.c.md#afp_flush)

Uses file-scope variables: `nforks`, `oforks`

### of_path_holds

```c
static int of_path_holds(const struct ofork *of, const char *path, struct stat *st)
```

Defined at lines 357 to 363.

Whether a stat of path, by the volume's symlink rule, finds an open fork's file.

Parameters:
* `of`: open fork
* `path`: path to stat
* `st`: receives the stat

Returns: 1 when the stat finds the fork's device and inode, else 0

Calls: [ostat](../../libatalk/util/unix.c.md#ostat)

Called by: [of_closefork](ofork.c.md#of_closefork), [of_name_holds](ofork.c.md#of_name_holds)

### of_name_holds

```c
static int of_name_holds(const struct ofork *of, const struct dir *dir)
```

Defined at lines 373 to 384.

Whether an open fork's name on disk still names its file in dir.

Parameters:
* `of`: open fork
* `dir`: the fork's directory, or NULL

Returns: 1 when a stat of the name, by the volume's symlink rule, finds the fork's device and inode, else 0

Calls: [fullpath_join](directory.c.md#fullpath_join), [of_path_holds](ofork.c.md#of_path_holds)

Called by: [of_hint_siblings](ofork.c.md#of_hint_siblings), [of_rename](ofork.c.md#of_rename)

### of_rename

```c
int of_rename(const struct vol *, struct ofork *, struct dir *, const char *, struct dir *, const char *, const char *)
```

Defined at lines 403 to 452. Declared in [etc/afpd/fork.h](fork.h.md).

Point the open forks of a renamed file at its new name and directory.

Every fork with the volume, parent directory, device, and inode of s_of that records the old name, or a name that no longer holds the file, follows: another name of a hard link keeps its forks. The name in the shared AppleDouble changes once, and each fork moves to newdir.

Parameters:
* `vol`: volume of the file
* `s_of`: one open fork of the file, or NULL when none is open
* `olddir`: previous parent directory
* `oldupath`: previous name on disk
* `newdir`: new parent directory
* `newpath`: new name
* `newupath`: new name on disk, recorded in each fork

Returns: AFP_OK, or AFPERR_MISC when strdup fails

Calls: [hashfn](ofork.c.md#hashfn), [of_name_holds](ofork.c.md#of_name_holds)

Called by: [afp_exchangefiles](file.c.md#afp_exchangefiles), [moveandrename](filedir.c.md#moveandrename)

Uses file-scope variables: `ofork_table`

### of_alloc

```c
struct ofork * of_alloc(struct vol *, struct dir *, char *, const char *, uint16_t *, const int, struct adouble *, struct stat *)
```

Defined at lines 475 to 611. Declared in [etc/afpd/fork.h](fork.h.md).

Allocate an open fork slot and its reference number.

The first call sizes the table to half the descriptor limit, capped at 65535 slots, because a fork can hold two descriptors. Slot 0 stays reserved, and the reference number equals the slot. A shared AppleDouble gains a reference, otherwise the fork gets a new one named path. The fork enters the hash table under the device and inode of st.

Parameters:
* `vol`: volume of the file
* `dir`: parent directory
* `path`: file name, for a new AppleDouble
* `upath`: file name on disk, recorded in the fork
* `ofrefnum`: receives the reference number
* `eid`: ADEID_DFORK or ADEID_RFORK
* `ad`: AppleDouble of another fork of the file, or NULL
* `st`: stat of the file

Returns: the open fork, or NULL when the table is full or an allocation failed

Calls: [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [log_backoff](../../libatalk/util/unix.c.md#log_backoff), [of_freeq_pop](ofork.c.md#of_freeq_pop), [of_freeq_push](ofork.c.md#of_freeq_push), [of_hash](ofork.c.md#of_hash), [of_usage_str](ofork.c.md#of_usage_str)

Called by: [afp_openfork](fork.c.md#afp_openfork)

Uses file-scope variables: `nforks`, `of_curr_forks`, `of_fdlimit`, `of_freeq`, `of_freeq_cap`, `of_full_refusals`, `of_peak_forks`, `oforks`

### of_find

```c
struct ofork * of_find(const uint16_t)
```

Defined at lines 622 to 643. Declared in [etc/afpd/fork.h](fork.h.md).

Open fork with a reference number.

A slot that now holds a different fork means the requested fork already closed, and the call logs that and fails.

Parameters:
* `ofrefnum`: reference number from the request

Returns: the open fork, or NULL

Called by: [afp_closefork](fork.c.md#afp_closefork), [afp_flushfork](fork.c.md#afp_flushfork), [afp_getforkparams](fork.c.md#afp_getforkparams), [afp_setforkparams](fork.c.md#afp_setforkparams), [afp_syncfork](fork.c.md#afp_syncfork), [byte_lock](fork.c.md#byte_lock), [read_fork](fork.c.md#read_fork), [write_fork](fork.c.md#write_fork)

Uses file-scope variables: `nforks`, `oforks`

### of_stat

```c
int of_stat(const struct vol *vol, struct path *)
```

Defined at lines 656 to 669. Declared in [etc/afpd/fork.h](fork.h.md).

Stat path->u_name in the working directory and record the outcome in path.

The stat follows symlinks only when the volume does. The st_errno field receives the errno of a failure, and st_valid becomes 1 either way.

Parameters:
* `vol`: volume of the path
* `path`: path whose stat to fill

Returns: 0, or -1 when the stat failed

Calls: [ostat](../../libatalk/util/unix.c.md#ostat)

Called by: [afp_createdir](directory.c.md#afp_createdir), [afp_createid](file.c.md#afp_createid), [afp_opendir](directory.c.md#afp_opendir), [afp_resolveid](file.c.md#afp_resolveid), [catsearch](catsearch.c.md#catsearch), [catsearch_db](catsearch.c.md#catsearch_db), [cname](directory.c.md#cname), [enumerate_in](enumerate.c.md#enumerate_in), [of_statdir](ofork.c.md#of_statdir), [sd_stat](enumerate.c.md#sd_stat), [setfilparams](file.c.md#setfilparams), [setfilunixmode](unix.c.md#setfilunixmode)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

### of_statdir

```c
int of_statdir(struct vol *vol, struct path *)
```

Defined at lines 684 to 729. Declared in [etc/afpd/fork.h](fork.h.md).

Stat the directory that path names, also when it is the working directory.

A path with a name defers to of_stat. For the working directory the call stats ../name instead of ".", because stat(".") succeeds even after the directory went away. EACCES on that stat moves up to the parent and stats the name from there.

Parameters:
* `vol`: volume of the directory
* `path`: path whose stat to fill, with st_errno and st_valid set

Returns: 0, or -1 when the stat or the change of directory failed

Calls: [dirlookup](directory.c.md#dirlookup), [movecwd](directory.c.md#movecwd), [of_stat](ofork.c.md#of_stat), [ostat](../../libatalk/util/unix.c.md#ostat)

Called by: [afp_access](acls.c.md#afp_access), [afp_delete](filedir.c.md#afp_delete), [afp_getacl](acls.c.md#afp_getacl), [afp_getfildirparams](filedir.c.md#afp_getfildirparams), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_setacl](acls.c.md#afp_setacl), [afp_setfildirparams](filedir.c.md#afp_setfildirparams), [ea_cache_entry](ad_cache.c.md#ea_cache_entry)

Uses file-scope variables: `curdir` in [etc/afpd/directory.c](directory.c.md)

Mentioned in the documentation of: [ea_cache_entry](ad_cache.c.md#ea_cache_entry)

### of_findname

```c
struct ofork * of_findname(const struct vol *vol, struct path *)
```

Defined at lines 740 to 743. Declared in [etc/afpd/fork.h](fork.h.md).

Open fork of the file that path names in the working directory, matched by device and inode.

Parameters:
* `vol`: volume of the path
* `path`: path of the file, its stat filled when missing

Returns: an open fork of the file, or NULL when none is open or the stat failed

Calls: [of_findnameat](ofork.c.md#of_findnameat)

Called by: [ad_addcomment](desktop.c.md#ad_addcomment), [ad_getcomment](desktop.c.md#ad_getcomment), [ad_rmvcomment](desktop.c.md#ad_rmvcomment), [adl_lkup](catsearch.c.md#adl_lkup), [afp_copyfile](file.c.md#afp_copyfile), [afp_createfile](file.c.md#afp_createfile), [afp_delete](filedir.c.md#afp_delete), [afp_getextattr](extattrs.c.md#afp_getextattr), [afp_listextattr](extattrs.c.md#afp_listextattr), [afp_openfork](fork.c.md#afp_openfork), [afp_remextattr](extattrs.c.md#afp_remextattr), [afp_setextattr](extattrs.c.md#afp_setextattr), [find_adouble](file.c.md#find_adouble), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [moveandrename](filedir.c.md#moveandrename), [of_ad](ofork.c.md#of_ad), [of_close_inode_forks](ofork.c.md#of_close_inode_forks)

### of_findnameat

```c
struct ofork * of_findnameat(const struct vol *vol, int dirfd, struct path *path)
```

Defined at lines 758 to 787. Declared in [etc/afpd/fork.h](fork.h.md).

Open fork of the file that path names relative to dirfd, matched by device and inode.

A path without a valid stat gets one first, following symlinks as the volume does: forks are keyed by that stat.

Parameters:
* `vol`: volume of the path
* `dirfd`: directory descriptor, -1 for the working directory
* `path`: path of the file, its stat filled when missing

Returns: an open fork of the file, or NULL when none is open or the stat failed

Calls: [hashfn](ofork.c.md#hashfn), [ostatat](../../libatalk/util/unix.c.md#ostatat)

Called by: [check_delete_inhibit](file.c.md#check_delete_inhibit), [moveandrename](filedir.c.md#moveandrename), [of_findname](ofork.c.md#of_findname), [of_get_locks](ofork.c.md#of_get_locks)

Uses file-scope variables: `ofork_table`

### of_delete_blocked

```c
int of_delete_blocked(const struct ofork *of, uint16_t *band, int *content)
```

Defined at lines 812 to 850. Declared in [etc/afpd/fork.h](fork.h.md).

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

Defined at lines 869 to 914. Declared in [etc/afpd/fork.h](fork.h.md).

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

Defined at lines 955 to 1197. Declared in [etc/afpd/fork.h](fork.h.md).

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

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_openat](../../libatalk/adouble/ad_open.c.md#ad_openat), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_testlock_range](../../libatalk/adouble/ad_lock.c.md#ad_testlock_range), [ad_testlock_whole](../../libatalk/adouble/ad_lock.c.md#ad_testlock_whole), [become_root](../../libatalk/util/unix.c.md#become_root), [of_findnameat](ofork.c.md#of_findnameat), [unbecome_root](../../libatalk/util/unix.c.md#unbecome_root)

Called by: [deletefile](file.c.md#deletefile)

### of_dealloc

```c
void of_dealloc(struct ofork *)
```

Defined at lines 1208 to 1243. Declared in [etc/afpd/fork.h](fork.h.md).

Free an open fork and return its slot to the free slot queue.

The fork leaves the hash table, and its AppleDouble drops one reference. The last reference frees the AppleDouble. A reference count that already reached zero logs an error and asserts.

Parameters:
* `of`: open fork, invalid afterwards

Calls: [of_freeq_push](ofork.c.md#of_freeq_push), [of_unhash](ofork.c.md#of_unhash), [usage_level_update](../../libatalk/util/unix.c.md#usage_level_update)

Called by: [afp_openfork](fork.c.md#afp_openfork), [of_closefork](ofork.c.md#of_closefork)

Uses file-scope variables: `nforks`, `of_curr_forks`, `of_fork_level`, `oforks`

### of_log_usage

```c
void of_log_usage(int level, const char *event)
```

Defined at lines 1254 to 1259. Declared in [etc/afpd/fork.h](fork.h.md).

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

Defined at lines 1267 to 1275. Declared in [etc/afpd/fork.h](fork.h.md).

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

Defined at lines 1286 to 1301. Declared in [etc/afpd/fork.h](fork.h.md).

Warn as fork or descriptor use climbs toward its limit.

Runs once the fork is fully open: the open counts, and the breakdown sees its access mode. open(2) returns the lowest free descriptor, so the highest one this open created bounds those in use from below, at no cost.

Parameters:
* `newfd`: highest descriptor the open created, or -1 for none

Calls: [of_usage_str](ofork.c.md#of_usage_str), [usage_level_pct](../../libatalk/util/unix.c.md#usage_level_pct), [usage_level_update](../../libatalk/util/unix.c.md#usage_level_update)

Called by: [afp_openfork](fork.c.md#afp_openfork)

Uses file-scope variables: `nforks`, `of_curr_forks`, `of_fd_level`, `of_fdlimit`, `of_fork_level`, `of_opens`

### of_dircache_entry

```c
struct dir * of_dircache_entry(const struct ofork *ofork)
```

Defined at lines 1313 to 1325. Declared in [etc/afpd/fork.h](fork.h.md).

The dircache entry of an open fork's file, or NULL.

The entry under the fork's name on disk counts only while it describes the open file: another session's rename or save can put a different file under that name.

Parameters:
* `ofork`: open fork

Returns: the entry whose inode is the open file's, or NULL

Calls: [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup)

Called by: [fork_modified](fork.c.md#fork_modified), [of_rfork_changed](ofork.c.md#of_rfork_changed), [read_fork](fork.c.md#read_fork)

### of_rfork_changed

```c
void of_rfork_changed(const struct ofork *ofork)
```

Defined at lines 1336 to 1353. Declared in [etc/afpd/fork.h](fork.h.md).

Drop the cached resource fork data of every open fork on ofork's adouble.

The session's forks of one file share its adouble, and each name of a hard link has a dircache entry of its own.

Parameters:
* `ofork`: open fork whose resource fork changed

Calls: [dir_modify](directory.c.md#dir_modify), [hashfn](ofork.c.md#hashfn), [of_dircache_entry](ofork.c.md#of_dircache_entry)

Called by: [fork_modified](fork.c.md#fork_modified)

Uses file-scope variables: `ofork_table`

### of_hint_siblings

```c
void of_hint_siblings(const AFPObj *obj, const struct ofork *ofork, const struct dir *cached)
```

Defined at lines 1367 to 1387. Declared in [etc/afpd/fork.h](fork.h.md).

Tell sibling sessions that an open fork's file has changed.

The file's id is that of cached, the fork's dircache entry, when there is one, else the one its metadata records for the open file's device and inode, else the CNID database's for the name, while the name holds the fork's file.

Parameters:
* `obj`: session object
* `ofork`: open fork whose file changed
* `cached`: the fork's dircache entry, or NULL

Calls: [ad_getid](../../libatalk/adouble/ad_attr.c.md#ad_getid), [ad_meta_loaded](../../include/atalk/adouble.h.md#ad_meta_loaded), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [dirlookup](directory.c.md#dirlookup), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [of_name_holds](ofork.c.md#of_name_holds)

Called by: [fork_modified](fork.c.md#fork_modified), [of_closefork](ofork.c.md#of_closefork)

### of_rfork_held

```c
int of_rfork_held(const struct ofork *ofork, const char *forkpath)
```

Defined at lines 1401 to 1419. Declared in [etc/afpd/fork.h](fork.h.md).

Whether the AppleDouble file under a fork's name holds the resource fork the fork's descriptor reads.

Forks of one inode share an adouble, while each name of a hard link has an AppleDouble file of its own. A resource fork in an EA belongs to the data file.

Parameters:
* `ofork`: open fork that holds the resource fork
* `forkpath`: path of the fork's name

Returns: 1 when they are one file, else 0

Called by: [of_closefork](ofork.c.md#of_closefork), [read_fork](fork.c.md#read_fork)

Calls through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_path](../../libatalk/adouble/ad_open.c.md#ad_path), [ad_path_osx](../../libatalk/adouble/ad_open.c.md#ad_path_osx)

### of_closefork

```c
int of_closefork(const AFPObj *obj, struct ofork *ofork)
```

Defined at lines 1445 to 1719. Declared in [etc/afpd/fork.h](fork.h.md).

Close an open fork and free it.

A closed resource fork gets its length refreshed and, when dirty, its modification date stamped. A modified fork sends a refresh hint to sibling afpd processes. The steps that go by the fork's name run only while the name holds the fork's file: a modified fork reports FCE and Spotlight events and refreshes its dircache entry. On a volume that is not read-only, without HAVE_EAFD, the call deletes the empty ._ resource fork of a regular file that no other fork holds while no other process has the file open. On a volume with 'legacy icon' that is not read-only, the close of a resource fork that is the session's last fork of a regular Icon file at the volume root also deletes the file, with its AppleDouble file and CNID, when its data fork is empty, its resource fork is a map without resources, or shorter than a header after the fork changed it or found a header in it at its open, no other process has the file open, and FPDelete's checks allow it; FCE and Spotlight hear of that as a deletion. Once that removal is chosen, the name's dircache entry goes, whether or not the file does. A stamped date marks the volume changed.

Parameters:
* `obj`: session object
* `ofork`: open fork, invalid afterwards

Returns: 0, or -1 with errno set when ad_close failed

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_data_open](../../include/atalk/adouble.h.md#ad_data_open), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_meta_loaded](../../include/atalk/adouble.h.md#ad_meta_loaded), [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [ad_testlock_range](../../libatalk/adouble/ad_lock.c.md#ad_testlock_range), [ad_unlock](../../libatalk/adouble/ad_lock.c.md#ad_unlock), [check_delete_inhibit](file.c.md#check_delete_inhibit), [cnid_delete](../../libatalk/cnid/cnid.c.md#cnid_delete), [cnid_get](../../libatalk/cnid/cnid.c.md#cnid_get), [deletefile](file.c.md#deletefile), [dir_modify](directory.c.md#dir_modify), [dir_offcnt_take](filedir.c.md#dir_offcnt_take), [dir_remove](directory.c.md#dir_remove), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [fce_register](fce_api.c.md#fce_register), [fullpath_join](directory.c.md#fullpath_join), [ipc_send_cache_hint](../../libatalk/util/server_ipc.c.md#ipc_send_cache_hint), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_dealloc](ofork.c.md#of_dealloc), [of_hint_siblings](ofork.c.md#of_hint_siblings), [of_path_holds](ofork.c.md#of_path_holds), [of_rfork_held](ofork.c.md#of_rfork_held), [of_unhash](ofork.c.md#of_unhash), [setvoltime](volume.c.md#setvoltime), [sl_index_event](spotlight.c.md#sl_index_event), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled)

Called by: [afp_closefork](fork.c.md#afp_closefork), [of_close_inode_forks](ofork.c.md#of_close_inode_forks), [of_close_sweep](ofork.c.md#of_close_sweep)

Calls through [`adouble_fops::ad_path`](../../include/atalk/adouble.h.md#struct-adouble_fops): [ad_path](../../libatalk/adouble/ad_open.c.md#ad_path), [ad_path_osx](../../libatalk/adouble/ad_open.c.md#ad_path_osx)

Mentioned in the documentation of: [fork_modified](fork.c.md#fork_modified), [of_close_inode_forks](ofork.c.md#of_close_inode_forks), [of_close_sweep](ofork.c.md#of_close_sweep)

### of_ad

```c
struct adouble * of_ad(const struct vol *, struct path *, struct adouble *)
```

Defined at lines 1731 to 1745. Declared in [etc/afpd/fork.h](fork.h.md).

AppleDouble to use for a file, shared with an open fork when there is one.

Parameters:
* `vol`: volume of the file
* `path`: path of the file, its stat filled when missing
* `ad`: caller AppleDouble to initialize when no fork holds the file

Returns: the AppleDouble of the open fork, or ad after ad_init

Calls: [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [of_findname](ofork.c.md#of_findname)

Called by: [afp_copyfile](file.c.md#afp_copyfile), [getfilparams](file.c.md#getfilparams), [setfilparams](file.c.md#setfilparams)

### of_close_sweep

```c
static void of_close_sweep(const AFPObj *obj, const struct vol *vol, const char *who)
```

Defined at lines 1758 to 1793.

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

Defined at lines 1801 to 1804. Declared in [etc/afpd/fork.h](fork.h.md).

Close every open fork of a volume.

Parameters:
* `obj`: session object
* `vol`: volume whose forks to close

Calls: [of_close_sweep](ofork.c.md#of_close_sweep)

Called by: [closevol](volume.c.md#closevol)

Mentioned in the documentation of: [of_close_inode_forks](ofork.c.md#of_close_inode_forks)

### of_close_all_forks

```c
void of_close_all_forks(const AFPObj *obj)
```

Defined at lines 1811 to 1814. Declared in [etc/afpd/fork.h](fork.h.md).

Close every open fork of the session.

Parameters:
* `obj`: session object

Calls: [of_close_sweep](ofork.c.md#of_close_sweep)

Called by: [afp_logout](auth.c.md#afp_logout)

# Types

### struct of_mix

Defined at line 85.
* `int n`

# Macros

* Undocumented: `OFORK_HASHSIZE`, `OF_MIX_LEN`, `OF_USAGE_LEN`

# File-scope variables

`nforks`, `of_curr_forks`, `of_fd_level`, `of_fdlimit`, `of_fork_level`, `of_freeq`, `of_freeq_cap`, `of_freeq_head`, `of_freeq_tail`, `of_full_refusals`, `of_nfile_refusals`, `of_opens`, `of_peak_forks`, `ofork_table`, `oforks`
