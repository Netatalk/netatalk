/*
 * Copyright (c) 1998 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All rights reserved. See COPYRIGHT.
 */

/*!
 * @file
 * @brief AFP attributes, CNID, and name entries of an AppleDouble
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/adouble.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/util.h>

#define FILEIOFF_ATTR 14
#define AFPFILEIOFF_ATTR 2

/*!
 * @brief Read the AFP attribute word of an AppleDouble entry
 *
 * The shared and invisible attributes are opaque and live in the Finder flags,
 * so they come from there, as bug #2802236 required. For directories
 * ATTRBIT_EXPFLDR occupies the shared bit and stays untouched. The adouble's
 * open-fork bits are also present in the result.
 *
 * @sa https://sourceforge.net/p/netatalk/bugs/350/
 *
 * @param[in]  ad    AppleDouble with its metadata loaded
 * @param[out] attr  receives the attribute word in network byte order
 * @returns 0
 */
int ad_getattr(const struct adouble *ad, uint16_t *attr)
{
    uint16_t fflags;
    char *adp = NULL;
    *attr = 0;

    if (ad_getentryoff(ad, ADEID_AFPFILEI)
            && (adp = ad_entry(ad, ADEID_AFPFILEI)) != NULL) {
        memcpy(attr, adp + AFPFILEIOFF_ATTR, 2);

        /* Now get opaque flags from FinderInfo */
        if ((adp = ad_entry(ad, ADEID_FINDERI)) != NULL) {
            memcpy(&fflags, adp + FINDERINFO_FRFLAGOFF, 2);
        } else {
            LOG(log_debug, logtype_default, "ad_getattr(%s): invalid FinderInfo",
                ad->ad_name);
            memset(&fflags, 0, 2);
        }

        if (fflags & htons(FINDERINFO_INVISIBLE)) {
            *attr |= htons(ATTRBIT_INVISIBLE);
        } else {
            *attr &= htons(~ATTRBIT_INVISIBLE);
        }

        /*
         * This one is tricky, I actually got it wrong the first time:
         * for directories bit 1<<1 is ATTRBIT_EXPFLDR and is NOT opaque !
         */
        if (!(ad->ad_adflags & ADFLAGS_DIR)) {
            if (fflags & htons(FINDERINFO_ISHARED)) {
                *attr |= htons(ATTRBIT_MULTIUSER);
            } else {
                *attr &= htons(~ATTRBIT_MULTIUSER);
            }
        }
    }

    *attr |= htons(ad->ad_open_forks);
    return 0;
}

/*!
 * @brief Store the AFP attribute word in an AppleDouble entry
 *
 * ad_setattr never stores the open-fork bits. For directories it drops the bits
 * that only files carry (ATTRBIT_MULTIUSER, ATTRBIT_NOWRITE, ATTRBIT_NOCOPY),
 * because Mac OS X 10.4 tries to set ATTRBIT_MULTIUSER on folders. It also
 * writes the invisible bit, and for files the shared bit, into the Finder
 * flags.
 *
 * @param[in] ad         AppleDouble with its metadata loaded
 * @param[in] attribute  attribute word in network byte order
 * @returns 0
 */
