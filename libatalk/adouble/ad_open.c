/*
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * Copyright (c) 2010 Frank Lahm
 *
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
 *
 */

/*!
 * @file
 * Part of Netatalk's AppleDouble implementatation
 * @note We don't use inlines because a good compiler should be
 *       able to optimize all the static funcs below.
 * @sa include/atalk/adouble.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/logger.h>
#include <atalk/adouble.h>
#include <atalk/util.h>
#include <atalk/ea.h>

#include "ad_private.h"

#ifndef MAX
#define MAX(a, b)  ((a) < (b) ? (b) : (a))
#endif /* ! MAX */

#ifdef  HAVE_PREAD
# define AD_SET(a)
#else
# define AD_SET(a) a = 0
#endif

/*
 * AppleDouble entry default offsets.
 * The layout looks like this:
 *
 * this is the v1 layout:
 *     255         200         16          32          N
 *  |  NAME |    COMMENT    | FILEI |    FINDERI    | RFORK |
 *
 * we need to change it to look like this:
 *
 * v2 layout:
 * field       length (in bytes)
 * NAME        255
 * COMMENT     200
 * FILEDATESI  16     replaces FILEI
 * FINDERI     32
 * DID          4     new
 * AFPFILEI     4     new
 * SHORTNAME   12     8.3 new
 * RFORK        N
 *
 * so, all we need to do is replace FILEI with FILEDATESI, move RFORK,
 * and add in the new fields.
 */

#define ADEDOFF_MAGIC        (0)
#define ADEDOFF_VERSION      (ADEDOFF_MAGIC + ADEDLEN_MAGIC)
#define ADEDOFF_FILLER       (ADEDOFF_VERSION + ADEDLEN_VERSION)
#define ADEDOFF_NENTRIES     (ADEDOFF_FILLER + ADEDLEN_FILLER)

/* initial lengths of some of the fields */
#define ADEDLEN_INIT     0

/* make sure we don't redefine ADEDOFF_FILEI */
#ifdef ADEDOFF_FILEI
#undef ADEDOFF_FILEI
#endif /* ADEDOFF_FILEI */

/* i stick things in a slightly different order than their eid order in
 * case i ever want to separate RootInfo behaviour from the rest of the
 * stuff. */

/* ad:v2 */
#define ADEDOFF_NAME_V2      (AD_HEADER_LEN + ADEID_NUM_V2*AD_ENTRY_LEN)
#define ADEDOFF_COMMENT_V2   (ADEDOFF_NAME_V2 + ADEDLEN_NAME)
#define ADEDOFF_FILEDATESI   (ADEDOFF_COMMENT_V2 + ADEDLEN_COMMENT)
#define ADEDOFF_FINDERI_V2   (ADEDOFF_FILEDATESI + ADEDLEN_FILEDATESI)
#define ADEDOFF_DID          (ADEDOFF_FINDERI_V2 + ADEDLEN_FINDERI)
#define ADEDOFF_AFPFILEI     (ADEDOFF_DID + ADEDLEN_DID)
#define ADEDOFF_SHORTNAME    (ADEDOFF_AFPFILEI + ADEDLEN_AFPFILEI)
#define ADEDOFF_PRODOSFILEI  (ADEDOFF_SHORTNAME + ADEDLEN_SHORTNAME)
#define ADEDOFF_PRIVDEV      (ADEDOFF_PRODOSFILEI + ADEDLEN_PRODOSFILEI)
#define ADEDOFF_PRIVINO      (ADEDOFF_PRIVDEV + ADEDLEN_PRIVDEV)
#define ADEDOFF_PRIVSYN      (ADEDOFF_PRIVINO + ADEDLEN_PRIVINO)
#define ADEDOFF_PRIVID       (ADEDOFF_PRIVSYN + ADEDLEN_PRIVSYN)
#define ADEDOFF_RFORK_V2     (ADEDOFF_PRIVID + ADEDLEN_PRIVID)

/* ad:ea */
#define ADEDOFF_FINDERI_EA   (AD_HEADER_LEN + ADEID_NUM_EA * AD_ENTRY_LEN)
#define ADEDOFF_COMMENT_EA   (ADEDOFF_FINDERI_EA + ADEDLEN_FINDERI)
#define ADEDOFF_FILEDATESI_EA (ADEDOFF_COMMENT_EA + ADEDLEN_COMMENT)
#define ADEDOFF_AFPFILEI_EA  (ADEDOFF_FILEDATESI_EA + ADEDLEN_FILEDATESI)

/* this is to prevent changing timezones from causing problems with
   localtime volumes. the screw-up is 30 years. we use a delta of 5 years */
#define TIMEWARP_DELTA 157680000

struct entry {
    uint32_t id, offset, len;
};

/* --------------------------- */
static uid_t default_uid = -1;

/* Forward declarations */
static int ad_mkrf(char *path);
static int ad_header_read(struct adouble *ad, struct stat *hst);
static int ad_header_upgrade(struct adouble *ad, char *name);

static int ad_mkrf_ea(char *path);
static int ad_header_read_ea(struct adouble *ad, struct stat *hst);
static int ad_header_upgrade_ea(struct adouble *ad, char *name);

static struct adouble_fops ad_adouble = {
    &ad_path,
    &ad_mkrf,
    &ad_rebuild_adouble_header,
    &ad_header_read,
    &ad_header_upgrade,
};

