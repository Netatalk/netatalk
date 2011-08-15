/*
 * $Id: adouble.h,v 1.55 2010-03-30 12:55:26 franklahm Exp $
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
 *  Research Systems Unix Group
 *  The University of Michigan
 *  c/o Mike Clark
 *  535 W. William Street
 *  Ann Arbor, Michigan
 *  +1-313-763-0525
 *  netatalk@itd.umich.edu
 */

/*!
 * @file
 * @brief Part of Netatalk's AppleDouble implementatation
 */

#ifndef _ATALK_ADOUBLE_H
#define _ATALK_ADOUBLE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* -------------------
 * need pread() and pwrite()
 */
#ifdef HAVE_PREAD

#ifndef HAVE_PWRITE
#undef HAVE_PREAD
#endif

#endif

#ifdef HAVE_PWRITE
#ifndef HAVE_PREAD
#undef HAVE_PWRITE
#endif
#endif

/*
  Still have to figure out which platforms really
  need _XOPEN_SOURCE defined for pread.
*/
#if defined(HAVE_PREAD) && !defined(SOLARIS) && !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(__FreeBSD__) && !defined(TRU64)
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#undef __USE_MISC
#define __USE_MISC
#include <unistd.h>
#endif

#include <sys/cdefs.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <sys/mman.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <netatalk/endian.h>

/* version info */
#define AD_VERSION1     0x00010000
#define SFM_VERSION     AD_VERSION1

#define AD_VERSION2     0x00020000
#define AD_VERSION2_OSX 0x00020001
/*
  #define AD_VERSION1_ADS 0x00010002
*/
#define AD_VERSION1_SFM 0x00010003
#define AD_VERSION      AD_VERSION2

/*
 * AppleDouble entry IDs.
 */
#define ADEID_DFORK         1
#define ADEID_RFORK         2
#define ADEID_NAME          3 /* Note: starting with Netatalk 2.1 we do NOT alway set the name */
#define ADEID_COMMENT       4
#define ADEID_ICONBW        5
#define ADEID_ICONCOL       6
#define ADEID_FILEI         7  /* v1, replaced by: */
#define ADEID_FILEDATESI    8  /* this */
#define ADEID_FINDERI       9
#define ADEID_MACFILEI      10 /* we don't use this */
#define ADEID_PRODOSFILEI   11 /* we store prodos info here */
#define ADEID_MSDOSFILEI    12 /* we don't use this */
#define ADEID_SHORTNAME     13
#define ADEID_AFPFILEI      14 /* where the rest of the FILEI info goes */
#define ADEID_DID           15

#if AD_VERSION == AD_VERSION1
#define ADEID_MAX           16
#else
/* netatalk private note fileid reused DID */
#define ADEID_PRIVDEV       16
#define ADEID_PRIVINO       17
#define ADEID_PRIVSYN       18 /* in synch with database */
#define ADEID_PRIVID        19
#define ADEID_SFMRESERVE1   20
#define ADEID_SFMRESERVE2   21

#define AD_DEV              0x80444556
#define AD_INO              0x80494E4F
#define AD_SYN              0x8053594E
#define AD_ID               0x8053567E
#define ADEID_MAX           22
#endif

/* magic */
#define AD_APPLESINGLE_MAGIC 0x00051600
#define AD_APPLEDOUBLE_MAGIC 0x00051607
#define AD_MAGIC             AD_APPLEDOUBLE_MAGIC
#define SFM_MAGIC            0x00504641

/* sizes of relevant entry bits */
#define ADEDLEN_MAGIC       4
#define ADEDLEN_VERSION     4
#define ADEDLEN_FILLER      16
#define ADEDLEN_NENTRIES    2

/* 26 */
#define AD_HEADER_LEN       (ADEDLEN_MAGIC + ADEDLEN_VERSION + \
                             ADEDLEN_FILLER + ADEDLEN_NENTRIES)
#define AD_ENTRY_LEN        12  /* size of a single entry header */

/* v1 field widths */
#define ADEDLEN_NAME        255
#define ADEDLEN_COMMENT     200
#define ADEDLEN_FILEI       16
#define ADEDLEN_FINDERI     32

