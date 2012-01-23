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
#include <atalk/volume.h>

#include "ad_lock.h"

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
static int ad_header_read(const char *path, struct adouble *ad, struct stat *hst);
static int ad_header_upgrade(struct adouble *ad, const char *name);

static int ad_mkrf_ea(const char *path);
static int ad_header_read_ea(const char *path, struct adouble *ad, struct stat *hst);
static int ad_header_upgrade_ea(struct adouble *ad, const char *name);
static int ad_reso_size(const char *path, int adflags, struct adouble *ad);
static int ad_mkrf_osx(const char *path);


static struct adouble_fops ad_adouble = {
    &ad_path,
    &ad_mkrf,
    &ad_rebuild_adouble_header,
    &ad_header_read,
    &ad_header_upgrade,
};

static struct adouble_fops ad_adouble_ea = {
#ifdef HAVE_EAFD
    &ad_path_ea,
    &ad_mkrf_ea,
#else
    &ad_path_osx,
    &ad_mkrf_osx,
#endif
    &ad_rebuild_adouble_header,
    &ad_header_read_ea,
    &ad_header_upgrade_ea,
};

static struct adouble_fops ad_osx = {
    &ad_path_osx,
    &ad_mkrf_osx,
    &ad_rebuild_adouble_header,
    &ad_header_read,
    &ad_header_upgrade,
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
    {ADEID_PRIVID,     ADEDOFF_PRIVID,        ADEDLEN_INIT},
    {0, 0, 0}
};

