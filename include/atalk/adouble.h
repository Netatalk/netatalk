/*
 * $Id: adouble.h,v 1.29 2005-09-28 09:46:37 didg Exp $
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
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 500
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
#define AD_VERSION1	0x00010000
#define AD_VERSION2	0x00020000
#define AD_VERSION2_OSX	0x00020001
#define AD_VERSION1_ADS	0x00010002
#define AD_VERSION	AD_VERSION2

/*
 * AppleDouble entry IDs. 
 */
#define ADEID_DFORK		1
#define ADEID_RFORK		2
#define ADEID_NAME		3
#define ADEID_COMMENT		4
#define ADEID_ICONBW		5
#define ADEID_ICONCOL		6
#define ADEID_FILEI		7  /* v1, replaced by: */
#define ADEID_FILEDATESI	8  /* this */
#define ADEID_FINDERI		9
#define ADEID_MACFILEI		10 /* we don't use this */
#define ADEID_PRODOSFILEI	11 /* we store prodos info here */
#define ADEID_MSDOSFILEI	12 /* we don't use this */
#define ADEID_SHORTNAME		13
#define ADEID_AFPFILEI		14 /* where the rest of the FILEI info goes */
#define ADEID_DID		15

#if AD_VERSION == AD_VERSION1
#define ADEID_MAX		16
#else
/* netatalk private note fileid reused DID */
#define ADEID_PRIVDEV           16
#define ADEID_PRIVINO           17
#define ADEID_PRIVSYN           18 /* in synch with database */
#define ADEID_PRIVID            19

#define AD_DEV                  0x80444556
#define AD_INO                  0x80494E4F
#define AD_SYN                  0x8053594E
#define AD_ID                   0x8053567E
#define ADEID_MAX		20
#endif

/* magic */
#define AD_APPLESINGLE_MAGIC 0x00051600
#define AD_APPLEDOUBLE_MAGIC 0x00051607
#define AD_MAGIC	     AD_APPLEDOUBLE_MAGIC


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
#define ADEDLEN_FILEI	    16
#define ADEDLEN_FINDERI	    32

/* v2 field widths */
#define ADEDLEN_FILEDATESI	16
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

#define ADEID_NUM_V1         5
#define ADEID_NUM_V2         13

/* 589 */
#define AD_DATASZ1      (AD_HEADER_LEN + ADEDLEN_NAME + ADEDLEN_COMMENT +ADEDLEN_FILEI +ADEDLEN_FINDERI+\
ADEID_NUM_V1*AD_ENTRY_LEN)

#if AD_DATASZ1 != 589
#error bad size for AD_DATASZ1
#endif

#define AD_NEWSZ2       (ADEDLEN_DID + ADEDLEN_AFPFILEI +ADEDLEN_SHORTNAME +ADEDLEN_PRODOSFILEI \
+ADEDLEN_PRIVDEV +ADEDLEN_PRIVINO +ADEDLEN_PRIVSYN+ ADEDLEN_PRIVID)

/* 725 */
#define AD_DATASZ2      (AD_DATASZ1 + AD_NEWSZ2 + (ADEID_NUM_V2 -ADEID_NUM_V1)*AD_ENTRY_LEN)

#if AD_DATASZ2 != 741
#error bad size for AD_DATASZ2
#endif

#define AD_DATASZ_MAX   1024
#if AD_VERSION == AD_VERSION1
#define AD_DATASZ	AD_DATASZ1 /* hold enough for the entries */
#elif AD_VERSION == AD_VERSION2
#define AD_DATASZ       AD_DATASZ2
#endif

/*
 * some legacy defines from netatalk-990130
 * (to keep from breaking certain packages)
 *
 */

#define ADEDOFF_RFORK	589
#define ADEDOFF_NAME	86
#define ADEDOFF_COMMENT	341
#define ADEDOFF_FINDERI	557
#ifndef ADEDOFF_FILEI
#define ADEDOFF_FILEI	541
#endif

typedef u_int32_t cnid_t;

/*
 * The header of the AppleDouble Header File looks like this:
 *
 *	NAME			SIZE
 *	====			====
 *	Magic			4
 *	Version			4
 * 	Home File System	16  (this becomes filler in ad v2)
 *	Number of Entries	2
 *	Entry Descriptors for each entry:
 *		Entry ID	4
 *		Offset		4
 *		Length		4
 */

struct ad_entry {
    u_int32_t	ade_off;
    u_int32_t	ade_len;
};

typedef struct adf_lock_t {
  struct flock lock;
  int user;
  int *refcount; /* handle read locks with multiple users */
} adf_lock_t;

struct ad_fd {
    int		 adf_fd;

#ifndef HAVE_PREAD
    off_t	 adf_off;
#endif

