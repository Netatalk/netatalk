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

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/logger.h>
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>

#include "ad_lock.h"

static const u_int32_t set_eid[] = {
    0,1,2,3,4,5,6,7,8,
    9,10,11,12,13,14,15,
    AD_DEV, AD_INO, AD_SYN, AD_ID
};

#define EID_DISK(a) (set_eid[a])

/*
 * Rebuild any header information that might have changed.
 */
int  ad_rebuild_adouble_header(struct adouble *ad)
{
    u_int32_t       eid;
    u_int32_t       temp;
    u_int16_t       nent;
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

    switch (ad->ad_flags) {
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
    u_int32_t       eid;
    u_int32_t       len;

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

    if (( ad->ad_md->adf_flags & O_RDWR )) {
        if (ad_getentryoff(ad, ADEID_RFORK)) {
            if (ad->ad_rlen > 0xffffffff)
                ad_setentrylen(ad, ADEID_RFORK, 0xffffffff);
            else
                ad_setentrylen(ad, ADEID_RFORK, ad->ad_rlen);
        }
        len = ad->ad_ops->ad_rebuild_header(ad);

        switch (ad->ad_flags) {
        case AD_VERSION2:
            if (adf_pwrite(ad->ad_md, ad->ad_data, len, 0) != len) {
                if (errno == 0)
                    errno = EIO;
                return( -1 );
            }
            break;
        case AD_VERSION_EA:
            if (sys_lsetxattr(cfrombstr(ad->ad_fullpath), AD_EA_META, ad->ad_data, AD_DATASZ_EA, 0) != 0) {
                LOG(log_error, logtype_afpd, "ad_flush: sys_fsetxattr error: %s",
                    strerror(errno));
                return -1;
            }
            break;
        default:
            LOG(log_error, logtype_afpd, "ad_flush: unexpected adouble version");
            return -1;
        }
    }

    return( 0 );
}

/*!
 * Close a struct adouble freeing all resources
 *
 * This close the whole thing, regardless of what you pass in adflags!
 */
int ad_close( struct adouble *ad, int adflags)
{
    int err = 0;

    LOG(log_debug, logtype_default, "ad_close(\"%s\", %s)",
        cfrombstr(ad->ad_fullpath),
        adflags2logstr(adflags));

    if (ad_data_fileno(ad) != -1) {
        if ((ad_data_fileno(ad) == -2) && (ad->ad_data_fork.adf_syml != NULL)) {
            free(ad->ad_data_fork.adf_syml);
            ad->ad_data_fork.adf_syml = NULL;
        } else {
            if ( close( ad_data_fileno(ad) ) < 0 )
                err = -1;
        }
        ad_data_fileno(ad) = -1;
        adf_lock_free(&ad->ad_data_fork);
        ad->ad_adflags &= ~ADFLAGS_DF;
    }

    if (ad_meta_fileno(ad) != -1) {
        if ( close( ad_meta_fileno(ad) ) < 0 )
            err = -1;
        ad_meta_fileno(ad) = -1;
        adf_lock_free(ad->ad_md);
        ad->ad_adflags &= ~ADFLAGS_HF;
    }

    if (ad->ad_resforkbuf) {
        free(ad->ad_resforkbuf);
        ad->ad_resforkbuf = NULL;
        ad->ad_adflags &= ~ADFLAGS_RF;
    }

    if (ad->ad_fullpath) {
        bdestroy(ad->ad_fullpath);
        ad->ad_fullpath = NULL;
    }

    return err;
}
