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

struct dir { // NOSONAR (max 20 fields) — fields are intentionally grouped for cache locality
    /* Fields requiring 8-byte alignment: pointers, time_t, ino_t, off_t */
    bstring     d_fullpath;          /*!< complete unix path to dir (or file) */
    /* be careful here! if d_m_name == d_u_name, d_u_name
     * will just point to the same storage as d_m_name !! */
    bstring     d_m_name;            /*!< mac name */
    bstring     d_u_name;            /*!< unix name */
    ucs2_t      *d_m_name_ucs2;      /*!< mac name as UCS2 */
    qnode_t     *qidx_node;          /*!< pointer to position in queue index */
    time_t      d_ctime;             /*!< inode ctime,
                                      * used and modified by reenumeration */
    time_t      dcache_ctime;        /*!< inode ctime,
                                      * used and modified by dircache */
    ino_t       dcache_ino;          /*!< inode number,
                                      * used to detect changes in the dircache */
    time_t      dcache_mtime;        /*!< st_mtime: modification time */
    off_t       dcache_size;         /*!< st_size: file size (for FILPBIT_DFLEN) */
    off_t       dcache_rlen;         /*!< Cached resource fork length.
                                      * -1 = not yet loaded (triggers ad_metadata on first access),
                                      * -2 = confirmed no AD exists (avoids repeated getxattr ENOENT),
                                      * >= 0 = AD loaded and cached resource fork length value. */

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
    struct stat st;
};

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
