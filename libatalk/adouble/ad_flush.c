/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * Copyright (c) 2010      Frank Lahm
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
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/logger.h>
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>

#include "ad_lock.h"

static const uint32_t set_eid[] = {
    0,1,2,3,4,5,6,7,8,
    9,10,11,12,13,14,15,
    AD_DEV, AD_INO, AD_SYN, AD_ID
};

#define EID_DISK(a) (set_eid[a])

int fsetrsrcea(struct adouble *ad, int fd, const char *eaname, const void *value, size_t size, int flags)
{
    if ((ad->ad_maxeafssize == 0) || (ad->ad_maxeafssize >= size)) {
        LOG(log_debug, logtype_default, "fsetrsrcea(\"%s\"): size: %zu", eaname, size);
        if (sys_fsetxattr(fd, eaname, value, size, 0) == -1)
            return -1;
        return 0;
    }

    /* rsrcfork is larger then maximum EA support by fs so we have to split it */
    int i;
    int eas = (size / ad->ad_maxeafssize);
    size_t remain = size - (eas * ad->ad_maxeafssize);
    bstring eachunk;

    LOG(log_debug, logtype_default, "fsetrsrcea(\"%s\"): size: %zu, maxea: %zu, eas: %d, remain: %zu",
        eaname, size, ad->ad_maxeafssize, eas, remain);

    for (i = 0; i < eas; i++) {
        if ((eachunk = bformat("%s.%d", eaname, i + 1)) == NULL)
            return -1;
        if (sys_fsetxattr(fd, bdata(eachunk), value + (i * ad->ad_maxeafssize), ad->ad_maxeafssize, 0) == -1) {
            LOG(log_error, logtype_default, "fsetrsrcea(\"%s\"): %s", bdata(eachunk), strerror(errno));
            bdestroy(eachunk);
            return -1;
        }
        bdestroy(eachunk);
    }

    if ((eachunk = bformat("%s.%d", eaname, i + 1)) == NULL)
        return -1;
    if (sys_fsetxattr(fd, bdata(eachunk), value + (i * ad->ad_maxeafssize), remain, 0) == -1) {
        LOG(log_error, logtype_default, "fsetrsrcea(\"%s\"): %s", bdata(eachunk), strerror(errno));
        return -1;
    }
    bdestroy(eachunk);

    return 0;
}

/*
 * Rebuild any header information that might have changed.
 */
int  ad_rebuild_adouble_header(struct adouble *ad)
{
    uint32_t       eid;
    uint32_t       temp;
    uint16_t       nent;
    char        *buf, *nentp;
    int             len;

    buf = ad->ad_data;

    temp = htonl( ad->ad_magic );
    memcpy(buf, &temp, sizeof( temp ));
    buf += sizeof( temp );

    temp = htonl( ad->ad_version );
    memcpy(buf, &temp, sizeof( temp ));
    buf += sizeof( temp );

    buf += sizeof( ad->ad_filler );

    nentp = buf;
    buf += sizeof( nent );
    for ( eid = 0, nent = 0; eid < ADEID_MAX; eid++ ) {
        if ( ad->ad_eid[ eid ].ade_off == 0 ) {
            continue;
        }
        temp = htonl( EID_DISK(eid) );
        memcpy(buf, &temp, sizeof( temp ));
        buf += sizeof( temp );

        temp = htonl( ad->ad_eid[ eid ].ade_off );
        memcpy(buf, &temp, sizeof( temp ));
        buf += sizeof( temp );

        temp = htonl( ad->ad_eid[ eid ].ade_len );
        memcpy(buf, &temp, sizeof( temp ));
        buf += sizeof( temp );
        nent++;
    }
    nent = htons( nent );
    memcpy(nentp, &nent, sizeof( nent ));

    switch (ad->ad_vers) {
    case AD_VERSION2:
        len = ad_getentryoff(ad, ADEID_RFORK);
        break;
    case AD_VERSION_EA:
        len = AD_DATASZ_EA;
        break;
    default:
        LOG(log_error, logtype_afpd, "Unexpected adouble version");
        len = 0;
        break;
    }

    return len;
}

/* -------------------
 * XXX copy only header with same size or comment
 * doesn't work well for adouble with different version.
 *
 */
int ad_copy_header(struct adouble *add, struct adouble *ads)
{
    uint32_t       eid;
    uint32_t       len;

    for ( eid = 0; eid < ADEID_MAX; eid++ ) {
        if ( ads->ad_eid[ eid ].ade_off == 0 ) {
            continue;
        }

        if ( add->ad_eid[ eid ].ade_off == 0 ) {
            continue;
        }

        len = ads->ad_eid[ eid ].ade_len;
        if (!len) {
            continue;
        }

        if (eid != ADEID_COMMENT && add->ad_eid[ eid ].ade_len != len ) {
            continue;
        }

        ad_setentrylen( add, eid, len );
        memcpy( ad_entry( add, eid ), ad_entry( ads, eid ), len );
    }
    add->ad_rlen = ads->ad_rlen;
    return 0;
}