static struct adouble_fops ad_adouble_ea = {
    &ad_path_ea,
    &ad_mkrf_ea,
    &ad_rebuild_adouble_header,
    &ad_header_read_ea,
    &ad_header_upgrade_ea,
};

static const struct entry entry_order2[ADEID_NUM_V2 + 1] = {
    {ADEID_NAME,        ADEDOFF_NAME_V2,     ADEDLEN_INIT},
    {ADEID_COMMENT,     ADEDOFF_COMMENT_V2,  ADEDLEN_INIT},
    {ADEID_FILEDATESI,  ADEDOFF_FILEDATESI,  ADEDLEN_FILEDATESI},
    {ADEID_FINDERI,     ADEDOFF_FINDERI_V2,  ADEDLEN_FINDERI},
    {ADEID_DID,         ADEDOFF_DID,         ADEDLEN_DID},
    {ADEID_AFPFILEI,    ADEDOFF_AFPFILEI,    ADEDLEN_AFPFILEI},
    {ADEID_SHORTNAME,   ADEDOFF_SHORTNAME,   ADEDLEN_INIT},
    {ADEID_PRODOSFILEI, ADEDOFF_PRODOSFILEI, ADEDLEN_PRODOSFILEI},
    {ADEID_PRIVDEV,     ADEDOFF_PRIVDEV,     ADEDLEN_INIT},
    {ADEID_PRIVINO,     ADEDOFF_PRIVINO,     ADEDLEN_INIT},
    {ADEID_PRIVSYN,     ADEDOFF_PRIVSYN,     ADEDLEN_INIT},
    {ADEID_PRIVID,      ADEDOFF_PRIVID,      ADEDLEN_INIT},
    {ADEID_RFORK,       ADEDOFF_RFORK_V2,    ADEDLEN_INIT},
    {0, 0, 0}
};

/* Using Extended Attributes */
static const struct entry entry_order_ea[ADEID_NUM_EA + 1] = {
    {ADEID_FINDERI,    ADEDOFF_FINDERI_EA,    ADEDLEN_FINDERI},
    {ADEID_COMMENT,    ADEDOFF_COMMENT_EA,    ADEDLEN_INIT},
    {ADEID_FILEDATESI, ADEDOFF_FILEDATESI_EA, ADEDLEN_FILEDATESI},
    {ADEID_AFPFILEI,   ADEDOFF_AFPFILEI_EA,   ADEDLEN_AFPFILEI},
    {0, 0, 0}
};

uint32_t get_eid(uint32_t eid)
{
    if (eid <= 15)
        return eid;
    if (eid == AD_DEV)
        return ADEID_PRIVDEV;
    if (eid == AD_INO)
        return ADEID_PRIVINO;
    if (eid == AD_SYN)
        return ADEID_PRIVSYN;
    if (eid == AD_ID)
        return ADEID_PRIVID;

    return 0;
}

/* ----------------------------------- */
static int new_ad_header(const char *path, struct adouble *ad, int adflags)
{
    const struct entry  *eid;
    u_int16_t           ashort;
    struct stat         st;

    ad->ad_magic = AD_MAGIC;
    ad->ad_version = ad->ad_flags & 0x0f0000;
    if (!ad->ad_version) {
        ad->ad_version = AD_VERSION;
    }


    memset(ad->ad_data, 0, sizeof(ad->ad_data));

    if (ad->ad_flags == AD_VERSION2)
        eid = entry_order2;
    else if (ad->ad_flags == AD_VERSION_EA)
        eid = entry_order_ea;
    else {
        return -1;
    }

    while (eid->id) {
        ad->ad_eid[eid->id].ade_off = eid->offset;
        ad->ad_eid[eid->id].ade_len = eid->len;
        eid++;
    }

    /* put something sane in the directory finderinfo */
    if ((adflags & ADFLAGS_DIR)) {
        /* set default view */
        ashort = htons(FINDERINFO_CLOSEDVIEW);
        memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRVIEWOFF, &ashort, sizeof(ashort));
    } else {
        /* set default creator/type fields */
        memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRTYPEOFF,"\0\0\0\0", 4);
        memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRCREATOFF,"\0\0\0\0", 4);
    }

    /* make things invisible */
    if ((ad->ad_options & ADVOL_INVDOTS) && (*path == '.')) {
        ashort = htons(ATTRBIT_INVISIBLE);
        ad_setattr(ad, ashort);
        ashort = htons(FINDERINFO_INVISIBLE);
        memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRFLAGOFF, &ashort, sizeof(ashort));
    }

    if (lstat(path, &st) < 0)
        return -1;

    /* put something sane in the date fields */
    ad_setdate(ad, AD_DATE_CREATE | AD_DATE_UNIX, st.st_mtime);
    ad_setdate(ad, AD_DATE_MODIFY | AD_DATE_UNIX, st.st_mtime);
    ad_setdate(ad, AD_DATE_ACCESS | AD_DATE_UNIX, st.st_mtime);
    ad_setdate(ad, AD_DATE_BACKUP, AD_DATE_START);
    return 0;
}