/* v2 field widths */
#define ADEDLEN_FILEDATESI      16
#define ADEDLEN_SHORTNAME       12 /* length up to 8.3 */
#define ADEDLEN_AFPFILEI        4
#define ADEDLEN_MACFILEI        4
#define ADEDLEN_PRODOSFILEI     8
#define ADEDLEN_MSDOSFILEI      2
#define ADEDLEN_DID             4
#define ADEDLEN_PRIVDEV         8
#define ADEDLEN_PRIVINO         8
#define ADEDLEN_PRIVSYN         8
#define ADEDLEN_PRIVID          4

#define ADEID_NUM_V1            5
#define ADEID_NUM_V2            13

// #define ADEID_NUM_SFM        5
/* sizeof SFM meta data */
#define AD_SFM_LEN 60

/* 589 */
#define AD_DATASZ1      (AD_HEADER_LEN + ADEDLEN_NAME + ADEDLEN_COMMENT + ADEDLEN_FILEI + ADEDLEN_FINDERI + \
                         (ADEID_NUM_V1 * AD_ENTRY_LEN))

#if AD_DATASZ1 != 589
#error bad size for AD_DATASZ1
#endif

#define AD_NEWSZ2       (ADEDLEN_DID + ADEDLEN_AFPFILEI + ADEDLEN_SHORTNAME + ADEDLEN_PRODOSFILEI \
                         + ADEDLEN_PRIVDEV + ADEDLEN_PRIVINO + ADEDLEN_PRIVSYN + ADEDLEN_PRIVID)

/* 725 */
#define AD_DATASZ2      (AD_DATASZ1 + AD_NEWSZ2 + ((ADEID_NUM_V2 - ADEID_NUM_V1) * AD_ENTRY_LEN))

#if AD_DATASZ2 != 741
#error bad size for AD_DATASZ2
#endif

#define AD_DATASZ_MAX   1024
#if AD_VERSION == AD_VERSION1
#define AD_DATASZ   AD_DATASZ1 /* hold enough for the entries */
#elif AD_VERSION == AD_VERSION2
#define AD_DATASZ   AD_DATASZ2
#endif

/*
 * some legacy defines from netatalk-990130
 * (to keep from breaking certain packages)
 *
 */

#define ADEDOFF_RFORK   589
#define ADEDOFF_NAME    86
#define ADEDOFF_COMMENT 341
#define ADEDOFF_FINDERI 557
#ifndef ADEDOFF_FILEI
#define ADEDOFF_FILEI   541
#endif

typedef u_int32_t cnid_t;

/*
 * The header of the AppleDouble Header File looks like this:
 *
 *  NAME            SIZE
 *  ====            ====
 *  Magic           4
 *  Version         4
 *  Home File System    16  (this becomes filler in ad v2)
 *  Number of Entries   2
 *  Entry Descriptors for each entry:
 *      Entry ID    4
 *      Offset      4
 *      Length      4
 */

struct ad_entry {
    off_t   ade_off;
    ssize_t   ade_len;
};

typedef struct adf_lock_t {
    struct flock lock;
    int user;
    int *refcount; /* handle read locks with multiple users */
} adf_lock_t;

struct ad_fd {
    int          adf_fd;        /* -1: invalid, -2: symlink */

#ifndef HAVE_PREAD
    off_t        adf_off;
#endif

    char         *adf_syml;
    int          adf_flags;
    int          adf_excl;
    adf_lock_t   *adf_lock;
    int          adf_refcount, adf_lockcount, adf_lockmax;
};

/* some header protection */
#define AD_INITED  0xad494e54  /* ad"INT" */
struct adouble_fops;

struct adouble {
    u_int32_t           ad_magic;
    u_int32_t           ad_version;
    char                ad_filler[ 16 ];
    struct ad_entry     ad_eid[ ADEID_MAX ];
    struct ad_fd        ad_data_fork, ad_resource_fork, ad_metadata_fork;
    struct ad_fd        *ad_md; /* either ad_resource or ad_metadata */

