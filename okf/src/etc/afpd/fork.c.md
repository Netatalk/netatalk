---
type: C Source File
title: "etc/afpd/fork.c"
description: "31 functions, includes 17 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/fork.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-10T08:19:07+02:00 }
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

Defined at lines 53 to 152.

Materialize a virtual Icon fork into a real file on disk.

Creates the physical Icon file, seeds its resource fork with the virtual icon data, sets FinderInfo, and re-opens the fork through ad_open so the ofork has real file descriptors for writing.

Returns: AFP_OK on success, or an AFP error code

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_write](../../libatalk/adouble/ad_write.c.md#ad_write), [ostat](../../libatalk/util/unix.c.md#ostat), [virtual_icon_get_rfork](virtual_icon.c.md#virtual_icon_get_rfork)

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [write_fork](fork.c.md#write_fork)

### getforkparams

```c
static int getforkparams(const AFPObj *obj, struct ofork *ofork, uint16_t bitmap, char *buf, size_t *buflen)
```

Defined at lines 154 to 244.

Calls: [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [dirlookup](directory.c.md#dirlookup), [get_afp_errno](directory.c.md#get_afp_errno), [getmetadata](file.c.md#getmetadata), [movecwd](directory.c.md#movecwd), [mtoupath](desktop.c.md#mtoupath), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Called by: [afp_getforkparams](fork.c.md#afp_getforkparams), [afp_openfork](fork.c.md#afp_openfork)

### get_off_t

```c
static off_t get_off_t(char **ibuf, int is64)
```

Defined at lines 246 to 264.

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [byte_lock](fork.c.md#byte_lock), [read_fork](fork.c.md#read_fork), [write_fork](fork.c.md#write_fork)

### set_off_t

```c
static int set_off_t(off_t offset, char *rbuf, int is64)
```

Defined at lines 266 to 284.

Called by: [byte_lock](fork.c.md#byte_lock), [write_fork](fork.c.md#write_fork)

### is_neg

```c
static int is_neg(int is64, off_t val)
```

Defined at lines 286 to 293.

Called by: [byte_lock](fork.c.md#byte_lock), [sum_neg](fork.c.md#sum_neg)

### sum_neg

```c
static int sum_neg(int is64, off_t offset, off_t reqcount)
```

Defined at lines 295 to 302.

Calls: [is_neg](fork.c.md#is_neg)

Called by: [write_fork](fork.c.md#write_fork)

### fork_setmode_deny

```c
int fork_setmode_deny(int access, int f_rddny, int f_wrdny, int f_nodny)
```

Defined at lines 308 to 312.

Called by: [fork_setmode](fork.c.md#fork_setmode)

### fork_setmode

```c
static int fork_setmode(const AFPObj *obj, struct adouble *adp, int eid, int access, int ofrefnum)
```

Defined at lines 314 to 441.

Calls: [ad_lock](../../libatalk/adouble/ad_lock.c.md#ad_lock), [ad_testlock](../../libatalk/adouble/ad_lock.c.md#ad_testlock), [fork_setmode_deny](fork.c.md#fork_setmode_deny)

Called by: [afp_openfork](fork.c.md#afp_openfork)

### afp_openfork

```c
int afp_openfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 444 to 883.

Calls: [ad_close](../../libatalk/adouble/ad_flush.c.md#ad_close), [ad_data_open](../../include/atalk/adouble.h.md#ad_data_open), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_getattr](../../libatalk/adouble/ad_attr.c.md#ad_getattr), [ad_init](../../libatalk/adouble/ad_open.c.md#ad_init), [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_open](../../libatalk/adouble/ad_open.c.md#ad_open), [ad_rsrc_open](../../include/atalk/adouble.h.md#ad_rsrc_open), [ad_setid](../../libatalk/adouble/ad_attr.c.md#ad_setid), [ad_setname](../../libatalk/adouble/ad_attr.c.md#ad_setname), [check_access](directory.c.md#check_access), [cname](directory.c.md#cname), [dir_remove](directory.c.md#dir_remove), [dircache_remove_children_defer](dircache.c.md#dircache_remove_children_defer), [dirlookup](directory.c.md#dirlookup), [dirlookup_strict](directory.c.md#dirlookup_strict), [file_access](directory.c.md#file_access), [fork_setmode](fork.c.md#fork_setmode), [fullpathname](../../libatalk/util/unix.c.md#fullpathname), [get_afp_errno](directory.c.md#get_afp_errno), [get_id](file.c.md#get_id), [getforkparams](fork.c.md#getforkparams), [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [is_virtual_icon_name](virtual_icon.c.md#is_virtual_icon_name), [of_alloc](ofork.c.md#of_alloc), [of_dealloc](ofork.c.md#of_dealloc), [of_findname](ofork.c.md#of_findname), [of_log_nfile](ofork.c.md#of_log_nfile), [of_note_open](ofork.c.md#of_note_open), [virtual_icon_enabled](virtual_icon.c.md#virtual_icon_enabled), [virtual_icon_get_rfork](virtual_icon.c.md#virtual_icon_get_rfork), [virtual_icon_getfilparams](virtual_icon.c.md#virtual_icon_getfilparams)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md), `curdir` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### rfork_invalidate_for_ofork

```c
static void rfork_invalidate_for_ofork(const struct vol *vol, struct ofork *ofork)
```

Defined at lines 891 to 907.

Invalidate cached rfork data for a file identified by its open fork.

Looks up the file's dircache entry via parent DID + name, and invalidates the AD cache (Tier 1 + Tier 2). Used after rfork writes and truncations.

Calls: [dir_modify](directory.c.md#dir_modify), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_setforkparams](fork.c.md#afp_setforkparams), [write_mark](fork.c.md#write_mark)

### fork_error

```c
static int fork_error(const struct ofork *ofork, const char *call)
```

Defined at lines 917 to 947.

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

Defined at lines 949 to 1125.

Calls: [ad_dtruncate](../../libatalk/adouble/ad_write.c.md#ad_dtruncate), [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [ad_rtruncate](../../libatalk/adouble/ad_write.c.md#ad_rtruncate), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [ad_tmplock](../../libatalk/adouble/ad_lock.c.md#ad_tmplock), [dirlookup](directory.c.md#dirlookup), [fork_error](fork.c.md#fork_error), [get_off_t](fork.c.md#get_off_t), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [movecwd](directory.c.md#movecwd), [mtoupath](desktop.c.md#mtoupath), [of_find](ofork.c.md#of_find), [rfork_invalidate_for_ofork](fork.c.md#rfork_invalidate_for_ofork)

Uses file-scope variables: `afp_errno` in [etc/afpd/directory.c](directory.c.md)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### byte_lock

```c
static int byte_lock(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int is64)
```

Defined at lines 1138 to 1226.

Calls: [ad_lock](../../libatalk/adouble/ad_lock.c.md#ad_lock), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [get_off_t](fork.c.md#get_off_t), [is_neg](fork.c.md#is_neg), [of_find](ofork.c.md#of_find), [set_off_t](fork.c.md#set_off_t)

Called by: [afp_bytelock](fork.c.md#afp_bytelock), [afp_bytelock_ext](fork.c.md#afp_bytelock_ext)

### afp_bytelock

```c
int afp_bytelock(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1229 to 1233.

Calls: [byte_lock](fork.c.md#byte_lock)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_bytelock_ext

```c
int afp_bytelock_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1236 to 1240.

Calls: [byte_lock](fork.c.md#byte_lock)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### read_file

```c
static int read_file(const struct ofork *ofork, int eid, off_t offset, char *rbuf, size_t *rbuflen)
```

Defined at lines 1255 to 1280.

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

Defined at lines 1298 to 1372.

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

Defined at lines 1374 to 1727.

Calls: [ad_fork_fileno](../../libatalk/adouble/ad_open.c.md#ad_fork_fileno), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [ad_tmplock](../../libatalk/adouble/ad_lock.c.md#ad_tmplock), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [dirlookup](directory.c.md#dirlookup), [dsi_read](../../libatalk/dsi/dsi_read.c.md#dsi_read), [dsi_readdone](../../libatalk/dsi/dsi_read.c.md#dsi_readdone), [dsi_readinit](../../libatalk/dsi/dsi_read.c.md#dsi_readinit), [dsi_stream_read_file](../../libatalk/dsi/dsi_stream.c.md#dsi_stream_read_file), [fork_range_within](fork.h.md#fork_range_within), [get_off_t](fork.c.md#get_off_t), [of_find](ofork.c.md#of_find), [read_file](fork.c.md#read_file), [rfork_cache_free](ad_cache.c.md#rfork_cache_free), [rfork_cache_serve_from_buf](fork.c.md#rfork_cache_serve_from_buf), [rfork_cache_store_from_fd](ad_cache.c.md#rfork_cache_store_from_fd), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [afp_read](fork.c.md#afp_read), [afp_read_ext](fork.c.md#afp_read_ext)

Calls through [`AFPObj::exit`](../../include/atalk/globals.h.md#struct-afpobj): no table assigns this field

Uses file-scope variables: `rfork_max_entry_size` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_added` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_hits` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_invalidated` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_lookups` in [etc/afpd/dircache.c](dircache.c.md), `rfork_stat_misses` in [etc/afpd/dircache.c](dircache.c.md)

Mentioned in the documentation of: [rfork_cache_serve_from_buf](fork.c.md#rfork_cache_serve_from_buf)

### afp_read

```c
int afp_read(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1730 to 1734.

Calls: [read_fork](fork.c.md#read_fork)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_read_ext

```c
int afp_read_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1737 to 1741.

Calls: [read_fork](fork.c.md#read_fork)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_flush

```c
int afp_flush(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1744 to 1759.

Calls: [getvolbyvid](../../libatalk/util/netatalk_conf.c.md#getvolbyvid), [of_flush](ofork.c.md#of_flush)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_flushfork

```c
int afp_flushfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1761 to 1787.

Calls: [flushfork](fork.c.md#flushfork), [of_find](ofork.c.md#of_find)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_syncfork

```c
int afp_syncfork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1795 to 1823.

Bug: There is a lot to tell about fsync, fdatasync, F_FULLFSYNC. fsync(2) on OSX is implemented differently than on other platforms.

See also: http://mirror.linux.org.au/pub/linux.conf.au/2007/video/talks/278.pdf

Calls: [flushfork](fork.c.md#flushfork), [of_find](ofork.c.md#of_find)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### flushfork

```c
int flushfork(struct ofork *ofork)
```

Defined at lines 1826 to 1870.

Calls: [ad_flush](../../libatalk/adouble/ad_flush.c.md#ad_flush), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [ad_setdate](../../libatalk/adouble/ad_date.c.md#ad_setdate)

Called by: [afp_flushfork](fork.c.md#afp_flushfork), [afp_syncfork](fork.c.md#afp_syncfork), [of_flush](ofork.c.md#of_flush)

### afp_closefork

```c
int afp_closefork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 1872 to 1903.

Calls: [of_closefork](ofork.c.md#of_closefork), [of_find](ofork.c.md#of_find)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### write_file

```c
static int write_file(struct ofork *ofork, int eid, off_t *offset, char *rbuf, size_t rbuflen)
```

Defined at lines 1912 to 1937.

Write rbuflen bytes into the fork at *offset, advanced past every byte written, also when the write fails.

Returns: AFP_OK, or the AFPERR_* reply for the failed write

Calls: [ad_write](../../libatalk/adouble/ad_write.c.md#ad_write), [fork_error](fork.c.md#fork_error)

Called by: [write_fork](fork.c.md#write_fork)

### write_mark

```c
static void write_mark(const AFPObj *obj, struct ofork *ofork, int eid)
```

Defined at lines 1940 to 1952.

Flag a fork that a write reached.

Calls: [rfork_invalidate_for_ofork](fork.c.md#rfork_invalidate_for_ofork)

Called by: [write_fork](fork.c.md#write_fork)

### write_fork

```c
static int write_fork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen, int is64)
```

Defined at lines 1960 to 2176.

Calls: [ad_recvfile](../../libatalk/adouble/ad_recvfile.c.md#ad_recvfile), [ad_size](../../libatalk/adouble/ad_size.c.md#ad_size), [ad_tmplock](../../libatalk/adouble/ad_lock.c.md#ad_tmplock), [asp_wrtcont](../../libatalk/asp/asp_write.c.md#asp_wrtcont), [bprint](../../libatalk/util/bprint.c.md#bprint), [dsi_write](../../libatalk/dsi/dsi_write.c.md#dsi_write), [dsi_writeflush](../../libatalk/dsi/dsi_write.c.md#dsi_writeflush), [dsi_writeinit](../../libatalk/dsi/dsi_write.c.md#dsi_writeinit), [fork_error](fork.c.md#fork_error), [get_off_t](fork.c.md#get_off_t), [materialize_virtual_icon](fork.c.md#materialize_virtual_icon), [of_find](ofork.c.md#of_find), [set_off_t](fork.c.md#set_off_t), [sum_neg](fork.c.md#sum_neg), [write_file](fork.c.md#write_file), [write_mark](fork.c.md#write_mark)

Called by: [afp_write](fork.c.md#afp_write), [afp_write_ext](fork.c.md#afp_write_ext)

### afp_write

```c
int afp_write(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2179 to 2183.

Calls: [write_fork](fork.c.md#write_fork)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

### afp_write_ext

```c
int afp_write_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2188 to 2192.

Calls: [write_fork](fork.c.md#write_fork)

Called by: [set_auth_switch](auth.c.md#set_auth_switch)

### afp_getforkparams

```c
int afp_getforkparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
```

Defined at lines 2195 to 2235.

Calls: [ad_meta_open](../../include/atalk/adouble.h.md#ad_meta_open), [ad_refresh](../../libatalk/adouble/ad_open.c.md#ad_refresh), [getforkparams](fork.c.md#getforkparams), [of_find](ofork.c.md#of_find)

Dispatched via: [postauth_switch](switch.c.md#postauth_switch)

# Macros

* Undocumented: `ENDBIT`, `UNLOCKBIT`
