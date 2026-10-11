/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * Copyright (c) 2010      Frank Lahm
 * Copyright (c) 2026 Andy Lemin (andylemin)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>

static const uint32_t set_eid[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 12, 13, 14, 15,
    AD_DEV, AD_INO, AD_SYN, AD_ID
};

#define EID_DISK(a) (set_eid[a])

/*!
 * @brief Serialize the header of a version 2 AppleDouble into ad->ad_data
 *
 * Writes the magic, the version, the filler, and one entry descriptor for every
 * entry with an offset, in on-disk byte order.
 *
 * @param[in,out] ad  AppleDouble whose in-memory entries are current
 * @returns the offset of the resource fork entry, the length of the header to
 *          write
 */
int ad_rebuild_adouble_header_v2(struct adouble *ad)
{
    uint32_t       eid;
    uint32_t       temp;
    uint16_t       nent;
    char        *buf, *nentp;
    LOG(log_debug, logtype_ad, "ad_rebuild_adouble_header_v2");
    buf = ad->ad_data;
    temp = htonl(ad->ad_magic);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    temp = htonl(ad->ad_version);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    buf += sizeof(ad->ad_filler);
    nentp = buf;
    buf += sizeof(nent);

    for (eid = 0, nent = 0; eid < ADEID_MAX; eid++) {
        if (ad->ad_eid[eid].ade_off == 0) {
            continue;
        }

        temp = htonl(EID_DISK(eid));
        memcpy(buf, &temp, sizeof(temp));
        buf += sizeof(temp);
        temp = htonl(ad->ad_eid[eid].ade_off);
        memcpy(buf, &temp, sizeof(temp));
        buf += sizeof(temp);
        temp = htonl(ad->ad_eid[eid].ade_len);
        memcpy(buf, &temp, sizeof(temp));
        buf += sizeof(temp);
        nent++;
    }

    nent = htons(nent);
    memcpy(nentp, &nent, sizeof(nent));
    return ad_getentryoff(ad, ADEID_RFORK);
}

/*!
 * @brief Serialize the header of an EA AppleDouble into ad->ad_data
 *
 * Like ad_rebuild_adouble_header_v2() without the resource fork entry, which
 * the EA form keeps in a separate extended attribute.
 *
 * @param[in,out] ad  AppleDouble whose in-memory entries are current
 * @returns AD_DATASZ_EA, the length of the header to write
 */
int ad_rebuild_adouble_header_ea(struct adouble *ad)
{
    uint32_t       eid;
    uint32_t       temp;
    uint16_t       nent;
    char        *buf, *nentp;
    LOG(log_debug, logtype_ad, "ad_rebuild_adouble_header_ea");
    buf = ad->ad_data;
    temp = htonl(ad->ad_magic);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    temp = htonl(ad->ad_version);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    buf += sizeof(ad->ad_filler);
    nentp = buf;
    buf += sizeof(nent);

    for (eid = 0, nent = 0; eid < ADEID_MAX; eid++) {
        if ((ad->ad_eid[eid].ade_off == 0) || (eid == ADEID_RFORK)) {
            continue;
        }

        temp = htonl(EID_DISK(eid));
        memcpy(buf, &temp, sizeof(temp));
        buf += sizeof(temp);
        temp = htonl(ad->ad_eid[eid].ade_off);
        memcpy(buf, &temp, sizeof(temp));
        buf += sizeof(temp);
        temp = htonl(ad->ad_eid[eid].ade_len);
        memcpy(buf, &temp, sizeof(temp));
        buf += sizeof(temp);
        nent++;
    }

    nent = htons(nent);
    memcpy(nentp, &nent, sizeof(nent));
    return AD_DATASZ_EA;
}

/*!
 * @brief Serialize the header of a macOS native AppleDouble into adbuf
 *
 * The macOS form carries the FinderInfo entry, the resource fork entry, and the
 * netatalk filler string. The FinderInfo bytes come from ad.
 *
 * @param[in]  ad     AppleDouble whose in-memory entries are current
 * @param[out] adbuf  buffer of at least AD_DATASZ_OSX bytes
 * @returns AD_DATASZ_OSX, the length of the header to write
 */
