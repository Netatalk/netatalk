/*
 * Copyright (c) 1999 Adrian Sun (asun@u.washington.edu)
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * Copyright (c) 2010 Frank Lahm
 * Copyright (c) 2026 Andy Lemin (andylemin)
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
 * @brief Opening and creating AppleDouble forks and reading metadata headers
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* On macOS ad_path_osx() resolves to the native APFS named resource fork,
 * not to an AppleDouble sidecar.  Building the sidecar backend there would
 * make it parse and rewrite that named stream as an AppleDouble file; among
 * other corruption risks, mmap()/truncate conversion can panic APFS.
 *
 * This is an extremely unlikely state, but the outcome is catastrophic
 * (kernel panic) so adding a guard out of an abundance of caution. */
#if defined(__APPLE__) && !defined(HAVE_EAFD)
#error "macOS requires full fledged fds for native resource fork handling"
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <atalk/adouble.h>
#include <atalk/compat.h>
#include <atalk/ea.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#define ADEDOFF_MAGIC        (0)
#define ADEDOFF_VERSION      (ADEDOFF_MAGIC + ADEDLEN_MAGIC)
#define ADEDOFF_FILLER       (ADEDOFF_VERSION + ADEDLEN_VERSION)
#define ADEDOFF_NENTRIES     (ADEDOFF_FILLER + ADEDLEN_FILLER)

/*! initial lengths of some of the fields */
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
#define ADEDOFF_FINDERI_EA    (AD_HEADER_LEN + ADEID_NUM_EA * AD_ENTRY_LEN)
#define ADEDOFF_COMMENT_EA    (ADEDOFF_FINDERI_EA    + ADEDLEN_FINDERI)
#define ADEDOFF_FILEDATESI_EA (ADEDOFF_COMMENT_EA    + ADEDLEN_COMMENT)
#define ADEDOFF_AFPFILEI_EA   (ADEDOFF_FILEDATESI_EA + ADEDLEN_FILEDATESI)
#define ADEDOFF_PRIVDEV_EA    (ADEDOFF_AFPFILEI_EA   + ADEDLEN_AFPFILEI)
#define ADEDOFF_PRIVINO_EA    (ADEDOFF_PRIVDEV_EA    + ADEDLEN_PRIVDEV)
#define ADEDOFF_PRIVSYN_EA    (ADEDOFF_PRIVINO_EA    + ADEDLEN_PRIVINO)
#define ADEDOFF_PRIVID_EA     (ADEDOFF_PRIVSYN_EA    + ADEDLEN_PRIVSYN)

/*! this is to prevent changing timezones from causing problems with
   localtime volumes. the screw-up is 30 years. we use a delta of 5 years */
#define TIMEWARP_DELTA 157680000

struct entry {
    uint32_t id, offset, len;
};

/* --------------------------- */
static uid_t default_uid = -1;

/* Forward declarations */
static int ad_mkrf(const char *path);
static int ad_header_read(const char *path, struct adouble *ad,
                          const struct stat *hst);
static int ad_header_upgrade(struct adouble *ad, const char *name);

#if defined (HAVE_EAFD) && defined (SOLARIS)
static int ad_mkrf_ea(const char *path);
#endif
static int ad_header_read_ea(const char *path, struct adouble *ad,
                             const struct stat *hst);
static int ad_header_upgrade_ea(struct adouble *ad, const char *name);
off_t ad_reso_size(const char *path, int adflags, struct adouble *ad);
static int ad_mkrf_osx(const char *path);


static struct adouble_fops ad_adouble = {
    &ad_path,
    &ad_mkrf,
    &ad_rebuild_adouble_header_v2,
    &ad_header_read,
    &ad_header_upgrade,
};

static struct adouble_fops ad_adouble_ea = {
#if defined (HAVE_EAFD) && defined (SOLARIS)
    &ad_path_ea,
    &ad_mkrf_ea,
#else
    &ad_path_osx,
    &ad_mkrf_osx,
#endif
    &ad_rebuild_adouble_header_ea,
    &ad_header_read_ea,
    &ad_header_upgrade_ea,
};

static const struct entry entry_order2[ADEID_NUM_V2 + 1] = {
    {ADEID_NAME,        ADEDOFF_NAME_V2,     ADEDLEN_INIT},
    {ADEID_COMMENT,     ADEDOFF_COMMENT_V2,  ADEDLEN_COMMENT},
    {ADEID_FILEDATESI,  ADEDOFF_FILEDATESI,  ADEDLEN_FILEDATESI},
    {ADEID_FINDERI,     ADEDOFF_FINDERI_V2,  ADEDLEN_FINDERI},
    {ADEID_DID,         ADEDOFF_DID,         ADEDLEN_DID},
    {ADEID_AFPFILEI,    ADEDOFF_AFPFILEI,    ADEDLEN_AFPFILEI},
    {ADEID_SHORTNAME,   ADEDOFF_SHORTNAME,   ADEDLEN_INIT},
    {ADEID_PRODOSFILEI, ADEDOFF_PRODOSFILEI, ADEDLEN_PRODOSFILEI},
    {ADEID_PRIVDEV,     ADEDOFF_PRIVDEV,     ADEDLEN_PRIVDEV},
    {ADEID_PRIVINO,     ADEDOFF_PRIVINO,     ADEDLEN_PRIVINO},
    {ADEID_PRIVSYN,     ADEDOFF_PRIVSYN,     ADEDLEN_PRIVSYN},
    {ADEID_PRIVID,      ADEDOFF_PRIVID,      ADEDLEN_PRIVID},
    {ADEID_RFORK,       ADEDOFF_RFORK_V2,    ADEDLEN_INIT},
    {0, 0, 0}
};

/* Using Extended Attributes */
static const struct entry entry_order_ea[ADEID_NUM_EA + 1] = {
    {ADEID_FINDERI,    ADEDOFF_FINDERI_EA,    ADEDLEN_FINDERI},
    {ADEID_COMMENT,    ADEDOFF_COMMENT_EA,    ADEDLEN_COMMENT},
    {ADEID_FILEDATESI, ADEDOFF_FILEDATESI_EA, ADEDLEN_FILEDATESI},
    {ADEID_AFPFILEI,   ADEDOFF_AFPFILEI_EA,   ADEDLEN_AFPFILEI},
    {ADEID_PRIVDEV,    ADEDOFF_PRIVDEV_EA,    ADEDLEN_PRIVDEV},
    {ADEID_PRIVINO,    ADEDOFF_PRIVINO_EA,    ADEDLEN_PRIVINO},
    {ADEID_PRIVSYN,    ADEDOFF_PRIVSYN_EA,    ADEDLEN_PRIVSYN},
    {ADEID_PRIVID,     ADEDOFF_PRIVID_EA,     ADEDLEN_PRIVID},
    {0, 0, 0}
};