    int                 ad_flags;    /* This really stores version info too (AD_VERSION*) */
    int                 ad_adflags;  /* ad_open flags adflags like ADFLAGS_DIR */
    unsigned int        ad_inited;
    int                 ad_options;
    int                 ad_fileordir;
    int                 ad_refcount; /* used in afpd/ofork.c */
    off_t               ad_rlen;     /* ressource fork len with AFP 3.0
                                        the header parameter size is too small.
                                     */
    char                *ad_m_name;   /* mac name for open fork */
    int                 ad_m_namelen;
    struct adouble_fops *ad_ops;
    u_int16_t       ad_open_forks;      /* open forks (by others) */

#ifdef USE_MMAPPED_HEADERS
    char                *ad_data;
#else
    char        ad_data[AD_DATASZ_MAX];
#endif
};

struct adouble_fops {
    char *(*ad_path)(const char *, int);
    int  (*ad_mkrf)(char *);
    int  (*ad_rebuild_header)(struct adouble *);
    int  (*ad_check_header)(struct adouble *, struct stat *);
    int  (*ad_header_read)(struct adouble *, struct stat *);
    int  (*ad_header_upgrade)(struct adouble *, char *);
};

#define ADFLAGS_DF        (1<<0)
#define ADFLAGS_HF        (1<<1)
#define ADFLAGS_DIR       (1<<2)
/*
#define ADFLAGS_NOADOUBLE (1<<3)
*/
#define ADFLAGS_V1COMPAT  (1<<4)
#define ADFLAGS_NOHF      (1<<5)  /* not an error if no ressource fork */
#define ADFLAGS_RDONLY    (1<<6)  /* don't try readwrite */
#define ADFLAGS_OPENFORKS (1<<7)  /* check for open fork in ad_metadata function */
#define ADFLAGS_RF        (1<<8)
#define ADFLAGS_MD        ADFLAGS_HF /* (1<<9) */
#define ADFLAGS_CREATE    (1<<9)

/* adouble v2 cnid cache */
#define ADVOL_NODEV      (1 << 0)
#define ADVOL_CACHE      (1 << 1)
#define ADVOL_UNIXPRIV   (1 << 2) /* adouble unix priv */
#define ADVOL_INVDOTS    (1 << 3) /* dot files (.DS_Store) are invisible) */
#define ADVOL_NOADOUBLE  (1 << 4)

/* lock flags */
#define ADLOCK_CLR      (0)
#define ADLOCK_RD       (1<<0)
#define ADLOCK_WR       (1<<1)
#define ADLOCK_MASK     (ADLOCK_RD | ADLOCK_WR)
#define ADLOCK_UPGRADE  (1<<2)
#define ADLOCK_FILELOCK (1<<3)

/* we use this so that we can use the same mechanism for both byte
 * locks and file synchronization locks. i do this by co-opting either
 * first bits on 32-bit machines or shifting above the last bit on
 * 64-bit machines. this only matters for the data fork. */
#if defined(TRY_64BITOFF_T) && (~0UL > 0xFFFFFFFFU)
/* synchronization locks */
#define AD_FILELOCK_BASE (0x80000000)
#else
#if _FILE_OFFSET_BITS == 64
#define AD_FILELOCK_BASE (0x7FFFFFFFFFFFFFFFULL - 9)
#else
#define AD_FILELOCK_BASE (0x7FFFFFFF -9)
#endif
#endif

/* FIXME:
 * AD_FILELOCK_BASE case
 */
#if _FILE_OFFSET_BITS == 64
#define BYTELOCK_MAX (0x7FFFFFFFFFFFFFFFULL)
#else
/* Tru64 is an always-64-bit OS; version 4.0 does not set _FILE_OFFSET_BITS */
#if defined(TRU64)
#define BYTELOCK_MAX (0x7FFFFFFFFFFFFFFFULL)
#else
#define BYTELOCK_MAX (0x7FFFFFFFU)
#endif
#endif

