/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#ifndef ATALK_DIRECTORY_H
#define ATALK_DIRECTORY_H 1

#include <arpa/inet.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/types.h>

#include <bstrlib.h>

#include <atalk/cnid.h>
#include <atalk/hash.h>
#include <atalk/queue.h>
#include <atalk/unicode.h>

/* setgid directories */
#ifndef DIRBITS
#  define DIRBITS S_ISGID
#endif /* DIRBITS */

/* reserved directory id's */
#define DIRDID_ROOT_PARENT    htonl(1)  /*!< parent directory of root */
#define DIRDID_ROOT           htonl(2)  /*!< root directory */

/* struct dir.d_flags */
#define DIRF_FSMASK	   (3<<0)
#define DIRF_NOFS	   (0<<0)
#define DIRF_UFS	   (1<<1)
#define DIRF_ISFILE    (1<<3) /*!< it's cached file, not a directory */
#define DIRF_OFFCNT    (1<<4) /*!< offsprings count is valid */
#define DIRF_CNID	   (1<<5) /*!< renumerate id */
#define DIRF_ARC_GHOST (1<<6) /*!< ARC ghost entry (in B1/B2) */
#define DIRF_INDEXED   (1<<7) /*!< linked in the dircache hash index */

/* dcache_rlen states. AD_RLEN_NO_RFORK is the floor of the positive
 * range, not a sentinel: test >= 0 for "metadata cached". */
#define AD_RLEN_NO_RFORK   ((off_t) 0)   /*!< AD metadata present, no rfork */
#define AD_RLEN_UNKNOWN    ((off_t) -1)  /*!< not yet loaded */
#define AD_RLEN_NO_AD      ((off_t) -2)  /*!< no AD metadata, no rfork */
#define AD_RLEN_RFORK_ONLY ((off_t) -3)  /*!< no AD metadata; rfork present,
                                          * size not cached */

/*! AD metadata confirmed absent — ad_metadata() would fail ENOENT. */
static inline int ad_rlen_meta_absent(off_t rlen)
{
    return rlen == AD_RLEN_NO_AD || rlen == AD_RLEN_RFORK_ONLY;
}

struct dir { // NOSONAR (max 20 fields) — fields are intentionally grouped for cache locality
    /* Fields requiring 8-byte alignment: pointers, time_t, ino_t, off_t */
    bstring     d_fullpath;          /*!< complete unix path to dir (or file) */
    /* be careful here! if d_m_name == d_u_name, d_u_name
     * will just point to the same storage as d_m_name !! */
    bstring     d_m_name;            /*!< mac name */
    bstring     d_u_name;            /*!< unix name */
    ucs2_t      *d_m_name_ucs2;      /*!< mac name as UCS2 */
    qnode_t     *qidx_node;          /*!< pointer to position in queue index */
    hnode_t     *d_index_node;       /*!< this entry's node in the dircache */
    hnode_t     *d_didname_node;     /*!< this entry's node in index_didname */

    /* Tier 2: Resource Fork data cache.
     * Dynamically allocated buffer containing dcache_rlen bytes of rfork data.
     * Freed automatically when the struct dir entry is evicted from dircache.
     * Only populated for files with resource forks <= rfork_max_entry_size.
     * NULL = not cached (may or may not have an rfork on disk).
     * Buffer size is always dcache_rlen (Tier 1) — no separate size field needed.
     * INVARIANT: dcache_rfork_buf != NULL implies dcache_rlen >= 0. */
    void        *dcache_rfork_buf;   /*!< malloc'd rfork data, or NULL */
    qnode_t     *rfork_lru_node;     /*!< position in rfork LRU list, or NULL */

    time_t      d_ctime;             /*!< inode ctime,
                                      * used and modified by reenumeration */
    time_t      dcache_ctime;        /*!< inode ctime,
                                      * used and modified by dircache */
    ino_t       dcache_ino;          /*!< inode number,
                                      * used to detect changes in the dircache */
    time_t      dcache_mtime;        /*!< st_mtime: modification time */
    off_t       dcache_size;         /*!< st_size: file size (for FILPBIT_DFLEN) */
    /*! Cached resource fork length: >= 0 when AD metadata is cached,
     * else one of the AD_RLEN_* sentinels above. */
    off_t       dcache_rlen;