/* -------------------------------------
   read in the entries
*/
static void parse_entries(struct adouble *ad, char *buf, uint16_t nentries)
{
    uint32_t   eid, len, off;
    int        warning = 0;

    /* now, read in the entry bits */
    for (; nentries > 0; nentries-- ) {
        memcpy(&eid, buf, sizeof( eid ));
        eid = get_eid(ntohl(eid));
        buf += sizeof( eid );
        memcpy(&off, buf, sizeof( off ));
        off = ntohl( off );
        buf += sizeof( off );
        memcpy(&len, buf, sizeof( len ));
        len = ntohl( len );
        buf += sizeof( len );

        if (eid && eid < ADEID_MAX && off < sizeof(ad->ad_data) &&
            (off + len <= sizeof(ad->ad_data) || eid == ADEID_RFORK)) {
            ad->ad_eid[ eid ].ade_off = off;
            ad->ad_eid[ eid ].ade_len = len;
        } else if (!warning) {
            warning = 1;
            LOG(log_warning, logtype_default, "parse_entries: bogus eid: %d", eid);
        }
    }
}

/* this reads enough of the header so that we can figure out all of
 * the entry lengths and offsets. once that's done, we just read/mmap
 * the rest of the header in.
 *
 * NOTE: we're assuming that the resource fork is kept at the end of
 *       the file. also, mmapping won't work for the hfs fs until it
 *       understands how to mmap header files. */
static int ad_header_read(struct adouble *ad, struct stat *hst)
{
    char                *buf = ad->ad_data;
    u_int16_t           nentries;
    int                 len;
    ssize_t             header_len;
    struct stat         st;

    /* read the header */
    if ((header_len = adf_pread( ad->ad_md, buf, sizeof(ad->ad_data), 0)) < 0) {
        return -1;
    }
    if (header_len < AD_HEADER_LEN) {
        errno = EIO;
        return -1;
    }

    memcpy(&ad->ad_magic, buf, sizeof( ad->ad_magic ));
    memcpy(&ad->ad_version, buf + ADEDOFF_VERSION, sizeof( ad->ad_version ));
    ad->ad_magic = ntohl( ad->ad_magic );
    ad->ad_version = ntohl( ad->ad_version );

    if ((ad->ad_magic != AD_MAGIC) || (ad->ad_version != AD_VERSION2)) {
        LOG(log_error, logtype_default, "ad_open: can't parse AppleDouble header.");
        errno = EIO;
        return -1;
    }

    memcpy(&nentries, buf + ADEDOFF_NENTRIES, sizeof( nentries ));
    nentries = ntohs( nentries );

    /* read in all the entry headers. if we have more than the
     * maximum, just hope that the rfork is specified early on. */
    len = nentries*AD_ENTRY_LEN;

    if (len + AD_HEADER_LEN > sizeof(ad->ad_data))
        len = sizeof(ad->ad_data) - AD_HEADER_LEN;

    buf += AD_HEADER_LEN;
    if (len > header_len - AD_HEADER_LEN) {
        LOG(log_error, logtype_default, "ad_header_read: can't read entry info.");
        errno = EIO;
        return -1;
    }

    /* figure out all of the entry offsets and lengths. if we aren't
     * able to read a resource fork entry, bail. */
    nentries = len / AD_ENTRY_LEN;
    parse_entries(ad, buf, nentries);
    if (!ad_getentryoff(ad, ADEID_RFORK)
        || (ad_getentryoff(ad, ADEID_RFORK) > sizeof(ad->ad_data))
        ) {
        LOG(log_error, logtype_default, "ad_header_read: problem with rfork entry offset.");
        errno = EIO;
        return -1;
    }

    if (ad_getentryoff(ad, ADEID_RFORK) > header_len) {
        LOG(log_error, logtype_default, "ad_header_read: can't read in entries.");
        errno = EIO;
        return -1;
    }

    if (hst == NULL) {
        hst = &st;
        if (fstat(ad->ad_md->adf_fd, &st) < 0) {
            return 1; /* fail silently */
        }
    }

    ad->ad_rlen = hst->st_size - ad_getentryoff(ad, ADEID_RFORK);

    return 0;
}

static int ad_header_read_ea(struct adouble *ad, struct stat *hst _U_)
{
    uint16_t nentries;
    int      len;
    ssize_t  header_len;
    char     *buf = ad->ad_data;

    /* read the header */
    if ((header_len = sys_fgetxattr(ad->ad_md->adf_fd, AD_EA_META, ad->ad_data, AD_DATASZ_EA)) < 1) {
        LOG(log_debug, logtype_default, "ad_header_read_ea: %s (%u)", strerror(errno), errno);
        return -1;
    }

    if (header_len < AD_HEADER_LEN) {
        LOG(log_error, logtype_default, "ad_header_read_ea: bogus AppleDouble header.");
        errno = EIO;
        return -1;
    }

    memcpy(&ad->ad_magic, buf, sizeof( ad->ad_magic ));
    memcpy(&ad->ad_version, buf + ADEDOFF_VERSION, sizeof( ad->ad_version ));

    ad->ad_magic = ntohl( ad->ad_magic );
    ad->ad_version = ntohl( ad->ad_version );

    if ((ad->ad_magic != AD_MAGIC) || (ad->ad_version != AD_VERSION2)) {
        LOG(log_error, logtype_default, "ad_header_read_ea: wrong magic or version");
        errno = EIO;
        return -1;
    }

    memcpy(&nentries, buf + ADEDOFF_NENTRIES, sizeof( nentries ));
    nentries = ntohs( nentries );

    /* Protect against bogus nentries */
    len = nentries * AD_ENTRY_LEN;
    if (len + AD_HEADER_LEN > sizeof(ad->ad_data))
        len = sizeof(ad->ad_data) - AD_HEADER_LEN;
    if (len > header_len - AD_HEADER_LEN) {
        LOG(log_error, logtype_default, "ad_header_read_ea: can't read entry info.");
        errno = EIO;
        return -1;
    }
    nentries = len / AD_ENTRY_LEN;

    /* Now parse entries */
    parse_entries(ad, buf + AD_HEADER_LEN, nentries);
    return 0;
}

