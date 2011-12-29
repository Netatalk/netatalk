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
 * @sa include/atalk/adouble.h
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <sys/param.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include <atalk/logger.h>
#include <atalk/adouble.h>
#include <atalk/util.h>
#include <atalk/ea.h>
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>
#include <atalk/compat.h>
#include <atalk/errchk.h>

#include "ad_lock.h"

#ifndef MAX
#define MAX(a, b)  ((a) < (b) ? (b) : (a))
#endif /* ! MAX */

#define ADEDOFF_MAGIC        (0)
#define ADEDOFF_VERSION      (ADEDOFF_MAGIC + ADEDLEN_MAGIC)
#define ADEDOFF_FILLER       (ADEDOFF_VERSION + ADEDLEN_VERSION)
#define ADEDOFF_NENTRIES     (ADEDOFF_FILLER + ADEDLEN_FILLER)

/* initial lengths of some of the fields */
#define ADEDLEN_INIT     0

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
static int ad_mkrf(const char *path);
static int ad_header_read(struct adouble *ad, struct stat *hst);
static int ad_header_upgrade(struct adouble *ad, const char *name);

static int ad_mkrf_ea(const char *path);
static int ad_header_read_ea(struct adouble *ad, struct stat *hst);
static int ad_header_upgrade_ea(struct adouble *ad, const char *name);

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

#define ADFLAGS2LOGSTRBUFSIZ 128
const char *adflags2logstr(int adflags)
{
    int first = 1;
    static char buf[ADFLAGS2LOGSTRBUFSIZ];

    buf[0] = 0;

    if (adflags & ADFLAGS_DF) {
        strlcat(buf, "DF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_RF) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "RF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_HF) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "HF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_NOHF) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "NOHF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_DIR) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "DIR", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_CHECK_OF) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "OF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_RDWR) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "O_RDWR", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_RDONLY) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "O_RDONLY", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_CREATE) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "O_CREAT", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_EXCL) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "O_EXCL", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }
    if (adflags & ADFLAGS_TRUNC) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "O_TRUNC", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    return buf;
}