/* fallback for EAs */
static const struct entry entry_order_osx[ADEID_NUM_OSX +1] = {
    {ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI},
    {ADEID_RFORK, ADEDOFF_RFORK_OSX, ADEDLEN_INIT},
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
    if (adflags & ADFLAGS_NORF) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "NORF", ADFLAGS2LOGSTRBUFSIZ);
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
    if (adflags & ADFLAGS_SETSHRMD) {
        if (!first)
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        strlcat(buf, "SHRMD", ADFLAGS2LOGSTRBUFSIZ);
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
static int new_ad_header(struct adouble *ad, const char *path, struct stat *stp, int adflags)
{
    const struct entry  *eid;
    uint16_t            ashort;
    struct stat         st;

    LOG(log_debug, logtype_default, "new_ad_header(\"%s\")", path);

    if (stp == NULL) {
        stp = &st;
        if (lstat(path, &st) != 0)
            return -1;
    }

    ad->ad_magic = AD_MAGIC;
    ad->ad_version = ad->ad_vers & 0x0f0000;
    if (!ad->ad_version) {
        ad->ad_version = AD_VERSION;
    }

    memset(ad->ad_data, 0, sizeof(ad->ad_data));

    if (ad->ad_vers == AD_VERSION2)
        eid = entry_order2;
    else if (ad->ad_vers == AD_VERSION_EA)
        eid = entry_order_ea;
    else if (ad->ad_vers == AD_VERSION2_OSX)
        eid = entry_order_osx;
    else {
        return -1;
    }

    while (eid->id) {
        ad->ad_eid[eid->id].ade_off = eid->offset;
        ad->ad_eid[eid->id].ade_len = eid->len;
        eid++;
    }

    /* put something sane in the directory finderinfo */
    if (ad->ad_vers != AD_VERSION2_OSX) {
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
        if ((ad->ad_options & ADVOL_INVDOTS)
            && (*path == '.')
            && !((adflags & ADFLAGS_DIR) && (path[1] == 0))
            ) {
            ashort = htons(ATTRBIT_INVISIBLE);
            ad_setattr(ad, ashort);
            ashort = htons(FINDERINFO_INVISIBLE);
            memcpy(ad_entry(ad, ADEID_FINDERI) + FINDERINFO_FRFLAGOFF, &ashort, sizeof(ashort));
        }

        /* put something sane in the date fields */
        ad_setdate(ad, AD_DATE_CREATE | AD_DATE_UNIX, stp->st_mtime);
        ad_setdate(ad, AD_DATE_MODIFY | AD_DATE_UNIX, stp->st_mtime);
        ad_setdate(ad, AD_DATE_ACCESS | AD_DATE_UNIX, stp->st_mtime);
        ad_setdate(ad, AD_DATE_BACKUP, AD_DATE_START);
    }
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

        if (eid
            && eid < ADEID_MAX
            && off < sizeof(ad->ad_data)
            && (off + len <= sizeof(ad->ad_data) || eid == ADEID_RFORK)) {
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
static int ad_header_read(const char *path _U_, struct adouble *ad, struct stat *hst)
{
    char                *buf = ad->ad_data;
    uint16_t            nentries;
    int                 len;
    ssize_t             header_len;
    struct stat         st;

    /* read the header */
    if ((header_len = adf_pread( ad->ad_mdp, buf, AD_DATASZ, 0)) < 0) {
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

/* Read an ._ file, only uses the resofork, finderinfo is taken from EA */
static int ad_header_read_osx(const char *path _U_, struct adouble *ad, struct stat *hst)
{
    EC_INIT;
    struct adouble      adosx;
    char                *buf = &adosx.ad_data[0];
    uint16_t            nentries;
    int                 len;
    ssize_t             header_len;
    struct stat         st;

    memset(buf, 0, sizeof(adosx.ad_data));

    /* read the header */
    EC_NEG1( header_len = adf_pread(ad->ad_rfp, buf, AD_DATASZ_OSX, 0) );

    if (header_len < AD_HEADER_LEN) {
        errno = EIO;
        return -1;
    }

    memcpy(&adosx.ad_magic, buf, sizeof(adosx.ad_magic));
    memcpy(&adosx.ad_version, buf + ADEDOFF_VERSION, sizeof(adosx.ad_version));
    adosx.ad_magic = ntohl(adosx.ad_magic);
    adosx.ad_version = ntohl(adosx.ad_version);

    if ((adosx.ad_magic != AD_MAGIC) || (adosx.ad_version != AD_VERSION2)) {
        LOG(log_error, logtype_afpd, "ad_header_read_osx: can't parse AppleDouble header");
        errno = EIO;
        return -1;
    }

    memcpy(&nentries, buf + ADEDOFF_NENTRIES, sizeof( nentries ));
    nentries = ntohs(nentries);
    len = nentries * AD_ENTRY_LEN;

    if (len + AD_HEADER_LEN > sizeof(adosx.ad_data))
        len = sizeof(adosx.ad_data) - AD_HEADER_LEN;

    buf += AD_HEADER_LEN;
    if (len > header_len - AD_HEADER_LEN) {
        LOG(log_error, logtype_afpd, "ad_header_read_osx: can't read entry info.");
        errno = EIO;
        return -1;
    }

    nentries = len / AD_ENTRY_LEN;
    parse_entries(&adosx, buf, nentries);

    if (ad_getentryoff(&adosx, ADEID_RFORK) == 0
        || ad_getentryoff(&adosx, ADEID_RFORK) > sizeof(ad->ad_data)
        || ad_getentryoff(&adosx, ADEID_RFORK) > header_len
        ) {
        LOG(log_error, logtype_afpd, "ad_header_read_osx: problem with rfork entry offset.");
        errno = EIO;
        return -1;
    }

    if (hst == NULL) {
        hst = &st;
        EC_NEG1( fstat(ad_reso_fileno(ad), &st) );
    }

    ad_setentryoff(ad, ADEID_RFORK, ad_getentryoff(&adosx, ADEID_RFORK));
    ad->ad_rlen = hst->st_size - ad_getentryoff(ad, ADEID_RFORK);

EC_CLEANUP:
    EC_EXIT;
}

static int ad_header_read_ea(const char *path, struct adouble *ad, struct stat *hst _U_)
{
    uint16_t nentries;
    int      len;
    ssize_t  header_len;
    char     *buf = ad->ad_data;

    if (ad_meta_fileno(ad) != -1)
        header_len = sys_fgetxattr(ad_meta_fileno(ad), AD_EA_META, ad->ad_data, AD_DATASZ_EA);
    else
        header_len = sys_lgetxattr(path, AD_EA_META, ad->ad_data, AD_DATASZ_EA);
     if (header_len < 1) {
        LOG(log_debug, logtype_default, "ad_header_read_ea: %s", strerror(errno));
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

/*!
 * Takes a path to an AppleDouble file and creates the parrent .AppleDouble directory
 *
 * Example:
 * path: "/path/.AppleDouble/file"
 * => mkdir("/path/.AppleDouble/") (in ad_mkdir())
 */
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

static int ad_mkrf_osx(const char *path _U_)
{
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
static int ad_mode_st(const char *path, mode_t *mode, struct stat *stbuf)
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
    if (adflags & ADFLAGS_NOHF) { /* 1 */
        return 0;
    }
    if (adflags & ADFLAGS_DF) { /* 2 */
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
    if (adflags & ADFLAGS_RDONLY) {
        if (adflags & ADFLAGS_SETSHRMD)
            oflags |= O_RDWR;
        else
            oflags |= O_RDONLY;
    }
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
        ad->ad_data_fork.adf_flags &= ~( O_TRUNC | O_CREAT );
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

/* TODO: error handling */
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
        if ((adflags & ADFLAGS_RDWR) &&
            /* and it was already denied: */
            (ad->ad_mdp->adf_flags & O_RDONLY)) {
            errno = EACCES;
            return -1;
        }
        ad_refresh(path, ad);
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

    adf_lock_init(ad->ad_mdp);
    ad->ad_mdp->adf_refcount = 1;

    if ((ad->ad_mdp->adf_flags & ( O_TRUNC | O_CREAT ))) {
        /* This is a new adouble header file, create it */
        if (new_ad_header(ad, path, pst, adflags) < 0) {
            int err = errno;
            /* the file is already deleted, perm, whatever, so return an error */
            ad_close(ad, adflags);
            errno = err;
            return -1;
        }
        ad_flush(ad);
    } else {
        /* Read the adouble header in and parse it.*/
        if (ad->ad_ops->ad_header_read(path, ad, pst) < 0
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
    EC_INIT;
    ssize_t rforklen;
    int oflags;
    int opened = 0;

    LOG(log_debug, logtype_default,
        "ad_open_hf_ea(\"%s\", %s): BEGIN [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags),
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);

    oflags = O_NOFOLLOW | (ad2openflags(adflags) & ~(O_CREAT | O_TRUNC));

    if (ad_meta_fileno(ad) != -1) {
        /* the file is already open, but we want write access: */
        if ((oflags & O_RDWR) &&
            /* and it was already denied: */
            (ad->ad_mdp->adf_flags & O_RDONLY)) {
            LOG(log_error, logtype_default, "ad_open_hf_ea(%s): rw request for ro file: %s",
                fullpathname(path), strerror(errno));
            errno = EACCES;
            EC_FAIL;
        }
        /* it's not new anymore */
        ad->ad_mdp->adf_flags &= ~( O_TRUNC | O_CREAT );
    } else {
        if (oflags & O_RDWR) {
            /* Fo a RDONLY adouble we just use sys_lgetxattr instead if sys_fgetxattr */
            if (adflags & ADFLAGS_DIR)
                /* For directories we open the directory RDONYL so we can later fchdir()  */
                oflags = (oflags & ~O_RDWR) | O_RDONLY;
            LOG(log_debug, logtype_default, "ad_open_hf_ea(\"%s\"): opening base file for meta adouble EA", path);
            EC_NEG1(ad_meta_fileno(ad) = open(path, oflags));
            opened = 1;
            ad->ad_mdp->adf_flags = oflags;
        }
    }

    /* Read the adouble header in and parse it.*/
    if (ad->ad_ops->ad_header_read(path, ad, NULL) != 0) {
        LOG(log_error, logtype_default, "ad_open_hf_ea: no EA adouble");

        if (!(adflags & ADFLAGS_CREATE)) {
            errno = ENOENT;
            EC_FAIL;
        }

        LOG(log_debug, logtype_default, "ad_open_hf_ea(\"%s\"): creating metadata EA", path);

        /* It doesnt exist, EPERM or another error */
        if (!(errno == ENOATTR || errno == ENOENT)) {
            LOG(log_error, logtype_default, "ad_open_hf_ea: unexpected: %s", strerror(errno));
            EC_FAIL;
        }

        /* Create one */
        EC_NEG1_LOG(new_ad_header(ad, path, NULL, adflags));
        ad->ad_mdp->adf_flags |= O_CREAT; /* mark as just created */
        ad_flush(ad);
        LOG(log_debug, logtype_default, "ad_open_hf_ea(\"%s\"): created metadata EA", path);
    }

    ad->ad_mdp->adf_refcount++;
    (void)ad_reso_size(path, adflags, ad);

EC_CLEANUP:
    if (ret != 0 && opened && ad_meta_fileno(ad) != -1) {
        close(ad_meta_fileno(ad));
        ad_meta_fileno(ad) = -1;
        ad->ad_mdp->adf_refcount = 0;
    }
    LOG(log_debug, logtype_default,
        "ad_open_hf_ea(\"%s\", %s): END: %d [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags), ret,
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);
        
    EC_EXIT;
}

static int ad_open_hf(const char *path, int adflags, int mode, struct adouble *ad)
{
    int ret = 0;

    memset(ad->ad_eid, 0, sizeof( ad->ad_eid ));
    ad->ad_rlen = 0;

    switch (ad->ad_vers) {
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

    if (ret != 0)
        ret = ad_error(ad, adflags);

    return ret;
}

/*!
 * Get resofork length for adouble:ea
 */
static int ad_reso_size(const char *path, int adflags, struct adouble *ad)
{
    EC_INIT;
    struct stat st;

    LOG(log_debug, logtype_default, "ad_reso_size(\"%s\")", path);

#ifdef HAVE_EAFD
    int opened = 0;
    int eafd = ad_reso_fileno(ad);
    if (eafd == -1) {
        EC_NEG1( eafd = sys_getxattrfd(path, O_RDONLY) );
        opened = 1;
    }
    EC_NEG1( rlen = fstat(eafd, &st) );
    ad->ad_rlen = st.st_size;
#else
    const char *rfpath;
    EC_NULL_LOG( rfpath = ad->ad_ops->ad_path(path, adflags));
    EC_ZERO( lstat(rfpath, &st));
    if (st.st_size > ADEDOFF_RFORK_OSX)
        ad->ad_rlen = st.st_size - ADEDOFF_RFORK_OSX;
    else
        ad->ad_rlen = 0;
#endif

    LOG(log_debug, logtype_default, "ad_reso_size(\"%s\"): size: %zd", path, ad->ad_rlen);

EC_CLEANUP:
#ifdef HAVE_EAFD
    if (opened)
        close(eafd);
#endif
    if (ret != 0)
        ad->ad_rlen = 0;
    EC_EXIT;
}

/*!
 * Open ressource fork
 *
 * Only for adouble:ea, a nullop otherwise because adouble:v2 has the ressource fork as part
 * of the adouble file which is openend by ADFLAGS_HF.
 */
static int ad_open_rf(const char *path, int adflags, int mode, struct adouble *ad)
{
    EC_INIT;
    int oflags;
    int opened = 0;
    int closeflags = adflags & (ADFLAGS_DF | ADFLAGS_HF);
    ssize_t rlen;
#ifndef HAVE_EAFD
    const char *rfpath;
    struct stat st;
#endif

    if (ad->ad_vers != AD_VERSION_EA)
        return 0;

    LOG(log_debug, logtype_default, "ad_open_rf(\"%s\"): BEGIN", fullpathname(path));

    oflags = O_NOFOLLOW | (ad2openflags(adflags) & ~O_CREAT);

    if (ad_reso_fileno(ad) != -1) {
        /* the file is already open, but we want write access: */
        if ((oflags & O_RDWR)
            /* and it was already denied: */
            && (ad->ad_rfp->adf_flags & O_RDONLY)) {
            errno = EACCES;
            EC_FAIL;
        }
        ad->ad_rfp->adf_flags &= ~( O_TRUNC | O_CREAT );
        ad->ad_rfp->adf_refcount++;
        EC_NEG1_LOG( ad_reso_size(path, adflags, ad));
        goto EC_CLEANUP;
    }
#ifdef HAVE_EAFD
    if ((ad_reso_fileno(ad) = sys_getxattrfd(path, oflags)) == -1) {
        if (!(adflags & ADFLAGS_CREATE))
            EC_FAIL;
        oflags |= O_CREAT;
        EC_NEG1_LOG( ad_reso_fileno(ad) = sys_getxattrfd(path, oflags, 0666) ); 
    }
#else
    EC_NULL_LOG( rfpath = ad->ad_ops->ad_path(path, adflags) );
    if ((ad_reso_fileno(ad) = open(rfpath, oflags)) == -1) {
        if (!(adflags & ADFLAGS_CREATE))
            EC_FAIL;
        oflags |= O_CREAT;
        EC_NEG1_LOG( ad_reso_fileno(ad) = open(rfpath, oflags, mode) );
        LOG(log_debug, logtype_default, "ad_open_rf(\"%s\"): created adouble rfork: \"%s\"",
            path, rfpath);
    }
#endif
    opened = 1;
    ad->ad_rfp->adf_refcount = 1;
    ad->ad_rfp->adf_flags = oflags;

#ifndef HAVE_EAFD
    EC_ZERO_LOG( fstat(ad_reso_fileno(ad), &st) );
    if (ad->ad_rfp->adf_flags & O_CREAT) {
        /* This is a new adouble header file, create it */
        LOG(log_debug, logtype_default, "ad_open_rf(\"%s\"): created adouble rfork, initializing: \"%s\"",
            path, rfpath);
        LOG(log_debug, logtype_default, "ad_open_rf(\"%s\"): created adouble rfork, flushing: \"%s\"",
            path, rfpath);
        ad_flush(ad);
    } else {
        /* Read the adouble header */
        LOG(log_debug, logtype_default, "ad_open_rf(\"%s\"): reading adouble rfork: \"%s\"",
            path, rfpath);
        EC_NEG1_LOG( ad_header_read_osx(NULL, ad, &st) );
    }
#endif

    (void)ad_reso_size(path, adflags, ad);

EC_CLEANUP:
    if (ret != 0) {
        if (opened && (ad_reso_fileno(ad) != -1)) {
            close(ad_reso_fileno(ad));
            ad_reso_fileno(ad) = -1;
            ad->ad_rfp->adf_refcount = 0;
        }
        if (adflags & ADFLAGS_NORF) {
            ret = 0;
        } else {
            int err = errno;
            (void)ad_close(ad, closeflags);
            errno = err;
        }
        ad->ad_rlen = 0;
    }

    LOG(log_debug, logtype_default, "ad_open_rf(\"%s\"): END: %d", fullpathname(path), ret);

    EC_EXIT;
}

/***********************************************************************************
 * API functions
 ********************************************************************************* */

const char *ad_path_ea( const char *path, int adflags _U_)
{
    return path;
}

const char *ad_path_osx(const char *path, int adflags _U_)
{
    static char pathbuf[ MAXPATHLEN + 1];
    char    c, *slash, buf[MAXPATHLEN + 1];

    if (!strcmp(path,".")) {
        /* fixme */
        getcwd(buf, MAXPATHLEN);
    }
    else {
        strlcpy(buf, path, MAXPATHLEN +1);
    }
    if (NULL != ( slash = strrchr( buf, '/' )) ) {
        c = *++slash;
        *slash = '\0';
        strlcpy( pathbuf, buf, MAXPATHLEN +1);
        *slash = c;
    } else {
        pathbuf[ 0 ] = '\0';
        slash = buf;
    }
    strlcat( pathbuf, "._", MAXPATHLEN  +1);
    strlcat( pathbuf, slash, MAXPATHLEN +1);
    return pathbuf;
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
int ad_mode( const char *path, mode_t mode)
{
    struct stat     stbuf;
    ad_mode_st(path, &mode, &stbuf);
    return mode;
}

/*
 * Use mkdir() with mode bits taken from ad_mode().
 */
int ad_mkdir( const char *path, mode_t mode)
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

static void ad_init_func(struct adouble *ad)
{
    switch (ad->ad_vers) {
    case AD_VERSION2:
        ad->ad_ops = &ad_adouble;
        ad->ad_rfp = &ad->ad_resource_fork;
        ad->ad_mdp = &ad->ad_resource_fork;
        break;
    case AD_VERSION_EA:
        ad->ad_ops = &ad_adouble_ea;
        ad->ad_rfp = &ad->ad_resource_fork;
        ad->ad_mdp = &ad->ad_data_fork;
        break;
    default:
        AFP_PANIC("ad_init: unknown AD version");
    }


    ad_data_fileno(ad) = -1;
    ad_reso_fileno(ad) = -1;
    ad_meta_fileno(ad) = -1;
    ad->ad_refcount = 1;
    return;
}

void ad_init_old(struct adouble *ad, int flags, int options)
{
    memset(ad, 0, sizeof(struct adouble));
    ad->ad_vers = flags;
    ad->ad_options = options;
    ad_init_func(ad);
}

void ad_init(struct adouble *ad, const struct vol * restrict vol)
{
    memset(ad, 0, sizeof(struct adouble));
    ad->ad_vers = vol->v_adouble;
    ad->ad_options = vol->v_ad_options;
    ad_init_func(ad);
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
 * @param adflags   (r)  Flags specifying which fork to open, can be or'd:
 *                         ADFLAGS_DF:        open data fork
 *                         ADFLAGS_RF:        open ressource fork
 *                         ADFLAGS_HF:        open header (metadata) file
 *                         ADFLAGS_NOHF:      it's not an error if header file couldn't be opened
 *                         ADFLAGS_NORF:      it's not an error if reso fork couldn't be opened
 *                         ADFLAGS_DIR:       if path is a directory you MUST or ADFLAGS_DIR to adflags
 *
 *                       Access mode for the forks:
 *                         ADFLAGS_RDONLY:    open read only
 *                         ADFLAGS_RDWR:      open read write
 *
 *                       Creation flags:
 *                         ADFLAGS_CREATE:    create if not existing
 *                         ADFLAGS_TRUNC:     truncate
 *
 *                       Special flags:
 *                         ADFLAGS_CHECK_OF:  check for open forks from us and other afpd's
 *                         ADFLAGS_SETSHRMD:  this adouble struct will be used to set sharemode locks.
 *                                            This basically results in the files being opened RW instead of RDONLY.
 * @param mode      (r)  mode used with O_CREATE
 *
 * The open mode flags (rw vs ro) have to take into account all the following requirements:
 * - we remember open fds for files because me must avoid a single close releasing fcntl locks for other
 *   fds of the same file
 *
 * @returns 0 on success, any other value indicates an error
 */
int ad_open(struct adouble *ad, const char *path, int adflags, ...)
{
    EC_INIT;
    va_list args;
    mode_t mode = 0;

    LOG(log_debug, logtype_default,
        "ad_open(\"%s\", %s): BEGIN [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags),
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);

    if (adflags & ADFLAGS_CHECK_OF)
        /* Checking for open forks requires sharemode lock support (ie RDWR instead of RDONLY) */
        adflags |= ADFLAGS_SETSHRMD;

    if ((ad->ad_vers == AD_VERSION2) && (adflags & ADFLAGS_RF)) {
        adflags |= ADFLAGS_HF;
        if (adflags & ADFLAGS_NORF)
            adflags |= ADFLAGS_NOHF;
    }

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

    if (adflags & ADFLAGS_CHECK_OF) {
        ad->ad_open_forks |= ad_openforks(ad, ad->ad_open_forks);
    }

EC_CLEANUP:
    LOG(log_debug, logtype_default,
        "ad_open(\"%s\"): END: %d [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), ret,
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);

    EC_EXIT;
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

    /* Sanitize flags */
    oflags = (flags & (ADFLAGS_CHECK_OF | ADFLAGS_DIR)) | ADFLAGS_HF | ADFLAGS_RDONLY;    

    if ((ret = ad_open(adp, name, oflags)) < 0 && errno == EACCES) {
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

int ad_refresh(const char *path, struct adouble *ad)
{
    switch (ad->ad_vers) {
    case AD_VERSION2:
        if (ad_meta_fileno(ad) == -1)
            return -1;
        return ad->ad_ops->ad_header_read(NULL, ad, NULL);
        break;
    case AD_VERSION_EA:
#ifdef HAVE_EAFD
        if (AD_META_OPEN(ad)) {
            if (ad_data_fileno(ad) == -1)
                return -1;
            // TODO: read meta EA
        }

        if (AD_RSRC_OPEN(ad)) {
            if (ad_reso_fileno(ad) == -1)
                return -1;
            ssize_t len;
            if ((len = fstat(ad_reso_fileno(ad))) == -1)
                return -1;
            ad->ad_rlen = len;
        }
#else
        if (AD_META_OPEN(ad)) {
            if (ad_data_fileno(ad) == -1)
                return -1;
            // TODO: read meta EA
        }
        if (AD_RSRC_OPEN(ad)) {
            if (ad_reso_fileno(ad) == -1)
                return -1;
            if (ad_header_read_osx(path, ad, NULL) < 0)
                return -1;
        }
#endif
        return ad->ad_ops->ad_header_read(path, ad, NULL);
        break;
    default:
        return -1;
        break;
    }

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