static int ad_mkrf(char *path)
{
    char *slash;
    /*
     * Probably .AppleDouble doesn't exist, try to mkdir it.
     */
    if (NULL == ( slash = strrchr( path, '/' )) ) {
        return -1;
    }
    *slash = '\0';
    errno = 0;
    if ( ad_mkdir( path, 0777 ) < 0 ) {
        return -1;
    }
    *slash = '/';
    return 0;
}

static int ad_mkrf_ea(char *path _U_)
{
    AFP_PANIC("ad_mkrf_ea: dont use");
    return 0;
}

/* ----------------
   if we are root change path user/ group
   It can be a native function for BSD cf. FAQ.Q10
   path:  pathname to chown
   stbuf: parent directory inode

   use fstat and fchown or lchown with linux?
*/
#define EMULATE_SUIDDIR

static int ad_chown(const char *path, struct stat *stbuf)
{
    int ret = 0;
#ifdef EMULATE_SUIDDIR
    uid_t id;

    if (default_uid != (uid_t)-1) {
        /* we are root (admin) */
        id = (default_uid)?default_uid:stbuf->st_uid;
        ret = lchown( path, id, stbuf->st_gid );
    }
#endif
    return ret;
}

#define DEFMASK 07700   /* be conservative */

/* ----------------
   return access right and inode of path parent directory
*/
static int ad_mode_st(const char *path, int *mode, struct stat *stbuf)
{
    if (*mode == 0) {
        return -1;
    }
    if (ad_stat(path, stbuf) != 0) {
        *mode &= DEFMASK;
        return -1;
    }
    *mode &= stbuf->st_mode;
    return 0;
}

/* ----------------- */
static int ad_error(struct adouble *ad, int adflags)
{
    int err = errno;
    if ((adflags & ADFLAGS_NOHF)) {
        /* FIXME double check : set header offset ?*/
        return 0;
    }
    if ((adflags & ADFLAGS_DF)) {
        ad_close( ad, ADFLAGS_DF );
        err = errno;
    }
    return -1 ;
}

/* --------------------------- */
static int ad_header_upgrade(struct adouble *ad _U_, char *name _U_)
{
    return 0;
}

static int ad_header_upgrade_ea(struct adouble *ad _U_, char *name _U_)
{
    AFP_PANIC("ad_header_upgrade_ea: dont use");
    return 0;
}

static int ad_open_df(const char *path, int adflags, int oflags, int mode, struct adouble *ad)
{
    struct stat st_dir;
    int         hoflags, admode;
    int         st_invalid = -1;

    LOG(log_maxdebug, logtype_default, "ad_open_df(\"%s/%s\", adf: 0x%04x, of: 0x%04x)",
        getcwdpath(), path, adflags, oflags);

    if (ad_data_fileno(ad) == -1) {
        hoflags = (oflags & ~(O_RDONLY | O_WRONLY)) | O_RDWR;
        admode = mode;
        if ((oflags & O_CREAT)) {
            st_invalid = ad_mode_st(path, &admode, &st_dir);
            if ((ad->ad_options & ADVOL_UNIXPRIV)) {
                admode = mode;
            }
        }

        ad->ad_data_fork.adf_fd = open(path, hoflags | O_NOFOLLOW, admode);

        if (ad->ad_data_fork.adf_fd == -1) {
            if ((errno == EACCES || errno == EROFS) && !(oflags & O_RDWR)) {
                hoflags = oflags;
                ad->ad_data_fork.adf_fd = open( path, hoflags | O_NOFOLLOW, admode );
            }
            if (ad->ad_data_fork.adf_fd == -1 && errno == OPEN_NOFOLLOW_ERRNO) {
                int lsz;

                ad->ad_data_fork.adf_syml = malloc(MAXPATHLEN+1);
                lsz = readlink(path, ad->ad_data_fork.adf_syml, MAXPATHLEN);
                if (lsz <= 0) {
                    free(ad->ad_data_fork.adf_syml);
                    return -1;
                }
                ad->ad_data_fork.adf_syml[lsz] = 0;
                ad->ad_data_fork.adf_fd = -2; /* -2 means its a symlink */
            }
        }

        if ( ad->ad_data_fork.adf_fd == -1 )
            return -1;

        AD_SET(ad->ad_data_fork.adf_off);
        ad->ad_data_fork.adf_flags = hoflags;
        if (!st_invalid) {
            /* just created, set owner if admin (root) */
            ad_chown(path, &st_dir);
        }
        adf_lock_init(&ad->ad_data_fork);
    } else {
        /* the file is already open... but */
        if ((oflags & ( O_RDWR | O_WRONLY))
            /* we want write access */
            && !(ad->ad_data_fork.adf_flags & ( O_RDWR | O_WRONLY))) {
            /* and it was denied the first time */
            errno = EACCES;
            return -1;
        }
        /* FIXME
         * for now ad_open is never called with O_TRUNC or O_EXCL if the file is
         * already open. Should we check for it? ie
         * O_EXCL --> error
         * O_TRUNC --> truncate the fork.
         * idem for ressource fork.
         */
    }
    ad->ad_data_fork.adf_refcount++;

    return 0;
}