int ad_setattr(const struct adouble *ad, const uint16_t attribute)
{
    uint16_t fflags;
    char *ade = NULL, *adp = NULL;
    /* we don't save open forks indicator */
    uint16_t attr = attribute & ~htons(ATTRBIT_DOPEN | ATTRBIT_ROPEN);

    /* Proactively (10.4 does indeed try to set ATTRBIT_MULTIUSER (=ATTRBIT_EXPFLDR)
       for dirs with SetFile -a M <dir> ) disable all flags not defined for dirs. */
    if (ad->ad_adflags & ADFLAGS_DIR) {
        attr &= ~htons(ATTRBIT_MULTIUSER | ATTRBIT_NOWRITE | ATTRBIT_NOCOPY);
    }

    if (ad_getentryoff(ad, ADEID_AFPFILEI)
            && (ade = ad_entry(ad, ADEID_AFPFILEI)) != NULL
            && ad_getentryoff(ad, ADEID_FINDERI)
            && (adp = ad_entry(ad, ADEID_FINDERI)) != NULL) {
        memcpy(ade + AFPFILEIOFF_ATTR, &attr, sizeof(attr));
        /* Now set opaque flags in FinderInfo too */
        memcpy(&fflags, adp + FINDERINFO_FRFLAGOFF, 2);

        if (attr & htons(ATTRBIT_INVISIBLE)) {
            fflags |= htons(FINDERINFO_INVISIBLE);
        } else {
            fflags &= htons(~FINDERINFO_INVISIBLE);
        }

        /* See above comment in ad_getattr() */
        if (attr & htons(ATTRBIT_MULTIUSER)) {
            if (!(ad->ad_adflags & ADFLAGS_DIR)) {
                fflags |= htons(FINDERINFO_ISHARED);
            }
        } else {
            fflags &= htons(~FINDERINFO_ISHARED);
        }

        memcpy(adp + FINDERINFO_FRFLAGOFF, &fflags, 2);
    }

    return 0;
}

/*!
 * @brief Store the CNID, the file identity, and the database stamp in the
 * private AppleDouble entries
 *
 * PRIVDEV and PRIVINO hold 8 bytes each, so the native dev_t and ino_t widen to
 * 64 bits in host byte order. ADVOL_NODEV zeroes the device. The CNID goes out
 * in network order for the EA form. The parent DID only exists in version 2.
 * When an entry cannot be addressed, ad_setid restores the lengths it changed
 * and reports nothing written.
 *
 * @param[in,out] adp    AppleDouble with its metadata loaded
 * @param[in]     dev    device of the file
 * @param[in]     ino    inode of the file
 * @param[in]     id     CNID to store
 * @param[in]     did    CNID of the parent directory, stored for version 2 only
 * @param[in]     stamp  CNID database stamp of ADEDLEN_PRIVSYN bytes
 * @returns 1 after writing the entries, so the header needs a flush, or 0 when
 *          an entry cannot be addressed
 */