#define AD_FILELOCK_OPEN_WR        (AD_FILELOCK_BASE + 0)
#define AD_FILELOCK_OPEN_RD        (AD_FILELOCK_BASE + 1)
#define AD_FILELOCK_DENY_WR        (AD_FILELOCK_BASE + 2)
#define AD_FILELOCK_DENY_RD        (AD_FILELOCK_BASE + 3)
#define AD_FILELOCK_OPEN_NONE      (AD_FILELOCK_BASE + 4)

/* time stuff. we overload the bits a little.  */
#define AD_DATE_CREATE         0
#define AD_DATE_MODIFY         4
#define AD_DATE_BACKUP         8
#define AD_DATE_ACCESS        12
#define AD_DATE_MASK          (AD_DATE_CREATE | AD_DATE_MODIFY | \
                              AD_DATE_BACKUP | AD_DATE_ACCESS)
#define AD_DATE_UNIX          (1 << 10)
#define AD_DATE_START         htonl(0x80000000)
#define AD_DATE_DELTA         946684800
#define AD_DATE_FROM_UNIX(x)  htonl((x) - AD_DATE_DELTA)
#define AD_DATE_TO_UNIX(x)    (ntohl(x) + AD_DATE_DELTA)

/* various finder offset and info bits */
#define FINDERINFO_FRTYPEOFF   0
#define FINDERINFO_FRCREATOFF  4
#define FINDERINFO_FRFLAGOFF   8

/* FinderInfo Flags, char in `ad ls`, valid for files|dirs */
#define FINDERINFO_ISONDESK      (1)     /* "d", fd */
#define FINDERINFO_COLOR         (0x0e)
#define FINDERINFO_HIDEEXT       (1<<4)  /* "e", fd */
#define FINDERINFO_ISHARED       (1<<6)  /* "m", f  */
#define FINDERINFO_HASNOINITS    (1<<7)  /* "n", f  */
#define FINDERINFO_HASBEENINITED (1<<8)  /* "i", fd */
#define FINDERINFO_HASCUSTOMICON (1<<10) /* "c", fd */
#define FINDERINFO_ISSTATIONNERY (1<<11) /* "t", f  */
#define FINDERINFO_NAMELOCKED    (1<<12) /* "s", fd */
#define FINDERINFO_HASBUNDLE     (1<<13) /* "b", fd */
#define FINDERINFO_INVISIBLE     (1<<14) /* "v", fd */
#define FINDERINFO_ISALIAS       (1<<15) /* "a", fd */

#define FINDERINFO_FRVIEWOFF  14
#define FINDERINFO_CUSTOMICON 0x4
#define FINDERINFO_CLOSEDVIEW 0x100

/*
  The "shared" and "invisible" attributes are opaque and stored and
  retrieved from the FinderFlags. This fixes Bug #2802236:
  <https://sourceforge.net/tracker/?func=detail&aid=2802236&group_id=8642&atid=108642>
*/

/* AFP attributes, char in `ad ls`, valid for files|dirs */
#define ATTRBIT_INVISIBLE (1<<0)  /* opaque from FinderInfo */
#define ATTRBIT_MULTIUSER (1<<1)  /* file: opaque, dir: see below */
#define ATTRBIT_SYSTEM    (1<<2)  /* "y", fd */
#define ATTRBIT_DOPEN     (1<<3)  /* data fork already open. Not stored, computed on the fly */
#define ATTRBIT_ROPEN     (1<<4)  /* resource fork already open. Not stored, computed on the fly */
#define ATTRBIT_NOWRITE   (1<<5)  /* "w", f, write inhibit(v2)/read-only(v1) bit */
#define ATTRBIT_BACKUP    (1<<6)  /* "p", fd */
#define ATTRBIT_NORENAME  (1<<7)  /* "r", fd */
#define ATTRBIT_NODELETE  (1<<8)  /* "l", fd */
#define ATTRBIT_NOCOPY    (1<<10) /* "o", f */
#define ATTRBIT_SETCLR    (1<<15) /* set/clear bit (d) */

/* AFP attributes for dirs. These should probably be computed on the fly.
 * We don't do that, nor does e.g. OS S X 10.5 Server */