int ad_rebuild_adouble_header_osx(struct adouble *ad, char *adbuf)
{
    uint32_t       temp;
    uint16_t       nent;
    char           *buf;
    char           *ade = NULL;
    LOG(log_debug, logtype_ad, "ad_rebuild_adouble_header_osx");
    buf = &adbuf[0];
    temp = htonl(ad->ad_magic);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    temp = htonl(ad->ad_version);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    memcpy(buf, AD_FILLER_NETATALK, strlen(AD_FILLER_NETATALK));
    buf += sizeof(ad->ad_filler);
    nent = htons(ADEID_NUM_OSX);
    memcpy(buf, &nent, sizeof(nent));
    buf += sizeof(nent);
    /* FinderInfo */
    temp = htonl(EID_DISK(ADEID_FINDERI));
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    temp = htonl(ADEDOFF_FINDERI_OSX);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    temp = htonl(ADEDLEN_FINDERI);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    ade = ad_entry(ad, ADEID_FINDERI);

    if (ade != NULL) {
        memcpy(adbuf + ADEDOFF_FINDERI_OSX, ade, ADEDLEN_FINDERI);
    } else {
        LOG(log_debug, logtype_ad,
            "ad_rebuild_adouble_header_osx(%s): invalid FinderInfo", ad->ad_name);
    }

    /* rfork */
    temp = htonl(EID_DISK(ADEID_RFORK));
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    temp = htonl(ADEDOFF_RFORK_OSX);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    temp = htonl(ad->ad_rlen);
    memcpy(buf, &temp, sizeof(temp));
    buf += sizeof(temp);
    return AD_DATASZ_OSX;
}

/*!
 * @brief Source entry to transfer for eid, or NULL to skip it
 *
 * Forks and the comment never transfer. Any other entry must exist on both
 * sides with a positive source length and pass the bounds test of ad_entry.
 *
 * @param[in]  add  destination AppleDouble
 * @param[in]  ads  source AppleDouble
 * @param[in]  eid  entry id, ADEID_*
 * @param[out] len  receives the source entry length
 * @returns the source entry, or NULL to skip it
 */
static char *ad_copy_header_entry(const struct adouble *add,
                                  const struct adouble *ads,
                                  uint32_t eid, uint32_t *len)
{
    char *src;
    ssize_t ade_len;

    switch (eid) {
    case ADEID_COMMENT:
    case ADEID_DFORK:
    case ADEID_RFORK:
        return NULL;

    default:
        break;
    }

    if (ads->ad_eid[eid].ade_off == 0 || add->ad_eid[eid].ade_off == 0) {
        return NULL;
    }

    ade_len = ads->ad_eid[eid].ade_len;

    if (ade_len <= 0) {
        return NULL;
    }

    *len = (uint32_t) ade_len;

    if ((src = ad_entry(ads, eid)) == NULL) {
        LOG(log_debug, logtype_ad, "ad_copy_header(%s): invalid src eid[%d]",
            ads->ad_name, eid);
    }

    return src;
}

/*!
 * @brief Transfer the header entries of one AppleDouble into another
 *
 * ad_copy_header checks every destination entry against the source length
 * before writing anything. exchangefiles runs this on live open forks, so a
 * failure must leave the header untouched. Between the version 2 and EA forms
 * the PRIVID entry changes byte order.
 *
 * @bug Only entries of the same size, or the comment, transfer cleanly. Pairs
 * of other AppleDouble versions do not work well.
 *
 * @param[in,out] add  destination AppleDouble with valid data
 * @param[in]     ads  source AppleDouble
 * @returns 0, or -1 when an entry does not fit (errno EIO) or the destination
 *          has no valid data
 */
