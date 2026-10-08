---
type: C Header File
title: "include/atalk/directory.h"
description: "3 functions, 2 types, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/directory.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/cnid.h](cnid.h.md)
* [atalk/hash.h](hash.h.md)
* [atalk/queue.h](queue.h.md)
* [atalk/unicode.h](unicode.h.md)
* System headers: `arpa/inet.h`, `bstrlib.h`, `dirent.h`, `stdint.h`, `sys/types.h`

# Included by

* [bin/nad/nad_find.c](../../bin/nad/nad_find.c.md)
* [bin/nad/nad_ls.c](../../bin/nad/nad_ls.c.md)
* [bin/nad/nad_rmdir.c](../../bin/nad/nad_rmdir.c.md)
* [etc/afpd/ad_cache.c](../../etc/afpd/ad_cache.c.md)
* [etc/afpd/ad_cache.h](../../etc/afpd/ad_cache.h.md)
* [etc/afpd/dircache.c](../../etc/afpd/dircache.c.md)
* [etc/afpd/dircache.h](../../etc/afpd/dircache.h.md)
* [etc/afpd/directory.h](../../etc/afpd/directory.h.md)
* [etc/afpd/pfd_cache.c](../../etc/afpd/pfd_cache.c.md)
* [etc/afpd/pfd_cache.h](../../etc/afpd/pfd_cache.h.md)
* [etc/spotlight/cnid/sl_cnid.c](../../etc/spotlight/cnid/sl_cnid.c.md)
* [libatalk/vfs/unix.c](../../libatalk/vfs/unix.c.md)
* [libatalk/vfs/vfs.c](../../libatalk/vfs/vfs.c.md)

# Functions

### ad_rlen_meta_absent

```c
static int ad_rlen_meta_absent(off_t rlen)
```

Defined at lines 67 to 70.

AD metadata confirmed absent — [ad_metadata()](../../libatalk/adouble/ad_open.c.md#ad_metadata) would fail ENOENT.

Called by: [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached)

### path_cached_file

```c
static struct dir * path_cached_file(const struct path *path)
```

Defined at lines 159 to 166.

d_cached accessor enforcing the staleness rule: an entry invalidated mid-request (dir_remove sets d_did = CNID_INVALID) reads as absent.

Called by: [getmetadata](../../etc/afpd/file.c.md#getmetadata), [path_resolve_cached_file](../../etc/afpd/file.c.md#path_resolve_cached_file)

### path_isadir

```c
static int path_isadir(struct path *o_path)
```

Defined at lines 176 to 185.

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_addappl](../../etc/afpd/appl.c.md#afp_addappl), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_createid](../../etc/afpd/file.c.md#afp_createid), [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_getappl](../../etc/afpd/appl.c.md#afp_getappl), [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_rename](../../etc/afpd/filedir.c.md#afp_rename), [afp_rmvappl](../../etc/afpd/appl.c.md#afp_rmvappl), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [afp_setfilparams](../../etc/afpd/file.c.md#afp_setfilparams), [path_error](../../etc/afpd/directory.c.md#path_error)

# Types

### struct dir

Defined at line 72.
* `bstring d_fullpath`
* `bstring d_m_name`
* `bstring d_u_name`
* `ucs2_t * d_m_name_ucs2`
* `qnode_t * qidx_node`
* `hnode_t * d_index_node`
* `hnode_t * d_didname_node`
* `void * dcache_rfork_buf`
* `qnode_t * rfork_lru_node`
* `time_t d_ctime`
* `time_t dcache_ctime`
* `ino_t dcache_ino`
* `time_t dcache_mtime`
* `off_t dcache_size`
* `off_t dcache_rlen`
* `int d_flags`
* `cnid_t d_pdid`
* `cnid_t d_did`
* `uint32_t d_offcnt`
* `uint32_t d_rights_cache`
* `mode_t dcache_mode`
* `uid_t dcache_uid`
* `gid_t dcache_gid`
* `uint16_t d_vid`
* `uint8_t arc_list`
* `uint8_t dcache_afpfilei`
* `uint8_t dcache_finderinfo`
* `uint8_t dcache_filedatesi`

### struct path

Defined at line 140.
* `int m_type`
* `char * m_name`
* `char * u_name`
* `cnid_t id`
* `struct dir * d_dir`
* `int st_valid`
* `int st_errno`
* `int st_fd`
* `struct stat st`
* `struct dir * d_cached`

# Macros

* Undocumented: `AD_RLEN_NO_AD`, `AD_RLEN_NO_RFORK`, `AD_RLEN_RFORK_ONLY`, `AD_RLEN_UNKNOWN`, `DIRBITS`, `DIRDID_ROOT`, `DIRDID_ROOT_PARENT`, `DIRF_ARC_GHOST`, `DIRF_CNID`, `DIRF_FSMASK`, `DIRF_INDEXED`, `DIRF_ISFILE`, `DIRF_NOFS`, `DIRF_OFFCNT`, `DIRF_UFS`