static int ad_open_hf_v2(const char *path, int adflags, int oflags, int mode, struct adouble *ad)
{
    struct stat st_dir;
    struct stat st_meta;
    struct stat *pst = NULL;
    char        *ad_p;
    int         hoflags, admode;
    int         st_invalid = -1;

    ad_p = ad->ad_ops->ad_path( path, adflags );

    hoflags = oflags & ~(O_CREAT | O_EXCL);
    if (!(adflags & ADFLAGS_RDONLY)) {
        hoflags = (hoflags & ~(O_RDONLY | O_WRONLY)) | O_RDWR;
    }
    ad->ad_md->adf_fd = open( ad_p, hoflags | O_NOFOLLOW, 0 );
    if (ad->ad_md->adf_fd < 0 ) {
        if ((errno == EACCES || errno == EROFS) && !(oflags & O_RDWR)) {
            hoflags = oflags & ~(O_CREAT | O_EXCL);
            ad->ad_md->adf_fd = open( ad_p, hoflags | O_NOFOLLOW, 0 );
        }
    }

    if ( ad->ad_md->adf_fd < 0 ) {
        if (errno == ENOENT && (oflags & O_CREAT) ) {
            /*
             * We're expecting to create a new adouble header file here
             * if ((oflags & O_CREAT) ==> (oflags & O_RDWR)
             */
            LOG(log_debug, logtype_default, "ad_open(\"%s/%s\"): creating adouble file",
                getcwdpath(), ad_p);
            admode = mode;
            errno = 0;
            st_invalid = ad_mode_st(ad_p, &admode, &st_dir);
            if ((ad->ad_options & ADVOL_UNIXPRIV)) {
                admode = mode;
            }
            admode = ad_hf_mode(admode);
            if ((errno == ENOENT)) {
                if (ad->ad_ops->ad_mkrf( ad_p) < 0) {
                    return ad_error(ad, adflags);
                }
                admode = mode;
                st_invalid = ad_mode_st(ad_p, &admode, &st_dir);
                if ((ad->ad_options & ADVOL_UNIXPRIV)) {
                    admode = mode;
                }
                admode = ad_hf_mode(admode);
            }
            /* retry with O_CREAT */
            ad->ad_md->adf_fd = open( ad_p, oflags,admode );
            if ( ad->ad_md->adf_fd < 0 ) {
                return ad_error(ad, adflags);
            }
            ad->ad_md->adf_flags = oflags;
            /* just created, set owner if admin owner (root) */
            if (!st_invalid) {
                ad_chown(ad_p, &st_dir);
            }
        } else {
            return ad_error(ad, adflags);
        }
    } else {
        ad->ad_md->adf_flags = hoflags;
        if (fstat(ad->ad_md->adf_fd, &st_meta) == 0 && st_meta.st_size == 0) {
            /* for 0 length files, treat them as new. */
            ad->ad_md->adf_flags |= O_TRUNC;
        } else {
            /* we have valid data in st_meta stat structure, reused it in ad_header_read */
            pst = &st_meta;
        }
    }
    AD_SET(ad->ad_md->adf_off);

    if ((ad->ad_md->adf_flags & ( O_TRUNC | O_CREAT ))) {
        /* This is a new adouble header file, create it */
        if (new_ad_header(path, ad, adflags) < 0) {
            int err = errno;
            /* the file is already deleted, perm, whatever, so return an error */
            ad_close(ad, adflags);
            errno = err;
            return -1;
        }
        ad_flush(ad);
    } else {
        /* Read the adouble header in and parse it.*/
        if (ad->ad_ops->ad_header_read( ad , pst) < 0
            || ad->ad_ops->ad_header_upgrade(ad, ad_p) < 0) {
            int err = errno;
            ad_close( ad, adflags );
            errno = err;
            return -1;
        }
    }

    return 0;
}

static int ad_open_hf_ea(const char *path, int adflags, int oflags, int mode, struct adouble *ad)
{
    ssize_t rforklen;
    int ret;
    int err;

    LOG(log_maxdebug, logtype_default, "ad_open_hf_ea(\"%s/%s\", adf: 0x%04x, of: 0x%04x)",
        getcwdpath(), path, adflags, oflags);

    if ((ad->ad_md->adf_fd = open(path, O_RDONLY | O_NOFOLLOW)) == -1)
        return -1;

    /* Read the adouble header in and parse it.*/
    if (ad->ad_ops->ad_header_read(ad, NULL) != 0) {
        if (!(oflags & O_CREAT)) {
            err = errno;
            ad_close(ad, adflags);
            errno = err;
            return -1;
        }

        /* It doesnt exist, EPERM or another error */
        if (errno != ENOATTR && errno != ENOENT) {
            LOG(log_error, logtype_default, "ad_open_hf_ea: unexpected: %s", strerror(errno));
            ret = -1;
            goto error;
        }

        /* Create one */
        if (new_ad_header(path, ad, adflags) < 0) {
            LOG(log_error, logtype_default, "ad_open_hf_ea: can't create new header: %s/%s",
                getcwdpath(), path);
            ret = -1;
            goto error;
        }

        ad_flush(ad);
    }

    ad->ad_md->adf_flags = O_RDWR; /* Pretend its rw, in fact for the EA API it is */

    if ((rforklen = sys_fgetxattr(ad->ad_md->adf_fd, AD_EA_RESO, NULL, 0)) > 0)
        ad->ad_rlen = rforklen;

    return 0;

error:
    err = errno;
    ad_close(ad, adflags);
    errno = err;
    return ret;
}