int ad_copy_header(struct adouble *add, struct adouble *ads)
{
    uint32_t       eid;
    uint32_t       len;
    char *src = NULL;
    char *dst = NULL;

    if (add->valid_data_len == 0) {
        LOG(log_error, logtype_ad, "ad_copy_header(%s): dst invalid valid_data_len",
            add->ad_name);
        return -1;
    }

    for (eid = 0; eid < ADEID_MAX; eid++) {
        if (ad_copy_header_entry(add, ads, eid, &len) == NULL) {
            continue;
        }

        /* FinderInfo accepts any length of at least 32 bytes, so a large but
         * source-valid entry could otherwise be copied past add->ad_data */
        if (!ad_entry_fits(add, eid, len)) {
            LOG(log_debug, logtype_ad, "ad_copy_header(%s): invalid dst eid[%d]",
                add->ad_name, eid);
            errno = EIO;
            return -1;
        }
    }

    for (eid = 0; eid < ADEID_MAX; eid++) {
        if ((src = ad_copy_header_entry(add, ads, eid, &len)) == NULL) {
            continue;
        }

        dst = add->ad_data + ad_getentryoff(add, eid);
        memcpy(dst, src, len);
        ad_setentrylen(add, eid, len);
    }

    add->ad_rlen = ads->ad_rlen;

    if (((ads->ad_vers == AD_VERSION2) && (add->ad_vers == AD_VERSION_EA))
            || ((ads->ad_vers == AD_VERSION_EA) && (add->ad_vers == AD_VERSION2))) {
        src = dst = NULL;
        cnid_t id;
        dst = ad_entry(add, ADEID_PRIVID);

        if (dst != NULL) {
            memcpy(&id, dst, sizeof(cnid_t));
            id = htonl(id);
            memcpy(dst, &id, sizeof(cnid_t));
        } else {
            LOG(log_debug, logtype_ad, "ad_copy_header(%s): invalid PRIVID", add->ad_name);
        }
    }

    return 0;
}

/*!
 * @brief Write the header of an AppleDouble to its metadata store
 *
 * The store must be open for writing, else nothing happens. The resource fork
 * entry takes the current fork length, capped at 4 GiB minus one byte. Version
 * 2 writes ad_data to the metadata file. The EA form stores it in the
 * AD_EA_META extended attribute of the file, or of the directory through a
 * temporary chdir. On macOS it also refreshes the native FinderInfo EA of an
 * existing file. A sticky directory that refuses the EA with EPERM is an error
 * unless the volume sets ADVOL_FORCE_STICKY_XATTR.
 *
 * @param[in,out] ad  AppleDouble whose header entries are current
 * @returns 0, or -1 with errno set
 */