int ad_setid(struct adouble *adp, const dev_t dev, const ino_t ino,
             const uint32_t id, const cnid_t did, const void *stamp)
{
    EC_INIT;
    uint32_t tmp;
    char *ade = NULL;
    ssize_t id_len = -1, dev_len = -1, ino_len = -1, did_len = -1, syn_len = -1;
    id_len = ad_getentrylen(adp, ADEID_PRIVID);
    ad_setentrylen(adp, ADEID_PRIVID, sizeof(id));
    tmp = id;

    if (adp->ad_vers == AD_VERSION_EA) {
        tmp = htonl(tmp);
    }

    ade = ad_entry(adp, ADEID_PRIVID);

    if (ade == NULL) {
        LOG(log_warning, logtype_ad, "ad_setid(%s): failed to set ADEID_PRIVID",
            adp->ad_name);
        EC_FAIL;
    }

    memcpy(ade, &tmp, sizeof(tmp));
    /* PRIVDEV/PRIVINO are fixed 8-byte on-disk fields; widen the native dev_t/
     * ino_t (dev_t is 4 bytes on DragonflyBSD) so the stored width never depends
     * on sizeof(). Byte-identical on 8-byte platforms; only ever compared against
     * a same-host stat(), so native byte order is fine. */
    dev_len = ad_getentrylen(adp, ADEID_PRIVDEV);
    ad_setentrylen(adp, ADEID_PRIVDEV, ADEDLEN_PRIVDEV);
    ade = ad_entry(adp, ADEID_PRIVDEV);

    if (ade == NULL) {
        LOG(log_warning, logtype_ad, "ad_setid(%s): failed to set ADEID_PRIVDEV",
            adp->ad_name);
        EC_FAIL;
    }

    if (adp->ad_options & ADVOL_NODEV) {
        memset(ade, 0, ADEDLEN_PRIVDEV);
    } else {
        uint64_t dev64 = (uint64_t)dev;
        memcpy(ade, &dev64, ADEDLEN_PRIVDEV);
    }

    ino_len = ad_getentrylen(adp, ADEID_PRIVINO);
    ad_setentrylen(adp, ADEID_PRIVINO, ADEDLEN_PRIVINO);
    ade = ad_entry(adp, ADEID_PRIVINO);

    if (ade == NULL) {
        LOG(log_warning, logtype_ad, "ad_setid(%s): failed to set ADEID_PRIVINO",
            adp->ad_name);
        EC_FAIL;
    }

    {
        uint64_t ino64 = (uint64_t)ino;
        memcpy(ade, &ino64, ADEDLEN_PRIVINO);
    }

    if (adp->ad_vers != AD_VERSION_EA) {
        did_len = ad_getentrylen(adp, ADEID_DID);
        ad_setentrylen(adp, ADEID_DID, sizeof(did));
        ade = ad_entry(adp, ADEID_DID);

        if (ade == NULL) {
            LOG(log_warning, logtype_ad, "ad_setid(%s): failed to set ADEID_DID",
                adp->ad_name);
            EC_FAIL;
        }

        memcpy(ade, &did, sizeof(did));
    }

    syn_len = ad_getentrylen(adp, ADEID_PRIVSYN);
    ad_setentrylen(adp, ADEID_PRIVSYN, ADEDLEN_PRIVSYN);
    ade = ad_entry(adp, ADEID_PRIVSYN);

    if (ade == NULL) {
        LOG(log_warning, logtype_ad, "ad_setid(%s): failed to set ADEID_PRIVSYN",
            adp->ad_name);
        EC_FAIL;
    }

    memcpy(ade, stamp, ADEDLEN_PRIVSYN);
EC_CLEANUP:

    if (ret != 0) {
        if (id_len != -1) {
            ad_setentrylen(adp, ADEID_PRIVID, id_len);
        }

        if (dev_len != -1) {
            ad_setentrylen(adp, ADEID_PRIVDEV, dev_len);
        }

        if (ino_len != -1) {
            ad_setentrylen(adp, ADEID_PRIVINO, ino_len);
        }

        if (did_len != -1) {
            ad_setentrylen(adp, ADEID_DID, did_len);
        }

        if (syn_len != -1) {
            ad_setentrylen(adp, ADEID_PRIVSYN, syn_len);
        }

        return 0;
    }

    return 1;
}

/*!
 * @brief Read the CNID stored in an AppleDouble entry when it matches the file
 *
 * The stored device and inode must equal the file's, unless the volume runs
 * with ADVOL_NODEV. The stored parent must equal did when both are non-zero.
 * Version 2 entries hold the CNID in host order, EA entries in network order.
 * Callers treat a return of CNID_INVALID (0) as an invalid value.
 *
 * @param[in] adp     AppleDouble with its metadata loaded, or NULL
 * @param[in] st_dev  device of the file from stat
 * @param[in] st_ino  inode of the file from stat
 * @param[in] did     CNID of the parent directory, or 0 to skip that check
 * @param[in] stamp   unused
 * @returns the stored CNID, or CNID_INVALID when the entry is absent or the
 *          identity differs
 */