static int ad_open_hf(const char *path, int adflags, int oflags, int mode, struct adouble *ad)
{
    int ret = 0;

    LOG(log_maxdebug, logtype_default, "ad_open_hf(\"%s/%s\", adf: 0x%04x, of: 0x%04x)",
        getcwdpath(), path, adflags, oflags);

    if (ad_meta_fileno(ad) != -1) { /* the file is already open */
        LOG(log_maxdebug, logtype_default,"the file is already open");
        if ((oflags & ( O_RDWR | O_WRONLY))
            && !(ad->ad_md->adf_flags & ( O_RDWR | O_WRONLY))) {
            if ((adflags & ADFLAGS_HF) && ad->ad_data_fork.adf_refcount)
                /* We just have openend the df, so we have to close it now */
                ad_close(ad, ADFLAGS_DF);
            errno = EACCES;
            return -1;
        }
        ad_refresh(ad);
        /* it's not new anymore */
        ad->ad_md->adf_flags &= ~( O_TRUNC | O_CREAT );
        ad->ad_md->adf_refcount++;
        return 0;
    }

    memset(ad->ad_eid, 0, sizeof( ad->ad_eid ));
    ad->ad_rlen = 0;
    adf_lock_init(ad->ad_md);

    switch (ad->ad_flags) {
    case AD_VERSION2:
        ret = ad_open_hf_v2(path, adflags, oflags, mode, ad);
        break;
    case AD_VERSION_EA:
        ret = ad_open_hf_ea(path, adflags, oflags, mode, ad);
        break;
    default:
        ret = -1;
        break;
    }

    ad->ad_md->adf_refcount = 1;

    return ret;
}

static int ad_open_rf(const char *path, int adflags, int oflags, int mode, struct adouble *ad)
{
    return 0;
}

/***********************************************************************************
 * API functions
 ********************************************************************************* */

char *ad_path_ea( const char *path, int adflags _U_)
{
    return path;
}

/*
 * Put the .AppleDouble where it needs to be:
 *
 *      /   a/.AppleDouble/b
 *  a/b
 *      \   b/.AppleDouble/.Parent
 *
 * FIXME: should do something for pathname > MAXPATHLEN
 */
char *ad_path( const char *path, int adflags)
{
    static char pathbuf[ MAXPATHLEN + 1];
    const char *slash;
    size_t  l ;

    if ( adflags & ADFLAGS_DIR ) {
        l = strlcpy( pathbuf, path, sizeof(pathbuf));

        if ( l && l < MAXPATHLEN) {
            pathbuf[l++] = '/';
        }
        strlcpy(pathbuf +l, ".AppleDouble/.Parent", sizeof(pathbuf) -l);
    } else {
        if (NULL != ( slash = strrchr( path, '/' )) ) {
            slash++;
            l = slash - path;
            /* XXX we must return NULL here and test in the caller */
            if (l > MAXPATHLEN)
                l = MAXPATHLEN;
            memcpy( pathbuf, path, l);
        } else {
            l = 0;
            slash = path;
        }
        l += strlcpy( pathbuf +l, ".AppleDouble/", sizeof(pathbuf) -l);
        strlcpy( pathbuf + l, slash, sizeof(pathbuf) -l);
    }

    return( pathbuf );
}

/* -------------------------
 * Support inherited protection modes for AppleDouble files.  The supplied
 * mode is ANDed with the parent directory's mask value in lieu of "umask",
 * and that value is returned.
 */
char *ad_dir(const char *path)
{
    static char     modebuf[ MAXPATHLEN + 1];
    char        *slash;
    /*
     * For a path with directories in it, remove the final component
     * (path or subdirectory name) to get the name we want to stat.
     * For a path which is just a filename, use "." instead.
     */
    slash = strrchr( path, '/' );
    if (slash) {
        size_t len;

        len = slash - path;
        if (len >= MAXPATHLEN) {
            errno = ENAMETOOLONG;
            return NULL;  /* can't do it */
        }
        memcpy( modebuf, path, len );
        modebuf[len] = '\0';
        /* is last char a '/' ? */
        if (slash[1] == 0) {
            slash = modebuf+ len;
            /* remove them */
            while (modebuf < slash && slash[-1] == '/') {
                --slash;
            }
            if (modebuf == slash) {
                goto use_cur;
            }
            *slash = '\0';
            while (modebuf < slash && *slash != '/') {
                --slash;
            }
            if (modebuf == slash) {
                goto use_cur;
            }
            *slash = '\0';      /* remove pathname component */
        }
        return modebuf;
    }
use_cur:
    modebuf[0] = '.';   /* use current directory */
    modebuf[1] = '\0';
    return modebuf;
}

int ad_setfuid(const uid_t id)
{
    default_uid = id;
    return 0;
}

/* ---------------- */
uid_t ad_getfuid(void)
{
    return default_uid;
}