static int ad_flush_hf(struct adouble *ad)
{
    EC_INIT;
    int len;
    int cwd = -1;
    int dirfd = -1;
    LOG(log_debug, logtype_ad, "ad_flush_hf(%s)", adflags2logstr(ad->ad_adflags));
    struct ad_fd *adf;

    switch (ad->ad_vers) {
    case AD_VERSION2:
        adf = ad->ad_mdp;
        break;

    case AD_VERSION_EA:
        adf = &ad->ad_data_fork;
        break;

    default:
        LOG(log_error, logtype_ad, "ad_flush: unexpected adouble version");
        return -1;
    }

    if ((adf->adf_flags & O_RDWR)
            || ((ad->ad_adflags & ADFLAGS_DIR) && (ad->ad_adflags & ADFLAGS_RDWR))) {
        if (ad_getentryoff(ad, ADEID_RFORK)) {
            if (ad->ad_rlen > 0xffffffff) {
                ad_setentrylen(ad, ADEID_RFORK, 0xffffffff);
            } else {
                ad_setentrylen(ad, ADEID_RFORK, ad->ad_rlen);
            }
        }

        len = ad->ad_ops->ad_rebuild_header(ad);

        switch (ad->ad_vers) {
        case AD_VERSION2:
            if (adf_pwrite(ad->ad_mdp, ad->ad_data, len, 0) != len) {
                if (errno == 0) {
                    errno = EIO;
                }

                return -1;
            }

            break;

        case AD_VERSION_EA:
            if (ad_meta_open(ad)) {
#ifdef __APPLE__
                char *FinderInfo;
                char NativeFinderInfo[ADEDLEN_FINDERI] = {'\0'};
                FinderInfo = ad_entry(ad, ADEID_FINDERI);
                memcpy(NativeFinderInfo, FinderInfo, ADEDLEN_FINDERI);
#endif

                if (ad->ad_adflags & ADFLAGS_DIR) {
                    EC_NEG1_LOG(cwd = open(".", O_RDONLY));
                    EC_NEG1_LOG(fchdir(ad_data_fileno(ad)));
                    EC_NEG1_LOG(dirfd = open(".", O_RDONLY | O_DIRECTORY));
                    ret = sys_fsetxattr(dirfd, AD_EA_META, ad->ad_data, AD_DATASZ_EA, 0);
#ifdef __APPLE__

                    if (!(ad->ad_mdp->adf_flags & O_CREAT)) {
                        sys_fsetxattr(dirfd, EA_FINFO, NativeFinderInfo, ADEDLEN_FINDERI, 0);
                    }

#endif

                    if (ret != 0) {
                        if (errno != EPERM) {
                            EC_FAIL;
                        }

                        if (!(ad->ad_options & ADVOL_FORCE_STICKY_XATTR)) {
                            EC_FAIL;
                        }

                        /*
                         * This may be a directory with a sticky bit
                         * set, which means even though we may have
                         * write access to the directory, only the
                         * owner is allowed to write xattrs
                         */
                        become_root();
                        ret = sys_fsetxattr(dirfd, AD_EA_META, ad->ad_data, AD_DATASZ_EA, 0);
#ifdef __APPLE__

                        if (!(ad->ad_mdp->adf_flags & O_CREAT)) {
                            sys_fsetxattr(dirfd, EA_FINFO, NativeFinderInfo, ADEDLEN_FINDERI, 0);
                        }

#endif
                        unbecome_root();

                        if (ret != 0) {
                            LOG(log_error, logtype_ad, "ad_flush_hf: %s", strerror(errno));
                            EC_FAIL;
                        }
                    }

                    close(dirfd);
                    dirfd = -1;
                    EC_NEG1_LOG(fchdir(cwd));
                    EC_NEG1_LOG(close(cwd));
                    cwd = -1;
                } else {
                    EC_ZERO_LOG(sys_fsetxattr(ad_data_fileno(ad), AD_EA_META, ad->ad_data,
                                              AD_DATASZ_EA, 0));
#ifdef __APPLE__

                    /* write back finderinfo to macos native ea, but only if we aren't creating a new adouble ea for netatalk */
                    if (!(ad->ad_mdp->adf_flags & O_CREAT)) {
                        EC_ZERO_LOG(sys_fsetxattr(ad_data_fileno(ad), EA_FINFO, NativeFinderInfo,
                                                  ADEDLEN_FINDERI, 0));
                    }

#endif
                }
            }

            break;

        default:
            LOG(log_error, logtype_ad, "ad_flush: unexpected adouble version");
            return -1;
        }
    }

EC_CLEANUP:

    if (dirfd != -1) {
        close(dirfd);
    }

    if (cwd != -1) {
        if (fchdir(cwd) != 0) {
            AFP_PANIC("ad_flush: can't fchdir");
        }

        close(cwd);
    }

    EC_EXIT;
}

/*!
 * @brief Rewrite the ._ sidecar header of an ea AppleDouble with an open
 * resource fork
 *
 * Only the ea layout without HAVE_EAFD keeps a sidecar header, so every other
 * case returns at once. A writable resource fork gets its length entry from
 * ad_rlen, capped at 0xffffffff. The ad_rebuild_adouble_header_osx call then
 * builds the header, and the call writes it at offset 0.
 *
 * @param[in,out] ad  AppleDouble whose sidecar header to write
 * @returns 0, or -1 when the write fell short, with EIO when errno was 0
 */
static int ad_flush_rf(struct adouble *ad)
{
    ssize_t len;
    char    adbuf[AD_DATASZ_OSX];
#ifdef HAVE_EAFD
    return 0;
#endif

    if (ad->ad_vers != AD_VERSION_EA) {
        return 0;
    }

    LOG(log_debug, logtype_ad, "ad_flush_rf(%s)", adflags2logstr(ad->ad_adflags));

    if (ad->ad_rfp->adf_flags & O_RDWR) {
        if (ad_getentryoff(ad, ADEID_RFORK)) {
            if (ad->ad_rlen > 0xffffffff) {
                ad_setentrylen(ad, ADEID_RFORK, 0xffffffff);
            } else {
                ad_setentrylen(ad, ADEID_RFORK, ad->ad_rlen);
            }
        }

        len = ad_rebuild_adouble_header_osx(ad, &adbuf[0]);

        if (adf_pwrite(ad->ad_rfp, adbuf, len, 0) != len) {
            if (errno == 0) {
                errno = EIO;
            }

            return -1;
        }
    }

    return 0;
}