uint32_t ad_getid(struct adouble *adp, const dev_t st_dev, const ino_t st_ino,
                  const cnid_t did, const void *stamp _U_)
{
    uint32_t aint = 0;
    dev_t  dev;
    ino_t  ino;
    cnid_t a_did = 0;

    /* gate on the fixed 8-byte stored width, not sizeof(dev_t) (4 on
     * DragonflyBSD); read back into a uint64_t and narrow to native. */
    if (adp && ad_getentrylen(adp, ADEID_PRIVDEV) == ADEDLEN_PRIVDEV) {
        char *ade = NULL;
        uint64_t dev64, ino64;
        ade = ad_entry(adp, ADEID_PRIVDEV);

        if (ade == NULL) {
            LOG(log_warning, logtype_ad, "ad_getid: failed to retrieve ADEID_PRIVDEV\n");
            return CNID_INVALID;
        }

        memcpy(&dev64, ade, ADEDLEN_PRIVDEV);
        dev = (dev_t)dev64;

        /* gate the inode read on its fixed 8-byte stored width too, so a
         * corrupt/short ADEID_PRIVINO entry can't be read past its end. */
        if (ad_getentrylen(adp, ADEID_PRIVINO) != ADEDLEN_PRIVINO) {
            LOG(log_warning, logtype_ad, "ad_getid: unexpected ADEID_PRIVINO length\n");
            return CNID_INVALID;
        }

        ade = ad_entry(adp, ADEID_PRIVINO);

        if (ade == NULL) {
            LOG(log_warning, logtype_ad, "ad_getid: failed to retrieve ADEID_PRIVINO\n");
            return CNID_INVALID;
        }

        memcpy(&ino64, ade, ADEDLEN_PRIVINO);
        ino = (ino_t)ino64;

        if (adp->ad_vers != AD_VERSION_EA) {
            /* ADEID_DID is not stored for AD_VERSION_EA */
            ade = ad_entry(adp, ADEID_DID);

            if (ade == NULL) {
                LOG(log_warning, logtype_ad, "ad_getid: failed to retrieve ADEID_DID\n");
                return CNID_INVALID;
            }

            memcpy(&a_did, ade, sizeof(cnid_t));
        }

        if (((adp->ad_options & ADVOL_NODEV) || (dev == st_dev))
                && ino == st_ino
                && (!did || a_did == 0 || a_did == did)) {
            ade = ad_entry(adp, ADEID_PRIVID);

            if (ade == NULL) {
                LOG(log_warning, logtype_ad, "ad_getid: failed to retrieve ADEID_PRIVID\n");
                return CNID_INVALID;
            }

            memcpy(&aint, ade, sizeof(aint));

            if (adp->ad_vers == AD_VERSION2) {
                return aint;
            } else {
                return ntohl(aint);
            }
        }
    }

    return CNID_INVALID;
}

/*!
 * @brief Read the CNID stored in an AppleDouble entry without checking the
 * file's identity
 *
 * @param[in] adp  AppleDouble with its metadata loaded, or NULL
 * @returns the stored CNID, or CNID_INVALID when adp is NULL or has no PRIVID
 *          entry
 */
uint32_t ad_forcegetid(struct adouble *adp)
{
    uint32_t aint = 0;

    if (adp) {
        char *ade = NULL;
        ade = ad_entry(adp, ADEID_PRIVID);

        if (ade == NULL) {
            return CNID_INVALID;
        }

        memcpy(&aint, ade, sizeof(aint));

        if (adp->ad_vers == AD_VERSION2) {
            return aint;
        } else {
            return ntohl(aint);
        }
    }

    return CNID_INVALID;
}

/*!
 * @brief Store a file name in the AppleDouble name entry
 *
 * ad_setname cuts the name at ADEDLEN_NAME bytes.
 *
 * @param[in,out] ad    AppleDouble with its metadata loaded
 * @param[in]     path  name to store
 * @returns 1 after storing the name, 0 when the entry does not exist, -1 when
 *          it cannot be addressed
 */
int ad_setname(struct adouble *ad, const char *path)
{
    int len;

    if ((len = strlen(path)) > ADEDLEN_NAME) {
        len = ADEDLEN_NAME;
    }

    if (path && ad_getentryoff(ad, ADEID_NAME)) {
        char *ade = NULL;
        ad_setentrylen(ad, ADEID_NAME, len);
        ade = ad_entry(ad, ADEID_NAME);

        if (ade == NULL) {
            return -1;
        }

        memcpy(ade, path, len);
        return 1;
    }

    return 0;
}