#define ATTRBIT_EXPFLDR   (1<<1)  /* Folder is a sharepoint */
#define ATTRBIT_MOUNTED   (1<<3)  /* Directory is mounted by a user */
#define ATTRBIT_SHARED    (1<<4)  /* Shared area, called IsExpFolder in spec */

/* private AFPFileInfo bits */
#define AD_AFPFILEI_OWNER       (1 << 0) /* any owner */
#define AD_AFPFILEI_GROUP       (1 << 1) /* ignore group */
#define AD_AFPFILEI_BLANKACCESS (1 << 2) /* blank access permissions */

#define ad_data_fileno(ad)  ((ad)->ad_data_fork.adf_fd)
#define ad_reso_fileno(ad)  ((ad)->ad_resource_fork.adf_fd)
#define ad_meta_fileno(ad)  ((ad)->ad_md->adf_fd)

#define ad_getversion(ad)   ((ad)->ad_version)

#define ad_getentrylen(ad,eid)     ((ad)->ad_eid[(eid)].ade_len)
#define ad_setentrylen(ad,eid,len) ((ad)->ad_eid[(eid)].ade_len = (len))
#define ad_getentryoff(ad,eid)     ((ad)->ad_eid[(eid)].ade_off)
#define ad_entry(ad,eid)           ((caddr_t)(ad)->ad_data + (ad)->ad_eid[(eid)].ade_off)

#define ad_get_HF_flags(ad) ((ad)->ad_resource_fork.adf_flags)
#define ad_get_MD_flags(ad) ((ad)->ad_md->adf_flags)

/* ad_flush.c */
extern int ad_rebuild_adouble_header (struct adouble *);
extern int ad_rebuild_sfm_header (struct adouble *);
extern int ad_copy_header (struct adouble *, struct adouble *);
extern int ad_flush (struct adouble *);
extern int ad_close (struct adouble *, int);

/* ad_lock.c */
extern int ad_fcntl_lock    (struct adouble *, const u_int32_t /*eid*/,
                                 const int /*type*/, const off_t /*offset*/,
                                 const off_t /*len*/, const int /*user*/);
extern void ad_fcntl_unlock (struct adouble *, const int /*user*/);
extern int ad_fcntl_tmplock (struct adouble *, const u_int32_t /*eid*/,
                                 const int /*type*/, const off_t /*offset*/,
                                 const off_t /*len*/, const int /*user*/);
extern int ad_testlock      (struct adouble * /*adp*/, int /*eid*/, off_t /*off*/);

extern u_int16_t ad_openforks (struct adouble * /*adp*/, u_int16_t);
extern int ad_excl_lock     (struct adouble * /*adp*/, const u_int32_t /*eid*/);

#define ad_lock ad_fcntl_lock
#define ad_tmplock ad_fcntl_tmplock
#define ad_unlock ad_fcntl_unlock

/* ad_open.c */
extern int ad_setfuid     (const uid_t );
extern uid_t ad_getfuid   (void );
extern char *ad_dir       (const char *);
extern char *ad_path      (const char *, int);
extern char *ad_path_osx  (const char *, int);
extern char *ad_path_ads  (const char *, int);
extern char *ad_path_sfm  (const char *, int);
extern int ad_mode        (const char *, int);
extern int ad_mkdir       (const char *, int);
extern void ad_init       (struct adouble *, int, int );
extern int ad_open        (const char *, int, int, int, struct adouble *);
extern int ad_openat      (int dirfd, const char *, int, int, int, struct adouble *);
extern int ad_refresh     (struct adouble *);
extern int ad_stat        (const char *, struct stat *);
extern int ad_metadata    (const char *, int, struct adouble *);
extern int ad_metadataat  (int, const char *, int, struct adouble *);

#define ad_open_metadata(name, flags, mode, adp)\
   ad_open(name, ADFLAGS_MD|(flags), O_RDWR |(mode), 0666, (adp))

#define ad_close_metadata(adp) ad_close( (adp), ADFLAGS_MD)

/* build a resource fork mode from the data fork mode:
 * remove X mode and extend header to RW if R or W (W if R for locking),
 */
