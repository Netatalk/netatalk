/*
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <atalk/adouble.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "ad_private.h"
#if AD_VERSION == AD_VERSION1

#define EID_DISK(a) (a)

#else

static const u_int32_t set_eid[] = {
    0,1,2,3,4,5,6,7,8,
    9,10,11,12,13,14,15,
    AD_DEV, AD_INO, AD_SYN, AD_ID
};

#define EID_DISK(a) (set_eid[a])
#endif

/* rebuild the adouble header
 * XXX should be in a separate file ?
 */
int  ad_rebuild_adouble_header(struct adouble *ad)
{
    u_int32_t       eid;
    u_int32_t       temp;

    u_int16_t       nent;
    char        *buf, *nentp;

    /*
     * Rebuild any header information that might have changed.
     */
    buf = ad->ad_data;

    temp = htonl( ad->ad_magic );
    memcpy(buf, &temp, sizeof( temp ));
    buf += sizeof( temp );

    temp = htonl( ad->ad_version );
    memcpy(buf, &temp, sizeof( temp ));
    buf += sizeof( temp );

    memcpy(buf, ad->ad_filler, sizeof( ad->ad_filler ));
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
    return ad_getentryoff(ad, ADEID_RFORK);
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

/* ------------------- */
int  ad_rebuild_sfm_header(struct adouble *ad)
{
    u_int32_t       temp;

    u_int16_t       attr;
    char        *buf;

    /*
     * Rebuild any header information that might have changed.
     */
    buf = ad->ad_data;
    /* FIXME */
/*    temp = htonl( ad->ad_magic ); */
    temp = ad->ad_magic;
    memcpy(buf, &temp, sizeof( temp ));

/*    temp = htonl( ad->ad_version ); */
    temp = ad->ad_version;
    memcpy(buf +4, &temp, sizeof( temp ));

    /* need to save attrib */
    if (!ad_getattr(ad, &attr)) {
        attr &= ~htons(ATTRBIT_DOPEN | ATTRBIT_ROPEN);

        memcpy(buf +48 +4, &attr, sizeof(attr));

    }
    return AD_SFM_LEN;
}


int ad_flush( struct adouble *ad)
{
    int len;

    if (( ad->ad_md->adf_flags & O_RDWR )) {
        /* sync our header */
        if (ad->ad_rlen > 0xffffffff) {
            ad_setentrylen(ad, ADEID_RFORK, 0xffffffff);
        }
        else {
            ad_setentrylen(ad, ADEID_RFORK, ad->ad_rlen);
        }
        len = ad->ad_ops->ad_rebuild_header(ad);

        if (adf_pwrite(ad->ad_md, ad->ad_data, len, 0) != len) {
            if ( errno == 0 ) {
                errno = EIO;
            }
            return( -1 );
        }
    }

    return( 0 );
}

/* use refcounts so that we don't have to re-establish fcntl locks. */
int ad_close( struct adouble *ad, int adflags)
{
    int         err = 0;

    if ((adflags & ADFLAGS_DF)
        && (ad_data_fileno(ad) >= 0 || ad_data_fileno(ad) == -2) /* -2 means symlink */
        && --ad->ad_data_fork.adf_refcount == 0) {
        if (ad->ad_data_fork.adf_syml != NULL) {
            free(ad->ad_data_fork.adf_syml);
            ad->ad_data_fork.adf_syml = 0;
        } else {
            if ( close( ad_data_fileno(ad) ) < 0 )
                err = -1;
        }
        ad_data_fileno(ad) = -1;
        adf_lock_free(&ad->ad_data_fork);
    }

    if (!( adflags & ADFLAGS_HF )) {
        return err;
    }

    /* meta /resource fork */

    if ( ad_meta_fileno(ad) != -1 && !(--ad->ad_md->adf_refcount)) {
        if ( close( ad_meta_fileno(ad) ) < 0 ) {
            err = -1;
        }
        ad_meta_fileno(ad) = -1;
        adf_lock_free(ad->ad_md);
    }

    if (ad->ad_flags != AD_VERSION1_SFM) {
        return err;
    }

    if ((adflags & ADFLAGS_DIR)) {
        return err;
    }

    if ( ad_reso_fileno(ad) != -1 && !(--ad->ad_resource_fork.adf_refcount)) {
        if ( close( ad_reso_fileno(ad) ) < 0 ) {
            err = -1;
        }
        ad_reso_fileno(ad) = -1;
        adf_lock_free(&ad->ad_resource_fork);
    }

    return err;
}