static uint32_t get_eid(uint32_t eid)
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
    uint16_t            ashort;
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
    uint16_t            nentries;
    int                 len;
    ssize_t             header_len;
    struct stat         st;

    /* read the header */
    if ((header_len = adf_pread( ad->ad_mdp, buf, sizeof(ad->ad_data), 0)) < 0) {
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
        if (fstat(ad->ad_mdp->adf_fd, &st) < 0) {
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
    if ((header_len = sys_fgetxattr(ad_data_fileno(ad), AD_EA_META, ad->ad_data, AD_DATASZ_EA)) < 1) {
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

static int ad_mkrf(const char *path)
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

static int ad_mkrf_ea(const char *path _U_)
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

/* --------------------------- */
static int ad_header_upgrade(struct adouble *ad _U_, const char *name _U_)
{
    return 0;
}

static int ad_header_upgrade_ea(struct adouble *ad _U_, const char *name _U_)
{
    AFP_PANIC("ad_header_upgrade_ea: dont use");
    return 0;
}

/*!
 * Error handling for adouble header(=metadata) file open error
 *
 * We're called because opening ADFLAGS_HF caused an error.
 * 1. In case ad_open is called with ADFLAGS_NOHF the error is suppressed.
 * 2. If ad_open was called with ADFLAGS_DF we may have opened the datafork and thus
 *    ought to close it before returning with an error condition.
 */
static int ad_error(struct adouble *ad, int adflags)
{
    int err = errno;
    if ((adflags & ADFLAGS_NOHF)) { /* 1 */
        /* FIXME double check : set header offset ?*/
        return 0;
    }
    if ((adflags & ADFLAGS_DF)) { /* 2 */
        ad_close( ad, ADFLAGS_DF );
        err = errno;
    }
    return -1 ;
}

/* Map ADFLAGS to open() flags */
static int ad2openflags(int adflags)
{
    int oflags = 0;

    if (adflags & ADFLAGS_RDWR)
        oflags |= O_RDWR;
    if ((adflags & ADFLAGS_RDONLY) && (adflags & ADFLAGS_SETSHRMD))
        oflags |= O_RDWR;
    else
        oflags |= O_RDONLY;
    if (adflags & ADFLAGS_CREATE)
        oflags |= O_CREAT;
    if (adflags & ADFLAGS_EXCL)
        oflags |= O_EXCL;
    if (adflags & ADFLAGS_TRUNC)
        oflags |= O_TRUNC;

    return oflags;
}

static int ad_open_df(const char *path, int adflags, mode_t mode, struct adouble *ad)
{
    struct stat st_dir;
    int         oflags;
    mode_t      admode;
    int         st_invalid = -1;
    ssize_t     lsz;

    LOG(log_debug, logtype_default, "ad_open_df(\"%s\", %04o)",
        fullpathname(path), mode);

    if (ad_data_fileno(ad) != -1) {
        /* the file is already open, but we want write access: */
        if ((adflags & ADFLAGS_RDWR)
            /* and it was denied the first time: */
            && (ad->ad_data_fork.adf_flags & O_RDONLY)) {
                errno = EACCES;
                return -1;
            }
        /* it's not new anymore */
        ad->ad_mdp->adf_flags &= ~( O_TRUNC | O_CREAT );
        ad->ad_data_fork.adf_refcount++;
        return 0;
    }

    oflags = O_NOFOLLOW | ad2openflags(adflags);

    admode = mode;
    if ((adflags & ADFLAGS_CREATE)) {
        st_invalid = ad_mode_st(path, &admode, &st_dir);
        if ((ad->ad_options & ADVOL_UNIXPRIV))
            admode = mode;
    }

    ad->ad_data_fork.adf_fd = open(path, oflags, admode);

    if (ad->ad_data_fork.adf_fd == -1) {
        switch (errno) {
        case EACCES:
        case EPERM:
        case EROFS:
            if ((adflags & ADFLAGS_SETSHRMD) && (adflags & ADFLAGS_RDONLY)) {
                oflags &= ~O_RDWR;
                oflags |= O_RDONLY;
                if ((ad->ad_data_fork.adf_fd = open(path, oflags, admode)) == -1)
                    return -1;
                break;
            }
            return -1;
        case OPEN_NOFOLLOW_ERRNO:
            ad->ad_data_fork.adf_syml = malloc(MAXPATHLEN+1);
            if ((lsz = readlink(path, ad->ad_data_fork.adf_syml, MAXPATHLEN)) <= 0) {
                free(ad->ad_data_fork.adf_syml);
                return -1;
            }
            ad->ad_data_fork.adf_syml[lsz] = 0;
            ad->ad_data_fork.adf_fd = -2; /* -2 means its a symlink */
            break;
        default:
            return -1;
        }
    }

    if (!st_invalid)
        ad_chown(path, &st_dir); /* just created, set owner if admin (root) */

    ad->ad_data_fork.adf_flags = oflags;
    adf_lock_init(&ad->ad_data_fork);
    ad->ad_data_fork.adf_refcount++;

    return 0;
}

static int ad_open_hf_v2(const char *path, int adflags, mode_t mode, struct adouble *ad)
{
    struct stat st_dir;
    struct stat st_meta;
    struct stat *pst = NULL;
    const char  *ad_p;
    int         oflags, nocreatflags;
    mode_t      admode;
    int         st_invalid = -1;

    if (ad_meta_fileno(ad) != -1) {
        /* the file is already open, but we want write access: */
        if (!(adflags & ADFLAGS_RDONLY) &&
            /* and it was already denied: */
            !(ad->ad_mdp->adf_flags & O_RDWR)) {
            errno = EACCES;
            return -1;
        }
        ad_refresh(ad);
        /* it's not new anymore */
        ad->ad_mdp->adf_flags &= ~( O_TRUNC | O_CREAT );
        ad->ad_mdp->adf_refcount++;
        return 0;
    }

    ad_p = ad->ad_ops->ad_path(path, adflags);
    oflags = O_NOFOLLOW | ad2openflags(adflags);
    nocreatflags = oflags & ~(O_CREAT | O_EXCL);

    ad->ad_mdp->adf_fd = open(ad_p, nocreatflags);

    if (ad->ad_mdp->adf_fd != -1) {
        ad->ad_mdp->adf_flags = nocreatflags;
    } else {
        switch (errno) {
        case EACCES:
        case EPERM:
        case EROFS:
            if ((adflags & ADFLAGS_RDONLY) && (adflags & ADFLAGS_SETSHRMD)) {
                nocreatflags &= ~O_RDWR;
                nocreatflags |= O_RDONLY;
                if ((ad->ad_mdp->adf_fd = open(ad_p, nocreatflags)) == -1)
                    return -1;
                ad->ad_mdp->adf_flags = nocreatflags;
                break;
            }
            return -1;
        case ENOENT:
            if (!(oflags & O_CREAT))
                return ad_error(ad, adflags);
            /*
             * We're expecting to create a new adouble header file here
             * if ((oflags & O_CREAT) ==> (oflags & O_RDWR)
             */
            LOG(log_debug, logtype_default, "ad_open(\"%s\"): creating adouble file",
                fullpathname(path));
            admode = mode;
            errno = 0;
            st_invalid = ad_mode_st(ad_p, &admode, &st_dir);
            if ((ad->ad_options & ADVOL_UNIXPRIV))
                admode = mode;
            admode = ad_hf_mode(admode);
            if (errno == ENOENT) {
                if (ad->ad_ops->ad_mkrf( ad_p) < 0) {
                    return ad_error(ad, adflags);
                }
                admode = mode;
                st_invalid = ad_mode_st(ad_p, &admode, &st_dir);
                if ((ad->ad_options & ADVOL_UNIXPRIV))
                    admode = mode;
                admode = ad_hf_mode(admode);
            }

            /* retry with O_CREAT */
            ad->ad_mdp->adf_fd = open(ad_p, oflags, admode);
            if ( ad->ad_mdp->adf_fd < 0 )
                return ad_error(ad, adflags);

            ad->ad_mdp->adf_flags = oflags;
            /* just created, set owner if admin owner (root) */
            if (!st_invalid)
                ad_chown(ad_p, &st_dir);
            break;
        default:
            return -1;
        }
    }

    if (!(ad->ad_mdp->adf_flags & O_CREAT)) {
        /* check for 0 length files, treat them as new. */
        if (fstat(ad->ad_mdp->adf_fd, &st_meta) == 0) {
            if (st_meta.st_size == 0)
                ad->ad_mdp->adf_flags |= O_TRUNC;
            else
                /* we have valid data in st_meta stat structure, reused it in ad_header_read */
                pst = &st_meta;
        }
    }

    ad->ad_mdp->adf_refcount = 1;
    adf_lock_init(ad->ad_mdp);

    if ((ad->ad_mdp->adf_flags & ( O_TRUNC | O_CREAT ))) {
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
            ad_close(ad, adflags);
            errno = err;
            return -1;
        }
    }

    return 0;
}

static int ad_open_hf_ea(const char *path, int adflags, int mode, struct adouble *ad)
{
    ssize_t rforklen;
    int oflags = O_NOFOLLOW;

    oflags = ad2openflags(adflags) & ~(O_CREAT | O_TRUNC);

    if (ad_meta_fileno(ad) == -1) {
        if ((ad_meta_fileno(ad) = open(path, oflags)) == -1)
            goto error;
        ad->ad_mdp->adf_flags = oflags;
        ad->ad_mdp->adf_refcount = 1;
        adf_lock_init(ad->ad_mdp);
    }

    /* Read the adouble header in and parse it.*/
    if (ad->ad_ops->ad_header_read(ad, NULL) != 0) {
        if (!(adflags & ADFLAGS_CREATE))
            goto error;

        /* It doesnt exist, EPERM or another error */
        if (!(errno == ENOATTR || errno == ENOENT)) {
            LOG(log_error, logtype_default, "ad_open_hf_ea: unexpected: %s", strerror(errno));
            goto error;
        }

        /* Create one */
        if (new_ad_header(path, ad, adflags) < 0) {
            LOG(log_error, logtype_default, "ad_open_hf_ea: can't create new header: %s",
                fullpathname(path));
            goto error;
        }
        ad->ad_mdp->adf_flags |= O_CREAT; /* mark as just created */
        ad_flush(ad);
        LOG(log_debug, logtype_default, "ad_open_hf_ea(\"%s\"): created metadata EA", path);
    }

    ad->ad_mdp->adf_refcount++;

    if ((rforklen = sys_fgetxattr(ad_meta_fileno(ad), AD_EA_RESO, NULL, 0)) > 0)
        ad->ad_rlen = rforklen;

    return 0;

error:
    if (ad_meta_fileno(ad) != -1) {
        close(ad_meta_fileno(ad));
        ad_meta_fileno(ad) = -1;
    }
    return ad_error(ad, adflags);
}

static int ad_open_hf(const char *path, int adflags, int mode, struct adouble *ad)
{
    int ret = 0;

    LOG(log_debug, logtype_default, "ad_open_hf(\"%s\", %04o)", path, mode);

    memset(ad->ad_eid, 0, sizeof( ad->ad_eid ));
    ad->ad_rlen = 0;

    switch (ad->ad_flags) {
    case AD_VERSION2:
        ret = ad_open_hf_v2(path, adflags, mode, ad);
        break;
    case AD_VERSION_EA:
        ret = ad_open_hf_ea(path, adflags, mode, ad);
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

/*!
 * Open ressource fork
 *
 * Only for adouble:ea, a nullop otherwise because adouble:v2 has the ressource fork as part
 * of the adouble file which is openend by ADFLAGS_HF.
 */
static int ad_open_rf(const char *path, int adflags, int mode, struct adouble *ad)
{
    int ret = 0;

    if (ad->ad_flags != AD_VERSION_EA)
        return 0;

    LOG(log_debug, logtype_default, "ad_open_rf(\"%s\", %04o)",
        path, mode);

    if ((ad->ad_rlen = sys_fgetxattr(ad_meta_fileno(ad), AD_EA_RESO, NULL, 0)) <= 0) {
        switch (errno) {
        case ENOATTR:
            ad->ad_rlen = 0;
            break;
        default:
            LOG(log_warning, logtype_default, "ad_open_rf(\"%s\"): %s",
                fullpathname(path), strerror(errno));
            ret = -1;
            goto exit;
        }
    }

    /* Round up and allocate buffer */
    size_t roundup = ((ad->ad_rlen / RFORK_EA_ALLOCSIZE) + 1) * RFORK_EA_ALLOCSIZE;
    if ((ad->ad_resforkbuf = malloc(roundup)) == NULL) {
        ret = -1;
        goto exit;
    }

    ad->ad_resforkbufsize = roundup;

    /* Read the EA into the buffer */
    if (ad->ad_rlen > 0) {
        if (sys_fgetxattr(ad_meta_fileno(ad), AD_EA_RESO, ad->ad_resforkbuf, ad->ad_rlen) == -1) {
            ret = -1;
            goto exit;
        }       
    }

exit:
    if (ret != 0) {
        free(ad->ad_resforkbuf);
        ad->ad_resforkbuf = NULL;
        ad->ad_rlen = 0;
        ad->ad_resforkbufsize = 0;
    }

    return ret;
}

/***********************************************************************************
 * API functions
 ********************************************************************************* */

const char *ad_path_ea( const char *path, int adflags _U_)
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
const char *ad_path( const char *path, int adflags)
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
    switch (flags) {
    case AD_VERSION2:
        ad->ad_ops = &ad_adouble;
        ad->ad_rfp = &ad->ad_resource_fork;
        ad->ad_mdp = &ad->ad_resource_fork;
        break;
    case AD_VERSION_EA:
        ad->ad_ops = &ad_adouble_ea;
        ad->ad_rfp = &ad->ad_data_fork;
        ad->ad_mdp = &ad->ad_data_fork;
        break;
    default:
        LOG(log_error, logtype_default, "ad_init: unknown AD version");
        errno = EIO;
        return;
    }

    ad->ad_flags = flags;
    ad->ad_options = options;
    ad_data_fileno(ad) = -1;
    ad_reso_fileno(ad) = -1;
    ad_meta_fileno(ad) = -1;
    memset(ad->ad_eid, 0, sizeof( ad->ad_eid ));
    ad->ad_rlen = 0;
    ad->ad_refcount = 1;
    ad->ad_open_forks = 0;
    ad->ad_resource_fork.adf_refcount = 0;
    ad->ad_resforkbuf = NULL;
    ad->ad_data_fork.adf_refcount = 0;
    ad->ad_data_fork.adf_syml=0;
    ad->ad_inited = 0;
}

/*!
 * Open data-, metadata(header)- or ressource fork
 *
 * ad_open(struct adouble *ad, const char *path, int adflags, int flags)
 * ad_open(struct adouble *ad, const char *path, int adflags, int flags, mode_t mode)
 *
 * You must call ad_init() before ad_open, usually you'll just call it like this: \n
 * @code
 *      struct adoube ad;
 *      ad_init(&ad, vol->v_adouble, vol->v_ad_options);
 * @endcode
 *
 * Open a files data fork, metadata fork or ressource fork.
 *
 * @param ad        (rw) pointer to struct adouble
 * @param path      (r)  Path to file or directory
 * @param adflags   (r)  ADFLAGS_DF:        open data fork \n
 *                       ADFLAGS_RF:        open ressource fork \n
 *                       ADFLAGS_HF:        open header (metadata) file \n
 *                       ADFLAGS_NOHF:      it's not an error if header file couldn't be created \n
 *                       ADFLAGS_DIR:       if path is a directory you MUST or ADFLAGS_DIR to adflags \n
 * The open mode flags (rw vs ro) have to take into account all the following requirements:
 * - we remember open fds for files because me must avoid a single close releasing fcntl locks for other
 *   fds of the same file
 * - a file may be opened first ro, then rw and theres no way to upgrade this -> fork.c always opens rw
 *                       ADFLAGS_CHECK_OF:  check for open forks from us and other afpd's
 * @param mode      (r)  mode used with O_CREATE
 *
 * @returns 0 on success, any other value indicates an error
 */
int ad_open(struct adouble *ad, const char *path, int adflags, ...)
{
    EC_INIT;
    va_list args;
    mode_t mode = 0;

    LOG(log_debug, logtype_default, "ad_open(\"%s\", %s)",
        fullpathname(path), adflags2logstr(adflags));

    if (ad->ad_inited != AD_INITED) {
        ad->ad_adflags = adflags;
        ad->ad_inited = AD_INITED;
    } else {
        ad->ad_open_forks = ((ad->ad_data_fork.adf_refcount > 0) ? ATTRBIT_DOPEN : 0);
        if (ad->ad_resource_fork.adf_refcount > 0)
            ad->ad_open_forks |= ATTRBIT_ROPEN;
    }

    va_start(args, adflags);
    if (adflags & ADFLAGS_CREATE)
        mode = va_arg(args, mode_t);
    va_end(args);

    if (adflags & ADFLAGS_DF) {
        EC_ZERO( ad_open_df(path, adflags, mode, ad) );
    }

    if (adflags & ADFLAGS_HF) {
        EC_ZERO( ad_open_hf(path, adflags, mode, ad) );
    }

    if (adflags & ADFLAGS_RF) {
        EC_ZERO( ad_open_rf(path, adflags, mode, ad) );
    }

EC_CLEANUP:
    return ret;
}

/*!
 * @brief open metadata, possibly as root
 *
 * Return only metadata but try very hard ie at first try as user, then try as root.
 *
 * @param name  name of file/dir
 * @param flags ADFLAGS_DIR: name is a directory \n
 *              ADFLAGS_CHECK_OF: test if name is open by us or another afpd process
 *
 * @param adp   pointer to struct adouble
 */
int ad_metadata(const char *name, int flags, struct adouble *adp)
{
    uid_t uid;
    int   ret, err, oflags;

    oflags = (flags & ADFLAGS_DIR) | ADFLAGS_HF | ADFLAGS_RDONLY;

    if ((ret = ad_open(adp, name, oflags | ADFLAGS_SETSHRMD)) < 0 && errno == EACCES) {
        uid = geteuid();
        if (seteuid(0)) {
            LOG(log_error, logtype_default, "ad_metadata(%s): seteuid failed %s", name, strerror(errno));
            errno = EACCES;
            return -1;
        }
        /* we are root open read only */
        ret = ad_open(adp, name, oflags);
        err = errno;
        if ( seteuid(uid) < 0) {
            LOG(log_error, logtype_default, "ad_metadata: can't seteuid back");
            exit(EXITERR_SYS);
        }
        errno = err;
    }

    if ((ret == 0) && (ADFLAGS_CHECK_OF & flags)) {
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

int ad_openat(struct adouble  *ad,
              int dirfd,  /* dir fd openat like */
              const char *path,
              int adflags, ...)
{
    EC_INIT;
    int cwdfd = -1;
    va_list args;
    mode_t mode;

    if (dirfd != -1) {
        if ((cwdfd = open(".", O_RDONLY) == -1) || (fchdir(dirfd) != 0))
            EC_FAIL;
    }

    va_start(args, adflags);
    if (adflags & ADFLAGS_CREATE)
        mode = va_arg(args, mode_t);
    va_end(args);

    EC_NEG1( ad_open(ad, path, adflags, mode) );

    if (dirfd != -1) {
        if (fchdir(cwdfd) != 0) {
            AFP_PANIC("ad_openat: cant chdir back");
        }
    }

EC_CLEANUP:
    if (cwdfd != -1)
        close(cwdfd);

    return ret;
}
