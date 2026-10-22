---
type: C Source File
title: "etc/afpd/fork.c"
description: "Fork commands: FPOpenFork, FPRead, FPWrite, FPCloseFork, byte range locks, and the flush commands."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fork.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-22T19:27:15+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/fce_api.h](../../include/atalk/fce_api.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
* [atalk/spotlight.h](../../include/atalk/spotlight.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [desktop.h](desktop.h.md)
* [dircache.h](dircache.h.md)
* [directory.h](directory.h.md)
* [file.h](file.h.md)
* [fork.h](fork.h.md)
* [virtual_icon.h](virtual_icon.h.md)
* [volume.h](volume.h.md)
* System headers: `bstrlib.h`, `errno.h`, `fcntl.h`, `inttypes.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/socket.h`, `unistd.h`

# Functions

### ad_refs_now

```c
static struct ad_refs ad_refs_now(const struct adouble *ad)
```

Defined at lines 72 to 77.

Snapshot the reference counts of an adouble's forks.

Parameters:
* `ad`: adouble about to be opened

Returns: its counts

Called by: [afp_openfork](fork.c.md#afp_openfork), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon)

### ad_refs_taken

```c
static int ad_refs_taken(const struct adouble *ad, struct ad_refs before)
```

Defined at lines 87 to 92.

ADFLAGS_DF, ADFLAGS_HF and ADFLAGS_RF of the forks whose references rose above a snapshot: what an open took, for its close.

Parameters:
* `ad`: adouble after the open
* `before`: its counts before the open

Returns: the ADFLAGS of the forks the open referenced

Called by: [afp_openfork](fork.c.md#afp_openfork), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon)

### adf_readonly

```c
static int adf_readonly(const struct ad_fd *adf)
```

Defined at lines 100 to 103.

Whether an AppleDouble descriptor is open read-only.

Parameters:
* `adf`: descriptor and the flags it was opened with

Returns: 1 for an open read-only descriptor, else 0

Called by: [materialize_virtual_icon](fork.c.md#materialize_virtual_icon)

### materialize_virtual_icon

```c
static int materialize_virtual_icon(const AFPObj *obj, struct ofork *ofork, bool is_write)
```

Defined at lines 131 to 354.

Turn the virtual Icon file into a real file before a client changes it.

The write path and FPSetForkParms call it. It creates the Icon file in the volume root, seeds its resource fork with the virtual icon data and marks it invisible; a file another fork created first keeps its content. The call then reopens the fork through ad_open, on the adouble of the session's open fork of the file when there is one, so the open fork holds real descriptors, takes the open-mode locks of its access, and drops its virtual state. A fork whose name a symlink holds on a volume without 'follow symlinks', whose seed, reopen or locks fail, or whose file is write-inhibited where the volume does not ignore it, stays virtual, with nothing open; a file the call created goes again while the name holds it, unless another process claims it, and then counts as created.

Parameters:
* `obj`: session object
* `ofork`: open fork on the virtual Icon file
* `is_write`: called for an FPWrite, else for FPSetForkParms

Returns: AFP_OK; AFPERR_LOCK when another open's deny modes refuse the locks, AFPERR_ACCESS when a symlink holds the name on a volume without 'follow symlinks', the session's open fork of the file holds a read-only descriptor the fork needs, or the file is write-inhibited where the volume does not ignore it, the reply [fork_error()](fork.c.md#fork_error) gives a failed create, seed or open, else AFPERR_MISC

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_refs_now](fork.c.md#ad_refs_now), [ad_refs_taken](fork.c.md#ad_refs_taken), [ad_unlock](../../libatalk/adouble/ad_lock.c.md#ad_unlock), [ad_write](../../libatalk/adouble/ad_write.c.md#ad_write), [adf_readonly](fork.c.md#adf_readonly), [deletefile](file.c.md#deletefile), [fce_register](fce_api.c.md#fce_register), [fork_error](fork.c.md#fork_error), [fork_setmode](fork.c.md#fork_setmode), [of_findname](ofork.c.md#of_findname), [of_rekey](ofork.c.md#of_rekey), [sl_index_event](spotlight.c.md#sl_index_event)

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [write_fork](fork.c.md#write_fork)

### getforkparams

```c
static int getforkparams(const AFPObj *obj, struct ofork *ofork, uint16_t bitmap, char *buf, size_t *buflen)
```

Defined at lines 373 to 465.

Pack the file parameters of an open fork.

Length bits must match the fork. A data length request on a resource fork fails with AFPERR_BITMAP, and a resource length request on a data fork fails too. Virtual forks report synthesized parameters. The stat comes from the fork descriptor when there is one, so a fork that outlived its path still reports the held object.

Parameters:
* `obj`: session object
* `ofork`: open fork
* `bitmap`: AFP file bitmap, FILPBIT_* bits
* `buf`: reply buffer receiving the parameters
* `buflen`: receives the bytes written

Returns: AFP_OK, AFPERR_BITMAP, AFPERR_NOOBJ, AFPERR_MISC, or the result of getmetadata

Calls: [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getmetadata](file.c.md#getmetadata), [movecwd](directory.c.md#movecwd), [mtoupath](desktop.c.md#mtoupath), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Called by: [afp_getforkparams](fork.c.md#afp_getforkparams), [afp_openfork](fork.c.md#afp_openfork)

### get_off_t

```c
static off_t get_off_t(char **ibuf, int is64)
```

Defined at lines 477 to 495.

Read a fork offset or length from a request.

A 64-bit field takes 8 bytes in network order. A 32-bit field takes 4 bytes, and the call sign-extends it.

Parameters:
* `ibuf`: request cursor, advanced past the field
* `is64`: nonzero for the 64-bit field of the Ext commands

Returns: the value

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [byte_lock](fork.c.md#byte_lock), [read_fork](fork.c.md#read_fork), [write_fork](fork.c.md#write_fork)

### set_off_t

```c
static int set_off_t(off_t offset, char *rbuf, int is64)
```

Defined at lines 506 to 524.

Write a fork offset or length into a reply in network order.

Parameters:
* `offset`: value to write
* `rbuf`: reply position
* `is64`: nonzero for the 8-byte field of the Ext commands, 0 for 4 bytes

Returns: the number of bytes written, 8 for the 64-bit field and 4 otherwise

Called by: [byte_lock](fork.c.md#byte_lock), [write_fork](fork.c.md#write_fork)

### is_neg

```c
static int is_neg(int is64, off_t val)
```

Defined at lines 537 to 544.

Whether an offset or length is negative in the field width of the request.

With a 64-bit off_t, a 32-bit request also counts a value with bit 31 set as negative.

Parameters:
* `is64`: nonzero for the 64-bit field of the Ext commands
* `val`: offset or length

Returns: 1 when the value is negative, 0 otherwise

Called by: [byte_lock](fork.c.md#byte_lock)

### sum_neg

```c
static int sum_neg(int is64, off_t offset, off_t reqcount, off_t base)
```

Defined at lines 558 to 562.

Whether offset plus reqcount passes the largest offset the field width of the request holds, or, in a 64-bit field, passes off_t once the fork's offset in its file is added.

Parameters:
* `is64`: nonzero for the 64-bit field of the Ext commands
* `offset`: start offset, not negative
* `reqcount`: byte count, not negative
* `base`: the fork's offset in its file

Returns: 1 when the sum passes INT32_MAX for a 32-bit field or INT64_MAX less base for a 64-bit one, 0 otherwise

Called by: [write_fork](fork.c.md#write_fork)

### fork_setmode_deny

```c
int fork_setmode_deny(int access, int f_rddny, int f_wrdny, int f_nodny)
```

Defined at lines 577 to 581.

F_SHARE deny bits for an FPOpenFork access mode.

Each deny bit maps separately, and the results combine with a bitwise or. The constants come in as parameters, so a unit test can exercise the mapping without fshare_t.

Parameters:
* `access`: OPENACC_* access mode
* `f_rddny`: value for a read deny, F_RDDNY where fshare_t exists
* `f_wrdny`: value for a write deny, F_WRDNY where fshare_t exists
* `f_nodny`: value for no deny, F_NODNY where fshare_t exists

Returns: the combined f_deny value

Called by: [fork_setmode](fork.c.md#fork_setmode)

### fork_setmode

```c
static int fork_setmode(const AFPObj *obj, struct adouble *adp, int eid, int access, int ofrefnum)
```

Defined at lines 599 to 724.

Apply the access and deny modes of an FPOpenFork through band locks.

Each requested access fails with EACCES when another fork denies it, and each requested deny fails when another fork already holds that access. The call then takes a band lock for every access and deny bit, or the OPEN_NONE band lock when the mode names none. With HAVE_FSHARE_T an F_SHARE reservation adds kernel enforcement.

Parameters:
* `obj`: session object, unused
* `adp`: AppleDouble of the fork
* `eid`: ADEID_DFORK or ADEID_RFORK
* `access`: OPENACC_* access and deny bits
* `ofrefnum`: reference number of the opening fork

Returns: 0, or -1 with errno set, EACCES for a mode conflict

Calls: [ad_lock](../../libatalk/adouble/ad_lock.c.md#ad_lock), [ad_testlock](../../libatalk/adouble/ad_lock.c.md#ad_testlock), [fork_setmode_deny](fork.c.md#fork_setmode_deny)

Called by: [afp_openfork](fork.c.md#afp_openfork), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon)

### afp_openfork

```c
int afp_openfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 751 to 1176.

FPOpenFork: open the data or resource fork of a file with access and deny modes.

OPENFORK_RSCS, bit 7 of the fork byte, selects the resource fork. A write open on a read-only volume fails with AFPERR_VLOCK, and a directory with AFPERR_BADTYPE. The virtual Icon file of the volume root opens without a descriptor. A real fork opens through ad_open with share modes, and the metadata opens too, created for a write open.

The write-inhibit attribute refuses a write open with AFPERR_OLOCK, and a deny mode conflict fails with AFPERR_DENYCONF. The reply carries the bitmap, the fork reference number, and the fork parameters. Running out of descriptors fails with AFPERR_NFILE.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_DENYCONF, AFPERR_OLOCK, AFPERR_ACCESS, AFPERR_LOCK, AFPERR_NFILE, AFPERR_VLOCK, AFPERR_BADTYPE, AFPERR_NOOBJ, or AFPERR_PARAM

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_refs_now](fork.c.md#ad_refs_now), [ad_refs_taken](fork.c.md#ad_refs_taken), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_unlock](../../libatalk/adouble/ad_lock.c.md#ad_unlock), [check_access](directory.c.md#check_access), [cname](directory.c.md#cname), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dirlookup](directory.c.md#dirlookup), [dirlookup_strict](directory.c.md#dirlookup_strict), [file_access](directory.c.md#file_access), [fork_setmode](fork.c.md#fork_setmode), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [get_afp_errno](directory.c.md#get_afp_errno), [get_id](file.c.md#get_id), [getforkparams](fork.c.md#getforkparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_alloc](ofork.c.md#of_alloc), [of_dealloc](ofork.c.md#of_dealloc), [of_findname](ofork.c.md#of_findname), [of_log_nfile](ofork.c.md#of_log_nfile), [of_note_open](ofork.c.md#of_note_open), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled), [virtual_icon_get_rfork](virtual_icon.c.md#virtual_icon_get_rfork)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### fork_modified

```c
static void fork_modified(const AFPObj *obj, struct ofork *ofork)
```

Defined at lines 1187 to 1200.

Mark an open fork changed: its volume is marked, a resource fork's cached content goes under every name the session has the file open by, and the fork's first change sends sibling sessions a refresh hint, [of_closefork()](ofork.c.md#of_closefork) the last.

Parameters:
* `obj`: session object
* `ofork`: open fork a write or a length set reached

Calls: [of_dircache_entry](ofork.c.md#of_dircache_entry), [of_hint_siblings](ofork.c.md#of_hint_siblings), [of_rfork_changed](ofork.c.md#of_rfork_changed), [setvoltime_write](volume.c.md#setvoltime_write)

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [write_fork](fork.c.md#write_fork)

### fork_error

```c
static int fork_error(const struct ofork *ofork, const char *call, bool is_write)
```

Defined at lines 1213 to 1244.

Map the errno of a failed fork write, truncate or header flush, or of the virtual Icon file's create or open, to the AFP reply.

Parameters:
* `ofork`: fork being changed
* `call`: libatalk call that failed, named in the log
* `is_write`: an FPWrite, whose replies hold no AFPERR_VLOCK

Returns: AFPERR_DFULL or AFPERR_ACCESS, AFPERR_VLOCK for a read-only file system outside an FPWrite, otherwise AFPERR_MISC

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [write_file](fork.c.md#write_file), [write_fork](fork.c.md#write_fork)

Mentioned in the documentation of: [materialize_virtual_icon](fork.c.md#materialize_virtual_icon)

### afp_setforkparams

```c
int afp_setforkparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1264 to 1432.

FPSetForkParms: change the length of an open fork.

The bitmap must name exactly the length of the open fork type, and the 64-bit form needs AFP 3.0. A negative length fails. Shrinking takes a temporary write lock on the cut range, so a conflicting byte lock fails the call with AFPERR_LOCK. The virtual Icon file becomes a real file first. A resource fork change invalidates the cached resource fork data, and a length change marks the volume.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_BITMAP, AFPERR_LOCK, AFPERR_ACCESS, AFPERR_VLOCK, AFPERR_PARAM, or the AFPERR_* code of fork_error

Calls: [ad_dtruncate](../../libatalk/adouble/ad_write.c.md#ad_dtruncate), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [ad_rtruncate](../../libatalk/adouble/ad_write.c.md#ad_rtruncate), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [ad_tmplock](../../libatalk/adouble/ad_lock.c.md#ad_tmplock), [dirlookup](directory.c.md#dirlookup), [fork_error](fork.c.md#fork_error), [fork_modified](fork.c.md#fork_modified), [get_off_t](fork.c.md#get_off_t), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [movecwd](directory.c.md#movecwd), [of_find](ofork.c.md#of_find)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### byte_lock

```c
static int byte_lock(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int is64)
```

Defined at lines 1462 to 1550.

Shared body of FPByteRangeLock and FPByteRangeLockExt.

The flag byte chooses between taking and releasing the lock, and whether the offset counts from the end of the fork. A length of -1 extends the range to BYTELOCK_MAX, below the share mode band. Locks on virtual forks succeed without effect. The reply carries the start offset of the range.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, receives the range start
* `rbuflen`: reply buffer capacity on entry, reply length on return
* `is64`: nonzero for FPByteRangeLockExt

Returns: AFP_OK, AFPERR_LOCK, AFPERR_NORANGE, AFPERR_NLOCK, AFPERR_RANGEOVR, or AFPERR_PARAM

Calls: [ad_lock](../../libatalk/adouble/ad_lock.c.md#ad_lock), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [get_off_t](fork.c.md#get_off_t), [is_neg](fork.c.md#is_neg), [of_find](ofork.c.md#of_find), [set_off_t](fork.c.md#set_off_t)

Called by: [afp_bytelock](fork.c.md#afp_bytelock), [afp_bytelock_ext](fork.c.md#afp_bytelock_ext)

### afp_bytelock

```c
int afp_bytelock(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1564 to 1568.

FPByteRangeLock: take or release a byte range lock on an open fork with 32-bit fields.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, receives the range start
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or the AFPERR_* code of byte_lock

Calls: [byte_lock](fork.c.md#byte_lock)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_bytelock_ext

```c
int afp_bytelock_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1582 to 1586.

FPByteRangeLockExt: take or release a byte range lock on an open fork with 64-bit fields.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, receives the range start
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or the AFPERR_* code of byte_lock

Calls: [byte_lock](fork.c.md#byte_lock)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### read_file

```c
static int read_file(const struct ofork *ofork, int eid, off_t offset, char *rbuf, size_t *rbuflen)
```

Defined at lines 1603 to 1629.

Read *rbuflen bytes from fork at offset.

Parameters:
* `ofork`: fork handle
* `eid`: data fork or resource fork entry id
* `offset`: offset
* `rbuf`: data buffer
* `rbuflen`: in: number of bytes to read, out: bytes read

Returns: AFP_OK, AFPERR_EOF when the fork ends first, AFPERR_PARAM when the request is shorter than the rest of a symlink's target, or AFPERR_MISC when the read fails

Calls: [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read)

Called by: [read_fork](fork.c.md#read_fork)

### rfork_cache_serve_from_buf

```c
static int rfork_cache_serve_from_buf(DSI *dsi, struct dir *cached_entry, off_t offset, off_t reqcount, int err)
```

Defined at lines 1647 to 1690.

Serve resource fork data from Tier 2 cache via [DSI](../../include/atalk/dsi.h.md#struct-dsi) framing.

Shared helper for cache-hit and just-populated cache paths in [read_fork()](fork.c.md#read_fork). Sends the reply from the cache buffer in one [dsi_readinit()](../../libatalk/dsi/dsi_read.c.md#dsi_readinit) and promotes the entry in the rfork LRU.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) connection
* `cached_entry`: dir entry with populated dcache_rfork_buf
* `offset`: Starting byte offset in the rfork
* `reqcount`: Requested byte count
* `err`: AFP error code (AFPERR_EOF or AFP_OK)

Returns: 0 when the reply went out, -1 when a send failed or the entry cannot serve the range, which [read_fork()](fork.c.md#read_fork)'s checks exclude

Calls: [dsi_readdone](../../libatalk/dsi/dsi_read.c.md#dsi_readdone), [dsi_readinit](../../libatalk/dsi/dsi_read.c.md#dsi_readinit), [fork_range_within](fork.h.md#fork_range_within), [queue_move_to_tail](../../libatalk/util/queue.c.md#queue_move_to_tail)

Called by: [read_fork](fork.c.md#read_fork)

Uses file-scope variables: `rfork_lru` in [etc/afpd/dircache.c](dircache.c.md)

### read_fork

```c
static int read_fork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int is64)
```

Defined at lines 1714 to 2035.

Shared body of FPRead and FPReadExt.

The call reads reqcount bytes from offset, clamped at the fork size with AFPERR_EOF, then cut at UINT32_MAX, the most a [DSI](../../include/atalk/dsi.h.md#struct-dsi) reply frames. It ignores the newline mask and character of the request. Strict locking takes a temporary read lock on the range. Virtual forks serve their in-memory data, and resource fork reads can come from the resource fork cache. Data goes out through sendfile when available, otherwise in dsi_read chunks. A failure in the middle of the reply ends the session.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return
* `is64`: nonzero for FPReadExt

Returns: AFP_OK, AFPERR_EOF, AFPERR_ACCESS, AFPERR_LOCK, AFPERR_MISC, or AFPERR_PARAM

Calls: [ad_fork_fileno](../../libatalk/adouble/ad_open.c.md#ad_fork_fileno), [ad_rlen_known](../../include/atalk/directory.h.md#ad_rlen_known), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [ad_tmplock](../../libatalk/adouble/ad_lock.c.md#ad_tmplock), [dir_modify](directory.c.md#dir_modify), [dsi_read](../../libatalk/dsi/dsi_read.c.md#dsi_read), [dsi_readdone](../../libatalk/dsi/dsi_read.c.md#dsi_readdone), [dsi_readinit](../../libatalk/dsi/dsi_read.c.md#dsi_readinit), [dsi_stream_read_file](../../libatalk/dsi/dsi_stream.c.md#dsi_stream_read_file), [fork_range_within](fork.h.md#fork_range_within), [get_off_t](fork.c.md#get_off_t), [of_dircache_entry](ofork.c.md#of_dircache_entry), [of_find](ofork.c.md#of_find), [of_rfork_held](ofork.c.md#of_rfork_held), [read_file](fork.c.md#read_file), [rfork_cache_serve_from_buf](fork.c.md#rfork_cache_serve_from_buf), [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd)

Called by: [afp_read](fork.c.md#afp_read), [afp_read_ext](fork.c.md#afp_read_ext)

Uses file-scope variables: `rfork_max_entry_size` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_added` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_hits` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_lookups` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_misses` in [etc/afpd/dircache.c](dircache.c.md)

Mentioned in the documentation of: [rfork_cache_serve_from_buf](fork.c.md#rfork_cache_serve_from_buf)

### afp_read

```c
int afp_read(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2048 to 2052.

FPRead: read from an open fork with 32-bit offset and count.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or the AFPERR_* code of read_fork

Calls: [read_fork](fork.c.md#read_fork)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_read_ext

```c
int afp_read_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2065 to 2069.

FPReadExt: read from an open fork with 64-bit offset and count.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or the AFPERR_* code of read_fork

Calls: [read_fork](fork.c.md#read_fork)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_flush

```c
int afp_flush(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2083 to 2098.

FPFlush: flush every open fork of a volume to stable storage.

Failures of single forks only log, and the reply still reports success.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, or AFPERR_PARAM for an unknown volume

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_flush](ofork.c.md#of_flush)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_flushfork

```c
int afp_flushfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2112 to 2138.

FPFlushFork: flush an open fork to stable storage.

A flush failure only logs, and the reply still reports success.

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, or AFPERR_PARAM for an unknown fork reference

Calls: [flushfork](fork.c.md#flushfork), [of_find](ofork.c.md#of_find)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_syncfork

```c
int afp_syncfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2155 to 2183.

FPSyncFork: flush an open fork to stable storage.

Bug: fsync, fdatasync, and F_FULLFSYNC differ in what they guarantee, and the macOS fsync behaves unlike the fsync of other platforms.

See also: http://mirror.linux.org.au/pub/linux.conf.au/2007/video/talks/278.pdf

Parameters:
* `obj`: session object, unused
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_PARAM for an unknown fork reference, or AFPERR_MISC when the flush fails

Calls: [flushfork](fork.c.md#flushfork), [of_find](ofork.c.md#of_find)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### flushfork

```c
int flushfork(struct ofork *ofork)
```

Defined at lines 2197 to 2248.

Flush the descriptors of an open fork to stable storage.

The data fork gets an fsync. For a resource fork the call rereads the resource fork length and stamps the modification date of a dirty fork. It then writes the header and syncs the metadata file. A written header leaves the fork clean and marks the volume changed; after a failed one the fork stays dirty for the next flush. Virtual forks need nothing.

Parameters:
* `ofork`: open fork

Returns: 0, or -1 when an fsync or the header write failed

Calls: [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate), [setvoltime](volume.c.md#setvoltime)

Called by: [afp_flushfork](fork.c.md#afp_flushfork), [afp_syncfork](fork.c.md#afp_syncfork), [of_flush](ofork.c.md#of_flush)

### afp_closefork

```c
int afp_closefork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2264 to 2295.

FPCloseFork: close an open fork.

An unknown reference number logs a warning, because a fork it targeted would stay open for the rest of the session.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_PARAM for an unknown fork reference, or AFPERR_MISC when of_closefork failed

Calls: [of_closefork](ofork.c.md#of_closefork), [of_find](ofork.c.md#of_find)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### write_file

```c
static int write_file(struct ofork *ofork, int eid, off_t *offset, char *rbuf, size_t rbuflen)
```

Defined at lines 2311 to 2336.

Write rbuflen bytes into the fork at offset.

The offset advances past every byte written, also when a later write fails. A short write repeats, so the retry reports why pwrite stopped.

Parameters:
* `ofork`: open fork
* `eid`: ADEID_DFORK or ADEID_RFORK
* `offset`: fork offset, advanced by the bytes written
* `rbuf`: bytes to write
* `rbuflen`: bytes in rbuf

Returns: AFP_OK, or the AFPERR_* code of fork_error for the failed write

Calls: [ad_write](../../libatalk/adouble/ad_write.c.md#ad_write), [fork_error](fork.c.md#fork_error)

Called by: [write_fork](fork.c.md#write_fork)

### write_fits_count

```c
static bool write_fits_count(const struct ofork *ofork, size_t len, off_t reqcount)
```

Defined at lines 2347 to 2358.

Tell whether an FPWrite's payload of len bytes fits its ReqCount, logging the refusal when not.

Parameters:
* `ofork`: the fork written, named in the log line
* `len`: payload length
* `reqcount`: the request's ReqCount

Returns: true when len fits reqcount, false when the write is refused

Called by: [write_fork](fork.c.md#write_fork)

### write_fork

```c
static int write_fork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int is64)
```

Defined at lines 2384 to 2623.

Shared body of FPWrite and FPWriteExt.

The end flag makes offset count from the end of the fork. A virtual Icon fork becomes a real file first. A block too short for the command fails with AFPERR_PARAM, a sum beyond the field width with AFPERR_DFULL, and strict locking takes a temporary write lock. The payload arrives through recvfile where available, otherwise in dsi_write chunks through write_file. A payload longer than a nonzero ReqCount fails with AFPERR_PARAM before any byte is written, and a ReqCount of 0 replies AFP_OK with the payload drained. A failure before afpd reads the payload drains the entire request, and a later failure drains the rest of it. The reply carries the offset after the last byte written.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, receives the new offset
* `rbuflen`: reply buffer capacity on entry, reply length on return
* `is64`: nonzero for FPWriteExt

Returns: AFP_OK, AFPERR_ACCESS, AFPERR_PARAM, AFPERR_DFULL, AFPERR_LOCK, or the AFPERR_* code of fork_error

Calls: [ad_getentryoff](../../libatalk/adouble/ad_open.c.md#ad_getentryoff), [ad_recvfile](../../libatalk/adouble/ad_recvfile.c.md#ad_recvfile), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [ad_tmplock](../../libatalk/adouble/ad_lock.c.md#ad_tmplock), [asp_wrtcont](../../libatalk/asp/asp_write.c.md#asp_wrtcont), [bprint](../../libatalk/util/bprint.c.md#bprint), [dsi_write](../../libatalk/dsi/dsi_write.c.md#dsi_write), [dsi_writedrain](../../libatalk/dsi/dsi_write.c.md#dsi_writedrain), [dsi_writeflush](../../libatalk/dsi/dsi_write.c.md#dsi_writeflush), [dsi_writeinit](../../libatalk/dsi/dsi_write.c.md#dsi_writeinit), [fork_error](fork.c.md#fork_error), [fork_modified](fork.c.md#fork_modified), [get_off_t](fork.c.md#get_off_t), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [of_find](ofork.c.md#of_find), [set_off_t](fork.c.md#set_off_t), [sum_neg](fork.c.md#sum_neg), [write_file](fork.c.md#write_file), [write_fits_count](fork.c.md#write_fits_count)

Called by: [afp_write](fork.c.md#afp_write), [afp_write_ext](fork.c.md#afp_write_ext)

### afp_write

```c
int afp_write(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2636 to 2640.

FPWrite: write to an open fork with 32-bit offset and count.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, receives the new offset
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or the AFPERR_* code of write_fork

Calls: [write_fork](fork.c.md#write_fork)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_write_ext

```c
int afp_write_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2656 to 2660.

FPWriteExt: write to an open fork with 64-bit offset and count.

afpd ignores SIGXFSZ, so a write past the file size limit fails with EFBIG and replies AFPERR_DFULL.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, receives the new offset
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, or the AFPERR_* code of write_fork

Calls: [write_fork](fork.c.md#write_fork)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_getforkparams

```c
int afp_getforkparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2678 to 2718.

FPGetForkParms: return the parameters of an open fork.

Open metadata gets a reread first, so the reply reflects changes of other processes. The reply carries the bitmap and the parameters of getforkparams.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return

Returns: AFP_OK, AFPERR_PARAM for an unknown fork reference or a failed reread, or the error of getforkparams

Calls: [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [getforkparams](fork.c.md#getforkparams), [of_find](ofork.c.md#of_find)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Types

### struct ad_refs

the reference counts of an adouble's three forks

Defined at line 60.
* `int data`
* `int meta`
* `int reso`

# Macros

* Undocumented: `ENDBIT`, `UNLOCKBIT`