int ad_flush(struct adouble *ad)
{
    int len;

    LOG(log_debug, logtype_default, "ad_flush(%s)", adflags2logstr(ad->ad_adflags));

    struct ad_fd *adf;

    switch (ad->ad_vers) {
    case AD_VERSION2:
        adf = ad->ad_mdp;
        break;
    case AD_VERSION_EA:
        adf = &ad->ad_data_fork;
        break;
    default:
        LOG(log_error, logtype_afpd, "ad_flush: unexpected adouble version");
        return -1;
    }

    if ((adf->adf_flags & O_RDWR)) {
        if (ad_getentryoff(ad, ADEID_RFORK)) {
            if (ad->ad_rlen > 0xffffffff)
                ad_setentrylen(ad, ADEID_RFORK, 0xffffffff);
            else
                ad_setentrylen(ad, ADEID_RFORK, ad->ad_rlen);
        }
        len = ad->ad_ops->ad_rebuild_header(ad);

        switch (ad->ad_vers) {
        case AD_VERSION2:
            if (adf_pwrite(ad->ad_mdp, ad->ad_data, len, 0) != len) {
                if (errno == 0)
                    errno = EIO;
                return( -1 );
            }
            break;
        case AD_VERSION_EA:
            if (AD_META_OPEN(ad)) {
                if (sys_fsetxattr(ad_data_fileno(ad), AD_EA_META, ad->ad_data, AD_DATASZ_EA, 0) != 0) {
                    LOG(log_error, logtype_afpd, "ad_flush: sys_fsetxattr error: %s",
                        strerror(errno));
                    return -1;
                }
            }
#ifndef HAVE_EAFD
            if (AD_RSRC_OPEN(ad) && (ad->ad_rlen > 0)) {
                if (fsetrsrcea(ad, ad_data_fileno(ad), AD_EA_RESO, ad->ad_resforkbuf, ad->ad_rlen, 0) == -1)
                    return -1;
            }
#endif
            break;
        default:
            LOG(log_error, logtype_afpd, "ad_flush: unexpected adouble version");
            return -1;
        }
    }

    return( 0 );
}

static int ad_data_closefd(struct adouble *ad)
{
    int ret = 0;

    if (ad_data_fileno(ad) == -2) {
        free(ad->ad_data_fork.adf_syml);
        ad->ad_data_fork.adf_syml = NULL;
    } else {
        if (close(ad_data_fileno(ad)) < 0)
            ret = -1;
    }
    ad_data_fileno(ad) = -1;
    return ret;
}

/*!
 * Close a struct adouble freeing all resources
 */
int ad_close(struct adouble *ad, int adflags)
{
    int err = 0;

    if (ad == NULL)
        return err;

    LOG(log_debug, logtype_default, "ad_close(%s)", adflags2logstr(adflags));

    if ((adflags & ADFLAGS_DF)
        && (ad_data_fileno(ad) >= 0 || ad_data_fileno(ad) == -2) /* -2 means symlink */
        && --ad->ad_data_fork.adf_refcount == 0) {
        if (ad_data_closefd(ad) < 0)
            err = -1;
        adf_lock_free(&ad->ad_data_fork);
    }

    if ((adflags & ADFLAGS_HF)) {
        switch (ad->ad_vers) {
        case AD_VERSION2:
            if ((ad_meta_fileno(ad) != -1) && !(--ad->ad_mdp->adf_refcount)) {
                if (close( ad_meta_fileno(ad) ) < 0)
                    err = -1;
                ad_meta_fileno(ad) = -1;
                adf_lock_free(ad->ad_mdp);
            }
            break;

        case AD_VERSION_EA:
            if ((ad_data_fileno(ad) >= 0 || ad_data_fileno(ad) == -2) /* -2 means symlink */ 
                && !(--ad->ad_data_fork.adf_refcount)) {
                if (ad_data_closefd(ad) < 0)
                    err = -1;
                adf_lock_free(&ad->ad_data_fork);
            }
            break;

        default:
            LOG(log_error, logtype_default, "ad_close: unknown AD version");
            errno = EIO;
            return -1;
        }
    }

    if ((adflags & ADFLAGS_RF)) {
        switch (ad->ad_vers) {
        case AD_VERSION2:
            /* Do nothing as ADFLAGS_RF == ADFLAGS_HF */
            break;

        case AD_VERSION_EA:
#ifndef HAVE_EAFD
            LOG(log_debug, logtype_default, "ad_close: ad->ad_rlen: %zu", ad->ad_rlen);
            if (ad->ad_rlen > 0)
                if (fsetrsrcea(ad, ad_data_fileno(ad), AD_EA_RESO, ad->ad_resforkbuf, ad->ad_rlen, 0) == -1)
                    err = -1;
#endif
            if ((ad_data_fileno(ad) >= 0 || ad_data_fileno(ad) == -2) /* -2 means symlink */ 
                && !(--ad->ad_data_fork.adf_refcount)) {
                if (ad_data_closefd(ad) < 0)
                    err = -1;
                adf_lock_free(&ad->ad_data_fork);
                if (ad->ad_resforkbuf)
                    free(ad->ad_resforkbuf);
                ad->ad_resforkbuf = NULL;
                ad->ad_rlen = 0;
            }
            break;

        default:
            LOG(log_error, logtype_default, "ad_close: unknown AD version");
            errno = EIO;
            return -1;
        }
    }

    return err;
}