/*!
 * @brief Write the header and the resource fork metadata of an AppleDouble to
 * disk
 *
 * The header goes out when the metadata store is open, the resource fork when
 * it is open.
 *
 * @param[in,out] ad  AppleDouble to flush
 * @returns 0, or -1 with errno set
 */
int ad_flush(struct adouble *ad)
{
    EC_INIT;
    LOG(log_debug, logtype_ad, "ad_flush(%s)", adflags2logstr(ad->ad_adflags));

    if (ad_meta_open(ad)) {
        EC_ZERO(ad_flush_hf(ad));
    }

    if (ad_rsrc_open(ad)) {
        EC_ZERO(ad_flush_rf(ad));
    }

EC_CLEANUP:
    EC_EXIT;
}

/*!
 * @brief Close the data fork descriptor of an AppleDouble, or drop the symlink
 * target it stands for
 *
 * @param[in,out] ad  AppleDouble whose data fork is open
 * @returns 0, or -1 when close fails
 */
static int ad_data_closefd(struct adouble *ad)
{
    int ret = 0;

    if (ad_data_fileno(ad) == AD_SYMLINK) {
        free(ad->ad_data_fork.adf_syml);
        ad->ad_data_fork.adf_syml = NULL;
    } else {
        if (close(ad_data_fileno(ad)) < 0) {
            ret = -1;
        }
    }

    ad_data_fileno(ad) = -1;
    return ret;
}

/*!
 * @brief Close the forks of an AppleDouble that adflags names and release their
 * resources
 *
 * Each fork carries a reference count, and its descriptor closes when the count
 * reaches zero. ADFLAGS_SETSHRMD and ADFLAGS_CHECK_OF also close the data fork,
 * because the share-mode locks live there. For version 2 closing the resource
 * fork also closes the metadata file, because opening the resource fork opens
 * it. ad_close logs and asserts a count that is already zero.
 *
 * @param[in,out] ad       AppleDouble to close, or NULL
 * @param[in]     adflags  ADFLAGS_DF, ADFLAGS_HF, ADFLAGS_RF, and the modifiers
 *                         above
 * @returns 0, or -1 when a close failed
 */