#define ADFLAGS2LOGSTRBUFSIZ 128
/*!
 * @brief Render an ADFLAGS_* mask as text, for log output
 *
 * @param[in] adflags  AppleDouble open flags
 * @returns a static buffer with the names of the set flags joined by "|"
 */
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
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "RF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_NORF) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "NORF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_NOHDR) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "NOHDR", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_NORLEN) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "NORLEN", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_HF) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "HF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_NOHF) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "NOHF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_DIR) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "DIR", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_CHECK_OF) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "OF", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_SETSHRMD) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "SHRMD", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_RDWR) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "O_RDWR", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_RDONLY) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "O_RDONLY", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_RDONLY_STRICT) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "RDONLY_STRICT", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_CREATE) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "O_CREAT", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_EXCL) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "O_EXCL", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (adflags & ADFLAGS_TRUNC) {
        if (!first) {
            strlcat(buf, "|", ADFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "O_TRUNC", ADFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    return buf;
}

#define OPENFLAGS2LOGSTRBUFSIZ 128
/*!
 * @brief Render open(2) flags as text, for log output
 *
 * Covers O_RDONLY, O_RDWR, O_CREAT, O_TRUNC, and O_EXCL.
 *
 * @param[in] oflags  open flags
 * @returns a static buffer with the names of the set flags joined by "|"
 */
const char *openflags2logstr(int oflags)
{
    int first = 1;
    static char buf[OPENFLAGS2LOGSTRBUFSIZ];
    buf[0] = 0;

    if ((oflags & O_RDONLY) || (oflags == O_RDONLY)) {
        strlcat(buf, "O_RDONLY", OPENFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (oflags & O_RDWR) {
        if (!first) {
            strlcat(buf, "|", OPENFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "O_RDWR", OPENFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (oflags & O_CREAT) {
        if (!first) {
            strlcat(buf, "|", OPENFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "O_CREAT", OPENFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (oflags & O_TRUNC) {
        if (!first) {
            strlcat(buf, "|", OPENFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "O_TRUNC", OPENFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    if (oflags & O_EXCL) {
        if (!first) {
            strlcat(buf, "|", OPENFLAGS2LOGSTRBUFSIZ);
        }

        strlcat(buf, "O_EXCL", OPENFLAGS2LOGSTRBUFSIZ);
        first = 0;
    }

    return buf;
}

/*!
 * @brief Map an on-disk entry ID to the in-memory entry index
 *
 * IDs up to 15 are the standard AppleDouble entries and pass through. The
 * netatalk private IDs AD_DEV, AD_INO, AD_SYN, and AD_ID map to ADEID_PRIVDEV,
 * ADEID_PRIVINO, ADEID_PRIVSYN, and ADEID_PRIVID.
 *
 * @param[in] eid  entry ID as stored on disk
 * @returns the entry index, or 0 for an unknown ID
 */
static uint32_t get_eid(uint32_t eid)
{
    if (eid <= 15) {
        return eid;
    }

    if (eid == AD_DEV) {
        return ADEID_PRIVDEV;
    }

    if (eid == AD_INO) {
        return ADEID_PRIVINO;
    }

    if (eid == AD_SYN) {
        return ADEID_PRIVSYN;
    }

    if (eid == AD_ID) {
        return ADEID_PRIVID;
    }

    return 0;
}


/*!
 * @brief Initialize the entry table of an AppleDouble for its version
 *
 * ad_init_offsets leaves a header that already carries AD_MAGIC alone.
 * Otherwise it sets the magic, the version, and the valid data length. The
 * entries then take the offsets and lengths of the version 2 or EA layout.
 * Without HAVE_EAFD the EA form also gets the macOS resource fork offset.
 *
 * @param[in,out] ad  AppleDouble with ad_vers set
 * @returns 0, or -1 for a version other than AD_VERSION2 or AD_VERSION_EA
 */
int ad_init_offsets(struct adouble *ad)
{
    const struct entry  *eid;

    if (ad->ad_magic == AD_MAGIC) {
        return 0;
    }

    ad->ad_magic = AD_MAGIC;
    ad->ad_version = ad->ad_vers & 0x0f0000;

    if (!ad->ad_version) {
        ad->ad_version = AD_VERSION;
    }

    memset(ad->ad_data, 0, sizeof(ad->ad_data));

    if (ad->ad_vers == AD_VERSION2) {
        eid = entry_order2;
        ad->valid_data_len = AD_DATASZ2;
    } else if (ad->ad_vers == AD_VERSION_EA) {
        eid = entry_order_ea;
        ad->valid_data_len = AD_DATASZ_EA;
    } else {
        return -1;
    }

    while (eid->id) {
        ad->ad_eid[eid->id].ade_off = eid->offset;
        ad->ad_eid[eid->id].ade_len = eid->len;
        eid++;
    }

    /*
     * Ensure the resource fork offset is always set
     */
#ifndef HAVE_EAFD

    if (ad->ad_vers == AD_VERSION_EA) {
        ad_setentryoff(ad, ADEID_RFORK, ADEDOFF_RFORK_OSX);
    }

#endif
    return 0;
}

/*!
 * @brief Fill a fresh AppleDouble header for a path
 *
 * Initializes the entry table and marks a dot file invisible when the volume
 * sets ADVOL_INVDOTS, the directory itself excepted. The create, modify, and
 * access dates take the file's mtime, the backup date AD_DATE_START.
 *
 * @param[in,out] ad       AppleDouble with ad_vers and ad_options set
 * @param[in]     path     path of the file, for the dot check and for lstat
 * @param[in]     stp      stat of the file, or NULL to lstat path here
 * @param[in]     adflags  ADFLAGS_DIR when path is a directory
 * @returns 0, or -1 when the version is unknown, FinderInfo cannot be
 *          addressed, or the stat fails
 */
static int new_ad_header(struct adouble *ad, const char *path, struct stat *stp,
                         int adflags)
{
    const struct entry  *eid _U_;
    uint16_t            ashort;
    struct stat         st;
    char                *adp = NULL;
    LOG(log_debug, logtype_ad, "new_ad_header(\"%s\")", path);

    if (ad_init_offsets(ad) != 0) {
        return -1;
    }

    if (ad->valid_data_len == 0) {
        ad->valid_data_len = ad->ad_vers == AD_VERSION_EA ? AD_DATASZ_EA : AD_DATASZ2;
    }

    adp = ad_entry(ad, ADEID_FINDERI);

    if (adp == NULL) {
        LOG(log_debug, logtype_ad, "new_ad_header(\"%s\"): invalid FinderInfo", path);
        return -1;
    }

    /* make things invisible */
    if ((ad->ad_options & ADVOL_INVDOTS)
            && (*path == '.')
            && !((adflags & ADFLAGS_DIR) && (path[1] == 0))
       ) {
        ashort = htons(ATTRBIT_INVISIBLE);
        ad_setattr(ad, ashort);
        ashort = htons(FINDERINFO_INVISIBLE);
        memcpy(adp + FINDERINFO_FRFLAGOFF, &ashort, sizeof(ashort));
    }

    /* put something sane in the date fields */
    if (stp == NULL) {
        stp = &st;

        if (lstat(path, &st) != 0) {
            ad->valid_data_len = 0;
            return -1;
        }
    }

    ad_setdate(ad, AD_DATE_CREATE | AD_DATE_UNIX, stp->st_mtime);
    ad_setdate(ad, AD_DATE_MODIFY | AD_DATE_UNIX, stp->st_mtime);
    ad_setdate(ad, AD_DATE_ACCESS | AD_DATE_UNIX, stp->st_mtime);
    ad_setdate(ad, AD_DATE_BACKUP, AD_DATE_START);
    return 0;
}

/*!
 * @brief Parse the entry descriptors of an AppleDouble header into the entry
 * table
 *
 * Every entry except the resource fork must lie inside valid_data_len.
 *
 * @param[in,out] ad              AppleDouble whose ad_data holds the header
 * @param[in]     nentries        number of entry descriptors in the header
 * @param[in]     valid_data_len  bytes of header data present in ad_data
 * @returns 0, or -1 for a malformed entry
 */
static int parse_entries(struct adouble *ad, uint16_t nentries,
                         size_t valid_data_len)
{
    uint32_t   eid, len, off;
    uint8_t *buf = (uint8_t *) ad->ad_data + AD_HEADER_LEN;

    /* now, read in the entry bits */
    for (; nentries > 0; nentries--) {
        memcpy(&eid, buf, sizeof(eid));
        eid = get_eid(ntohl(eid));
        buf += sizeof(eid);
        memcpy(&off, buf, sizeof(off));
        off = ntohl(off);
        buf += sizeof(off);
        memcpy(&len, buf, sizeof(len));
        len = ntohl(len);
        buf += sizeof(len);

        if (!eid
                || eid > ADEID_MAX
                || ((eid != ADEID_RFORK)
                    && (off > valid_data_len
                        || (size_t)len > valid_data_len - (size_t)off))) {
            LOG(log_warning, logtype_ad, "parse_entries: bogus eid: %u, off: %u, len: %u",
                (uint)eid, (uint)off, (uint)len);
            return -1;
        }

        ad->ad_eid[eid].ade_off = off;
        ad->ad_eid[eid].ade_len = len;
    }

    ad->valid_data_len = valid_data_len;
    return 0;
}

/*!
 * @brief Read and check the header of a version 2 AppleDouble file
 *
 * Reads up to AD_DATASZ2 bytes from the metadata file and checks the magic, the
 * version, and the entry count. It then parses the entries and requires a
 * resource fork entry whose offset lies inside the buffer. The resource fork
 * sits at the end of the file: its length is the file size minus its offset.
 *
 * @param[in]     path  path of the data file, for log messages
 * @param[in,out] ad    AppleDouble whose metadata descriptor is open
 * @param[in]     hst   stat of the metadata file, or NULL to fstat it here
 * @returns 0 on success, -1 with errno EIO for a malformed header, or 1 when
 *          the stat fails
 */
static int ad_header_read(const char *path, struct adouble *ad,
                          const struct stat *hst)
{
    char                *buf = ad->ad_data;
    uint16_t            nentries;
    ssize_t             header_len;
    struct stat         st;

    /* read the header */
    if ((header_len = adf_pread(ad->ad_mdp, buf, AD_DATASZ2, 0)) < 0) {
        return -1;
    }

    if (header_len < AD_HEADER_LEN) {
        errno = EIO;
        return -1;
    }

    memcpy(&ad->ad_magic, buf, sizeof(ad->ad_magic));
    memcpy(&ad->ad_version, buf + ADEDOFF_VERSION, sizeof(ad->ad_version));
    ad->ad_magic = ntohl(ad->ad_magic);
    ad->ad_version = ntohl(ad->ad_version);

    if ((ad->ad_magic != AD_MAGIC) || (ad->ad_version != AD_VERSION2)) {
        LOG(log_error, logtype_ad, "ad_open: can't parse AppleDouble header.");
        errno = EIO;
        return -1;
    }

    memcpy(&nentries, buf + ADEDOFF_NENTRIES, sizeof(nentries));
    nentries = ntohs(nentries);

    if (nentries > 16) {
        LOG(log_error, logtype_ad, "ad_open: too many entries: %"PRIu16, nentries);
        errno = EIO;
        return -1;
    }

    if ((nentries * AD_ENTRY_LEN) + AD_HEADER_LEN > header_len) {
        LOG(log_error, logtype_ad, "ad_header_read: too many entries: %zd", header_len);
        errno = EIO;
        return -1;
    }

    if (parse_entries(ad, nentries, header_len) != 0) {
        LOG(log_warning, logtype_ad, "ad_header_read(%s): malformed AppleDouble",
            path ? fullpathname(path) : "");
        errno = EIO;
        return -1;
    }

    if (!ad_getentryoff(ad, ADEID_RFORK)
            || (ad_getentryoff(ad, ADEID_RFORK) > sizeof(ad->ad_data))
       ) {
        LOG(log_error, logtype_ad, "ad_header_read: problem with rfork entry offset.");
        errno = EIO;
        return -1;
    }

    if (ad_getentryoff(ad, ADEID_RFORK) > header_len) {
        LOG(log_error, logtype_ad, "ad_header_read: can't read in entries.");
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

/*!
 * @brief Check whether a ._ file carries a netatalk-written AppleDouble header
 *
 * Reads the header, checks the magic and the version, and requires the netatalk
 * filler string. A ._ file that macOS itself wrote fails the check: it is not
 * one of ours, so enumeration and access treat it as plain data.
 *
 * @param[in] path  path of the ._ file
 * @returns 0 for a netatalk header, 1 otherwise, including when the file cannot
 *          be read
 */
int ad_valid_header_osx(const char *path)
{
    EC_INIT;
    int fd = -1;
    struct adouble      adosx;
    char                *buf = &adosx.ad_data[0];
    ssize_t             header_len;
    LOG(log_debug, logtype_ad, "ad_valid_header_osx(\"%s\"): BEGIN",
        fullpathname(path));
    EC_NEG1(fd = open(path, O_RDONLY));
    /* read the header */
    EC_NEG1(header_len = read(fd, buf, AD_DATASZ_OSX));

    if (header_len < AD_HEADER_LEN) {
        EC_FAIL;
    }

    memcpy(&adosx.ad_magic, buf, sizeof(adosx.ad_magic));
    memcpy(&adosx.ad_version, buf + ADEDOFF_VERSION, sizeof(adosx.ad_version));
    adosx.ad_magic = ntohl(adosx.ad_magic);
    adosx.ad_version = ntohl(adosx.ad_version);

    if ((adosx.ad_magic != AD_MAGIC) || (adosx.ad_version != AD_VERSION2)) {
        LOG(log_warning, logtype_ad,
            "ad_valid_header_osx(\"%s\"): not an adouble:osx file", fullpathname(path));
        EC_FAIL;
    }

    if (strncmp(buf + ADEDOFF_FILLER,
                AD_FILLER_NETATALK,
                strlen(AD_FILLER_NETATALK)) != 0)
        /*
         * It's a split fork created by OS X, it's not our "own" ._ file
         * and thus not a valid header in this context.
         * We allow enumeration and access.
         */
    {
        EC_FAIL;
    }

EC_CLEANUP:
    LOG(log_debug, logtype_ad, "ad_valid_header_osx(\"%s\"): END: %d",
        fullpathname(path), ret);

    if (fd != -1) {
        close(fd);
    }

    if (ret != 0) {
        return 1;
    }

    return 0;
}

/*!
 * @brief Read an exact byte range from an AppleDouble fork
 *
 * Retries interrupted and short reads. Reaching EOF before @p count bytes
 * have been read is reported as an I/O error.
 *
 * @param[in] adf      open AppleDouble fork
 * @param[out] buf     destination buffer
 * @param[in] count    number of bytes to read
 * @param[in] offset   starting file offset
 * @returns 0 on success, -1 on error with errno set
 */
static int ad_convert_pread_full(struct ad_fd *adf, void *buf, size_t count,
                                 off_t offset)
{
    size_t done = 0;

    while (done < count) {
        ssize_t cc = adf_pread(adf, (char *)buf + done, count - done,
                               offset + (off_t)done);

        if (cc < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (cc == 0) {
            errno = EIO;
            return -1;
        }

        done += (size_t)cc;
    }

    return 0;
}

/*!
 * @brief Write an exact byte range to an AppleDouble fork
 *
 * Retries interrupted and short writes. A zero-length write before @p count
 * bytes have been written is reported as an I/O error.
 *
 * @param[in] adf      open AppleDouble fork
 * @param[in] buf      source buffer
 * @param[in] count    number of bytes to write
 * @param[in] offset   starting file offset
 * @returns 0 on success, -1 on error with errno set
 */
static int ad_convert_pwrite_full(struct ad_fd *adf, const void *buf,
                                  size_t count, off_t offset)
{
    size_t done = 0;

    while (done < count) {
        ssize_t cc = adf_pwrite(adf, (const char *)buf + done, count - done,
                                offset + (off_t)done);

        if (cc < 0) {
            if (errno == EINTR) {
                continue;
            }

            return -1;
        }

        if (cc == 0) {
            errno = EIO;
            return -1;
        }

        done += (size_t)cc;
    }

    return 0;
}

/*!
 * @brief Move a resource fork towards the start of its AppleDouble file
 *
 * The caller must validate both ranges and ensure @p src is not before
 * @p dst. Copying proceeds forwards in bounded chunks so overlapping ranges
 * are safe when moving data towards the start of the file.
 *
 * @param[in] adf      open AppleDouble fork to modify
 * @param[in] src      source range offset
 * @param[in] dst      destination range offset
 * @param[in] len      number of bytes to move
 * @returns 0 on success, -1 on read or write error with errno set
 */
static int ad_convert_move_rfork(struct ad_fd *adf, off_t src, off_t dst,
                                 off_t len)
{
    char buf[8192];

    while (len > 0) {
        size_t count = len > (off_t)sizeof(buf) ? sizeof(buf) : (size_t)len;

        if (ad_convert_pread_full(adf, buf, count, src) != 0
                || ad_convert_pwrite_full(adf, buf, count, dst) != 0) {
            return -1;
        }

        src += (off_t)count;
        dst += (off_t)count;
        len -= (off_t)count;
    }

    return 0;
}

/*!
 * @brief Convert a macOS ._ AppleDouble file to the netatalk layout
 *
 * A macOS FinderInfo entry longer than 32 bytes carries packed xattrs, which
 * netatalk cannot use. The conversion discards them and moves the resource fork
 * to its canonical offset. Entry offsets and lengths come from the file, so
 * ad_convert_osx validates them and refuses a malformed sidecar instead of
 * rewriting it. ad_open() runs during the conversion and could recurse, so a
 * static flag makes a nested call a no-op.
 *
 * @param[in]     path  path of the ._ file, for log messages
 * @param[in,out] ad    AppleDouble whose entries describe the file as read
 * @param[in]     hst   stat of the open sidecar the header came from
 * @returns -1 on error, 0 when nothing needed converting, 1 after converting
 */
static int ad_convert_osx(const char *path, struct adouble *ad,
                          const struct stat *hst)
{
    EC_INIT;
    static bool in_conversion = false;
    char header[AD_DATASZ_OSX];
    uint32_t finderlen = (uint32_t)ad_getentrylen(ad, ADEID_FINDERI);
    off_t roff, doff, rlen;
    int headerlen;

    if (in_conversion || finderlen == ADEDLEN_FINDERI) {
        return 0;
    }

    in_conversion = true;
    /* Entry offsets and lengths are attacker controlled. The header
     * buffer only holds the first AD_DATASZ_OSX bytes of the sidecar
     * (valid_data_len tracks the whole file size), so ad_entry() alone
     * does not prove that FinderInfo lies within the bytes actually
     * read. Require the canonical, in-buffer position; together with
     * ad_header_read_osx() requiring a complete AD_DATASZ_OSX read, this
     * proves that the first ADEDLEN_FINDERI bytes of FinderInfo were read. */
    _Static_assert(ADEDOFF_FINDERI_OSX + ADEDLEN_FINDERI <= AD_DATASZ_OSX,
                   "FinderInfo must be inside the OS X header buffer");

    if ((uint32_t)ad_getentryoff(ad, ADEID_FINDERI) != ADEDOFF_FINDERI_OSX) {
        LOG(log_error, logtype_ad,
            "ad_convert_osx(%s): malformed FinderInfo entry, refusing conversion",
            path ? fullpathname(path) : "");
        errno = EIO;
        EC_FAIL;
    }

    /* Apple packs xattrs into FinderInfo entries longer than 32 bytes;
     * anything shorter than the canonical 32 is malformed. */
    if (finderlen < ADEDLEN_FINDERI) {
        LOG(log_error, logtype_ad,
            "ad_convert_osx(%s): FinderInfo length %" PRIu32 " too short, "
            "refusing conversion",
            path ? fullpathname(path) : "", finderlen);
        errno = EIO;
        EC_FAIL;
    }

    LOG(log_debug, logtype_ad,
        "Converting OS X AppleDouble %s, FinderInfo length: %" PRIu32,
        path ? fullpathname(path) : "", finderlen);

    /* Reject sidecars too small to hold a complete OS X header before
     * modifying them. Use the caller's fstat() result in hst, which
     * describes the open sidecar the header was read from. Do not restat:
     * the size we validate must describe the same file accessed below. */
    if (hst->st_size < AD_DATASZ_OSX) {
        errno = EIO;
        EC_FAIL;
    }

    /* Widen the on-disk uint32 offset/length pair explicitly. Reading
     * them through the ssize_t accessors would make values >= 0x80000000
     * negative on 32-bit builds and defeat the bounds checks below.
     * parse_entries() exempts ADEID_RFORK from its bounds check, so both
     * values are still exactly what the attacker wrote. */
    roff = (off_t)(uint32_t)ad_getentryoff(ad, ADEID_RFORK);
    rlen = (off_t)(uint32_t)ad_getentrylen(ad, ADEID_RFORK);
    doff = ADEDOFF_RFORK_OSX;

    /* The fork must not sit before its canonical destination, or the move
     * below would overwrite the header and the rebuilt entries. Reject
     * instead of clamping: rewriting a malformed sidecar would be
     * destructive. The lower bound also keeps the subtraction below from
     * going negative. */
    if (roff < doff) {
        LOG(log_error, logtype_ad,
            "ad_convert_osx(%s): resource fork entry starts before its "
            "canonical destination (off %" PRIu64 ", destination %" PRIu64 ")",
            path ? fullpathname(path) : "", (uint64_t)roff, (uint64_t)doff);
        errno = EIO;
        EC_FAIL;
    }

    /* Validate the source range [roff, roff + rlen) against the opened
     * sidecar before copying anything. The comparison is written as a
     * subtraction so an oversized offset cannot wrap the addition. Because
     * roff >= doff, the source proof also proves that the destination range
     * [doff, doff + rlen) fits; no second, unreachable branch is needed.
     *
     * For a well-formed sidecar the fork runs to end of file, i.e.
     * roff + rlen == st_size, so validating the declared length here is
     * equivalent to the old st_size - roff derivation. Oversized declared
     * ranges are rejected rather than silently truncated. */
    if (roff > hst->st_size || rlen > hst->st_size - roff) {
        LOG(log_error, logtype_ad,
            "ad_convert_osx(%s): resource fork entry out of bounds "
            "(off %" PRIu64 ", len %" PRIu64 ", file size %" PRId64 ")",
            path ? fullpathname(path) : "",
            (uint64_t)roff, (uint64_t)rlen, (int64_t)hst->st_size);
        errno = EIO;
        EC_FAIL;
    }

    /* roff >= doff, so the validated source end also proves that the
     * destination end fits. Use bounded positioned I/O instead of mmap():
     * a concurrent truncate becomes a short read and EIO rather than
     * SIGBUS, and a sparse sidecar never consumes a file-sized mapping. */
    if (ad_convert_move_rfork(ad->ad_rfp, roff, doff, rlen) != 0) {
        LOG(log_error, logtype_ad, "relocate AppleDouble resource fork: %s",
            strerror(errno));
        EC_FAIL;
    }

    ad_setentrylen(ad, ADEID_FINDERI, ADEDLEN_FINDERI);
    ad->ad_rlen = rlen;
    ad_setentrylen(ad, ADEID_RFORK, rlen);
    ad_setentryoff(ad, ADEID_RFORK, doff);
    headerlen = ad_rebuild_adouble_header_osx(ad, header);

    if (ftruncate(ad_reso_fileno(ad), doff + rlen) != 0) {
        LOG(log_error, logtype_ad, "ftruncate AppleDouble: %s", strerror(errno));
        EC_FAIL;
    }

    if (ad_convert_pwrite_full(ad->ad_rfp, header, (size_t)headerlen, 0) != 0) {
        LOG(log_error, logtype_ad, "rewrite AppleDouble header: %s",
            strerror(errno));
        EC_FAIL;
    }

    /* Create a metadata EA if one doesn't exit. path can be NULL when
     * reached through ad_refresh(); also, the data fork name is derived
     * from the sidecar path below, which only works for the afpd-style
     * "._<name>" form. */
    if (path == NULL || strlen(path) < 3) {
        EC_EXIT_STATUS(0);
    }

    struct adouble adea;

    ad_init_old(&adea, AD_VERSION_EA, ad->ad_options);

    if (ad_init_offsets(&adea) != 0) {
        LOG(log_error, logtype_ad, "ad_init_offsets failed");
        EC_FAIL;
    }

    if (ad_open(&adea, path + 2, ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0666) < 0) {
        LOG(log_error, logtype_ad, "create metadata: %s\n", strerror(errno));
        EC_FAIL;
    }

    if (adea.ad_mdp->adf_flags & O_CREAT) {
        if (ad_entry(ad, ADEID_FINDERI)) {
            memcpy(ad_entry(&adea, ADEID_FINDERI),
                   ad_entry(ad, ADEID_FINDERI),
                   ADEDLEN_FINDERI);
        } else {
            LOG(log_debug, logtype_ad, "ad_convert_osx(%s): invalid FinderInfo",
                path ? fullpathname(path) : "");
        }

        ad_flush(&adea);
    }

    ad_close(&adea, ADFLAGS_HF);
EC_CLEANUP:
    in_conversion = false;

    if (ret != 0) {
        return -1;
    }

    return 1;
}

/*!
 * @brief Length of a resource fork past the header of its ._ file
 *
 * @param[in] st  stat of the file
 * @returns the bytes past the header, 0 for a file of the header's length
 *          or shorter
 */
static off_t rlen_osx(const struct stat *st)
{
    return st->st_size > ADEDOFF_RFORK_OSX ? st->st_size - ADEDOFF_RFORK_OSX
           : 0;
}

/*!
 * @brief Read the header of a macOS ._ file and take its resource fork
 *
 * Only the resource fork comes from the ._ file. The FinderInfo lives in the
 * file's extended attribute. A FinderInfo entry of the wrong length means a
 * macOS-written file, which ad_convert_osx() rewrites once before a second
 * read. The resource fork entry must sit at ADEDOFF_RFORK_OSX.
 *
 * @param[in]     path  path of the data file, for log messages
 * @param[in,out] ad    AppleDouble whose resource fork descriptor is open
 * @param[in]     hst   stat of the ._ file, or NULL to fstat it here
 * @returns 0 on success, -1 with errno EIO for a malformed or unconvertible
 *          header, or the errno of fstat or pread
 */
static int ad_header_read_osx(const char *path, struct adouble *ad,
                              struct stat *hst)
{
    EC_INIT;
    struct adouble      adosx;
    char                *buf;
    uint16_t            nentries;
    ssize_t             header_len;
    struct stat         st;
    int                 retry_read = 0;
reread:

    if (hst == NULL) {
        hst = &st;
        EC_NEG1(fstat(ad_reso_fileno(ad), hst));
    }

    LOG(log_debug, logtype_ad, "ad_header_read_osx: %s",
        path ? fullpathname(path) : "");
    ad_init_old(&adosx, AD_VERSION_EA, ad->ad_options);
    buf = &adosx.ad_data[0];
    memset(buf, 0, sizeof(adosx.ad_data));
    adosx.ad_rfp->adf_fd = ad_reso_fileno(ad);
    /* read the header */
    EC_NEG1(header_len = adf_pread(ad->ad_rfp, buf, AD_DATASZ_OSX, 0));

    if (header_len < AD_DATASZ_OSX) {
        errno = EIO;
        return -1;
    }

    memcpy(&adosx.ad_magic, buf, sizeof(adosx.ad_magic));
    memcpy(&adosx.ad_version, buf + ADEDOFF_VERSION, sizeof(adosx.ad_version));
    adosx.ad_magic = ntohl(adosx.ad_magic);
    adosx.ad_version = ntohl(adosx.ad_version);

    if ((adosx.ad_magic != AD_MAGIC) || (adosx.ad_version != AD_VERSION2)) {
        LOG(log_error, logtype_ad,
            "ad_header_read_osx: can't parse AppleDouble header");
        errno = EIO;
        return -1;
    }

    memcpy(&nentries, buf + ADEDOFF_NENTRIES, sizeof(nentries));
    nentries = ntohs(nentries);

    if (nentries > 16) {
        LOG(log_error, logtype_ad, "ad_header_read_osx: too many entries: %"PRIu16,
            nentries);
        errno = EIO;
        return -1;
    }

    if ((nentries * AD_ENTRY_LEN) + AD_HEADER_LEN > header_len) {
        LOG(log_error, logtype_ad, "ad_header_read_osx: too many entries: %zd",
            header_len);
        errno = EIO;
        return -1;
    }

    if (parse_entries(&adosx, nentries, hst->st_size) != 0) {
        LOG(log_warning, logtype_ad, "ad_header_read(%s): malformed AppleDouble",
            path ? fullpathname(path) : "");
        errno = EIO;
        EC_FAIL;
    }

    if (ad_getentrylen(&adosx, ADEID_FINDERI) != ADEDLEN_FINDERI) {
        LOG(log_warning, logtype_ad, "Convert OS X to Netatalk AppleDouble: %s",
            path ? fullpathname(path) : "");

        if (retry_read > 0) {
            LOG(log_error, logtype_ad, "ad_header_read_osx: %s, giving up",
                path ? fullpathname(path) : "");
            errno = EIO;
            EC_FAIL;
        }

        retry_read++;

        if (ad_convert_osx(path, &adosx, hst) == 1) {
            hst = NULL;
            goto reread;
        }

        errno = EIO;
        EC_FAIL;
    }

    /* the offset ad_read(), ad_write() and the rebuilt header use */
    if (ad_getentryoff(&adosx, ADEID_RFORK) != ADEDOFF_RFORK_OSX) {
        LOG(log_error, logtype_ad,
            "ad_header_read_osx: problem with rfork entry offset.");
        errno = EIO;
        return -1;
    }

    ad_setentryoff(ad, ADEID_RFORK, ADEDOFF_RFORK_OSX);
    ad->ad_rlen = rlen_osx(hst);
EC_CLEANUP:
    EC_EXIT;
}

/*!
 * @brief Read and check the metadata EA of an AppleDouble ea file
 *
 * The EA comes from the metadata descriptor when one is open, otherwise from
 * path. The header must hold AD_DATASZ_EA bytes, the version 2 magic and
 * version, ADEID_NUM_EA entries, and every entry the ea layout defines.
 * Without HAVE_EAFD the function forces the resource fork offset to
 * ADEDOFF_RFORK_OSX. A failed read leaves ad as it was. A malformed EA fails
 * with EINVAL, and the log names the file by its Mac name when there is no
 * path. When ad carries ADFLAGS_CREATE and path is set, the function deletes
 * the malformed EA as root and fails with ENOENT instead, so the caller
 * creates a fresh one.
 *
 * @param[in]     path  file whose metadata EA to read, or NULL
 * @param[in,out] ad    AppleDouble receiving the parsed header
 * @param[in]     hst   unused
 * @returns 0, or -1 with errno set
 */
static int ad_header_read_ea(const char *path, struct adouble *ad,
                             const struct stat *hst _U_)
{
    EC_INIT;
    uint16_t nentries = 0;
    ssize_t  header_len;
    char     *buf = ad->ad_data;
    /* what a failed read leaves in place */
    const struct adouble was = *ad;

    if (ad_meta_fileno(ad) != -1) {
        header_len = sys_fgetxattr(ad_meta_fileno(ad), AD_EA_META, ad->ad_data,
                                   AD_DATASZ_EA);
    } else {
        header_len = sys_getxattr(path, AD_EA_META, ad->ad_data, AD_DATASZ_EA);
    }

    if (header_len < 1) {
        if (header_len == 0) {
            errno = EINVAL;
        }

        LOG(log_debug, logtype_ad, "ad_header_read_ea: %s", strerror(errno));
        EC_FAIL;
    }

    if (header_len < AD_DATASZ_EA) {
        LOG(log_error, logtype_ad, "ad_header_read_ea(\"%s\"): short metadata EA",
            path ? fullpathname(path) : ad->ad_name);
        errno = EINVAL;
        EC_FAIL;
    }

    memcpy(&ad->ad_magic, buf, sizeof(ad->ad_magic));
    memcpy(&ad->ad_version, buf + ADEDOFF_VERSION, sizeof(ad->ad_version));
    ad->ad_magic = ntohl(ad->ad_magic);
    ad->ad_version = ntohl(ad->ad_version);

    if ((ad->ad_magic != AD_MAGIC) || (ad->ad_version != AD_VERSION2)) {
        LOG(log_error, logtype_ad, "ad_header_read_ea(\"%s\"): wrong magic or version",
            path ? fullpathname(path) : ad->ad_name);
        errno = EINVAL;
        EC_FAIL;
    }

    memcpy(&nentries, buf + ADEDOFF_NENTRIES, sizeof(nentries));
    nentries = ntohs(nentries);

    if (nentries != ADEID_NUM_EA) {
        LOG(log_error, logtype_ad,
            "ad_header_read_ea(\"%s\"): invalid number of entries: %d",
            path ? fullpathname(path) : ad->ad_name, nentries);
        errno = EINVAL;
        EC_FAIL;
    }

    /* Now parse entries */
    if (parse_entries(ad, nentries, header_len)) {
        LOG(log_warning, logtype_ad, "ad_header_read(%s): malformed AppleDouble",
            path ? fullpathname(path) : ad->ad_name);
        errno = EINVAL;
        EC_FAIL;
    }

    if (nentries != ADEID_NUM_EA
            || !ad_entry(ad, ADEID_FINDERI)
            || !ad_entry(ad, ADEID_COMMENT)
            || !ad_entry(ad, ADEID_FILEDATESI)
            || !ad_entry(ad, ADEID_AFPFILEI)
            || !ad_entry(ad, ADEID_PRIVDEV)
            || !ad_entry(ad, ADEID_PRIVINO)
            || !ad_entry(ad, ADEID_PRIVSYN)
            || !ad_entry(ad, ADEID_PRIVID)) {
        errno = EINVAL;
        EC_FAIL;
    }

    /*
     * Ensure the resource fork offset is always set
     */
#ifndef HAVE_EAFD

    if (ad->ad_vers == AD_VERSION_EA) {
        ad_setentryoff(ad, ADEID_RFORK, ADEDOFF_RFORK_OSX);
    }

#endif
EC_CLEANUP:

    if (ret != 0) {
        *ad = was;
    }

    /* A read-only caller must see corrupt metadata as an error, rather than
     * silently deleting it and treating the source as having no header.
     * Retain the repair path for callers that explicitly allow creation. */
    if (ret != 0 && errno == EINVAL && path != NULL
            && (ad->ad_adflags & ADFLAGS_CREATE)) {
        become_root();
        (void)sys_removexattr(path, AD_EA_META);
        unbecome_root();
        LOG(log_error, logtype_ad,
            "ad_header_read_ea(%s, %d): deleted invalid metadata EA",
            fullpathname(path), nentries);
        errno = ENOENT;
    }

    EC_EXIT;
}

/*!
 * @brief Create the .AppleDouble directory that holds a version 2 metadata file
 *
 * The directory is the parent of path. For "/dir/.AppleDouble/file" the call
 * creates "/dir/.AppleDouble" through ad_mkdir.
 *
 * @param[in,out] path  metadata file path, truncated at its last slash during
 *                      the mkdir and restored only on success
 * @returns 0, or -1 when path has no slash or ad_mkdir failed
 */
static int ad_mkrf(const char *path)
{
    char *slash;

    /*
     * Probably .AppleDouble doesn't exist, try to mkdir it.
     */
    if (NULL == (slash = strrchr(path, '/'))) {
        return -1;
    }

    *slash = '\0';
    errno = 0;

    if (ad_mkdir(path, 0777) < 0) {
        return -1;
    }

    *slash = '/';
    return 0;
}

#ifdef HAVE_EAFD
static int ad_mkrf_ea(const char *path _U_)
{
    AFP_PANIC("ad_mkrf_ea: don't use");
    return 0;
}
#endif

/*!
 * @brief Create nothing, because the ea layout stores forks without a parent
 * directory
 *
 * @param[in] path  unused
 * @returns 0
 */
static int ad_mkrf_osx(const char *path _U_)
{
    return 0;
}

#define EMULATE_SUIDDIR

/*!
 * @brief Give a freshly created AppleDouble file the owner that the parent
 * directory implies
 *
 * Only a root afpd changes ownership, and it signals that through ad_setfuid.
 * The new owner is the configured default user, or the parent directory owner
 * when the default is zero. The group always follows the parent directory.
 *
 * @param[in] path   file to chown
 * @param[in] stbuf  stat of the parent directory
 * @returns 0, or -1 when lchown failed
 */
static int ad_chown(const char *path, struct stat *stbuf)
{
    int ret = 0;
#ifdef EMULATE_SUIDDIR
    uid_t id;

    if (default_uid != (uid_t) -1) {
        /* we are root (admin) */
        id = default_uid ? default_uid : stbuf->st_uid;
        ret = lchown(path, id, stbuf->st_gid);
    }

#endif
    return ret;
}

#define DEFMASK 07700   /*!< be conservative */

/*!
 * @brief Restrict a creation mode to the permissions of the parent directory
 *
 * @param[in]     path   file whose parent directory supplies the permissions
 * @param[in,out] mode   requested mode, masked with the parent directory mode,
 *                       or with DEFMASK when the stat failed
 * @param[out]    stbuf  receives the parent directory stat
 * @returns 0, or -1 when mode is zero or the parent directory stat failed
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

/*!
 * @brief Upgrade nothing, because the version 2 layout has a single header
 * format
 *
 * @param[in] ad    unused
 * @param[in] name  unused
 * @returns 0
 */
static int ad_header_upgrade(struct adouble *ad _U_, const char *name _U_)
{
    return 0;
}

/*!
 * @brief Abort, because the ea layout never upgrades a header
 *
 * @param[in] ad    unused
 * @param[in] name  unused
 * @returns 0, though AFP_PANIC ends the process first
 */
static int ad_header_upgrade_ea(struct adouble *ad _U_, const char *name _U_)
{
    AFP_PANIC("ad_header_upgrade_ea: don't use");
    return 0;
}

/*!
 * @brief Decide whether a failed metadata open fails the entire ad_open
 *
 * A missing header is no error under ADFLAGS_NOHF, and a read-only
 * ADFLAGS_RF open tolerates ENOENT. Only ENOENT and ENOATTR count as missing,
 * so an unreadable header still fails. Any other failure also closes a data
 * fork that this ad_open opened under ADFLAGS_DF, ADFLAGS_SETSHRMD, or
 * ADFLAGS_CHECK_OF.
 *
 * @param[in,out] ad       AppleDouble whose metadata open failed, errno still
 *                         set
 * @param[in]     adflags  ADFLAGS_* bits of the ad_open call
 * @returns 0 when the flags tolerate the failure, -1 otherwise
 */
static int ad_error(struct adouble *ad, int adflags)
{
    int err _U_ = errno;

    /* (1) ADFLAGS_NOHF: a *missing* metadata/header fork is not an error.  Suppress
     * only the benign "absent" errnos; a hard error (EIO, EACCES, ...) propagates so
     * a caller cannot mistake an unreadable header for "no header". */
    if ((adflags & ADFLAGS_NOHF)
            && (errno == ENOENT || errno == ENOATTR)) {
        return 0;
    }

    if ((adflags & ADFLAGS_RDONLY) && (adflags & ADFLAGS_RF)
            && (errno == ENOENT)) { /* 2 */
        return 0;
    }

    if (adflags & (ADFLAGS_DF | ADFLAGS_SETSHRMD | ADFLAGS_CHECK_OF)) { /* 3 */
        ad_close(ad, ADFLAGS_DF);
        err = errno;
    }

    return -1;
}

/*!
 * @brief Map ADFLAGS to open() flags
 *
 * @param[in] ad       the adouble structure
 * @param[in] adfile   the file you really want to open: ADFLAGS_DF or ADFLAGS_HF
 * @param[in] adflags  flags from ad_open(..., adflags, ...)
 * @returns            mapped flags suitable for calling open()
 */
static int ad2openflags(const struct adouble *ad, int adfile, int adflags)
{
    int oflags = 0;

    if (adflags & ADFLAGS_RDWR) {
        oflags |= O_RDWR;
    }

    if ((adflags & ADFLAGS_RDONLY) && !(adflags & ADFLAGS_RDONLY_STRICT)) {
        if (((adfile == ADFLAGS_DF || adfile == ADFLAGS_RF)
                && (adflags & ADFLAGS_SETSHRMD))
                /* need rw access for locks */
                || ((adfile == ADFLAGS_HF) && (ad->ad_vers == AD_VERSION2)))
            /* need rw access for adouble file for the case:
             1) openfork(data+meta:O_RDONLY), 2) openfork(reso(=meta):O_RDWR) */
        {
            oflags |= O_RDWR;
        } else {
            oflags |= O_RDONLY;
        }
    }

    if (adflags & ADFLAGS_CREATE) {
        oflags |= O_CREAT;
    }

    if (adflags & ADFLAGS_EXCL) {
        oflags |= O_EXCL;
    }

    if (adflags & ADFLAGS_TRUNC) {
        oflags |= O_TRUNC;
    }

    if (!(ad->ad_options & ADVOL_FOLLO_SYML)) {
        oflags |= O_NOFOLLOW;
    }

    /* POSIX open(2) needs an explicit access mode.  Detect a missing one at the
     * adflags level: O_RDONLY is 0, so testing oflags can't tell "no mode" from
     * a real read-only open, but ADFLAGS_RDWR/ADFLAGS_RDONLY are distinct bits.
     * Strict backends (FreeBSD, FUSE) reject access-mode-less flags with EINVAL;
     * default to O_RDONLY. */
    if (!(adflags & (ADFLAGS_RDWR | ADFLAGS_RDONLY))) {
        LOG(log_warning, logtype_ad,
            "ad2openflags: missing access mode (adflags=0x%x adfile=0x%x); "
            "defaulting to O_RDONLY", adflags, adfile);
        /* No-op on the bits (O_RDONLY==0); documents the read-only default. */
        oflags |= O_RDONLY;
    }

    return oflags;
}
#ifdef __APPLE__
int ad_open_native_finderinfo(const char *path, char *ret)
{
    ssize_t size;
    LOG(log_debug, logtype_ad,
        "ad_open_native_finderinfo(\"%s\"): BEGIN",
        fullpathname(path));
    size = sys_getxattr(path, EA_FINFO, 0, 0);

    if (size < 0) {
        LOG(log_debug, logtype_ad,
            "ad_open_native_finderinfo(\"%s\"): Native macOS FinderInfo not found",
            fullpathname(path));
        return 0;
    } else {
        sys_getxattr(path, EA_FINFO, ret, ADEDLEN_FINDERI);
        LOG(log_debug, logtype_ad,
            "ad_open_native_finderinfo(\"%s\"): Native macOS FinderInfo found",
            fullpathname(path));
        return 1;
    }
}
#endif

/*!
 * @brief Open the data fork of an AppleDouble
 *
 * An already open fork gains a reference. A new open maps adflags through
 * ad2openflags and, under ADFLAGS_CREATE, restricts mode to the parent
 * directory permissions unless ADVOL_UNIXPRIV keeps it. When O_NOFOLLOW hits a
 * symlink the fork stores the link target instead of a descriptor and marks it
 * AD_SYMLINK. EACCES, EPERM, or EROFS on a read-only ADFLAGS_SETSHRMD open
 * retries read-only without O_CREAT, O_EXCL, or O_TRUNC. Under ADFLAGS_CREATE,
 * ad_chown then sets the owner of the opened file.
 *
 * @param[in]     path     data file
 * @param[in]     adflags  ADFLAGS_* bits of the ad_open call
 * @param[in]     mode     creation mode under ADFLAGS_CREATE
 * @param[in,out] ad       AppleDouble receiving the data fork descriptor
 * @returns 0, or -1 with errno set
 */
static int ad_open_df(const char *path, int adflags, mode_t mode,
                      struct adouble *ad)
{
    EC_INIT;
    struct stat st_dir;
    int         oflags, oflags_orig;
    mode_t      admode;
    int         st_invalid = -1;
    ssize_t     lsz;
    LOG(log_debug, logtype_ad,
        "ad_open_df(\"%s\", %s): BEGIN [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags),
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);

    if (ad_data_fileno(ad) != -1) {
        /* the file is already open, but we want write access: */
        if ((adflags & ADFLAGS_RDWR)
                /* and it was denied the first time: */
                && (ad->ad_data_fork.adf_flags & O_RDONLY)) {
            errno = EACCES;
            return -1;
        }

        /* it's not new anymore */
        ad->ad_data_fork.adf_flags &= ~(O_TRUNC | O_CREAT);
        ad->ad_data_fork.adf_refcount++;
        goto EC_CLEANUP;
    }

    oflags = ad2openflags(ad, ADFLAGS_DF, adflags);
    oflags_orig = oflags;       /* caller's intent, for the read-only-retry check */
    admode = mode;

    if (adflags & ADFLAGS_CREATE) {
        st_invalid = ad_mode_st(path, &admode, &st_dir);

        if (ad->ad_options & ADVOL_UNIXPRIV) {
            admode = mode;
        }
    }

    ad->ad_data_fork.adf_fd = open(path, oflags, admode);

    if (ad->ad_data_fork.adf_fd == -1) {
        int open_errno = errno;

        /* Symlink emulation (O_NOFOLLOW set, i.e. 'follow symlinks' off): detect
         * the symlink with lstat() rather than by matching errno. The errno for
         * an O_NOFOLLOW symlink open varies by platform (EMLINK/EFTYPE/ELOOP),
         * and DragonflyBSD returns EACCES for an unprivileged caller because its
         * permission check precedes the symlink check. lstat() is unambiguous. */
        if (oflags & O_NOFOLLOW) {
            struct stat lst;

            if (lstat(path, &lst) == 0 && S_ISLNK(lst.st_mode)) {
                EC_NULL(ad->ad_data_fork.adf_syml = malloc(MAXPATHLEN + 1));
                lsz = readlink(path, ad->ad_data_fork.adf_syml, MAXPATHLEN);

                if (lsz <= 0 || lsz > MAXPATHLEN) {
                    free(ad->ad_data_fork.adf_syml);
                    ad->ad_data_fork.adf_syml = NULL;
                    EC_FAIL;
                }

                ad->ad_data_fork.adf_syml[lsz] = 0;
                ad->ad_data_fork.adf_fd = AD_SYMLINK;
                goto symlink_emulated;
            }
        }

        switch (open_errno) {
        case EACCES:
        case EPERM:
        case EROFS:
            if ((adflags & ADFLAGS_SETSHRMD) && (adflags & ADFLAGS_RDONLY)) {
                /* Strict read-only re-open: strip any write-implying or
                 * destructive flag ad2openflags() may have set; only the
                 * access mode and the surviving flags (e.g. O_NOFOLLOW) stay. */
                if (oflags_orig & (O_CREAT | O_EXCL | O_TRUNC)) {
                    LOG(log_warning, logtype_ad,
                        "ad_open_df: SETSHRMD|RDONLY caller passed "
                        "O_CREAT/O_EXCL/O_TRUNC (0x%x); stripped on retry",
                        oflags_orig);
                }

                oflags &= ~(O_RDWR | O_WRONLY | O_CREAT | O_EXCL | O_TRUNC);
                oflags |= O_RDONLY;
                EC_NEG1(ad->ad_data_fork.adf_fd = open(path, oflags, admode));
                break;
            }

            errno = open_errno;
            return -1;

        default:
            errno = open_errno;
            EC_FAIL;
        }
    }

symlink_emulated:

    if (!st_invalid) {
        /* just created, set owner if admin (root) */
        ad_chown(path, &st_dir);
    }

    ad->ad_data_fork.adf_flags = oflags;
    adf_lock_init(&ad->ad_data_fork);
    ad->ad_data_fork.adf_refcount++;
EC_CLEANUP:
    LOG(log_debug, logtype_ad,
        "ad_open_df(\"%s\", %s): END: %d [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags), ret,
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);
    EC_EXIT;
}

/*!
 * @brief Open or create the version 2 metadata file of an AppleDouble
 *
 * A symlink data fork short-circuits with nothing opened. An already open
 * metadata file gains a reference after ad_refresh. Otherwise the call opens
 * the ad_path file without O_CREAT or O_EXCL. EACCES, EPERM, or EROFS on a
 * read-only ADFLAGS_SETSHRMD open retries read-only without O_TRUNC.
 *
 * ENOENT under ADFLAGS_CREATE creates the .AppleDouble directory when needed,
 * retries with O_CREAT and a mode from ad_hf_mode, and lets ad_chown set the
 * owner. A new or empty file receives a fresh header through new_ad_header
 * and ad_flush, except that ADFLAGS_RDONLY_STRICT rejects an empty file with
 * EIO. Any other file goes through ad_header_read. A failure after the open
 * closes the descriptor again.
 *
 * @param[in]     path     data file whose metadata file to open
 * @param[in]     adflags  ADFLAGS_* bits of the ad_open call
 * @param[in]     mode     creation mode for a new metadata file
 * @param[in,out] ad       AppleDouble receiving the metadata descriptor and
 *                         header
 * @returns 0, or -1 with errno set
 */
static int ad_open_hf_v2(const char *path, int adflags, mode_t mode,
                         struct adouble *ad)
{
    EC_INIT;
    struct stat st_dir;
    struct stat st_meta;
    struct stat *pst = NULL;
    const char  *ad_p;
    int         oflags, nocreatflags, opened = 0;
    mode_t      admode;
    int         st_invalid = -1;
    LOG(log_debug, logtype_ad,
        "ad_open_hf_v2(\"%s\", %s): BEGIN [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags),
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);

    /* A symlink (data fd == AD_SYMLINK, set by ad_open_df) has no .AppleDouble
     * sidecar to open; short-circuit as ad_open_hf_ea() does. */
    if (ad_data_fileno(ad) == AD_SYMLINK) {
        goto EC_CLEANUP;
    }

    if (ad_meta_fileno(ad) != -1) {
        /* the file is already open, but we want write access: */
        if ((adflags & ADFLAGS_RDWR) &&
                /* and it was already denied: */
                (ad->ad_mdp->adf_flags & O_RDONLY)) {
            errno = EACCES;
            EC_FAIL;
        }

        ad_refresh(path, ad);
        /* it's not new anymore */
        ad->ad_mdp->adf_flags &= ~(O_TRUNC | O_CREAT);
        ad->ad_mdp->adf_refcount++;
        goto EC_CLEANUP;
    }

    ad_p = ad->ad_ops->ad_path(path, adflags);
    oflags = ad2openflags(ad, ADFLAGS_HF, adflags);
    LOG(log_debug, logtype_ad, "ad_open_hf_v2(\"%s\"): open flags: %s",
        fullpathname(path), openflags2logstr(oflags));
    nocreatflags = oflags & ~(O_CREAT | O_EXCL);
    ad_meta_fileno(ad) = open(ad_p, nocreatflags);

    if (ad_meta_fileno(ad) != -1) {
        ad->ad_mdp->adf_flags = nocreatflags;
    } else {
        switch (errno) {
        case EACCES:
        case EPERM:
        case EROFS:
            if ((adflags & ADFLAGS_RDONLY) && (adflags & ADFLAGS_SETSHRMD)) {
                /* Strict read-only re-open: also drop O_TRUNC so a downgrade
                 * retry can never truncate the metadata file.  O_CREAT/O_EXCL
                 * are already absent from nocreatflags (stripped at its
                 * construction).  Mirrors ad_open_df()'s retry mask. */
                nocreatflags &= ~(O_RDWR | O_TRUNC);
                nocreatflags |= O_RDONLY;
                EC_NEG1(ad_meta_fileno(ad) = open(ad_p, nocreatflags));
                ad->ad_mdp->adf_flags = nocreatflags;
                break;
            }

            EC_FAIL;

        case ENOENT:
            if (!(oflags & O_CREAT)) {
                EC_FAIL;
            }

            /*
             * We're expecting to create a new adouble header file here
             */
            LOG(log_debug, logtype_ad, "ad_open(\"%s\"): creating adouble file",
                fullpathname(path));
            admode = mode;
            errno = 0;
            st_invalid = ad_mode_st(ad_p, &admode, &st_dir);

            if (ad->ad_options & ADVOL_UNIXPRIV) {
                admode = mode;
            }

            admode = ad_hf_mode(admode);

            if (errno == ENOENT) {
                EC_NEG1_LOG(ad->ad_ops->ad_mkrf(ad_p));
                admode = mode;
                st_invalid = ad_mode_st(ad_p, &admode, &st_dir);

                if (ad->ad_options & ADVOL_UNIXPRIV) {
                    admode = mode;
                }

                admode = ad_hf_mode(admode);
            }

            /* retry with O_CREAT */
            EC_NEG1(ad_meta_fileno(ad) = open(ad_p, oflags, admode));
            ad->ad_mdp->adf_flags = oflags;

            /* just created, set owner if admin owner (root) */
            if (!st_invalid) {
                ad_chown(ad_p, &st_dir);
            }

            break;

        default:
            EC_FAIL;
        }
    }

    /* Now we've got a new opened fd, we need to check that in the error case */
    opened = 1;

    if (!(ad->ad_mdp->adf_flags & O_CREAT)) {
        /* check for 0 length files, treat them as new. */
        if (fstat(ad->ad_mdp->adf_fd, &st_meta) == 0) {
            if (st_meta.st_size == 0) {
                if (adflags & ADFLAGS_RDONLY_STRICT) {
                    errno = EIO;
                    EC_FAIL;
                }

                ad->ad_mdp->adf_flags |= O_TRUNC;
            } else
                /* we have valid data in st_meta stat structure, reused it in ad_header_read */
            {
                pst = &st_meta;
            }
        }
    }

    adf_lock_init(ad->ad_mdp);
    ad->ad_mdp->adf_refcount = 1;

    if (ad->ad_mdp->adf_flags & (O_TRUNC | O_CREAT)) {
        /* This is a new adouble header file, create it */
        EC_NEG1_LOG(new_ad_header(ad, path, pst, adflags));
        ad_flush(ad);
    } else {
        /* Read the adouble header in and parse it.*/
        EC_NEG1_LOG(ad->ad_ops->ad_header_read(path, ad, pst));
    }

EC_CLEANUP:

    if (ret != 0 && opened && ad_meta_fileno(ad) != -1) {
        close(ad_meta_fileno(ad));
        ad_meta_fileno(ad) = -1;
        ad->ad_mdp->adf_refcount = 0;
    }

    LOG(log_debug, logtype_ad,
        "ad_open_hf_v2(\"%s\", %s): END: %d [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags), ret,
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);
    EC_EXIT;
}

/*!
 * @brief Open or create the metadata EA of an AppleDouble ea file
 *
 * A symlink short-circuits with nothing opened. An already open metadata
 * descriptor gains a reference. A write request on a closed descriptor opens
 * the file itself, read-only for a directory so that fchdir still works. A
 * read-only request reads the EA by path and opens nothing.
 *
 * A missing EA fails with ENOENT unless ADFLAGS_CREATE allows creating one.
 * Creation on an ADVOL_RO volume fails with EROFS. new_ad_header and ad_flush
 * write the fresh header, and the descriptor flags gain O_CREAT. On macOS the
 * native FinderInfo EA overwrites the FinderInfo entry. ad_rlen receives the
 * resource fork size unless ADFLAGS_NORLEN says the caller's resource fork
 * open has just measured it, and a failure after the open closes the
 * descriptor again.
 *
 * @param[in]     path     file whose metadata EA to open
 * @param[in]     adflags  ADFLAGS_* bits of the ad_open call
 * @param[in]     mode     unused
 * @param[in,out] ad       AppleDouble receiving the metadata descriptor and
 *                         header
 * @returns 0, or -1 with errno set
 */
static int ad_open_hf_ea(const char *path, int adflags, int mode _U_,
                         struct adouble *ad)
{
    EC_INIT;
    int oflags;
    int opened = 0;
    LOG(log_debug, logtype_ad,
        "ad_open_hf_ea(\"%s\", %s): BEGIN [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags),
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);
    oflags = ad2openflags(ad, ADFLAGS_DF, adflags) & ~(O_CREAT | O_TRUNC);

    if (ad_meta_fileno(ad) == AD_SYMLINK) {
        goto EC_CLEANUP;
    }

    if (ad_meta_fileno(ad) != -1) {
        /* the file is already open, but we want write access: */
        if ((oflags & O_RDWR) &&
                /* and it was already denied: */
                (ad->ad_mdp->adf_flags & O_RDONLY)) {
            LOG(log_error, logtype_ad, "ad_open_hf_ea(%s): rw request for ro file: %s",
                fullpathname(path), strerror(errno));
            errno = EACCES;
            EC_FAIL;
        }

        /* it's not new anymore */
        ad->ad_mdp->adf_flags &= ~(O_TRUNC | O_CREAT);
    } else {
        if (oflags & O_RDWR) {
            /* Fo a RDONLY adouble we just use sys_lgetxattr instead if sys_fgetxattr */
            if (adflags & ADFLAGS_DIR)
                /* For directories we open the directory RDONYL so we can later fchdir()  */
            {
                oflags = (oflags & ~O_RDWR) | O_RDONLY;
            }

            LOG(log_debug, logtype_ad,
                "ad_open_hf_ea(\"%s\"): opening base file for meta adouble EA", path);
            EC_NEG1(ad_meta_fileno(ad) = open(path, oflags));
            opened = 1;
            ad->ad_mdp->adf_flags = oflags;
        }
    }

#ifdef __APPLE__
    char *FinderInfo;
    char NativeFinderInfo[ADEDLEN_FINDERI] = {'\0'};
#endif

    /* Read the adouble header in and parse it.*/
    if (ad->ad_ops->ad_header_read(path, ad, NULL) != 0) {
        if (!(adflags & ADFLAGS_CREATE)) {
            LOG(log_debug, logtype_ad, "ad_open_hf_ea(\"%s\"): can't read metadata EA",
                path);

            if (errno == ENOATTR) {
                errno = ENOENT;
            }

            EC_FAIL;
        }

        if ((adflags & ADFLAGS_CREATE) && (ad->ad_options & ADVOL_RO)) {
            errno = EROFS;
            EC_FAIL;
        }

        LOG(log_debug, logtype_ad, "ad_open_hf_ea(\"%s\"): creating metadata EA", path);

        /* It doesnt exist, EPERM or another error */
        if (!(errno == ENOATTR || errno == ENOENT)) {
            LOG(log_error, logtype_ad, "ad_open_hf_ea: unexpected: %s", strerror(errno));
            EC_FAIL;
        }

        /* Create one */
        EC_NEG1_LOG(new_ad_header(ad, path, NULL, adflags));
        ad->ad_mdp->adf_flags |= O_CREAT; /* mark as just created */
#ifdef __APPLE__
        /* Read in FinderInfo from macOS */
        FinderInfo = ad_entry(ad, ADEID_FINDERI);

        if (ad_open_native_finderinfo(path, NativeFinderInfo)) {
            memcpy(FinderInfo, NativeFinderInfo, ADEDLEN_FINDERI);
        }

#endif
        ad_flush(ad);
        LOG(log_debug, logtype_ad, "ad_open_hf_ea(\"%s\"): created metadata EA", path);
    }

#ifdef __APPLE__
    /* Read in FinderInfo from macOS */
    FinderInfo = ad_entry(ad, ADEID_FINDERI);

    if (ad_open_native_finderinfo(path, NativeFinderInfo)) {
        memcpy(FinderInfo, NativeFinderInfo, ADEDLEN_FINDERI);
    }

#endif

    if (ad_meta_fileno(ad) != -1) {
        ad->ad_mdp->adf_refcount++;
    }

    if (!(adflags & ADFLAGS_NORLEN)) {
        ad->ad_rlen = ad_reso_size(path, adflags, ad);
    }

EC_CLEANUP:

    if (ret != 0 && opened && ad_meta_fileno(ad) != -1) {
        close(ad_meta_fileno(ad));
        ad_meta_fileno(ad) = -1;
        ad->ad_mdp->adf_refcount = 0;
    }

    LOG(log_debug, logtype_ad,
        "ad_open_hf_ea(\"%s\", %s): END: %d [dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags), ret,
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);
    EC_EXIT;
}

/*!
 * @brief Open AppleDouble metadata with the layout that ad_vers selects
 *
 * The metadata reference count rises before the layout-specific open runs and
 * falls again when that open fails. ad_error then decides whether the failure
 * is tolerable.
 *
 * @param[in]     path     data file whose metadata to open
 * @param[in]     adflags  ADFLAGS_* bits of the ad_open call
 * @param[in]     mode     creation mode for new metadata
 * @param[in,out] ad       AppleDouble receiving the metadata
 * @returns 0, or -1 with errno set
 */
static int ad_open_hf(const char *path, int adflags, int mode,
                      struct adouble *ad)
{
    int ret = 0;
    ad->ad_meta_refcount++;

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

    if (ret != 0) {
        ad->ad_meta_refcount--;
        ret = ad_error(ad, adflags);
    }

    return ret;
}

/*!
 * @brief Size of the resource fork of an AppleDouble ea file
 *
 * An open resource fork descriptor gives the size on every layout. Failing
 * that, with HAVE_EAFD it comes from the resource fork EA of the metadata
 * descriptor or of path, and without HAVE_EAFD it is the size of the
 * ad_path_osx file beyond its header, by path.
 *
 * @param[in] path     file whose resource fork to measure
 * @param[in] adflags  ADFLAGS_* bits, ADFLAGS_DIR yields 0
 * @param[in] ad       AppleDouble with possibly open descriptors, or NULL
 * @returns the size in bytes, or 0 for a directory or when the lookup fails
 */
off_t ad_reso_size(const char *path, int adflags, struct adouble *ad)
{
    EC_INIT;
    struct stat st;
    off_t rlen;

    if (adflags & ADFLAGS_DIR) {
        EC_FAIL;
    }

    LOG(log_debug, logtype_ad, "ad_reso_size(\"%s\"): BEGIN", path);
#ifdef HAVE_EAFD

    if (ad && ad_reso_fileno(ad) != -1) {
        EC_NEG1(fstat(ad_reso_fileno(ad), &st));
        rlen = st.st_size;
    } else if (ad && ad_meta_fileno(ad) != -1) {
        EC_NEG1((rlen = sys_fgetxattr(ad_meta_fileno(ad), AD_EA_RESO, NULL, 0)));
    } else {
        EC_NEG1((rlen = sys_lgetxattr(path, AD_EA_RESO, NULL, 0)));
    }

#else

    /* an open resource fork holds the fork the forks read */
    if (ad && ad_rsrc_open(ad)) {
        EC_ZERO(fstat(ad_reso_fileno(ad), &st));
    } else {
        const char *rfpath;
        EC_NULL_LOG(rfpath = ad_path_osx(path, adflags));
        EC_ZERO(lstat(rfpath, &st));
    }

    rlen = rlen_osx(&st);
#endif
    LOG(log_debug, logtype_ad, "ad_reso_size(\"%s\"): size: %lld", path,
        (long long)rlen);
EC_CLEANUP:

    if (ret != 0) {
        rlen = 0;
    }

    return rlen;
}

/*!
 * @brief Take a resource fork reference on a version 2 AppleDouble
 *
 * The version 2 resource fork lives in the metadata file, so ad_open_hf_v2
 * already opened it. A closed metadata file fails the call unless ADFLAGS_NORF
 * or ADFLAGS_RDONLY tolerates it, and then the count stays unchanged.
 *
 * @param[in]     path     data file, for logging only
 * @param[in]     adflags  ADFLAGS_* bits of the ad_open call
 * @param[in]     mode     unused
 * @param[in,out] ad       AppleDouble whose resource fork count rises
 * @returns 0, or -1 when the metadata file did not open and the flags require
 *          it
 */
static int ad_open_rf_v2(const char *path, int adflags, int mode _U_,
                         struct adouble *ad)
{
    EC_INIT;
    /*
     * ad_open_hf_v2() does the work, but if it failed and adflags are ADFLAGS_NOHF | ADFLAGS_RF
     * ad_open_hf_v2() didn't give an error, but we're supposed to return a reso fork fd
     */
    LOG(log_debug, logtype_ad, "ad_open_rf_v2(\"%s\"): BEGIN", fullpathname(path));

    if (!ad_meta_open(ad) && !(adflags & (ADFLAGS_NORF | ADFLAGS_RDONLY))) {
        EC_FAIL;
    }

    if (ad_meta_open(ad)) {
        ad->ad_reso_refcount++;
    }

EC_CLEANUP:
    LOG(log_debug, logtype_ad, "ad_open_rf_v2(\"%s\"): END: %d", fullpathname(path),
        ret);
    EC_EXIT;
}

/*!
 * @brief Open or create the resource fork of an AppleDouble ea file
 *
 * An already open resource fork gains a reference and refreshes ad_rlen. A new
 * open uses sys_getxattrfd on the metadata descriptor on Solaris, and open on
 * the ad_path file elsewhere, both without O_CREAT. EACCES, EPERM, or EROFS on
 * a read-only open retries read-only without O_TRUNC. ENOENT without
 * ADFLAGS_CREATE returns 0 with no fork open. ADFLAGS_CREATE adds O_CREAT and
 * creates the fork with mode.
 *
 * Without HAVE_EAFD a created fork receives a header through new_ad_header
 * and ad_flush, and an existing one goes through ad_header_read_osx, but
 * under ADFLAGS_NOHDR, a lock probe's open that keeps the ._ descriptor for
 * its lock test, no header is read, so it converts and refuses nothing.
 * ad_rlen receives the fork size. A failure closes a
 * descriptor this call opened, and ENOENT or ENOATTR under ADFLAGS_NORF then
 * counts as success. Any other failure also closes the data and metadata
 * forks that adflags name.
 *
 * @param[in]     path     data file whose resource fork to open
 * @param[in]     adflags  ADFLAGS_* bits of the ad_open call
 * @param[in]     mode     creation mode under ADFLAGS_CREATE
 * @param[in,out] ad       AppleDouble receiving the resource fork descriptor
 * @returns 0, or -1 with errno set
 */
static int ad_open_rf_ea(const char *path, int adflags, int mode,
                         struct adouble *ad)
{
    EC_INIT;
    int oflags;
    int opened = 0;
    int closeflags = adflags & (ADFLAGS_DF | ADFLAGS_HF);
#ifndef SOLARIS
    const char *rfpath;
    struct stat st;
#endif
    LOG(log_debug, logtype_ad, "ad_open_rf(\"%s\"): BEGIN", fullpathname(path));
    oflags = ad2openflags(ad, ADFLAGS_RF, adflags) & ~O_CREAT;

    if (ad_reso_fileno(ad) != -1) {
        /* the file is already open, but we want write access: */
        if ((oflags & O_RDWR)
                /* and it was already denied: */
                && (ad->ad_rfp->adf_flags & O_RDONLY)) {
            errno = EACCES;
            EC_FAIL;
        }

        ad->ad_rfp->adf_flags &= ~(O_TRUNC | O_CREAT);
        ad->ad_reso_refcount++;
        ad->ad_rfp->adf_refcount++;
        ad->ad_rlen = ad_reso_size(path, adflags, ad);
        goto EC_CLEANUP;
    }

#if defined (HAVE_EAFD) && defined (SOLARIS)

    if (ad_meta_fileno(ad) < 0) {
        EC_FAIL;
    }

    if ((ad_reso_fileno(ad) = sys_getxattrfd(ad_meta_fileno(ad), AD_EA_RESO,
                                             oflags)) == -1) {
        if (!(adflags & ADFLAGS_CREATE)) {
            switch (errno) {
            case EACCES:
            case EPERM:
            case EROFS:
                if (!(adflags & ADFLAGS_RDONLY)) {
                    LOG(log_error, logtype_ad, "ad_open_rf_ea(\"%s\"): \"%s\"", fullpathname(path),
                        strerror(errno));
                    EC_FAIL;
                }

                /* Strict read-only re-open: also drop O_TRUNC so a downgrade
                 * retry can never truncate the resource fork.  O_CREAT is
                 * already absent (stripped at oflags construction above).
                 * Mirrors ad_open_df()'s retry mask. */
                oflags &= ~(O_RDWR | O_TRUNC);
                oflags |= O_RDONLY;

                if ((ad_reso_fileno(ad) = sys_getxattrfd(ad_meta_fileno(ad), AD_EA_RESO,
                                                         oflags)) == -1) {
                    LOG(log_error, logtype_ad, "ad_open_rf_ea(\"%s\"): \"%s\"", fullpathname(path),
                        strerror(errno));
                    EC_FAIL;
                }

                break;

            case ENOENT:
                EC_EXIT_STATUS(0);

            default:
                LOG(log_error, logtype_ad, "ad_open_rf_ea(\"%s\"): \"%s\"", fullpathname(path),
                    strerror(errno));
                EC_FAIL;
            }
        } else {
            oflags |= O_CREAT;
            EC_NEG1_LOG(ad_reso_fileno(ad) = sys_getxattrfd(ad_meta_fileno(ad),
                                                            AD_EA_RESO, oflags, 0666));
        }
    }

#else
    EC_NULL_LOG(rfpath = ad->ad_ops->ad_path(path, adflags));

    if ((ad_reso_fileno(ad) = open(rfpath, oflags)) == -1) {
        if (!(adflags & ADFLAGS_CREATE)) {
            switch (errno) {
            case EACCES:
            case EPERM:
            case EROFS:
                if (!(adflags & ADFLAGS_RDONLY)) {
                    LOG(log_error, logtype_ad, "ad_open_rf_ea(\"%s\"): \"%s\"",
                        fullpathname(rfpath), strerror(errno));
                    EC_FAIL;
                }

                /* Strict read-only re-open: also drop O_TRUNC so a downgrade
                 * retry can never truncate the resource fork.  O_CREAT is
                 * already absent (stripped at oflags construction above).
                 * Mirrors ad_open_df()'s retry mask. */
                oflags &= ~(O_RDWR | O_TRUNC);
                oflags |= O_RDONLY;

                if ((ad_reso_fileno(ad) = open(rfpath, oflags)) == -1) {
                    LOG(log_error, logtype_ad, "ad_open_rf_ea(\"%s\"): \"%s\"",
                        fullpathname(rfpath), strerror(errno));
                    EC_FAIL;
                }

                break;

            case ENOENT:
                EC_EXIT_STATUS(0);

            default:
                LOG(log_error, logtype_ad, "ad_open_rf_ea(\"%s\"): \"%s\"",
                    fullpathname(rfpath), strerror(errno));
                EC_FAIL;
            }
        } else {
            oflags |= O_CREAT;
            EC_NEG1_LOG(ad_reso_fileno(ad) = open(rfpath, oflags, mode));
            LOG(log_debug, logtype_ad, "ad_open_rf(\"%s\"): created adouble rfork: \"%s\"",
                path, rfpath);
        }
    }

#endif
    opened = 1;
    ad->ad_rfp->adf_refcount = 1;
    ad->ad_rfp->adf_flags = oflags;
    ad->ad_reso_refcount++;
#ifndef HAVE_EAFD
    EC_ZERO_LOG(fstat(ad_reso_fileno(ad), &st));

    if (ad->ad_rfp->adf_flags & O_CREAT) {
        /* the new file's length, for the header the flush writes */
        ad->ad_rlen = rlen_osx(&st);
        /* This is a new adouble header file, create it */
        LOG(log_debug, logtype_ad,
            "ad_open_rf(\"%s\"): created adouble rfork, initializing: \"%s\"",
            path, rfpath);
        EC_NEG1_LOG(new_ad_header(ad, path, NULL, adflags));
        LOG(log_debug, logtype_ad,
            "ad_open_rf(\"%s\"): created adouble rfork, flushing: \"%s\"",
            path, rfpath);
        ad_flush(ad);
    } else if (adflags & ADFLAGS_NOHDR) {
        /* a lock probe's open reads no header: the stat's length */
        ad->ad_rlen = rlen_osx(&st);
    } else {
        /* the header read sets the length from this stat */
        LOG(log_debug, logtype_ad, "ad_open_rf(\"%s\"): reading adouble rfork: \"%s\"",
            path, rfpath);
        EC_NEG1_LOG(ad_header_read_osx(rfpath, ad, &st));
    }

#else
    ad->ad_rlen = ad_reso_size(path, adflags, ad);
#endif
EC_CLEANUP:

    if (ret != 0) {
        /* Capture the open-failure errno before the close()/ad_close() below can
         * clobber it: ADFLAGS_NORF suppresses only a *missing* resource fork
         * (ENOENT/ENOATTR), not a hard error (EIO, EACCES, ...), so a caller cannot
         * mistake an unreadable rfork for "no rfork". */
        int open_errno = errno;

        if (opened && (ad_reso_fileno(ad) != -1)) {
            close(ad_reso_fileno(ad));
            ad_reso_fileno(ad) = -1;
            ad->ad_reso_refcount--;
            ad->ad_rfp->adf_refcount = 0;
        }

        if ((adflags & ADFLAGS_NORF)
                && (open_errno == ENOENT || open_errno == ENOATTR)) {
            ret = 0;
        } else {
            int err = errno;
            (void)ad_close(ad, closeflags);
            errno = err;
        }

        /* a missing fork has no length; a fork still holding the adouble
         * keeps the length of a fork this open could not read */
        if (open_errno == ENOENT || open_errno == ENOATTR || !ad_held(ad)) {
            ad->ad_rlen = 0;
        }
    }

    LOG(log_debug, logtype_ad, "ad_open_rf(\"%s\"): END: %d", fullpathname(path),
        ret);
    EC_EXIT;
}

/*!
 * @brief Open the resource fork of an AppleDouble with the layout that ad_vers
 * selects
 *
 * @param[in]     path     data file whose resource fork to open
 * @param[in]     adflags  ADFLAGS_* bits of the ad_open call
 * @param[in]     mode     creation mode under ADFLAGS_CREATE
 * @param[in,out] ad       AppleDouble receiving the resource fork
 * @returns 0, or -1 on failure or an unknown ad_vers
 */
static int ad_open_rf(const char *path, int adflags, int mode,
                      struct adouble *ad)
{
    int ret = 0;

    switch (ad->ad_vers) {
    case AD_VERSION2:
        ret = ad_open_rf_v2(path, adflags, mode, ad);
        break;

    case AD_VERSION_EA:
        ret = ad_open_rf_ea(path, adflags, mode, ad);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

/***********************************************************************************
 * API functions
 ********************************************************************************* */

/*!
 * @brief Check that entry eid with got_len bytes at off fits the ad_data buffer
 *
 * An empty entry always passes. Fixed-size entries must have exactly their
 * defined length. FinderInfo must hold at least ADEDLEN_FINDERI bytes, because
 * a macOS sidecar packs xattrs behind the canonical 32, and ad_convert_osx
 * discards them later. Other variable entries must not exceed their defined
 * length, and the buffer must have room for that maximum. The data and
 * resource forks never live in the buffer, so they always pass.
 *
 * @param[in] eid      entry id, ADEID_*
 * @param[in] bufsize  valid bytes in the ad_data buffer
 * @param[in] off      entry offset in the buffer
 * @param[in] got_len  entry length from the header
 * @returns true when the entry fits, false when it does not or eid is out of
 *          range
 */
static bool ad_entry_check_size(uint32_t eid,
                                size_t bufsize,
                                uint32_t off,
                                uint32_t got_len)
{
    struct {
        off_t expected_len;
        bool fixed_size;
        bool minimum_size;
    } ad_checks[] = {
        [ADEID_DFORK] = {-1, false, false}, /* not applicable */
        [ADEID_RFORK] = {-1, false, false}, /* no limit */
        [ADEID_NAME] = {ADEDLEN_NAME, false, false},
        [ADEID_COMMENT] = {ADEDLEN_COMMENT, false, false},
        [ADEID_ICONBW] = {ADEDLEN_ICONBW, true, false},
        [ADEID_ICONCOL] = {ADEDLEN_ICONCOL, false, false},
        [ADEID_FILEI] = {ADEDLEN_FILEI, true, false},
        [ADEID_FILEDATESI] = {ADEDLEN_FILEDATESI, true, false},
        [ADEID_FINDERI] = {ADEDLEN_FINDERI, false, true},
        [ADEID_MACFILEI] = {ADEDLEN_MACFILEI, true, false},
        [ADEID_PRODOSFILEI] = {ADEDLEN_PRODOSFILEI, true, false},
        [ADEID_MSDOSFILEI] = {ADEDLEN_MSDOSFILEI, true, false},
        [ADEID_SHORTNAME] = {ADEDLEN_SHORTNAME, false, false},
        [ADEID_AFPFILEI] = {ADEDLEN_AFPFILEI, true, false},
        [ADEID_DID] = {ADEDLEN_DID, true, false},
        [ADEID_PRIVDEV] = {ADEDLEN_PRIVDEV, true, false},
        [ADEID_PRIVINO] = {ADEDLEN_PRIVINO, true, false},
        [ADEID_PRIVSYN] = {ADEDLEN_PRIVSYN, true, false},
        [ADEID_PRIVID] = {ADEDLEN_PRIVID, true, false},
    };
    uint32_t required_len;

    if (eid >= ADEID_MAX) {
        return false;
    }

    if (got_len == 0) {
        /* Entry present, but empty, allow */
        return true;
    }

    if (ad_checks[eid].expected_len == 0) {
        /*
         * Shouldn't happen: implicitly initialized to zero because
         * explicit initializer missing.
         */
        return false;
    }

    if (ad_checks[eid].expected_len == -1) {
        /* Unused or no limit */
        return true;
    }

    if (ad_checks[eid].fixed_size) {
        if (ad_checks[eid].expected_len != got_len) {
            /* Wrong size fo fixed size entry. */
            return false;
        }

        required_len = got_len;
    } else {
        if (ad_checks[eid].minimum_size) {
            if (got_len < ad_checks[eid].expected_len) {
                /*
                 * Too small for variable sized entry with
                 * minimum size.
                 */
                return false;
            }

            required_len = got_len;
        } else {
            if (got_len > ad_checks[eid].expected_len) {
                /* Too big for variable sized entry. */
                return false;
            }

            /*
             * Expect the buffer to provide enough room for the maximum possible
             * size.
             */
            required_len = ad_checks[eid].expected_len;
        }
    }

    if (off + required_len < off) {
        /* wrap around */
        return false;
    }

    if (off + required_len > bufsize) {
        /* overflow */
        return false;
    }

    return true;
}

/*!
 * @brief Whether entry eid in ad has room for len bytes at its offset
 *
 * This is the bounds check of ad_entry against a caller-supplied length, so a
 * caller can test a destination before it commits the length.
 *
 * @param[in] ad   AppleDouble with its metadata loaded
 * @param[in] eid  entry id, ADEID_*
 * @param[in] len  length to test
 * @returns true when the entry has an offset and the length fits
 */
bool ad_entry_fits(const struct adouble *ad, int eid, uint32_t len)
{
    off_t off = ad_getentryoff(ad, eid);
    return off != 0
           && ad_entry_check_size(eid, ad->valid_data_len, (uint32_t) off, len);
}

/*!
 * @brief Pointer to entry eid inside the ad_data buffer of ad
 *
 * @param[in] ad   AppleDouble with its metadata loaded
 * @param[in] eid  entry id, ADEID_*
 * @returns the entry start, or NULL when the entry is absent or does not fit
 *          the buffer
 */
void *ad_entry(const struct adouble *ad, int eid)
{
    off_t off = ad_getentryoff(ad, eid);
    uint32_t len = (uint32_t) ad_getentrylen(ad, eid);

    if (!ad_entry_fits(ad, eid, len)) {
        LOG(log_debug, logtype_ad,
            "ad_entry(%s, %d): invalid off: %lld, len: %u, buf: %zu",
            ad->ad_name, eid, (long long) off, len, ad->valid_data_len);
        return NULL;
    }

    return ((struct adouble *)ad)->ad_data + off;
}

/*!
 * @brief Offset of entry eid in the layout of ad
 *
 * Version 2 returns the header offset. The ea layout returns 0 for the data
 * fork and, with HAVE_EAFD, for the resource fork, because those forks live
 * outside the metadata EA.
 *
 * @param[in] ad   AppleDouble with its metadata loaded
 * @param[in] eid  entry id, ADEID_*
 * @returns the entry offset, 0 for a fork kept outside the buffer or an absent
 *          entry
 */
off_t ad_getentryoff(const struct adouble *ad, int eid)
{
    if (ad->ad_vers == AD_VERSION2) {
        return ad->ad_eid[eid].ade_off;
    }

    switch (eid) {
    case ADEID_DFORK:
        return 0;

    case ADEID_RFORK:
#ifdef HAVE_EAFD
        return 0;
#else
        return ad->ad_eid[eid].ade_off;
#endif

    default:
        return ad->ad_eid[eid].ade_off;
    }

    /* deadc0de */
    AFP_PANIC("What am I doing here?");
}

/*!
 * @brief The descriptor that holds fork eid
 *
 * @param[in]     ad   adouble holding the fork
 * @param[in]     eid  ADEID_DFORK or ADEID_RFORK
 * @param[in,out] off  fork offset, returned as the offset in that descriptor
 *
 * @returns the data fork or resource fork descriptor
 */
int ad_fork_fileno(const struct adouble *ad, int eid, off_t *off)
{
    if (eid == ADEID_DFORK) {
        return ad_data_fileno(ad);
    }

    *off += ad_getentryoff(ad, eid);
    return ad_reso_fileno(ad);
}

/*!
 * @brief Return path unchanged, because the Solaris ea layout keeps every fork
 * in EAs of the file
 *
 * @param[in] path     file or directory
 * @param[in] adflags  unused
 * @returns path
 */
const char *ad_path_ea(const char *path, int adflags _U_)
{
    return path;
}

/*!
 * @brief Sidecar path of the resource fork in the ea layout
 *
 * The result is "._name" beside the file, or "name/..namedfork/rsrc" on macOS
 * where the kernel exposes the native resource fork as a file. A path of "."
 * names the current directory and getcwd resolves it first. The result lives
 * in a static buffer that the next call overwrites.
 *
 * @param[in] path     file whose resource fork sidecar to name
 * @param[in] adflags  unused
 * @returns the sidecar path in a static buffer
 */
const char *ad_path_osx(const char *path, int adflags _U_)
{
    static char pathbuf[MAXPATHLEN + 1];
    char    c, *slash, buf[MAXPATHLEN + 1];

    if (!strcmp(path, ".")) {
        if (getcwd(buf, MAXPATHLEN) == NULL) {
            LOG(log_error, logtype_default,
                "ad_path_osx(\"%s\"): getcwd() failed in ad_path_osx: %s", path,
                strerror(errno));
            buf[0] = '\0';
        } else if (strnlen(buf, MAXPATHLEN + 1) >= MAXPATHLEN) {
            LOG(log_error, logtype_default,
                "ad_path_osx(\"%s\"): Current working directory path too long, truncating to MAXPATHLEN",
                path);
            buf[MAXPATHLEN - 1] = '\0';
        }
    } else {
        strlcpy(buf, path, MAXPATHLEN + 1);
    }

    if (NULL != (slash = strrchr(buf, '/'))) {
        c = *++slash;
        *slash = '\0';
        strlcpy(pathbuf, buf, MAXPATHLEN + 1);
        *slash = c;
    } else {
        pathbuf[0] = '\0';
        slash = buf;
    }

#ifdef __APPLE__
    /* macos abstracts the resource fork as "filename/..namedfork/rsrc" which we can treat as a file */
    strlcat(pathbuf, slash, MAXPATHLEN + 1);
    strlcat(pathbuf, "/..namedfork/rsrc", MAXPATHLEN  + 1);
#else
    strlcat(pathbuf, "._", MAXPATHLEN  + 1);
    strlcat(pathbuf, slash, MAXPATHLEN + 1);
#endif
    return pathbuf;
}

/*!
 * @brief Metadata file path of the version 2 layout
 *
 * A file "a/b" maps to "a/.AppleDouble/b". A directory "a/b" maps to
 * "a/b/.AppleDouble/.Parent". The result lives in a static buffer that the
 * next call overwrites.
 *
 * @param[in] path     file or directory
 * @param[in] adflags  ADFLAGS_DIR marks path as a directory
 * @returns the metadata path in a static buffer, or NULL when it would exceed
 *          MAXPATHLEN
 */
const char *ad_path(const char *path, int adflags)
{
    static char pathbuf[MAXPATHLEN + 1];
    const char *slash;
    size_t  l;

    if (adflags & ADFLAGS_DIR) {
        l = strlcpy(pathbuf, path, sizeof(pathbuf));

        if (l >= sizeof(pathbuf)) {
            return NULL;
        }

        if (l && l < MAXPATHLEN) {
            pathbuf[l++] = '/';
        }

        if (strlcpy(pathbuf + l, ".AppleDouble/.Parent", sizeof(pathbuf) - l)
                >= sizeof(pathbuf) - l) {
            return NULL;
        }
    } else {
        if (NULL != (slash = strrchr(path, '/'))) {
            slash++;
            l = slash - path;

            /* XXX we must return NULL here and test in the caller */
            if (l > MAXPATHLEN) {
                l = MAXPATHLEN;
            }

            memcpy(pathbuf, path, l);
        } else {
            l = 0;
            slash = path;
        }

        size_t remaining = sizeof(pathbuf) - l;
        l += strlcpy(pathbuf + l, ".AppleDouble/", remaining);

        if (l >= sizeof(pathbuf)) {
            return NULL;
        }

        if (strlcpy(pathbuf + l, slash, sizeof(pathbuf) - l) >= sizeof(pathbuf) - l) {
            return NULL;
        }
    }

    return pathbuf;
}

/*!
 * @brief Parent directory of path, for permission inheritance
 *
 * The result drops the last component and any trailing slashes, and a bare
 * file name yields ".". It lives in a static buffer that the next call
 * overwrites.
 *
 * @param[in] path  file or directory
 * @returns the parent directory in a static buffer, or NULL with ENAMETOOLONG
 */
char *ad_dir(const char *path)
{
    static char     modebuf[MAXPATHLEN + 1];
    char        *slash;
    /*
     * For a path with directories in it, remove the final component
     * (path or subdirectory name) to get the name we want to stat.
     * For a path which is just a filename, use "." instead.
     */
    slash = strrchr(path, '/');

    if (slash) {
        size_t len;
        len = slash - path;

        if (len >= MAXPATHLEN) {
            errno = ENAMETOOLONG;
            return NULL;  /* can't do it */
        }

        memcpy(modebuf, path, len);
        modebuf[len] = '\0';

        /* is last char a '/' ? */
        if (slash[1] == 0) {
            slash = modebuf + len;

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

/*!
 * @brief Register the user that ad_chown gives to new AppleDouble files
 *
 * @param[in] id  default owner, 0 to inherit from the parent directory, or -1
 *                to disable
 * @returns 0
 */
int ad_setfuid(const uid_t id)
{
    default_uid = id;
    return 0;
}

/*!
 * @brief The user that ad_setfuid registered
 *
 * @returns the default owner, -1 before any ad_setfuid call
 */
uid_t ad_getfuid(void)
{
    return default_uid;
}

/*!
 * @brief Stat the parent directory of path
 *
 * @param[in]  path   file or directory
 * @param[out] stbuf  receives the parent directory stat
 * @returns 0, or -1 when ad_dir or stat failed
 */
int ad_stat(const char *path, struct stat *stbuf)
{
    char *p;
    p = ad_dir(path);

    if (!p) {
        return -1;
    }

    return stat(p, stbuf);
}

/*!
 * @brief Mask a creation mode with the parent directory permissions of path
 *
 * @param[in] path  file whose parent directory supplies the permissions
 * @param[in] mode  requested mode
 * @returns the masked mode, limited to DEFMASK when the parent directory stat
 *          failed
 */
int ad_mode(const char *path, mode_t mode)
{
    struct stat     stbuf;
    ad_mode_st(path, &mode, &stbuf);
    return mode;
}

/*!
 * @brief Create a directory with a mode inherited from its parent
 *
 * ad_mode_st restricts mode to the parent permissions, and ad_chown then
 * gives the new directory the owner that the parent implies.
 *
 * @param[in] path  directory to create
 * @param[in] mode  requested mode
 * @returns 0, or -1 with errno from mkdir
 */
int ad_mkdir(const char *path, mode_t mode)
{
    int ret;
    int st_invalid;
    struct stat stbuf;
    LOG(log_debug, logtype_ad, "ad_mkdir(\"%s\", %04o) {cwd: \"%s\"}",
        path, mode, getcwdpath());
    st_invalid = ad_mode_st(path, &mode, &stbuf);
    ret = mkdir(path, mode);

    if (ret || st_invalid) {
        return ret;
    }

    ad_chown(path, &stbuf);
    return ret;
}

/*!
 * @brief Select the fork layout of ad from ad_vers
 *
 * Version 2 shares the metadata descriptor with the resource fork, and the ea
 * layout shares it with the data fork. Every descriptor starts closed, the
 * reference count at 1, and the resource fork length at 0. An unknown version
 * aborts.
 *
 * @param[in,out] ad  AppleDouble with ad_vers set
 */
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
    ad->ad_rlen = 0;
    return;
}

/*!
 * @brief Initialize an AppleDouble from explicit version and option values
 *
 * ad_init takes the same values from a volume. Unlike ad_init this function
 * leaves the entry offset table empty until a header read fills it.
 *
 * @param[out] ad       AppleDouble to initialize
 * @param[in]  flags    AD_VERSION2 or AD_VERSION_EA
 * @param[in]  options  ADVOL_* option bits
 */
void ad_init_old(struct adouble *ad, int flags, int options)
{
    memset(ad, 0, sizeof(struct adouble));
    ad->ad_vers = flags;
    ad->ad_options = options;
    ad_init_func(ad);
}

/*!
 * @brief Initialize an AppleDouble for the layout and options of a volume
 *
 * ad_init_offsets fills the entry offset table at once, so ad_entry,
 * ad_getattr, and ad_getdate work on a cache hit that never reads a header.
 * ad_init_offsets ignores a header that already carries AD_MAGIC, so a later
 * header read can call it again.
 *
 * @param[out] ad   AppleDouble to initialize
 * @param[in]  vol  volume supplying v_adouble and v_ad_options
 */
void ad_init(struct adouble *ad, const struct vol *vol)
{
    memset(ad, 0, sizeof(struct adouble));
    ad->ad_vers = vol->v_adouble;
    ad->ad_options = vol->v_ad_options;
    ad_init_func(ad);
    /* Initialize offset tables so ad_entry()/ad_getattr()/ad_getdate()
     * work on cache-hit paths where ad_metadata() is never called.
     * Idempotent — guarded by ad_magic == AD_MAGIC, so calling again
     * later via ad_open() → ad_header_read() is safe. */
    ad_init_offsets(ad);
}

/*!
 * @brief Open data-, metadata(header)- or resource fork
 *
 * @code
 * ad_open(struct adouble *ad, const char *path, int adflags, int flags)
 * ad_open(struct adouble *ad, const char *path, int adflags, int flags, mode_t mode)
 * @endcode
 *
 * You must call ad_init() before ad_open, usually you'll just call it like this:
 * @code
 *      struct adoube ad;
 *      ad_init(&ad, vol->v_adouble, vol->v_ad_options);
 * @endcode
 *
 * Open a files data fork, metadata fork or resource fork.
 *
 * @param[in,out] ad     pointer to struct adouble
 * @param[in] path       Path to file or directory
 * @param[in] adflags    Flags specifying which fork to open, can be or'd (see below)
 * @param[in] ...        mode used with O_CREATE
 *
 * Regular adflags:
 *  - ADFLAGS_DF:        open data fork
 *  - ADFLAGS_RF:        open resource fork
 *  - ADFLAGS_HF:        open header (metadata) file
 *  - ADFLAGS_NOHF:      it's not an error if header file couldn't be opened
 *  - ADFLAGS_NORF:      it's not an error if reso fork couldn't be opened
 *  - ADFLAGS_DIR:       if path is a directory you MUST or ADFLAGS_DIR to adflags
 *
 * Access mode for the forks:
 *  - ADFLAGS_RDONLY:    read intent; permits descriptor promotion and metadata
 *                       repair as described below
 *  - ADFLAGS_RDWR:      open read write
 *  - ADFLAGS_RDONLY_STRICT: open existing metadata with O_RDONLY and no repair.
 *                       Implies ADFLAGS_RDONLY; only ADFLAGS_HF and ADFLAGS_DIR
 *                       may accompany it. Requires a freshly initialized
 *                       adouble, which must be reinitialized before reuse.
 *
 * Creation flags:
 *  - ADFLAGS_CREATE:    create if not existing
 *  - ADFLAGS_TRUNC:     truncate
 *
 * Special flags:
 *  - ADFLAGS_CHECK_OF:  check for open forks from us and other afpd's
 *  - ADFLAGS_SETSHRMD:  this adouble struct will be used to set sharemode locks.
 *                       This basically results in the files being opened RW instead of RDONLY.
 *
 * ADFLAGS_RDONLY does not guarantee an O_RDONLY descriptor or an open without
 * filesystem modifications. With ADFLAGS_SETSHRMD (also implied by
 * ADFLAGS_CHECK_OF), data and resource descriptors request O_RDWR so they can
 * carry write locks. Version 2 metadata also requests O_RDWR because it shares
 * a descriptor with the resource fork, which may later be opened for writing.
 * Opening an existing zero-length v2 metadata file may initialize and flush a
 * fresh header, even without ADFLAGS_CREATE.
 *
 * Use ADFLAGS_RDONLY_STRICT for isolated metadata inspection, such as a dbd
 * scan. It bypasses descriptor promotion and rejects an empty v2 header with
 * EIO before repair. It requires ADFLAGS_HF and permits ADFLAGS_DIR and the
 * implied ADFLAGS_RDONLY; other flags or a previously opened adouble cause
 * EINVAL. Close a successful open with ad_close(ad, ADFLAGS_HF), then
 * reinitialize the object before any further ad_open(). Reinitialization is
 * also required before retrying a failed strict open.
 *
 * Normal AFP fork operations retain shared descriptors: closing another
 * descriptor for the same file can release the process's POSIX locks.
 * Strict metadata inspection therefore uses a private, freshly initialized
 * adouble and cannot replace opens on a shared fork object.
 *
 * @bug on Solaris (HAVE_EAFD) ADFLAGS_RF doesn't work without
 *   ADFLAGS_HF, because it checks whether ad_meta_fileno() is already
 *   opened. As a workaround pass ADFLAGS_SETSHRMD.
 *
 * @returns 0 on success, any other value indicates an error
 */
int ad_open(struct adouble *ad, const char *path, int adflags, ...)
{
    EC_INIT;
    va_list args;
    mode_t mode = 0;

    /* A scan needs an isolated metadata read, without the descriptor sharing
     * and write upgrades used by AFP fork opens. Reject conflicting flags
     * and reuse before opening anything or changing the reference counts. */
    if (adflags & ADFLAGS_RDONLY_STRICT) {
        if (!(adflags & ADFLAGS_HF)
                || (adflags & ~(ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_RDONLY
                                | ADFLAGS_RDONLY_STRICT))
                || ad->ad_inited == AD_INITED) {
            errno = EINVAL;
            return -1;
        }

        adflags |= ADFLAGS_RDONLY;
    } else if (ad->ad_inited == AD_INITED
               && (ad->ad_adflags & ADFLAGS_RDONLY_STRICT)) {
        errno = EINVAL;
        return -1;
    }

    LOG(log_debug, logtype_ad,
        "ad_open(\"%s\", %s): BEGIN {d: %d, m: %d, r: %d}"
        "[dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), adflags2logstr(adflags),
        ad->ad_data_refcount, ad->ad_meta_refcount, ad->ad_reso_refcount,
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);

    if (adflags & ADFLAGS_CHECK_OF)
        /* Checking for open forks requires sharemode lock support (ie RDWR instead of RDONLY) */
    {
        adflags |= ADFLAGS_SETSHRMD;
    }

    if (adflags & ADFLAGS_SETSHRMD)
        /* sharemode locks are stored in the data fork */
    {
        adflags |= ADFLAGS_DF;
    }

    if (ad->ad_vers == AD_VERSION2) {
        if (adflags & ADFLAGS_RF) {
            adflags |= ADFLAGS_HF;
        }

        if (adflags & ADFLAGS_NORF) {
            adflags |= ADFLAGS_NOHF;
        }
    }

    if (ad->ad_inited != AD_INITED) {
        ad->ad_adflags = adflags;
        ad->ad_inited = AD_INITED;
    } else {
        ad->ad_open_forks = ((ad->ad_data_fork.adf_refcount > 0) ? ATTRBIT_DOPEN : 0);

        if (ad->ad_resource_fork.adf_refcount > 0) {
            ad->ad_open_forks |= ATTRBIT_ROPEN;
        }
    }

    va_start(args, adflags);

    if (adflags & ADFLAGS_CREATE) {
        mode = (sizeof(mode_t) < sizeof(int) ? va_arg(args, int) : va_arg(args,
            mode_t));
    }

    va_end(args);

    if (adflags & ADFLAGS_DF) {
        ad->ad_data_refcount++;

        if (ad_open_df(path, adflags, mode, ad) != 0) {
            ad->ad_data_refcount--;
            EC_FAIL;
        }
    }

    if (adflags & ADFLAGS_HF) {
        if (ad_open_hf(path, adflags, mode, ad) != 0) {
            EC_FAIL;
        }
    }

    if (adflags & ADFLAGS_RF) {
        if (ad_open_rf(path, adflags, mode, ad) != 0) {
            EC_FAIL;
        }
    }

    if (adflags & ADFLAGS_CHECK_OF) {
        ad->ad_open_forks |= ad_openforks(ad, ad->ad_open_forks);
    }

EC_CLEANUP:
    LOG(log_debug, logtype_ad,
        "ad_open(\"%s\"): END: %d {d: %d, m: %d, r: %d}"
        "[dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        fullpathname(path), ret,
        ad->ad_data_refcount, ad->ad_meta_refcount, ad->ad_reso_refcount,
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);
    EC_EXIT;
}

/*!
 * @brief Open only the metadata of a file or directory read-only, as root when
 * needed
 *
 * The call keeps ADFLAGS_DIR and ADFLAGS_CHECK_OF from flags, adds ADFLAGS_HF
 * and ADFLAGS_RDONLY, and retries as root after EACCES.
 *
 * @param[in]     name   file or directory
 * @param[in]     flags  ADFLAGS_DIR when name is a directory, ADFLAGS_CHECK_OF
 *                       to look for open forks of this or another afpd
 * @param[in,out] adp    initialized AppleDouble receiving the metadata
 * @returns 0, or -1 with errno set
 */
int ad_metadata(const char *name, int flags, struct adouble *adp)
{
    int   ret, err, oflags;
    /* Sanitize flags */
    oflags = (flags & (ADFLAGS_CHECK_OF | ADFLAGS_DIR)) | ADFLAGS_HF |
             ADFLAGS_RDONLY;

    if ((ret = ad_open(adp, name, oflags)) < 0 && errno == EACCES) {
        become_root();
        ret = ad_open(adp, name, oflags);
        unbecome_root();
        err = errno;
        errno = err;
    }

    return ret;
}

/*!
 * @brief ad_metadata relative to a directory descriptor
 *
 * With a dirfd other than -1 the call changes into that directory around
 * ad_metadata and restores the previous working directory afterwards. When
 * the restore fails it anchors the process at "/" and reports the restore
 * error only when ad_metadata itself succeeded.
 *
 * @param[in]     dirfd  directory that name is relative to, or -1 for the
 *                       working directory
 * @param[in]     name   file or directory
 * @param[in]     flags  ADFLAGS_DIR and ADFLAGS_CHECK_OF, as for ad_metadata
 * @param[in,out] adp    initialized AppleDouble receiving the metadata
 * @returns 0, or -1 with errno set
 */
int ad_metadataat(int dirfd, const char *name, int flags, struct adouble *adp)
{
    int ret = 0;
    int cwdfd = -1;

    if (dirfd != -1) {
        if ((cwdfd = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC)) == -1) {
            LOG(log_error, logtype_ad,
                "ad_metadataat: cannot snapshot cwd: %s", strerror(errno));
            return -1;
        }

        if (fchdir(dirfd) != 0) {
            /* Preserve fchdir's errno across close(). */
            int saved = errno;
            LOG(log_error, logtype_ad,
                "ad_metadataat: fchdir(target) failed: %s", strerror(saved));
            close(cwdfd);
            errno = saved;
            return -1;
        }
    }

    if (ad_metadata(name, flags, adp) < 0) {
        ret = -1;
        /* fall through: restore cwd before returning */
    }

    if (dirfd != -1) {
        /* Captured before the restore so its chdir()/close() can't clobber the
         * operation's errno (which callers map to an AFPERR). */
        int saved = errno;

        if (fchdir(cwdfd) != 0) {
            int restore_errno = errno;
            /* The saved fd is the thing that failed, so cwd cannot be
             * restored.  Anchor to "/" (always valid) so no relative path
             * resolves against an unknown directory; the caller re-establishes
             * cwd via movecwd() on its next path op. */
            LOG(log_error, logtype_ad,
                "ad_metadataat: cannot restore cwd (%s); anchoring to \"/\"",
                strerror(restore_errno));

            if (chdir("/") != 0) {
                LOG(log_error, logtype_ad,
                    "ad_metadataat: chdir(/) also failed: %s",
                    strerror(errno));
            }

            /* When only the restore failed, that is the error to report. */
            if (ret == 0) {
                saved = restore_errno;
            }

            ret = -1;
            errno = saved;
        }
    }

    if (cwdfd != -1) {
        close(cwdfd);
    }

    return ret;
}

/*!
 * @brief Reread the metadata of an open AppleDouble
 *
 * Version 2 rereads the header from the open metadata descriptor. The ea
 * layout refreshes the resource fork length from the open resource fork, and
 * without HAVE_EAFD rereads the sidecar header first. It then rereads the
 * metadata EA.
 *
 * @param[in]     path  file, unused for version 2
 * @param[in,out] ad    AppleDouble with the forks to refresh open
 * @returns 0, or -1 when a descriptor it needs stays closed or a read failed
 */
int ad_refresh(const char *path, struct adouble *ad)
{
    switch (ad->ad_vers) {
    case AD_VERSION2:
        if (ad_meta_fileno(ad) == -1) {
            return -1;
        }

        return ad->ad_ops->ad_header_read(NULL, ad, NULL);

    case AD_VERSION_EA:
        if (ad_refresh_rfork(path, ad) != 0) {
            return -1;
        }

        return ad_refresh_meta(path, ad);

    default:
        return -1;
    }
}

/*!
 * @brief Re-read what the open resource fork's own file holds
 *
 * The metadata stays as it was read. On the ea layout the read is the ._
 * file's header, converted as ad_refresh() converts it; on version 2 the
 * whole header; under HAVE_EAFD the fork's length.
 *
 * @param[in]     path  file, for the log and the conversion, or NULL
 * @param[in,out] ad    AppleDouble with the resource fork open
 * @returns 0, or -1 with errno set when a read failed; a fork without a file
 *          of its own leaves nothing to read
 */
int ad_refresh_rfork(const char *path, struct adouble *ad)
{
#ifdef HAVE_EAFD
    struct stat st;
#endif

    if (ad->ad_vers == AD_VERSION2) {
        return ad_refresh(path, ad);
    }

    if (!ad_rsrc_open(ad)) {
        return 0;
    }

#ifdef HAVE_EAFD

    if (fstat(ad_reso_fileno(ad), &st) == -1) {
        return -1;
    }

    ad->ad_rlen = st.st_size;
    return 0;
#else
    return ad_header_read_osx(path, ad, NULL);
#endif
}

/*!
 * @brief Re-read the metadata of an open AppleDouble
 *
 * The resource fork's own file stays as it was read. On the ea layout the
 * read is the metadata EA; on version 2 the whole header.
 *
 * @param[in]     path  file, read by path when the metadata is not open
 * @param[in,out] ad    AppleDouble
 * @returns 0, or -1 with errno set when the read failed
 */
int ad_refresh_meta(const char *path, struct adouble *ad)
{
    if (ad->ad_vers == AD_VERSION2) {
        return ad_refresh(path, ad);
    }

    return ad->ad_ops->ad_header_read(path, ad, NULL);
}

/*!
 * @brief ad_open relative to a directory descriptor
 *
 * With a dirfd other than -1 the call changes into that directory around
 * ad_open and restores the previous working directory afterwards. When the
 * restore fails it anchors the process at "/" and reports the restore error
 * only when ad_open itself succeeded. ADFLAGS_CREATE takes the creation mode
 * from the variable arguments, as ad_open does.
 *
 * @param[in,out] ad       initialized AppleDouble
 * @param[in]     dirfd    directory that path is relative to, or -1 for the
 *                         working directory
 * @param[in]     path     file or directory
 * @param[in]     adflags  ADFLAGS_* bits, as for ad_open
 * @param[in]     ...      creation mode under ADFLAGS_CREATE
 * @returns 0, or -1 with errno set
 */
int ad_openat(struct adouble  *ad,
              int dirfd,  /* dir fd openat like */
              const char *path,
              int adflags, ...)
{
    EC_INIT;
    int cwdfd = -1;
    int changed_cwd = 0;
    va_list args;
    mode_t mode = 0;

    if (dirfd != -1) {
        if ((cwdfd = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC)) == -1) {
            LOG(log_error, logtype_ad,
                "ad_openat: cannot snapshot cwd: %s", strerror(errno));
            EC_FAIL;
        }

        if (fchdir(dirfd) != 0) {
            /* Preserve fchdir's errno across close(). */
            int saved = errno;
            LOG(log_error, logtype_ad,
                "ad_openat: fchdir(target) failed: %s", strerror(saved));
            close(cwdfd);
            cwdfd = -1;
            errno = saved;
            EC_FAIL;
        }

        changed_cwd = 1;
    }

    va_start(args, adflags);

    if (adflags & ADFLAGS_CREATE) {
        mode = (sizeof(mode_t) < sizeof(int) ? va_arg(args, int) : va_arg(args,
            mode_t));
    }

    va_end(args);
    EC_NEG1(ad_open(ad, path, adflags, mode));
EC_CLEANUP:

    /* Restore cwd on every exit path once we changed it, including the EC_NEG1
     * failure above.  The saved fd is the thing that failed, so on restore
     * failure anchor to "/" (always valid) rather than leaving cwd unknown;
     * the caller re-establishes cwd via movecwd() on its next path op.
     * If the operation failed, keep its errno so callers can map it to an
     * AFPERR.  If it succeeded but the restore fails, surface the restore errno
     * instead (otherwise we would return -1 with a stale/zero errno). */
    if (changed_cwd) {
        int saved = errno;

        if (fchdir(cwdfd) != 0) {
            int restore_errno = errno;
            LOG(log_error, logtype_ad,
                "ad_openat: cannot restore cwd (%s); anchoring to \"/\"",
                strerror(restore_errno));

            if (chdir("/") != 0) {
                LOG(log_error, logtype_ad,
                    "ad_openat: chdir(/) also failed: %s", strerror(errno));
            }

            /* When only the restore failed, that is the error to report. */
            if (ret == 0) {
                saved = restore_errno;
            }

            ret = -1;
            errno = saved;
        }
    }

    if (cwdfd != -1) {
        close(cwdfd);
    }

    return ret;
}

/*!
 * @brief Derive the metadata file mode from a data fork mode
 *
 * Execute bits go away. Any read permission also grants write, because fcntl
 * locks need write access, and any write permission also grants read.
 *
 * @param[in] mode  data fork mode
 * @returns the metadata file mode
 */
mode_t ad_hf_mode(mode_t mode)
{
    mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH);

    /* fnctl lock need write access */
    if (mode & S_IRUSR) {
        mode |= S_IWUSR;
    }

    if (mode & S_IRGRP) {
        mode |= S_IWGRP;
    }

    if (mode & S_IROTH) {
        mode |= S_IWOTH;
    }

    /* if write mode set add read mode */
    if (mode & S_IWUSR) {
        mode |= S_IRUSR;
    }

    if (mode & S_IWGRP) {
        mode |= S_IRGRP;
    }

    if (mode & S_IWOTH) {
        mode |= S_IROTH;
    }

    return mode;
}