/* ----------------
   stat path parent directory
*/
int ad_stat(const char *path, struct stat *stbuf)
{
    char *p;

    p = ad_dir(path);
    if (!p)
        return -1;
    return lstat( p, stbuf );
}

/* ----------------
   return access right of path parent directory
*/
int ad_mode( const char *path, int mode)
{
    struct stat     stbuf;
    ad_mode_st(path, &mode, &stbuf);
    return mode;
}

/*
 * Use mkdir() with mode bits taken from ad_mode().
 */
int ad_mkdir( const char *path, int mode)
{
    int ret;
    int st_invalid;
    struct stat stbuf;

    LOG(log_debug, logtype_default, "ad_mkdir(\"%s\", %04o) {cwd: \"%s\"}",
        path, mode, getcwdpath());

    st_invalid = ad_mode_st(path, &mode, &stbuf);
    ret = mkdir( path, mode );
    if (ret || st_invalid)
        return ret;
    ad_chown(path, &stbuf);

    return ret;
}

void ad_init(struct adouble *ad, int flags, int options)
{
    ad->ad_inited = 0;
    ad->ad_flags = flags;
    if (flags == AD_VERSION2) {
        ad->ad_ops = &ad_adouble;
        ad->ad_md = &ad->ad_resource_fork;
    }
    else if (flags == AD_VERSION_EA) {
        ad->ad_ops = &ad_adouble_ea;
        ad->ad_md = &ad->ad_metadata_fork;
    } else {
        LOG(log_error, logtype_default, "ad_init: unknown AD version");
        errno = EIO;
        return;
    }

    ad->ad_options = options;

    ad_data_fileno(ad) = -1;
    ad_reso_fileno(ad) = -1;
    ad_meta_fileno(ad) = -1;

    /* following can be read even if there's no meda data. */
    memset(ad->ad_eid, 0, sizeof( ad->ad_eid ));
    ad->ad_rlen = 0;
}

static const char *adflags2logstr(int adflags)
{
    int first = 1;
    static char buf[64];

    buf[0] = 0;

    if (adflags & ADFLAGS_DF) {
        strlcat(buf, "DF", 64);
        first = 0;
    }
    if (adflags & ADFLAGS_RF) {
        if (!first)
            strlcat(buf, "|", 64);
        strlcat(buf, "RF", 64);
        first = 0;
    }
    if (adflags & ADFLAGS_HF) {
        if (!first)
            strlcat(buf, "|", 64);
        strlcat(buf, "HF", 64);
        first = 0;
    }
    if (adflags & ADFLAGS_NOHF) {
        if (!first)
            strlcat(buf, "|", 64);
        strlcat(buf, "NOHF", 64);
        first = 0;
    }
    if (adflags & ADFLAGS_DIR) {
        if (!first)
            strlcat(buf, "|", 64);
        strlcat(buf, "DIR", 64);
        first = 0;
    }
    if (adflags & ADFLAGS_RDONLY) {
        if (!first)
            strlcat(buf, "|", 64);
        strlcat(buf, "RDONLY", 64);
        first = 0;
    }
    if (adflags & ADFLAGS_OPENFORKS) {
        if (!first)
            strlcat(buf, "|", 64);
        strlcat(buf, "OF", 64);
        first = 0;
    }
    return buf;
}

static const char *oflags2logstr(int oflags)
{
    int first = 1;
    static char buf[64];

    buf[0] = 0;

    if (oflags & O_RDONLY) {
        strlcat(buf, "O_RDONLY", 64);
        first = 0;
    }
    if (oflags & O_RDWR) {
        if (!first)
            strlcat(buf, "|", 64);
        strlcat(buf, "O_RDWR", 64);
        first = 0;
    }
    if (oflags & O_CREAT) {
        if (!first)
            strlcat(buf, "|", 64);
        strlcat(buf, "O_CREAT", 64);
        first = 0;
    }
    if (oflags & O_EXCL) {
        if (!first)
            strlcat(buf, "|", 64);
        strlcat(buf, "O_EXCL", 64);
        first = 0;
    }
    return buf;
}

/*!
 * Open data-, metadata(header)- or ressource fork
 *
 * You must call ad_init() before ad_open, usually you'll just call it like this: \n
 * @code
 *      struct adoube ad;
 *      ad_init(&ad, vol->v_adouble, vol->v_ad_options);
 * @endcode
 *
 * @param path    Path to file or directory
 *
 * @param adflags ADFLAGS_DF:        open data fork \n
 *                ADFLAGS_RF:        open ressource fork \n
 *                ADFLAGS_HF:        open header (metadata) file \n
 *                ADFLAGS_NOHF:      it's not an error if header file couldn't be created \n
 *                ADFLAGS_DIR:       if path is a directory you MUST or ADFLAGS_DIR to adflags \n
 *                ADFLAGS_RDONLY:    open read only \n
 *                ADFLAGS_OPENFORKS: check for open forks from other processes
 *
 * @param oflags  flags passed through to open syscall: \n
 *                O_RDONLY: *** FIXME *** \n
 *                O_RDWR: *** FIXME *** \n
 *                O_CREAT: create fork \n
 *                O_EXCL: fail if exists with O_CREAT
 *
 * @param mode    passed to open with O_CREAT
 * @param ad      pointer to struct adouble
 *
 * @returns 0 on success
 *
 * @note It's not possible to open the header file O_RDONLY -- the read
 *       will fail and return an error. this refcounts things now.\n
 *       metadata(ressource)-fork only gets created with O_CREAT.
 */
