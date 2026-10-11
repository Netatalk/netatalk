---
type: C Header File
title: "include/atalk/directory.h"
description: "3 functions, 2 types, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/directory.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Tell whether a cached rlen records absent AppleDouble metadata.

Called by: [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_metadata_cached](../../etc/afpd/ad_cache.c.md#ad_metadata_cached)

### path_cached_file

```c
static struct dir * path_cached_file(const struct path *path)
```

Defined at lines 158 to 165.

Return the dircache entry of a file path, or NULL after dir_remove.

Called by: [getmetadata](../../etc/afpd/file.c.md#getmetadata), [path_resolve_cached_file](../../etc/afpd/file.c.md#path_resolve_cached_file)

### path_isadir

```c
static int path_isadir(struct path *o_path)
```

Defined at lines 173 to 182.

Tell whether a path resolved to a directory.

Called by: [ad_addcomment](../../etc/afpd/desktop.c.md#ad_addcomment), [ad_getcomment](../../etc/afpd/desktop.c.md#ad_getcomment), [ad_rmvcomment](../../etc/afpd/desktop.c.md#ad_rmvcomment), [afp_addappl](../../etc/afpd/appl.c.md#afp_addappl), [afp_copyfile](../../etc/afpd/file.c.md#afp_copyfile), [afp_createid](../../etc/afpd/file.c.md#afp_createid), [afp_delete](../../etc/afpd/filedir.c.md#afp_delete), [afp_exchangefiles](../../etc/afpd/file.c.md#afp_exchangefiles), [afp_getappl](../../etc/afpd/appl.c.md#afp_getappl), [afp_getextattr](../../etc/afpd/extattrs.c.md#afp_getextattr), [afp_listextattr](../../etc/afpd/extattrs.c.md#afp_listextattr), [afp_moveandrename](../../etc/afpd/filedir.c.md#afp_moveandrename), [afp_remextattr](../../etc/afpd/extattrs.c.md#afp_remextattr), [afp_rename](../../etc/afpd/filedir.c.md#afp_rename), [afp_rmvappl](../../etc/afpd/appl.c.md#afp_rmvappl), [afp_setextattr](../../etc/afpd/extattrs.c.md#afp_setextattr), [afp_setfilparams](../../etc/afpd/file.c.md#afp_setfilparams), [path_error](../../etc/afpd/directory.c.md#path_error)

# Types

### struct dir

Defined at line 72.
* `bstring d_fullpath`: complete unix path to dir (or file)
* `bstring d_m_name`: mac name
* `bstring d_u_name`: unix name
* `ucs2_t * d_m_name_ucs2`: mac name as UCS2
* `qnode_t * qidx_node`: pointer to position in queue index
* `hnode_t * d_index_node`: this entry's node in the dircache
* `hnode_t * d_didname_node`: this entry's node in index_didname
* `void * dcache_rfork_buf`: malloc'd rfork data, or NULL
* `qnode_t * rfork_lru_node`: position in rfork LRU list, or NULL
* `time_t d_ctime`: inode ctime, used and modified by reenumeration
* `time_t dcache_ctime`: inode ctime, used and modified by dircache
* `ino_t dcache_ino`: inode number, used to detect changes in the dircache
* `time_t dcache_mtime`: st_mtime: modification time
* `off_t dcache_size`: st_size: file size (for FILPBIT_DFLEN)
* `off_t dcache_rlen`: Cached resource fork length: >= 0 when AD metadata is cached, else one of the AD_RLEN_* sentinels above.
* `int d_flags`: directory flags
* `cnid_t d_pdid`: CNID of parent directory.
* `cnid_t d_did`: CNID of directory.
* `uint32_t d_offcnt`: offspring count
* `uint32_t d_rights_cache`: cached rights combined from mode and possible ACL.
* `mode_t dcache_mode`: st_mode: file type + permissions
* `uid_t dcache_uid`: st_uid: owner user ID
* `gid_t dcache_gid`: st_gid: owner group ID
* `uint16_t d_vid`: only needed in the dircache, because we put all directories in one cache.
* `uint8_t arc_list`: Which ARC list: 0=NONE, 1=T1, 2=T2, 3=B1, 4=B2.
* `uint8_t dcache_afpfilei`: ADEID_AFPFILEI: AFP attributes.
* `uint8_t dcache_finderinfo`: ADEID_FINDERI: Finder info.
* `uint8_t dcache_filedatesi`: ADEID_FILEDATESI: create/modify/backup dates (SERVED values).

### struct path

Defined at line 140.
* `int m_type`: mac name type (long name, unicode
* `char * m_name`: mac name
* `char * u_name`: unix name
* `cnid_t id`: file id (only for getmetadata)
* `struct dir * d_dir`
* `int st_valid`: does st_errno and st set
* `int st_errno`
* `int st_fd`
* `struct stat st`
* `struct dir * d_cached`

# Macros

* `AD_RLEN_NO_AD`: no AD metadata, no rfork
* `AD_RLEN_NO_RFORK`: AD metadata present, no rfork.
* `AD_RLEN_RFORK_ONLY`: no AD metadata; rfork present, size not cached
* `AD_RLEN_UNKNOWN`: not yet loaded
* `DIRDID_ROOT`: root directory
* `DIRDID_ROOT_PARENT`: parent directory of root
* `DIRF_ARC_GHOST`: ARC ghost entry (in B1/B2).
* `DIRF_CNID`: renumerate id
* `DIRF_INDEXED`: linked in the dircache hash index
* `DIRF_ISFILE`: it's cached file, not a directory
* `DIRF_OFFCNT`: offsprings count is valid
* Undocumented: `DIRBITS`, `DIRF_FSMASK`, `DIRF_NOFS`, `DIRF_UFS`