    int		 adf_flags;
    int          adf_excl;
    adf_lock_t   *adf_lock;
    int          adf_refcount, adf_lockcount, adf_lockmax;
};

/* some header protection */
#define AD_INITED  0xad494e54  /* ad"INT" */
struct adouble {
    u_int32_t		ad_magic;
    u_int32_t		ad_version;
    char		ad_filler[ 16 ];
    struct ad_entry	ad_eid[ ADEID_MAX ];
    struct ad_fd	ad_df, ad_hf;
    int                 ad_flags;
    unsigned int        ad_inited;
    int                 ad_options;
    int                 ad_refcount; /* used in afpd/ofork.c */
    off_t               ad_rlen;     /* ressource fork len with AFP 3.0
                                        the header parameter size is too small.
                                     */
    char                *ad_m_name;   /* mac name for open fork */
    int                 ad_m_namelen;

    char                *(*ad_path)(const char *, int);
    int                 (*ad_mkrf)(char *);
                           
#ifdef USE_MMAPPED_HEADERS
    char                *ad_data;
#else
    char		ad_data[AD_DATASZ_MAX];
#endif
};

#define ADFLAGS_DF	  (1<<0)
#define ADFLAGS_HF	  (1<<1)
#define ADFLAGS_DIR	  (1<<2)
#define ADFLAGS_NOADOUBLE (1<<3)
#define ADFLAGS_V1COMPAT  (1<<4)
#define ADFLAGS_NOHF      (1<<5)  /* not an error if no ressource fork */
#define ADFLAGS_RDONLY    (1<<6)  /* don't try readwrite */

/* adouble v2 cnid cache */
#define ADVOL_NODEV      (1 << 0)   
#define ADVOL_CACHE      (1 << 1)
/* adouble unix priv */
#define ADVOL_UNIXPRIV   (1 << 2)   

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
#define AD_FILELOCK_BASE (0x7FFFFFFF -9)
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
#define AD_FILELOCK_OPEN_RD   	   (AD_FILELOCK_BASE + 1)
#define AD_FILELOCK_DENY_WR   	   (AD_FILELOCK_BASE + 2)
#define AD_FILELOCK_DENY_RD        (AD_FILELOCK_BASE + 3)
#define AD_FILELOCK_OPEN_NONE      (AD_FILELOCK_BASE + 4)

/* time stuff. we overload the bits a little.  */
#define AD_DATE_CREATE	       0
#define AD_DATE_MODIFY	       4
#define AD_DATE_BACKUP	       8
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
/* finderinfo flags */
#define FINDERINFO_ISONDESK      (1)
#define FINDERINFO_COLOR         (0x0e)
#define FINDERINFO_ISHARED       (1<<6)
#define FINDERINFO_HASNOINITS    (1<<7)
#define FINDERINFO_HASBEENINITED (1<<8)
#define FINDERINFO_HASCUSTOMICON (1<<10)
#define FINDERINFO_ISSTATIONNERY (1<<11)
#define FINDERINFO_NAMELOCKED    (1<<12)
#define FINDERINFO_HASBUNDLE     (1<<13)
#define FINDERINFO_INVISIBLE     (1<<14)
#define FINDERINFO_ISALIAS       (1<<15)

#define FINDERINFO_FRVIEWOFF  14 
#define FINDERINFO_CUSTOMICON 0x4
#define FINDERINFO_CLOSEDVIEW 0x100   

 
/* private AFPFileInfo bits */
#define AD_AFPFILEI_OWNER       (1 << 0) /* any owner */
#define AD_AFPFILEI_GROUP       (1 << 1) /* ignore group */
#define AD_AFPFILEI_BLANKACCESS (1 << 2) /* blank access permissions */

#define ad_dfileno(ad)		((ad)->ad_df.adf_fd)
#define ad_hfileno(ad)		((ad)->ad_hf.adf_fd)
#define ad_getversion(ad)	((ad)->ad_version)

#define ad_getentrylen(ad,eid)	((ad)->ad_eid[(eid)].ade_len)
#define ad_setentrylen(ad,eid,len) \
	((ad)->ad_eid[(eid)].ade_len = (len))
#define ad_getentryoff(ad,eid)  ((ad)->ad_eid[(eid)].ade_off)
#define ad_entry(ad,eid)        ((caddr_t)(ad)->ad_data + \
				 (ad)->ad_eid[(eid)].ade_off)     
#define ad_getoflags(ad,adf)	(((adf)&ADFLAGS_HF) ? \
	(ad)->ad_hf.adf_flags : (ad)->ad_df.adf_flags)

#define ad_get_HF_flags(ad)	((ad)->ad_hf.adf_flags)

/* ad_flush.c */
extern void ad_rebuild_header __P((struct adouble *));
extern int ad_flush           __P((struct adouble *, int));
extern int ad_close           __P((struct adouble *, int));