int ad_open(const char *path, int adflags, int oflags, int mode, struct adouble  *ad)
{
    LOG(log_debug, logtype_default, "ad_open(\"%s\", %s, %s, 0%04o)",
        abspath(path), adflags2logstr(adflags), oflags2logstr(oflags), mode);

    if (ad->ad_inited != AD_INITED) {
        ad->ad_inited = AD_INITED;
        ad->ad_refcount = 1;
        ad->ad_open_forks = 0;
        ad->ad_adflags = adflags;
        ad->ad_resource_fork.adf_refcount = 0;
        ad->ad_data_fork.adf_refcount = 0;
        ad->ad_data_fork.adf_syml = 0;
    } else {
        ad->ad_open_forks = ((ad->ad_data_fork.adf_refcount > 0) ? ATTRBIT_DOPEN : 0);
        /* XXX not true if we have a meta data fork ? */
        if ((ad->ad_resource_fork.adf_refcount > ad->ad_data_fork.adf_refcount))
            ad->ad_open_forks |= ATTRBIT_ROPEN;
    }

    if ((adflags & ADFLAGS_DF)) {
        if (ad_open_df(path, adflags, oflags, mode, ad) != 0)
            return -1;
    }

    if ((adflags & ADFLAGS_HF)) {
        if (ad_open_hf(path, adflags, oflags, mode, ad) != 0)
            return -1;
    }

#if 0
    if ((adflags & ADFLAGS_RF)) {
        if (ad_open_rf(path, adflags, oflags, mode, ad) != 0)
            return -1;
    }
#endif

    return 0;
}

/*!
 * @brief open metadata, possibly as root
 *
 * Return only metadata but try very hard ie at first try as user, then try as root.
 * Caller must pass ADFLAGS_DIR for directories.
 *
 * @param name  name of file/dir
 * @param flags ADFLAGS_DIR: name is a directory \n
 *              ADFLAGS_OPENFORKS: test if name is open by another afpd process
 *
 * @param adp   pointer to struct adouble
 */
int ad_metadata(const char *name, int flags, struct adouble *adp)
{
    uid_t uid;
    int   ret, err, dir;
    int   create = O_RDONLY;

    dir = flags & ADFLAGS_DIR;

    if ((ret = ad_open(name, ADFLAGS_HF | dir, create, 0666, adp)) < 0 && errno == EACCES) {
        uid = geteuid();
        if (seteuid(0)) {
            LOG(log_error, logtype_default, "ad_metadata(%s): seteuid failed %s", name, strerror(errno));
            errno = EACCES;
            return -1;
        }
        /* we are root open read only */
        ret = ad_open(name, ADFLAGS_HF|ADFLAGS_RDONLY| dir, O_RDONLY, 0, adp);
        err = errno;
        if ( seteuid(uid) < 0) {
            LOG(log_error, logtype_default, "ad_metadata: can't seteuid back");
            exit(EXITERR_SYS);
        }
        errno = err;
    }

    if (!ret && (ADFLAGS_OPENFORKS & flags)) {
        /*
          we need to check if the file is open by another process.
          it's slow so we only do it if we have to:
          - it's requested.
          - we don't already have the answer!
        */
        adp->ad_open_forks |= ad_openforks(adp, adp->ad_open_forks);
    }
    return ret;
}

/*
 * @brief openat like wrapper for ad_metadata
 */
int ad_metadataat(int dirfd, const char *name, int flags, struct adouble *adp)
{
    int ret = 0;
    int cwdfd = -1;

    if (dirfd != -1) {
        if ((cwdfd = open(".", O_RDONLY) == -1) || (fchdir(dirfd) != 0)) {
            ret = -1;
            goto exit;
        }
    }

    if (ad_metadata(name, flags, adp) < 0) {
        ret = -1;
        goto exit;
    }

    if (dirfd != -1) {
        if (fchdir(cwdfd) != 0) {
            LOG(log_error, logtype_afpd, "ad_openat: cant chdir back, exiting");
            exit(EXITERR_SYS);
        }
    }

exit:
    if (cwdfd != -1)
        close(cwdfd);

    return ret;

}

int ad_refresh(struct adouble *ad)
{

    if (ad_meta_fileno(ad) == -1)
        return -1;

    return ad->ad_ops->ad_header_read(ad, NULL);
}

int ad_openat(int dirfd,  /* dir fd openat like */
              const char *path,
              int adflags,
              int oflags,
              int mode,
              struct adouble  *ad)
{
    int ret = 0;
    int cwdfd = -1;

    if (dirfd != -1) {
        if ((cwdfd = open(".", O_RDONLY) == -1) || (fchdir(dirfd) != 0)) {
            ret = -1;
            goto exit;
        }
    }

    if (ad_open(path, adflags, oflags, mode, ad) < 0) {
        ret = -1;
        goto exit;
    }

    if (dirfd != -1) {
        if (fchdir(cwdfd) != 0) {
            LOG(log_error, logtype_afpd, "ad_openat: cant chdir back, exiting");
            exit(EXITERR_SYS);
        }
    }

exit:
    if (cwdfd != -1)
        close(cwdfd);

    return ret;
}