    /* Fields requiring 4-byte alignment: int, uint32_t, cnid_t, mode_t, uid_t, gid_t */
    int         d_flags;             /*!< directory flags */
    cnid_t      d_pdid;              /*!< CNID of parent directory */
    cnid_t      d_did;               /*!< CNID of directory */
    uint32_t    d_offcnt;            /*!< offspring count */
    uint32_t    d_rights_cache;      /*!< cached rights combined from mode
                                      * and possible ACL. Validate
                                      * dcache_ctime == st_ctime before use!
                                      * Value 0xffffffff indicates invalid/unset. */
    mode_t      dcache_mode;         /*!< st_mode: file type + permissions */
    uid_t       dcache_uid;          /*!< st_uid: owner user ID */
    gid_t       dcache_gid;          /*!< st_gid: owner group ID */

    /* Fields requiring 2-byte alignment */
    uint16_t    d_vid;               /*!< only needed in the dircache, because
                                      * we put all directories in one cache. */

    /* === 1-byte + byte arrays (no alignment requirement, packed densely) === */
    uint8_t
    arc_list;            /*!< Which ARC list: 0=NONE, 1=T1, 2=T2, 3=B1, 4=B2 */

    /* Cached AppleDouble metadata (Tier 1 AD cache).
     * Populated from ad_metadata() via ad_store_to_cache().
     * dcache_filedatesi stores the SERVED representation (not raw AD):
     * the AD_DATE_MODIFY slot contains max(ad_mdate, st_mtime).
     * Trusted implicitly (same model as stat fields). Invalidated by
     * ctime-based validation or dir_modify(DCMOD_AD_INV) on AFP writes.
     * Zero-initialized by calloc — zeros are valid defaults. */
    uint8_t     dcache_afpfilei[4];  /*!< ADEID_AFPFILEI: AFP attributes */
    uint8_t     dcache_finderinfo[32]; /*!< ADEID_FINDERI: Finder info */
    uint8_t
    dcache_filedatesi[16]; /*!< ADEID_FILEDATESI: create/modify/backup dates (SERVED values) */
};

struct path {
    int         m_type;             /*!< mac name type (long name, unicode */
    char        *m_name;            /*!< mac name */
    char        *u_name;            /*!< unix name */
    cnid_t      id;                 /*!< file id (only for getmetadata) */
    struct dir  *d_dir;
    int         st_valid;           /*!< does st_errno and st set */
    int         st_errno;
    /* st was filled by fstat on an open fd: the object is live but the
     * path may no longer name it. 0 = path-derived stat. */
    int         st_fd;
    struct stat st;
    /* Caller-resolved dircache entry for a FILE path; NULL = unknown.
     * Directories use d_dir. Valid for the current request only. */
    struct dir  *d_cached;
};

/*! d_cached accessor enforcing the staleness rule: an entry invalidated
 * mid-request (dir_remove sets d_did = CNID_INVALID) reads as absent. */
static inline struct dir *path_cached_file(const struct path *path)
{
    if (path->d_cached && path->d_cached->d_did != CNID_INVALID) {
        return path->d_cached;
    }

    return NULL;
}

/*! Absolute path of a child of parent for FCE/Spotlight event strings,
 * built from the cached d_fullpath — no getcwd. Falls back to
 * fullpathname() when the parent or its path is unavailable. Returns
 * buf, or fullpathname()'s static buffer on the fallback path. */
extern const char *dir_event_path(char *buf, size_t buflen,
                                  const struct dir *parent,
                                  const char *name);

static inline int path_isadir(struct path *o_path)
{
    return o_path->d_dir != NULL;
#if 0
    return o_path->m_name == '\0' || /* we are in a it */
           !o_path->st_valid ||      /* in cache but we can't chdir in it */
           /* not in cache and can't chdir */
           (!o_path->st_errno && S_ISDIR(o_path->st.st_mode));
#endif
}

/* directory.c */
extern struct dir rootParent;

#endif /* ATALK_DIRECTORY_H */