static inline mode_t ad_hf_mode (mode_t mode)
{
    mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH);
    /* fnctl lock need write access */
    if ((mode & S_IRUSR))
        mode |= S_IWUSR;
    if ((mode & S_IRGRP))
        mode |= S_IWGRP;
    if ((mode & S_IROTH))
        mode |= S_IWOTH;

    /* if write mode set add read mode */
    if ((mode & S_IWUSR))
        mode |= S_IRUSR;
    if ((mode & S_IWGRP))
        mode |= S_IRGRP;
    if ((mode & S_IWOTH))
        mode |= S_IROTH;

    return mode;
}

/* ad_ea.c */
ssize_t sys_getxattr (const char *path, const char *name, void *value, size_t size);
ssize_t sys_lgetxattr (const char *path, const char *name, void *value, size_t size);
ssize_t sys_fgetxattr (int filedes, const char *name, void *value, size_t size);
ssize_t sys_listxattr (const char *path, char *list, size_t size);
ssize_t sys_llistxattr (const char *path, char *list, size_t size);
ssize_t sys_flistxattr (int filedes, char *list, size_t size);
int sys_removexattr (const char *path, const char *name);
int sys_lremovexattr (const char *path, const char *name);
int sys_fremovexattr (int filedes, const char *name);
int sys_setxattr (const char *path, const char *name, const void *value, size_t size, int flags);
int sys_lsetxattr (const char *path, const char *name, const void *value, size_t size, int flags);
int sys_fsetxattr (int filedes, const char *name, const void *value, size_t size, int flags);
int sys_copyxattr (const char *src, const char *dst);

/* ad_read.c/ad_write.c */
extern int     sys_ftruncate(int fd, off_t length);

extern ssize_t ad_read (struct adouble *, const u_int32_t,
                            const off_t, char *, const size_t);
extern ssize_t ad_pread (struct ad_fd *, void *, size_t, off_t);
extern ssize_t ad_write (struct adouble *, const u_int32_t, off_t,
                             const int, const char *, const size_t);
extern ssize_t adf_pread  (struct ad_fd *, void *, size_t, off_t);
extern ssize_t adf_pwrite (struct ad_fd *, const void *, size_t, off_t);
extern int     ad_dtruncate (struct adouble *, const off_t);
extern int     ad_rtruncate (struct adouble *, const off_t);

/* ad_size.c */
extern off_t ad_size (const struct adouble *, const u_int32_t );

/* ad_mmap.c */
extern void *ad_mmapread (struct adouble *, const u_int32_t,
                              const off_t, const size_t);
extern void *ad_mmapwrite (struct adouble *, const u_int32_t,
                               const off_t, const int, const size_t);
#define ad_munmap(buf, len)  (munmap((buf), (len)))

/* ad_date.c */
extern int ad_setdate (struct adouble *, unsigned int, u_int32_t);
extern int ad_getdate (const struct adouble *, unsigned int, u_int32_t *);

/* ad_attr.c */
extern int       ad_setattr (const struct adouble *, const u_int16_t);
extern int       ad_getattr (const struct adouble *, u_int16_t *);

/* Note: starting with Netatalk 2.1 we do NOT alway set the name */
extern int       ad_setname (struct adouble *, const char *);

#if AD_VERSION == AD_VERSION2
extern int       ad_setid (struct adouble *, const dev_t dev,const ino_t ino, const u_int32_t, const u_int32_t, const void *);
extern u_int32_t ad_getid (struct adouble *, const dev_t, const ino_t, const cnid_t, const void *);
extern u_int32_t ad_forcegetid (struct adouble *adp);
#else
#define ad_setid(a, b, c)
#endif

#ifdef WITH_SENDFILE
extern int ad_readfile_init(const struct adouble *ad, 
				       const int eid, off_t *off,
				       const int end);
#endif

#if 0
#ifdef HAVE_SENDFILE_WRITE
extern ssize_t ad_writefile (struct adouble *, const int,
                                 const int, off_t, const int, const size_t);
#endif /* HAVE_SENDFILE_WRITE */
#endif /* 0 */

#endif /* _ATALK_ADOUBLE_H */
