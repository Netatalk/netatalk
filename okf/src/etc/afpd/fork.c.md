---
type: C Source File
title: "etc/afpd/fork.c"
description: "31 functions, includes 17 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fork.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [ad_cache.h](ad_cache.h.md)
* [atalk/adouble.h](../../include/atalk/adouble.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/asp.h](../../include/atalk/asp.h.md)
* [atalk/cnid.h](../../include/atalk/cnid.h.md)
* [atalk/dsi.h](../../include/atalk/dsi.h.md)
* [atalk/globals.h](../../include/atalk/globals.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/netatalk_conf.h](../../include/atalk/netatalk_conf.h.md)
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

### materialize_virtual_icon

```c
static int materialize_virtual_icon(struct ofork *ofork)
```

Defined at lines 56 to 155.

Turn the virtual Icon file into a real file before a client changes it.

The write path and FPSetForkParms call it. It creates the Icon file in the volume root and seeds its resource fork with the virtual icon data. The Finder info marks the file invisible. The call then reopens the fork through ad_open, so the open fork holds real descriptors, and drops its virtual state.

Parameters:
* `ofork`: open fork on the virtual Icon file

Returns: AFP_OK, or AFPERR_MISC when a create or open failed

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_entry](../../libatalk/adouble/ad_open.c.md#ad_entry), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_write](../../libatalk/adouble/ad_write.c.md#ad_write), [ostat](../../libatalk/util/unix.c.md#ostat), [virtual_icon_get_rfork](virtual_icon.c.md#virtual_icon_get_rfork)

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [write_fork](fork.c.md#write_fork)

### getforkparams

```c
static int getforkparams(const AFPObj *obj, struct ofork *ofork, uint16_t bitmap, char *buf, size_t *buflen)
```

Defined at lines 174 to 264.

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

Defined at lines 276 to 294.

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

Defined at lines 305 to 323.

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

Defined at lines 336 to 343.

Whether an offset or length is negative in the field width of the request.

With a 64-bit off_t, a 32-bit request also counts a value with bit 31 set as negative.

Parameters:
* `is64`: nonzero for the 64-bit field of the Ext commands
* `val`: offset or length

Returns: 1 when the value is negative, 0 otherwise

Called by: [byte_lock](fork.c.md#byte_lock), [sum_neg](fork.c.md#sum_neg)

### sum_neg

```c
static int sum_neg(int is64, off_t offset, off_t reqcount)
```

Defined at lines 354 to 361.

Whether offset plus reqcount is negative in the field width of the request.

Parameters:
* `is64`: nonzero for the 64-bit field of the Ext commands
* `offset`: start offset
* `reqcount`: byte count

Returns: 1 when the sum is negative, 0 otherwise

Calls: [is_neg](fork.c.md#is_neg)

Called by: [write_fork](fork.c.md#write_fork)

### fork_setmode_deny

```c
int fork_setmode_deny(int access, int f_rddny, int f_wrdny, int f_nodny)
```

Defined at lines 376 to 380.

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

Defined at lines 398 to 525.

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

Called by: [afp_openfork](fork.c.md#afp_openfork)

### afp_openfork

```c
int afp_openfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 552 to 991.

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

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_data_open](../../include/atalk/adouble.h.md#ad_data_open), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [check_access](directory.c.md#check_access), [cname](directory.c.md#cname), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dirlookup](directory.c.md#dirlookup), [dirlookup_strict](directory.c.md#dirlookup_strict), [file_access](directory.c.md#file_access), [fork_setmode](fork.c.md#fork_setmode), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [get_afp_errno](directory.c.md#get_afp_errno), [get_id](file.c.md#get_id), [getforkparams](fork.c.md#getforkparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_alloc](ofork.c.md#of_alloc), [of_dealloc](ofork.c.md#of_dealloc), [of_findname](ofork.c.md#of_findname), [of_log_nfile](ofork.c.md#of_log_nfile), [of_note_open](ofork.c.md#of_note_open), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled), [virtual_icon_get_rfork](virtual_icon.c.md#virtual_icon_get_rfork), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### rfork_invalidate_for_ofork

```c
static void rfork_invalidate_for_ofork(const struct vol *vol, struct ofork *ofork)
```

Defined at lines 1004 to 1020.

Invalidate the cached resource fork data of the file behind an open fork.

The dircache entry comes from the parent DID and the name of the fork. Resource fork writes and truncations call it, and it clears both tiers of the AppleDouble cache.

Parameters:
* `vol`: volume of the fork
* `ofork`: open fork whose file changed

Calls: [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [write_mark](fork.c.md#write_mark)

### fork_error

```c
static int fork_error(const struct ofork *ofork, const char *call)
```

Defined at lines 1030 to 1060.

Map the errno of a failed fork write or truncate to the AFP reply.

Parameters:
* `ofork`: fork being changed
* `call`: libatalk call that failed, named in the log

Returns: AFPERR_DFULL, AFPERR_VLOCK, AFPERR_ACCESS or AFPERR_PARAM

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [write_file](fork.c.md#write_file), [write_fork](fork.c.md#write_fork)

### afp_setforkparams

```c
int afp_setforkparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1079 to 1255.

FPSetForkParms: change the length of an open fork.

The bitmap must name exactly the length of the open fork type, and the 64-bit form needs AFP 3.0. A negative length fails. Shrinking takes a temporary write lock on the cut range, so a conflicting byte lock fails the call with AFPERR_LOCK. The virtual Icon file becomes a real file first. A resource fork change invalidates the cached resource fork data.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length
* `rbuf`: reply block, unused
* `rbuflen`: reply buffer capacity on entry, 0 on return

Returns: AFP_OK, AFPERR_BITMAP, AFPERR_LOCK, AFPERR_ACCESS, AFPERR_VLOCK, AFPERR_PARAM, or the AFPERR_* code of fork_error

Calls: [ad_dtruncate](../../libatalk/adouble/ad_write.c.md#ad_dtruncate), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [ad_rtruncate](../../libatalk/adouble/ad_write.c.md#ad_rtruncate), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [ad_tmplock](../../libatalk/adouble/ad_lock.c.md#ad_tmplock), [dirlookup](directory.c.md#dirlookup), [fork_error](fork.c.md#fork_error), [get_off_t](fork.c.md#get_off_t), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [movecwd](directory.c.md#movecwd), [mtoupath](desktop.c.md#mtoupath), [of_find](ofork.c.md#of_find), [rfork_invalidate_for_ofork](fork.c.md#rfork_invalidate_for_ofork)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### byte_lock

```c
static int byte_lock(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int is64)
```

Defined at lines 1285 to 1373.

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

Defined at lines 1387 to 1391.

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

Defined at lines 1405 to 1409.

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

Defined at lines 1424 to 1449.

Read *rbuflen bytes from fork at offset.

Parameters:
* `ofork`: fork handle
* `eid`: data fork or resource fork entry id
* `offset`: offset
* `rbuf`: data buffer
* `rbuflen`: in: number of bytes to read, out: bytes read

Returns: AFP status code

Calls: [ad_read](../../libatalk/adouble/ad_read.c.md#ad_read)

Called by: [read_fork](fork.c.md#read_fork)

### rfork_cache_serve_from_buf

```c
static int rfork_cache_serve_from_buf(DSI *dsi, struct dir *cached_entry, char *ibuf, off_t offset, off_t reqcount, int err, size_t *rbuflen)
```

Defined at lines 1467 to 1541.

Serve resource fork data from Tier 2 cache via [DSI](../../include/atalk/dsi.h.md#struct-dsi) framing.

Shared helper for cache-hit and just-populated cache paths in [read_fork()](fork.c.md#read_fork). Handles dsi_readinit/dsi_read/dsi_readdone loop and LRU promotion.

Parameters:
* `dsi`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) connection
* `cached_entry`: dir entry with populated dcache_rfork_buf
* `ibuf`: [DSI](../../include/atalk/dsi.h.md#struct-dsi) write buffer (repurposed from command buffer)
* `offset`: Starting byte offset in the rfork
* `reqcount`: Requested byte count
* `err`: AFP error code (AFPERR_EOF or AFP_OK)
* `rbuflen`: Output: bytes remaining (set by dsi_read)

Returns: 0 on success (goto afp_read_done), -1 on [DSI](../../include/atalk/dsi.h.md#struct-dsi) error (goto afp_read_exit)

Calls: [dsi_read](../../libatalk/dsi/dsi_read.c.md#dsi_read), [dsi_readdone](../../libatalk/dsi/dsi_read.c.md#dsi_readdone), [dsi_readinit](../../libatalk/dsi/dsi_read.c.md#dsi_readinit), [fork_range_within](fork.h.md#fork_range_within), [queue_move_to_tail](../../libatalk/util/queue.c.md#queue_move_to_tail)

Called by: [read_fork](fork.c.md#read_fork)

Uses file-scope variables: `rfork_lru` in [etc/afpd/dircache.c](dircache.c.md)

### read_fork

```c
static int read_fork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int is64)
```

Defined at lines 1563 to 1916.

Shared body of FPRead and FPReadExt.

The call reads reqcount bytes from offset, clamped at the fork size with AFPERR_EOF. It ignores the newline mask and character of the request. Strict locking takes a temporary read lock on the range. Virtual forks serve their in-memory data, and resource fork reads can come from the resource fork cache. Data goes out through sendfile when available, otherwise in dsi_read chunks. A failure in the middle of the reply ends the session.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block
* `rbuflen`: reply buffer capacity on entry, reply length on return
* `is64`: nonzero for FPReadExt

Returns: AFP_OK, AFPERR_EOF, AFPERR_ACCESS, AFPERR_LOCK, or AFPERR_PARAM

Calls: [ad_fork_fileno](../../libatalk/adouble/ad_open.c.md#ad_fork_fileno), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [ad_tmplock](../../libatalk/adouble/ad_lock.c.md#ad_tmplock), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [dsi_read](../../libatalk/dsi/dsi_read.c.md#dsi_read), [dsi_readdone](../../libatalk/dsi/dsi_read.c.md#dsi_readdone), [dsi_readinit](../../libatalk/dsi/dsi_read.c.md#dsi_readinit), [dsi_stream_read_file](../../libatalk/dsi/dsi_stream.c.md#dsi_stream_read_file), [fork_range_within](fork.h.md#fork_range_within), [get_off_t](fork.c.md#get_off_t), [of_find](ofork.c.md#of_find), [read_file](fork.c.md#read_file), [rfork_cache_free](ad_cache.c.md#rfork_cache_free), [rfork_cache_serve_from_buf](fork.c.md#rfork_cache_serve_from_buf), [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_read](fork.c.md#afp_read), [afp_read_ext](fork.c.md#afp_read_ext)

Uses file-scope variables: `rfork_max_entry_size` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_added` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_hits` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_invalidated` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_lookups` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_misses` in [etc/afpd/dircache.c](dircache.c.md)

Mentioned in the documentation of: [rfork_cache_serve_from_buf](fork.c.md#rfork_cache_serve_from_buf)

### afp_read

```c
int afp_read(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1929 to 1933.

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

Defined at lines 1946 to 1950.

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

Defined at lines 1964 to 1979.

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

Defined at lines 1993 to 2019.

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

Defined at lines 2036 to 2064.

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

Defined at lines 2077 to 2121.

Flush the descriptors of an open fork to stable storage.

The data fork gets an fsync. For a resource fork the call rereads the resource fork length and stamps the modification date of a dirty fork. It then writes the header and syncs the metadata file. Virtual forks need nothing.

Parameters:
* `ofork`: open fork

Returns: 0, or -1 when an fsync or the header write failed

Calls: [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate)

Called by: [afp_flushfork](fork.c.md#afp_flushfork), [afp_syncfork](fork.c.md#afp_syncfork), [of_flush](ofork.c.md#of_flush)

### afp_closefork

```c
int afp_closefork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2137 to 2168.

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

Defined at lines 2184 to 2209.

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

### write_mark

```c
static void write_mark(const AFPObj *obj, struct ofork *ofork, int eid)
```

Defined at lines 2223 to 2235.

Flag a fork that a write reached.

A fork with open metadata turns dirty, so the next flush or close stamps its modification date. A resource fork write also invalidates the resource fork cache when dircache_rfork_budget is nonzero. AFPFORK_MODIFIED stays until the fork closes.

Parameters:
* `obj`: session object
* `ofork`: open fork
* `eid`: ADEID_DFORK or ADEID_RFORK

Calls: [rfork_invalidate_for_ofork](fork.c.md#rfork_invalidate_for_ofork)

Called by: [write_fork](fork.c.md#write_fork)

### write_fork

```c
static int write_fork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int is64)
```

Defined at lines 2259 to 2475.

Shared body of FPWrite and FPWriteExt.

The end flag makes offset count from the end of the fork. A virtual Icon fork becomes a real file first. A sum beyond the field width fails with AFPERR_DFULL, and strict locking takes a temporary write lock. The payload arrives through recvfile where available, otherwise in dsi_write chunks through write_file. A failure before afpd reads the payload drains the entire request, and a later failure drains the rest of it. The reply carries the offset after the last byte written.

Parameters:
* `obj`: session object
* `ibuf`: request block
* `ibuflen`: request block length, unused
* `rbuf`: reply block, receives the new offset
* `rbuflen`: reply buffer capacity on entry, reply length on return
* `is64`: nonzero for FPWriteExt

Returns: AFP_OK, AFPERR_ACCESS, AFPERR_PARAM, AFPERR_DFULL, AFPERR_LOCK, or the AFPERR_* code of fork_error

Calls: [ad_recvfile](../../libatalk/adouble/ad_recvfile.c.md#ad_recvfile), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [ad_tmplock](../../libatalk/adouble/ad_lock.c.md#ad_tmplock), [asp_wrtcont](../../libatalk/asp/asp_write.c.md#asp_wrtcont), [bprint](../../libatalk/util/bprint.c.md#bprint), [dsi_write](../../libatalk/dsi/dsi_write.c.md#dsi_write), [dsi_writeflush](../../libatalk/dsi/dsi_write.c.md#dsi_writeflush), [dsi_writeinit](../../libatalk/dsi/dsi_write.c.md#dsi_writeinit), [fork_error](fork.c.md#fork_error), [get_off_t](fork.c.md#get_off_t), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [of_find](ofork.c.md#of_find), [set_off_t](fork.c.md#set_off_t), [sum_neg](fork.c.md#sum_neg), [write_file](fork.c.md#write_file), [write_mark](fork.c.md#write_mark)

Called by: [afp_write](fork.c.md#afp_write), [afp_write_ext](fork.c.md#afp_write_ext)

### afp_write

```c
int afp_write(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2488 to 2492.

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

Defined at lines 2508 to 2512.

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

Defined at lines 2530 to 2570.

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

# Macros

* Undocumented: `ENDBIT`, `UNLOCKBIT`