/* ad_lock.c */
extern int ad_fcntl_lock __P((struct adouble *, const u_int32_t /*eid*/,
			      const int /*type*/, const off_t /*offset*/,
			      const off_t /*len*/, const int /*user*/));
extern void ad_fcntl_unlock __P((struct adouble *, const int /*user*/));

extern int ad_fcntl_tmplock __P((struct adouble *, const u_int32_t /*eid*/,
				 const int /*type*/, const off_t /*offset*/,
				 const off_t /*len*/, const int /*user*/));

extern int ad_testlock      __P((struct adouble * /*adp*/, int /*eid*/, off_t /*off*/));
extern int ad_excl_lock     __P((struct adouble * /*adp*/, const u_int32_t /*eid*/));

#define ad_lock ad_fcntl_lock
#define ad_tmplock ad_fcntl_tmplock
#define ad_unlock ad_fcntl_unlock

/* ad_open.c */
extern int ad_setfuid     __P((const uid_t ));
extern uid_t ad_getfuid   __P((void ));

extern char *ad_dir       __P((const char *));
extern char *ad_path      __P((const char *, int));
extern char *ad_path_osx  __P((const char *, int));
extern char *ad_path_ads  __P((const char *, int));

extern int ad_mode        __P((const char *, int));
extern int ad_mkdir       __P((const char *, int));
extern void ad_init       __P((struct adouble *, int, int ));

extern int ad_open        __P((const char *, int, int, int, struct adouble *)); 
extern int ad_refresh     __P((struct adouble *));
extern int ad_stat        __P((const char *, struct stat *));
extern int ad_metadata    __P((const char *, int, struct adouble *));

#define ad_open_metadata(name, flags, mode, adp)\
   ad_open(name, ADFLAGS_HF|(flags), O_RDWR |(mode), 0666, (adp))

#define ad_flush_metadata(adp) ad_flush( (adp), ADFLAGS_HF)
#define ad_close_metadata(adp) ad_close( (adp), ADFLAGS_HF)

/* build a resource fork mode from the data fork mode:
 * remove X mode and extend header to RW if R or W (W if R for locking),
 */ 
#ifndef ATACC
#ifndef __inline__
#define __inline__
#endif
static __inline__ mode_t ad_hf_mode (mode_t mode)
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
#else
extern mode_t ad_hf_mode __P((mode_t ));
#endif

/* ad_read.c/ad_write.c */
extern ssize_t ad_read __P((struct adouble *, const u_int32_t, 
			    const off_t, char *, const size_t));
extern ssize_t ad_pread __P((struct ad_fd *, void *, size_t, off_t));

extern ssize_t ad_write __P((struct adouble *, const u_int32_t, off_t,
			     const int, const char *, const size_t));

extern ssize_t adf_pread  __P((struct ad_fd *, void *, size_t, off_t));
extern ssize_t adf_pwrite __P((struct ad_fd *, const void *, size_t, off_t));

extern int ad_dtruncate __P((struct adouble *, const off_t));
extern int ad_rtruncate __P((struct adouble *, const off_t));

/* ad_size.c */
extern off_t ad_size __P((const struct adouble *, const u_int32_t ));

/* ad_mmap.c */
extern void *ad_mmapread __P((struct adouble *, const u_int32_t,
			      const off_t, const size_t));
extern void *ad_mmapwrite __P((struct adouble *, const u_int32_t,
			       const off_t, const int, const size_t));
#define ad_munmap(buf, len)  (munmap((buf), (len)))

/* ad_date.c */
extern int ad_setdate __P((const struct adouble *, unsigned int, u_int32_t));
extern int ad_getdate __P((const struct adouble *, unsigned int, u_int32_t *));

/* ad_attr.c */
extern int ad_setattr __P((const struct adouble *, const u_int16_t));
extern int ad_getattr __P((const struct adouble *, u_int16_t *));

extern int ad_setname __P((struct adouble *, const char *));
#if AD_VERSION == AD_VERSION2
extern int ad_setid __P((struct adouble *, const dev_t dev,const ino_t ino, const u_int32_t, const u_int32_t, const void *));
extern u_int32_t ad_getid __P((struct adouble *, const dev_t, const ino_t, const cnid_t, const void *));
#else
#define ad_setid(a, b, c)
#endif

#ifdef WITH_SENDFILE
extern ssize_t ad_readfile __P((const struct adouble *, const int, 
				const int, off_t, const size_t));
#endif

#if 0
#ifdef HAVE_SENDFILE_WRITE
extern ssize_t ad_writefile __P((struct adouble *, const int, 
				 const int, off_t, const int, const size_t));
#endif
#endif
     
#endif