int ad_close(struct adouble *ad, int adflags)
{
    int err = 0;

    if (ad == NULL) {
        return err;
    }

    LOG(log_debug, logtype_ad,
        "ad_close(%s): BEGIN: {d: %d, m: %d, r: %d} "
        "[dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        adflags2logstr(adflags),
        ad->ad_data_refcount, ad->ad_meta_refcount, ad->ad_reso_refcount,
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);

    if (adflags & (ADFLAGS_SETSHRMD | ADFLAGS_CHECK_OF)) {
        /* sharemode locks stored in the data fork, adouble:v2 needs extra handling
         * ADFLAGS_CHECK_OF is semantically an open-mode flag ("check for open
         * forks during metadata open"), but ad_close() gives it close-mode
         * behavior: also close the data fork. This is correct because
         * ad_metadataat() with ADFLAGS_CHECK_OF opens the DF for share-mode
         * lock checking via ad_open(ADFLAGS_SETSHRMD), so the DF must be
         * closed here too. */
        adflags |= ADFLAGS_DF;
    }

    if ((ad->ad_vers == AD_VERSION2) && (adflags & ADFLAGS_RF)) {
        adflags |= ADFLAGS_HF;
    }

    if ((adflags & ADFLAGS_DF) && (ad_data_fileno(ad) >= 0
                                   || ad_data_fileno(ad) == AD_SYMLINK)) {
        if (ad->ad_data_refcount)
            if (--ad->ad_data_refcount == 0) {
                adf_lock_free(&ad->ad_data_fork);
            }

        if (ad->ad_data_fork.adf_refcount > 0) {
            if (--ad->ad_data_fork.adf_refcount == 0) {
                if (ad_data_closefd(ad) < 0) {
                    err = -1;
                }
            }
        } else {
            LOG(log_error, logtype_ad,
                "ad_close: data-fork adf_refcount underflow (already %d) — "
                "stray double ad_close()? fd=%d",
                ad->ad_data_fork.adf_refcount, ad_data_fileno(ad));
            AFP_ASSERT(ad->ad_data_fork.adf_refcount > 0);
        }
    }

    if ((adflags & ADFLAGS_HF) && (ad_meta_fileno(ad) != -1)) {
        if (ad->ad_meta_refcount) {
            ad->ad_meta_refcount--;
        }

        if (ad->ad_mdp->adf_refcount > 0) {
            if (!(--ad->ad_mdp->adf_refcount)) {
                if (close(ad_meta_fileno(ad)) < 0) {
                    err = -1;
                }

                ad_meta_fileno(ad) = -1;
            }
        } else {
            LOG(log_error, logtype_ad,
                "ad_close: meta-fork adf_refcount underflow (already %d) — "
                "stray double ad_close()? fd=%d",
                ad->ad_mdp->adf_refcount, ad_meta_fileno(ad));
            AFP_ASSERT(ad->ad_mdp->adf_refcount > 0);
        }
    }

    if (adflags & ADFLAGS_RF) {
        /* HF is automatically opened when opening an RF, close it. */
        if ((ad->ad_vers == AD_VERSION2) && (ad_meta_fileno(ad) != -1)) {
            if (ad->ad_meta_refcount) {
                ad->ad_meta_refcount--;
            }

            if (ad->ad_mdp->adf_refcount > 0) {
                if (!(--ad->ad_mdp->adf_refcount)) {
                    if (close(ad_meta_fileno(ad)) < 0) {
                        err = -1;
                    }

                    ad_meta_fileno(ad) = -1;
                }
            } else {
                LOG(log_error, logtype_ad,
                    "ad_close: meta-fork (v2 RF) adf_refcount underflow "
                    "(already %d) — stray double ad_close()? fd=%d",
                    ad->ad_mdp->adf_refcount, ad_meta_fileno(ad));
                AFP_ASSERT(ad->ad_mdp->adf_refcount > 0);
            }
        }

        if (ad->ad_reso_refcount)
            if (--ad->ad_reso_refcount == 0) {
                adf_lock_free(ad->ad_rfp);
            }

        /* Check version is EA and fd is not AD_SYMLINK (-2) or -1 */
        if ((ad->ad_vers == AD_VERSION_EA)
                && (ad_reso_fileno(ad) >= 0)) {
            if (ad->ad_rfp->adf_refcount > 0) {
                if (!(--ad->ad_rfp->adf_refcount)) {
                    if (close(ad->ad_rfp->adf_fd) < 0) {
                        err = -1;
                    }

                    ad->ad_rlen = 0;
                    ad_reso_fileno(ad) = -1;
                }
            } else {
                LOG(log_error, logtype_ad,
                    "ad_close: reso-fork adf_refcount underflow (already %d) — "
                    "stray double ad_close()? fd=%d",
                    ad->ad_rfp->adf_refcount, ad_reso_fileno(ad));
                AFP_ASSERT(ad->ad_rfp->adf_refcount > 0);
            }
        }
    }

    LOG(log_debug, logtype_ad,
        "ad_close(%s): END: %d {d: %d, m: %d, r: %d} "
        "[dfd: %d (ref: %d), mfd: %d (ref: %d), rfd: %d (ref: %d)]",
        adflags2logstr(adflags), err,
        ad->ad_data_refcount, ad->ad_meta_refcount, ad->ad_reso_refcount,
        ad_data_fileno(ad), ad->ad_data_fork.adf_refcount,
        ad_meta_fileno(ad), ad->ad_mdp->adf_refcount,
        ad_reso_fileno(ad), ad->ad_rfp->adf_refcount);
    return err;
}
